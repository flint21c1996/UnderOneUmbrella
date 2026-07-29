// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightExposureSourceComponent.h"

#include "Components/LocalLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightExposureReceiverComponent.h"
#include "World/Light/UOULightInteractionSurfaceComponent.h"
#include "Player/UOUUmbrellaLightShadeVolumeComponent.h"
#include "World/Light/UOULightReceivableInterface.h"

UUOULightExposureSourceComponent::UUOULightExposureSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	ReceiverObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ReceiverObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	ReceiverObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
}

void UUOULightExposureSourceComponent::BeginPlay()
{
	Super::BeginPlay();
	ValidateSettings();
}

void UUOULightExposureSourceComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEmitLight || DeltaTime <= 0.0f)
	{
		return;
	}

	PendingDeltaTime += DeltaTime;
	if (SampleInterval > 0.0f && PendingDeltaTime < SampleInterval)
	{
		return;
	}

	EmitLight(PendingDeltaTime);
	PendingDeltaTime = 0.0f;
}

TArray<FString> UUOULightExposureSourceComponent::GetPuzzleDebugInfo_Implementation() const
{
	return {
		FString::Printf(TEXT("Light Source: %s"), bEmitLight ? TEXT("On") : TEXT("Off")),
		FString::Printf(TEXT("Intensity: %.2f"), Intensity),
		FString::Printf(
			TEXT("Receivers / Lit / Blocked: %d / %d / %d"),
			LastReceiverCount,
			LastLitCount,
			LastBlockedCount),
		FString::Printf(TEXT("Reflection Bounces: %d"), LastReflectionBounceCount),
		FString::Printf(TEXT("Reflection Path: %s"), *LastReflectionPath),
		FString::Printf(TEXT("Last Lit: %s"), *LastLitTargetName)
	};
}

void UUOULightExposureSourceComponent::EmitLight(float DeltaTime)
{
	ValidateSettings();

	LastReceiverCount = 0;
	LastLitCount = 0;
	LastBlockedCount = 0;
	LastReflectedCount = 0;
	LastLitTargetName = TEXT("None");
	LastBlockedName = TEXT("None");
	LastReflectorName = TEXT("None");
	LastReflectionBounceCount = 0;
	LastReflectionPath = TEXT("None");
	ReflectionPaths.Reset();

	UWorld* World = GetWorld();
	const float ExposureRange = GetExposureRange();
	if (World == nullptr || DeltaTime <= 0.0f || ExposureRange <= 0.0f || Intensity <= 0.0f)
	{
		NotifyReflectionPathsUpdatedIfChanged();
		return;
	}

	DrawDebugSource();

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOULightReceiverOverlap), false, GetOwner());
	if (bIgnoreOwner && GetOwner() != nullptr)
	{
		QueryParams.AddIgnoredActor(GetOwner());
	}

	const bool bHasOverlaps = World->OverlapMultiByObjectType(
		OverlapResults,
		GetSourceLocation(),
		FQuat::Identity,
		BuildReceiverObjectQueryParams(),
		FCollisionShape::MakeSphere(ExposureRange),
		QueryParams);

	if (!bHasOverlaps)
	{
		NotifyReflectionPathsUpdatedIfChanged();
		return;
	}

	TSet<UObject*> ProcessedReceivers;
	TSet<UObject*> LitReceivers;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* OverlapActor = OverlapResult.GetActor();
		UPrimitiveComponent* OverlapComponent = OverlapResult.GetComponent();
		if (OverlapActor == nullptr && OverlapComponent == nullptr)
		{
			continue;
		}

		TArray<UObject*> Receivers;
		AppendReceivableObjects(OverlapActor, OverlapComponent, Receivers);
		for (UObject* ReceiverObject : Receivers)
		{
			if (ReceiverObject == nullptr || ProcessedReceivers.Contains(ReceiverObject))
			{
				continue;
			}

			ProcessedReceivers.Add(ReceiverObject);
			++LastReceiverCount;

			FUOULightExposureData ExposureData;
			FHitResult BlockingHit;
			if (TryBuildExposureData(ReceiverObject, DeltaTime, ExposureData, BlockingHit))
			{
				IUOULightReceivableInterface::Execute_ReceiveLightExposure(ReceiverObject, ExposureData);
				LitReceivers.Add(ReceiverObject);
				++LastLitCount;
				LastLitTargetName = GetReceivableDebugName(ReceiverObject);
				DrawDebugResult(ExposureData, true);
				continue;
			}

			if (BlockingHit.bBlockingHit)
			{
				++LastBlockedCount;
				LastBlockedName = GetNameSafe(BlockingHit.GetComponent());
				DrawDebugBlockedHit(GetSourceLocation(), BlockingHit);
			}
		}
	}

	if (!bEnableReflectedLight || MaxReflectionSurfacesPerTick <= 0)
	{
		NotifyReflectionPathsUpdatedIfChanged();
		return;
	}

	TSet<UUOULightInteractionSurfaceComponent*> ProcessedSurfaces;
	int32 ReflectedSurfaceCount = 0;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		if (ReflectedSurfaceCount >= MaxReflectionSurfacesPerTick)
		{
			break;
		}

		TArray<UUOULightInteractionSurfaceComponent*> InteractionSurfaces;
		AppendLightInteractionSurfaces(OverlapResult.GetActor(), OverlapResult.GetComponent(), InteractionSurfaces);
		for (UUOULightInteractionSurfaceComponent* InteractionSurface : InteractionSurfaces)
		{
			if (InteractionSurface == nullptr ||
				ProcessedSurfaces.Contains(InteractionSurface) ||
				ReflectedSurfaceCount >= MaxReflectionSurfacesPerTick)
			{
				continue;
			}

			ProcessedSurfaces.Add(InteractionSurface);

			FHitResult SurfaceHit;
			if (!TryBuildLightInteractionSurfaceHit(InteractionSurface, SurfaceHit))
			{
				continue;
			}

			++ReflectedSurfaceCount;
			LastReflectorName = GetNameSafe(InteractionSurface);
			EmitReflectedLightFromSurface(InteractionSurface, SurfaceHit, DeltaTime, LitReceivers);
		}
	}

	NotifyReflectionPathsUpdatedIfChanged();
}

void UUOULightExposureSourceComponent::Configure(float NewConeAngle, float NewIntensity)
{
	FallbackOuterConeAngle = NewConeAngle;
	Intensity = NewIntensity;
	ValidateSettings();
}

void UUOULightExposureSourceComponent::ValidateSettings()
{
	FallbackOuterConeAngle = FMath::Clamp(FallbackOuterConeAngle, 1.0f, 89.0f);
	FallbackInnerConeRatio = FMath::Clamp(FallbackInnerConeRatio, 0.0f, 1.0f);
	Intensity = FMath::Max(0.0f, Intensity);
	SampleInterval = FMath::Max(0.0f, SampleInterval);
	MaxReflectionSurfacesPerTick = FMath::Max(0, MaxReflectionSurfacesPerTick);
	MaxReflectionBouncesPerPath = FMath::Clamp(MaxReflectionBouncesPerPath, 1, 16);
	MinimumReflectedIntensity = FMath::Max(0.0f, MinimumReflectedIntensity);
	DebugDrawTime = FMath::Max(0.0f, DebugDrawTime);
}

void UUOULightExposureSourceComponent::NotifyReflectionPathsUpdatedIfChanged()
{
	if (bHasPublishedReflectionPaths &&
		AreReflectionPathsEquivalent(LastPublishedReflectionPaths, ReflectionPaths))
	{
		return;
	}

	LastPublishedReflectionPaths = ReflectionPaths;
	bHasPublishedReflectionPaths = true;
	OnReflectionPathsUpdated.Broadcast(ReflectionPaths);
}

bool UUOULightExposureSourceComponent::AreReflectionPathsEquivalent(
	const TArray<FUOULightReflectionPathData>& A,
	const TArray<FUOULightReflectionPathData>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (int32 PathIndex = 0; PathIndex < A.Num(); ++PathIndex)
	{
		const FUOULightReflectionPathData& LeftPath = A[PathIndex];
		const FUOULightReflectionPathData& RightPath = B[PathIndex];
		if (LeftPath.PathIndex != RightPath.PathIndex ||
			LeftPath.SourcePosition != RightPath.SourcePosition ||
			LeftPath.EndReason != RightPath.EndReason ||
			LeftPath.FinalIntensity != RightPath.FinalIntensity ||
			LeftPath.Segments.Num() != RightPath.Segments.Num())
		{
			return false;
		}

		for (int32 SegmentIndex = 0; SegmentIndex < LeftPath.Segments.Num(); ++SegmentIndex)
		{
			const FUOULightReflectionSegmentData& LeftSegment = LeftPath.Segments[SegmentIndex];
			const FUOULightReflectionSegmentData& RightSegment = RightPath.Segments[SegmentIndex];
			if (LeftSegment.BounceIndex != RightSegment.BounceIndex ||
				LeftSegment.Reflector != RightSegment.Reflector ||
				LeftSegment.NextReflector != RightSegment.NextReflector ||
				LeftSegment.BlockingComponent != RightSegment.BlockingComponent ||
				LeftSegment.ReachedReceivers != RightSegment.ReachedReceivers ||
				LeftSegment.IncomingStart != RightSegment.IncomingStart ||
				LeftSegment.ImpactPoint != RightSegment.ImpactPoint ||
				LeftSegment.ReflectionStart != RightSegment.ReflectionStart ||
				LeftSegment.SegmentEnd != RightSegment.SegmentEnd ||
				LeftSegment.IncomingDirection != RightSegment.IncomingDirection ||
				LeftSegment.ReflectedDirection != RightSegment.ReflectedDirection ||
				LeftSegment.SegmentLength != RightSegment.SegmentLength ||
				LeftSegment.BeamStartRadius != RightSegment.BeamStartRadius ||
				LeftSegment.BeamEndRadius != RightSegment.BeamEndRadius ||
				LeftSegment.IncomingIntensity != RightSegment.IncomingIntensity ||
				LeftSegment.ReflectedIntensity != RightSegment.ReflectedIntensity)
			{
				return false;
			}
		}
	}

	return true;
}

USceneComponent* UUOULightExposureSourceComponent::GetReferencedSourceTransform() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	if (SourceTransformReference.ComponentProperty == NAME_None &&
		SourceTransformReference.PathToComponent.IsEmpty() &&
		!SourceTransformReference.OverrideComponent.IsValid())
	{
		return nullptr;
	}

	return Cast<USceneComponent>(SourceTransformReference.GetComponent(Owner));
}

USpotLightComponent* UUOULightExposureSourceComponent::GetSourceSpotLightComponent() const
{
	if (USpotLightComponent* SourceSpotLight = Cast<USpotLightComponent>(GetReferencedSourceTransform()))
	{
		return SourceSpotLight;
	}

	if (!bAutoFindSourceSpotLight)
	{
		return nullptr;
	}

	const AActor* Owner = GetOwner();
	return Owner != nullptr ? Owner->FindComponentByClass<USpotLightComponent>() : nullptr;
}

ULocalLightComponent* UUOULightExposureSourceComponent::GetSourceLocalLightComponent() const
{
	if (ULocalLightComponent* SourceLight = Cast<ULocalLightComponent>(GetReferencedSourceTransform()))
	{
		return SourceLight;
	}

	return GetSourceSpotLightComponent();
}

FVector UUOULightExposureSourceComponent::GetSourceLocation() const
{
	if (const USceneComponent* SourceTransform = GetReferencedSourceTransform())
	{
		return SourceTransform->GetComponentLocation();
	}

	if (const USpotLightComponent* SourceSpotLight = GetSourceSpotLightComponent())
	{
		return SourceSpotLight->GetComponentLocation();
	}

	const AActor* Owner = GetOwner();
	return Owner != nullptr ? Owner->GetActorLocation() : FVector::ZeroVector;
}

FVector UUOULightExposureSourceComponent::GetSourceForwardVector() const
{
	if (const USceneComponent* SourceTransform = GetReferencedSourceTransform())
	{
		return SourceTransform->GetForwardVector();
	}

	if (const USpotLightComponent* SourceSpotLight = GetSourceSpotLightComponent())
	{
		return SourceSpotLight->GetForwardVector();
	}

	const AActor* Owner = GetOwner();
	return Owner != nullptr ? Owner->GetActorForwardVector() : FVector::ForwardVector;
}

float UUOULightExposureSourceComponent::GetExposureRange() const
{
	if (const ULocalLightComponent* SourceLight = GetSourceLocalLightComponent())
	{
		return FMath::Max(0.0f, SourceLight->AttenuationRadius);
	}

	return 0.0f;
}

float UUOULightExposureSourceComponent::GetEffectiveOuterConeAngle() const
{
	if (const USpotLightComponent* SourceSpotLight = GetSourceSpotLightComponent())
	{
		return FMath::Clamp(SourceSpotLight->OuterConeAngle, 1.0f, 89.0f);
	}

	return FallbackOuterConeAngle;
}

float UUOULightExposureSourceComponent::GetEffectiveInnerConeAngle(float OuterConeAngle) const
{
	if (const USpotLightComponent* SourceSpotLight = GetSourceSpotLightComponent())
	{
		return FMath::Clamp(SourceSpotLight->InnerConeAngle, 0.0f, OuterConeAngle);
	}

	return FMath::Clamp(OuterConeAngle * FallbackInnerConeRatio, 0.0f, OuterConeAngle);
}

FCollisionObjectQueryParams UUOULightExposureSourceComponent::BuildReceiverObjectQueryParams() const
{
	FCollisionObjectQueryParams ObjectQueryParams;
	for (const TEnumAsByte<EObjectTypeQuery>& ObjectType : ReceiverObjectTypes)
	{
		const ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(ObjectType.GetValue());
		if (FCollisionObjectQueryParams::IsValidObjectQuery(CollisionChannel))
		{
			ObjectQueryParams.AddObjectTypesToQuery(CollisionChannel);
		}
	}

	if (!ObjectQueryParams.IsValid())
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	}

	return ObjectQueryParams;
}

void UUOULightExposureSourceComponent::AppendReceivableObjects(
	AActor* TargetActor,
	UPrimitiveComponent* TargetComponent,
	TArray<UObject*>& OutReceivers) const
{
	if (TargetActor == nullptr)
	{
		if (TargetComponent != nullptr &&
			TargetComponent->GetClass()->ImplementsInterface(UUOULightReceivableInterface::StaticClass()))
		{
			OutReceivers.AddUnique(TargetComponent);
		}

		return;
	}

	TInlineComponentArray<UUOULightExposureReceiverComponent*> LightReceiverComponents(TargetActor);
	for (UUOULightExposureReceiverComponent* LightReceiverComponent : LightReceiverComponents)
	{
		if (LightReceiverComponent != nullptr)
		{
			OutReceivers.AddUnique(LightReceiverComponent);
		}
	}

	if (!OutReceivers.IsEmpty())
	{
		return;
	}

	if (TargetComponent != nullptr &&
		TargetComponent->GetClass()->ImplementsInterface(UUOULightReceivableInterface::StaticClass()))
	{
		OutReceivers.AddUnique(TargetComponent);
	}

	if (TargetActor->GetClass()->ImplementsInterface(UUOULightReceivableInterface::StaticClass()))
	{
		OutReceivers.AddUnique(TargetActor);
	}

	TInlineComponentArray<UActorComponent*> ActorComponents(TargetActor);
	for (UActorComponent* ActorComponent : ActorComponents)
	{
		if (ActorComponent != nullptr &&
			ActorComponent->GetClass()->ImplementsInterface(UUOULightReceivableInterface::StaticClass()))
		{
			OutReceivers.AddUnique(ActorComponent);
		}
	}
}

void UUOULightExposureSourceComponent::AppendLightInteractionSurfaces(
	AActor* TargetActor,
	UPrimitiveComponent* TargetComponent,
	TArray<UUOULightInteractionSurfaceComponent*>& OutSurfaces) const
{
	if (UUOULightInteractionSurfaceComponent* SurfaceComponent = Cast<UUOULightInteractionSurfaceComponent>(TargetComponent))
	{
		OutSurfaces.AddUnique(SurfaceComponent);
	}

	if (TargetActor == nullptr)
	{
		return;
	}

	TInlineComponentArray<UUOULightInteractionSurfaceComponent*> SurfaceComponents(TargetActor);
	for (UUOULightInteractionSurfaceComponent* SurfaceComponent : SurfaceComponents)
	{
		if (SurfaceComponent != nullptr)
		{
			OutSurfaces.AddUnique(SurfaceComponent);
		}
	}
}

bool UUOULightExposureSourceComponent::TryBuildExposureData(
	UObject* ReceiverObject,
	float DeltaTime,
	FUOULightExposureData& OutExposureData,
	FHitResult& OutBlockingHit) const
{
	OutExposureData = FUOULightExposureData();
	OutBlockingHit = FHitResult();

	if (ReceiverObject == nullptr ||
		!ReceiverObject->GetClass()->ImplementsInterface(UUOULightReceivableInterface::StaticClass()))
	{
		return false;
	}

	const FVector SourcePosition = GetSourceLocation();
	const FVector ReceiverPosition = IUOULightReceivableInterface::Execute_GetLightReceiverPosition(ReceiverObject);
	const FVector ToReceiver = ReceiverPosition - SourcePosition;
	const float Distance = ToReceiver.Size();
	const float ExposureRange = GetExposureRange();
	if (Distance <= KINDA_SMALL_NUMBER || Distance > ExposureRange)
	{
		return false;
	}

	const FVector Direction = ToReceiver / Distance;
	const FVector SourceForward = GetSourceForwardVector().GetSafeNormal();
	const float Dot = FMath::Clamp(FVector::DotProduct(SourceForward, Direction), -1.0f, 1.0f);
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));
	const float OuterConeAngle = GetEffectiveOuterConeAngle();
	if (Angle > OuterConeAngle)
	{
		return false;
	}

	if (IsWorldPositionInsideUmbrellaLightShade(ReceiverPosition))
	{
		return false;
	}

	if (bRequireLineOfSight && !HasLineOfSight(ReceiverObject, SourcePosition, ReceiverPosition, OutBlockingHit))
	{
		return false;
	}

	float DistanceFactor = 0.0f;
	float AngleFactor = 0.0f;
	const float FinalIntensity = CalculateIntensity(Distance, Angle, DistanceFactor, AngleFactor);
	if (FinalIntensity <= 0.0f)
	{
		return false;
	}

	OutExposureData = FUOULightExposureData(
		const_cast<UUOULightExposureSourceComponent*>(this),
		SourcePosition,
		ReceiverPosition,
		Direction,
		Distance,
		FinalIntensity,
		DistanceFactor,
		AngleFactor,
		DeltaTime);

	return true;
}

bool UUOULightExposureSourceComponent::HasLineOfSight(
	UObject* ReceiverObject,
	const FVector& SourcePosition,
	const FVector& ReceiverPosition,
	FHitResult& OutBlockingHit) const
{
	return HasLineOfSightFrom(ReceiverObject, SourcePosition, ReceiverPosition, OutBlockingHit, nullptr, nullptr);
}

bool UUOULightExposureSourceComponent::HasLineOfSightFrom(
	UObject* ReceiverObject,
	const FVector& TraceStart,
	const FVector& ReceiverPosition,
	FHitResult& OutBlockingHit,
	const AActor* IgnoredActor,
	const UPrimitiveComponent* IgnoredComponent) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOULightOcclusionTrace), false, GetOwner());
	if (bIgnoreOwner && GetOwner() != nullptr)
	{
		QueryParams.AddIgnoredActor(GetOwner());
	}

	if (IgnoredActor != nullptr)
	{
		AddActorPrimitiveComponentsToIgnore(IgnoredActor, QueryParams);
	}

	if (IgnoredComponent != nullptr)
	{
		QueryParams.AddIgnoredComponent(IgnoredComponent);
	}

	AddReceiverSelfComponentsToIgnore(ReceiverObject, QueryParams);

	const FVector ToReceiver = ReceiverPosition - TraceStart;
	const float TraceDistance = FMath::Max(0.0f, ToReceiver.Size() - 2.0f);
	if (TraceDistance <= KINDA_SMALL_NUMBER)
	{
		return true;
	}

	const FVector TraceEnd = TraceStart + ToReceiver.GetSafeNormal() * TraceDistance;
	return !World->LineTraceSingleByChannel(OutBlockingHit, TraceStart, TraceEnd, OcclusionTraceChannel, QueryParams);
}

bool UUOULightExposureSourceComponent::IsWorldPositionInsideUmbrellaLightShade(const FVector& WorldPosition) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOUUmbrellaLightShadeOverlap), false, GetOwner());
	const bool bHasOverlaps = World->OverlapMultiByObjectType(
		OverlapResults,
		WorldPosition,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(1.0f),
		QueryParams);
	if (!bHasOverlaps)
	{
		return false;
	}

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		const UUOUUmbrellaLightShadeVolumeComponent* ShadeVolume =
			Cast<UUOUUmbrellaLightShadeVolumeComponent>(OverlapResult.GetComponent());
		if (ShadeVolume != nullptr && ShadeVolume->ContainsWorldPosition(WorldPosition))
		{
			return true;
		}
	}

	return false;
}

bool UUOULightExposureSourceComponent::TryBuildLightInteractionSurfaceHit(
	UUOULightInteractionSurfaceComponent* SurfaceComponent,
	FHitResult& OutSurfaceHit) const
{
	OutSurfaceHit = FHitResult();

	UWorld* World = GetWorld();
	if (World == nullptr || SurfaceComponent == nullptr || !SurfaceComponent->CanReflectLight())
	{
		return false;
	}

	const FVector SourcePosition = GetSourceLocation();
	const FVector SurfacePosition = SurfaceComponent->GetComponentLocation();
	const FVector ToSurface = SurfacePosition - SourcePosition;
	const float SurfaceDistance = ToSurface.Size();
	const float ExposureRange = GetExposureRange();
	if (SurfaceDistance <= KINDA_SMALL_NUMBER || SurfaceDistance > ExposureRange)
	{
		return false;
	}

	const FVector DirectionToSurface = ToSurface / SurfaceDistance;
	const FVector SourceForward = GetSourceForwardVector().GetSafeNormal();
	const float Dot = FMath::Clamp(FVector::DotProduct(SourceForward, DirectionToSurface), -1.0f, 1.0f);
	const float SurfaceAngle = FMath::RadiansToDegrees(FMath::Acos(Dot));
	if (SurfaceAngle > GetEffectiveOuterConeAngle())
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOULightInteractionSurfaceTrace), false, GetOwner());
	if (bIgnoreOwner && GetOwner() != nullptr)
	{
		QueryParams.AddIgnoredActor(GetOwner());
	}

	const FVector TraceEnd = SurfacePosition + DirectionToSurface * SurfaceComponent->ReflectionStartPadding;
	if (!World->LineTraceSingleByChannel(OutSurfaceHit, SourcePosition, TraceEnd, OcclusionTraceChannel, QueryParams))
	{
		return false;
	}

	return OutSurfaceHit.GetComponent() == SurfaceComponent &&
		SurfaceComponent->CanReflectIncomingLight(DirectionToSurface, OutSurfaceHit.ImpactNormal);
}

void UUOULightExposureSourceComponent::EmitReflectedLightFromSurface(
	UUOULightInteractionSurfaceComponent* SurfaceComponent,
	const FHitResult& SurfaceHit,
	float DeltaTime,
	TSet<UObject*>& LitReceivers)
{
	UWorld* World = GetWorld();
	if (World == nullptr || SurfaceComponent == nullptr || DeltaTime <= 0.0f)
	{
		return;
	}

	const FVector SourcePosition = GetSourceLocation();
	const FVector InitialIncomingDirection = (SurfaceHit.ImpactPoint - SourcePosition).GetSafeNormal();
	if (InitialIncomingDirection.IsNearlyZero())
	{
		return;
	}

	const FVector SourceForward = GetSourceForwardVector().GetSafeNormal();
	const float SurfaceDistance = FVector::Dist(SourcePosition, SurfaceHit.ImpactPoint);
	const float Dot = FMath::Clamp(FVector::DotProduct(SourceForward, InitialIncomingDirection), -1.0f, 1.0f);
	const float SurfaceAngle = FMath::RadiansToDegrees(FMath::Acos(Dot));

	float SourceDistanceFactor = 0.0f;
	float SourceAngleFactor = 0.0f;
	float IncomingSurfaceIntensity = CalculateIntensity(
		SurfaceDistance,
		SurfaceAngle,
		SourceDistanceFactor,
		SourceAngleFactor);
	if (IncomingSurfaceIntensity <= 0.0f)
	{
		return;
	}

	UUOULightInteractionSurfaceComponent* CurrentSurface = SurfaceComponent;
	FHitResult CurrentSurfaceHit = SurfaceHit;
	FVector CurrentRayOrigin = SourcePosition;
	TSet<const UUOULightInteractionSurfaceComponent*> VisitedSurfaces;
	TArray<FString> ReflectionPath;
	FUOULightReflectionPathData PathData;
	PathData.PathIndex = ReflectionPaths.Num();
	PathData.SourcePosition = SourcePosition;
	int32 ReflectionDepth = 0;
	float IncomingBeamRadius = SurfaceDistance * FMath::Tan(
		FMath::DegreesToRadians(FMath::Clamp(GetEffectiveOuterConeAngle(), 1.0f, 89.0f)));

	while (CurrentSurface != nullptr && ReflectionDepth < MaxReflectionBouncesPerPath)
	{
		if (VisitedSurfaces.Contains(CurrentSurface))
		{
			break;
		}

		const FVector IncomingDirection = (CurrentSurfaceHit.ImpactPoint - CurrentRayOrigin).GetSafeNormal();
		const FVector ReflectedDirection = CurrentSurface->GetReflectionDirection(
			IncomingDirection,
			CurrentSurfaceHit.ImpactNormal);
		if (IncomingDirection.IsNearlyZero() || ReflectedDirection.IsNearlyZero())
		{
			PathData.EndReason = EUOULightReflectionPathEndReason::InvalidReflection;
			break;
		}
		if (!CurrentSurface->CanReflectIncomingLight(
			IncomingDirection,
			CurrentSurfaceHit.ImpactNormal))
		{
			PathData.EndReason = EUOULightReflectionPathEndReason::InvalidReflection;
			break;
		}

		const float CurrentBeamStartRadius = CurrentSurface->ClampReflectionBeamRadius(
			IncomingBeamRadius,
			IncomingDirection,
			CurrentSurfaceHit.ImpactNormal);

		VisitedSurfaces.Add(CurrentSurface);
		ReflectionPath.Add(FString::Printf(
			TEXT("%s.%s"),
			*GetNameSafe(CurrentSurface->GetOwner()),
			*GetNameSafe(CurrentSurface)));
		++ReflectionDepth;
		LastReflectorName = GetNameSafe(CurrentSurface);

		const FVector ReflectionOrigin =
			CurrentSurfaceHit.ImpactPoint + ReflectedDirection * CurrentSurface->ReflectionStartPadding;

		UUOULightInteractionSurfaceComponent* NextSurface = nullptr;
		FHitResult NextSurfaceHit;
		float NextSurfaceDistance = 0.0f;
		float NextSurfaceAngle = 0.0f;
		const bool bHasNextSurface = ReflectionDepth < MaxReflectionBouncesPerPath &&
			TryFindNextReflectionSurface(
				CurrentSurface,
				ReflectionOrigin,
				ReflectedDirection,
				CurrentBeamStartRadius,
				VisitedSurfaces,
				NextSurface,
				NextSurfaceHit,
				NextSurfaceDistance,
				NextSurfaceAngle);

		FVector SegmentEnd = bHasNextSurface
			? NextSurfaceHit.ImpactPoint
			: ReflectionOrigin + ReflectedDirection * CurrentSurface->ReflectionRange;
		FHitResult SegmentBlockingHit;
		if (!bHasNextSurface)
		{
			FCollisionQueryParams SegmentTraceQueryParams(
				SCENE_QUERY_STAT(UOULightReflectionSegmentTrace),
				false,
				GetOwner());
			if (CurrentSurface->GetOwner() != nullptr)
			{
				AddActorPrimitiveComponentsToIgnore(
					CurrentSurface->GetOwner(),
					SegmentTraceQueryParams);
			}

			const FVector SegmentTraceEnd =
				ReflectionOrigin + ReflectedDirection * CurrentSurface->ReflectionRange;
			if (World->LineTraceSingleByChannel(
				SegmentBlockingHit,
				ReflectionOrigin,
				SegmentTraceEnd,
				OcclusionTraceChannel,
				SegmentTraceQueryParams))
			{
				SegmentEnd = SegmentBlockingHit.ImpactPoint;
			}
		}

		const float SegmentLength = FVector::Dist(ReflectionOrigin, SegmentEnd);
		const float ReflectedIntensity = CalculateReflectedSegmentIntensity(
			CurrentSurface,
			IncomingSurfaceIntensity,
			0.0f,
			0.0f);
		const float BeamEndRadius = CurrentBeamStartRadius + SegmentLength * FMath::Tan(
			FMath::DegreesToRadians(FMath::Clamp(CurrentSurface->ReflectionConeAngle, 1.0f, 89.0f)));

		const float NextSurfaceIntensity = bHasNextSurface
			? CalculateReflectedSegmentIntensity(
				CurrentSurface,
				IncomingSurfaceIntensity,
				NextSurfaceDistance,
				NextSurfaceAngle)
			: 0.0f;
		FColor ReflectionDebugColor = FColor::White;
		if (ReflectionDepth >= MaxReflectionBouncesPerPath)
		{
			ReflectionDebugColor = FColor::Purple;
		}
		else if (bHasNextSurface && NextSurfaceIntensity <= MinimumReflectedIntensity)
		{
			ReflectionDebugColor = FColor::Yellow;
		}
		else if (bHasNextSurface)
		{
			ReflectionDebugColor = FColor::Blue;
		}
		else if (SegmentBlockingHit.bBlockingHit)
		{
			ReflectionDebugColor = FColor::Red;
		}

		DrawDebugReflectionFrustum(
			ReflectionOrigin,
			ReflectedDirection,
			SegmentLength,
			CurrentSurface->ReflectionConeAngle,
			CurrentBeamStartRadius,
			ReflectionDebugColor);

		TArray<TObjectPtr<UObject>> ReachedReceivers;
		EmitReflectedLightToReceivers(
			CurrentSurface,
			ReflectionOrigin,
			ReflectedDirection,
			CurrentBeamStartRadius,
			IncomingSurfaceIntensity,
			DeltaTime,
			LitReceivers,
			ReachedReceivers);

		FUOULightReflectionSegmentData& SegmentData = PathData.Segments.AddDefaulted_GetRef();
		SegmentData.BounceIndex = ReflectionDepth;
		SegmentData.Reflector = CurrentSurface;
		SegmentData.NextReflector = NextSurface;
		SegmentData.BlockingComponent = SegmentBlockingHit.GetComponent();
		SegmentData.ReachedReceivers = MoveTemp(ReachedReceivers);
		SegmentData.IncomingStart = CurrentRayOrigin;
		SegmentData.ImpactPoint = CurrentSurfaceHit.ImpactPoint;
		SegmentData.ReflectionStart = ReflectionOrigin;
		SegmentData.SegmentEnd = SegmentEnd;
		SegmentData.IncomingDirection = IncomingDirection;
		SegmentData.ReflectedDirection = ReflectedDirection;
		SegmentData.SegmentLength = SegmentLength;
		SegmentData.BeamStartRadius = CurrentBeamStartRadius;
		SegmentData.BeamEndRadius = BeamEndRadius;
		SegmentData.IncomingIntensity = IncomingSurfaceIntensity;
		SegmentData.ReflectedIntensity = ReflectedIntensity;
		PathData.FinalIntensity = ReflectedIntensity;

		if (ReflectionDepth >= MaxReflectionBouncesPerPath)
		{
			PathData.EndReason = EUOULightReflectionPathEndReason::MaxBounces;
			break;
		}

		if (!bHasNextSurface || NextSurface == nullptr)
		{
			PathData.EndReason = SegmentBlockingHit.bBlockingHit
				? EUOULightReflectionPathEndReason::Blocked
				: EUOULightReflectionPathEndReason::RangeEnded;
			break;
		}

		if (NextSurfaceIntensity <= MinimumReflectedIntensity)
		{
			PathData.FinalIntensity = NextSurfaceIntensity;
			PathData.EndReason = EUOULightReflectionPathEndReason::MinimumIntensity;
			break;
		}

		IncomingBeamRadius = CurrentBeamStartRadius + NextSurfaceDistance * FMath::Tan(
			FMath::DegreesToRadians(FMath::Clamp(CurrentSurface->ReflectionConeAngle, 1.0f, 89.0f)));
		CurrentRayOrigin = ReflectionOrigin;
		CurrentSurface = NextSurface;
		CurrentSurfaceHit = NextSurfaceHit;
		IncomingSurfaceIntensity = NextSurfaceIntensity;
	}

	if (!PathData.Segments.IsEmpty())
	{
		ReflectionPaths.Add(MoveTemp(PathData));
	}

	if (ReflectionDepth >= LastReflectionBounceCount)
	{
		LastReflectionBounceCount = ReflectionDepth;
		LastReflectionPath = ReflectionPath.IsEmpty()
			? TEXT("None")
			: FString::Join(ReflectionPath, TEXT(" -> "));
	}
}

void UUOULightExposureSourceComponent::EmitReflectedLightToReceivers(
	UUOULightInteractionSurfaceComponent* SurfaceComponent,
	const FVector& ReflectionOrigin,
	const FVector& ReflectedDirection,
	float BeamStartRadius,
	float SurfaceIntensity,
	float DeltaTime,
	TSet<UObject*>& LitReceivers,
	TArray<TObjectPtr<UObject>>& OutReachedReceivers)
{
	UWorld* World = GetWorld();
	if (World == nullptr ||
		SurfaceComponent == nullptr ||
		ReflectedDirection.IsNearlyZero() ||
		SurfaceIntensity <= 0.0f ||
		DeltaTime <= 0.0f)
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOULightReflectionReceiverOverlap), false, GetOwner());
	if (bIgnoreOwner && GetOwner() != nullptr)
	{
		QueryParams.AddIgnoredActor(GetOwner());
	}

	if (SurfaceComponent->GetOwner() != nullptr)
	{
		QueryParams.AddIgnoredActor(SurfaceComponent->GetOwner());
	}
	QueryParams.AddIgnoredComponent(SurfaceComponent);

	const bool bHasOverlaps = World->OverlapMultiByObjectType(
		OverlapResults,
		ReflectionOrigin,
		FQuat::Identity,
		BuildReceiverObjectQueryParams(),
		FCollisionShape::MakeSphere(SurfaceComponent->ReflectionRange),
		QueryParams);

	if (!bHasOverlaps)
	{
		return;
	}

	TSet<UObject*> ProcessedReflectionReceivers;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		TArray<UObject*> Receivers;
		AppendReceivableObjects(OverlapResult.GetActor(), OverlapResult.GetComponent(), Receivers);
		for (UObject* ReceiverObject : Receivers)
		{
			if (ReceiverObject == nullptr ||
				LitReceivers.Contains(ReceiverObject) ||
				ProcessedReflectionReceivers.Contains(ReceiverObject))
			{
				continue;
			}

			ProcessedReflectionReceivers.Add(ReceiverObject);

			FUOULightExposureData ExposureData;
			FHitResult BlockingHit;
			if (TryBuildReflectedExposureData(
				ReceiverObject,
				SurfaceComponent,
				ReflectionOrigin,
				ReflectedDirection,
				BeamStartRadius,
				SurfaceIntensity,
				DeltaTime,
				ExposureData,
				BlockingHit))
			{
				IUOULightReceivableInterface::Execute_ReceiveLightExposure(ReceiverObject, ExposureData);
				LitReceivers.Add(ReceiverObject);
				OutReachedReceivers.Add(ReceiverObject);
				++LastLitCount;
				++LastReflectedCount;
				LastLitTargetName = GetReceivableDebugName(ReceiverObject);
				DrawDebugResult(ExposureData, true);
				continue;
			}

			if (BlockingHit.bBlockingHit)
			{
				++LastBlockedCount;
				LastBlockedName = GetNameSafe(BlockingHit.GetComponent());
				DrawDebugBlockedHit(ReflectionOrigin, BlockingHit);
			}
		}
	}
}

bool UUOULightExposureSourceComponent::TryFindNextReflectionSurface(
	const UUOULightInteractionSurfaceComponent* CurrentSurface,
	const FVector& ReflectionOrigin,
	const FVector& ReflectedDirection,
	float BeamStartRadius,
	const TSet<const UUOULightInteractionSurfaceComponent*>& VisitedSurfaces,
	UUOULightInteractionSurfaceComponent*& OutSurface,
	FHitResult& OutSurfaceHit,
	float& OutDistance,
	float& OutAngle) const
{
	OutSurface = nullptr;
	OutSurfaceHit = FHitResult();
	OutDistance = 0.0f;
	OutAngle = 0.0f;

	UWorld* World = GetWorld();
	if (World == nullptr ||
		CurrentSurface == nullptr ||
		ReflectedDirection.IsNearlyZero() ||
		CurrentSurface->ReflectionRange <= 0.0f)
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams OverlapQueryParams(SCENE_QUERY_STAT(UOULightReflectionSurfaceOverlap), false, GetOwner());
	if (bIgnoreOwner && GetOwner() != nullptr)
	{
		OverlapQueryParams.AddIgnoredActor(GetOwner());
	}
	if (CurrentSurface->GetOwner() != nullptr)
	{
		OverlapQueryParams.AddIgnoredActor(CurrentSurface->GetOwner());
	}

	TArray<FOverlapResult> OverlapResults;
	if (!World->OverlapMultiByObjectType(
		OverlapResults,
		ReflectionOrigin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(CurrentSurface->ReflectionRange),
		OverlapQueryParams))
	{
		return false;
	}

	const FVector SafeReflectedDirection = ReflectedDirection.GetSafeNormal();
	float ClosestHitDistance = TNumericLimits<float>::Max();
	TSet<UUOULightInteractionSurfaceComponent*> ProcessedSurfaces;

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		TArray<UUOULightInteractionSurfaceComponent*> CandidateSurfaces;
		AppendLightInteractionSurfaces(
			OverlapResult.GetActor(),
			OverlapResult.GetComponent(),
			CandidateSurfaces);

		for (UUOULightInteractionSurfaceComponent* CandidateSurface : CandidateSurfaces)
		{
			if (CandidateSurface == nullptr ||
				CandidateSurface == CurrentSurface ||
				VisitedSurfaces.Contains(CandidateSurface) ||
				ProcessedSurfaces.Contains(CandidateSurface) ||
				!CandidateSurface->CanReflectLight())
			{
				continue;
			}

			ProcessedSurfaces.Add(CandidateSurface);

			const FVector ToCandidate = CandidateSurface->GetComponentLocation() - ReflectionOrigin;
			const float CandidateDistance = ToCandidate.Size();
			if (CandidateDistance <= KINDA_SMALL_NUMBER ||
				CandidateDistance > CurrentSurface->ReflectionRange ||
				CandidateDistance >= ClosestHitDistance)
			{
				continue;
			}

			const FVector DirectionToCandidate = ToCandidate / CandidateDistance;
			const float AxialDistance = FVector::DotProduct(ToCandidate, SafeReflectedDirection);
			if (AxialDistance <= KINDA_SMALL_NUMBER)
			{
				continue;
			}
			const float RadialDistance =
				(ToCandidate - SafeReflectedDirection * AxialDistance).Size();
			const float ConeAngleRadians = FMath::DegreesToRadians(
				FMath::Clamp(CurrentSurface->ReflectionConeAngle, 1.0f, 89.0f));
			const float SafeBeamStartRadius = FMath::Max(0.0f, BeamStartRadius);
			const float MaximumBeamRadius =
				SafeBeamStartRadius + AxialDistance * FMath::Tan(ConeAngleRadians);
			if (RadialDistance > MaximumBeamRadius)
			{
				continue;
			}
			const float CandidateAngle = FMath::RadiansToDegrees(FMath::Atan2(
				FMath::Max(0.0f, RadialDistance - SafeBeamStartRadius),
				AxialDistance));

			FCollisionQueryParams TraceQueryParams(
				SCENE_QUERY_STAT(UOULightChainedReflectionSurfaceTrace),
				false,
				GetOwner());
			if (bIgnoreOwner && GetOwner() != nullptr)
			{
				TraceQueryParams.AddIgnoredActor(GetOwner());
			}
			if (CurrentSurface->GetOwner() != nullptr)
			{
				AddActorPrimitiveComponentsToIgnore(CurrentSurface->GetOwner(), TraceQueryParams);
			}

			FHitResult CandidateHit;
			const FVector TraceEnd = CandidateSurface->GetComponentLocation() +
				DirectionToCandidate * CandidateSurface->ReflectionStartPadding;
			if (!World->LineTraceSingleByChannel(
				CandidateHit,
				ReflectionOrigin,
				TraceEnd,
				OcclusionTraceChannel,
				TraceQueryParams) ||
				CandidateHit.GetComponent() != CandidateSurface)
			{
				continue;
			}
			if (!CandidateSurface->CanReflectIncomingLight(
				SafeReflectedDirection,
				CandidateHit.ImpactNormal))
			{
				continue;
			}

			const float HitDistance = FVector::Dist(ReflectionOrigin, CandidateHit.ImpactPoint);
			if (HitDistance >= ClosestHitDistance)
			{
				continue;
			}

			ClosestHitDistance = HitDistance;
			OutSurface = CandidateSurface;
			OutSurfaceHit = CandidateHit;
			OutDistance = HitDistance;
			OutAngle = CandidateAngle;
		}
	}

	return OutSurface != nullptr;
}

float UUOULightExposureSourceComponent::CalculateReflectedSegmentIntensity(
	const UUOULightInteractionSurfaceComponent* SurfaceComponent,
	float IncomingIntensity,
	float Distance,
	float Angle) const
{
	if (SurfaceComponent == nullptr || IncomingIntensity <= 0.0f)
	{
		return 0.0f;
	}

	const float DistanceFactor = bUseDistanceFalloff && SurfaceComponent->ReflectionRange > 0.0f
		? 1.0f - FMath::Clamp(Distance / SurfaceComponent->ReflectionRange, 0.0f, 1.0f)
		: 1.0f;
	const float AngleFactor = bUseAngleFalloff
		? CalculateConeFactor(Angle, SurfaceComponent->ReflectionConeAngle)
		: 1.0f;

	return IncomingIntensity *
		SurfaceComponent->ReflectionIntensityMultiplier *
		FMath::Clamp(DistanceFactor, 0.0f, 1.0f) *
		FMath::Clamp(AngleFactor, 0.0f, 1.0f);
}

bool UUOULightExposureSourceComponent::TryBuildReflectedExposureData(
	UObject* ReceiverObject,
	const UUOULightInteractionSurfaceComponent* SurfaceComponent,
	const FVector& ReflectionOrigin,
	const FVector& ReflectedDirection,
	float BeamStartRadius,
	float SurfaceIntensity,
	float DeltaTime,
	FUOULightExposureData& OutExposureData,
	FHitResult& OutBlockingHit) const
{
	OutExposureData = FUOULightExposureData();
	OutBlockingHit = FHitResult();

	if (ReceiverObject == nullptr ||
		SurfaceComponent == nullptr ||
		!ReceiverObject->GetClass()->ImplementsInterface(UUOULightReceivableInterface::StaticClass()))
	{
		return false;
	}

	const FVector ReceiverPosition = IUOULightReceivableInterface::Execute_GetLightReceiverPosition(ReceiverObject);
	const FVector ToReceiver = ReceiverPosition - ReflectionOrigin;
	const float Distance = ToReceiver.Size();
	if (Distance <= KINDA_SMALL_NUMBER || Distance > SurfaceComponent->ReflectionRange)
	{
		return false;
	}

	const FVector SafeReflectedDirection = ReflectedDirection.GetSafeNormal();
	const FVector DirectionToReceiver = ToReceiver / Distance;
	const float AxialDistance = FVector::DotProduct(ToReceiver, SafeReflectedDirection);
	if (AxialDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const float RadialDistance =
		(ToReceiver - SafeReflectedDirection * AxialDistance).Size();
	const float ConeAngleRadians = FMath::DegreesToRadians(
		FMath::Clamp(SurfaceComponent->ReflectionConeAngle, 1.0f, 89.0f));
	const float SafeBeamStartRadius = FMath::Max(0.0f, BeamStartRadius);
	const float MaximumBeamRadius =
		SafeBeamStartRadius + AxialDistance * FMath::Tan(ConeAngleRadians);
	if (RadialDistance > MaximumBeamRadius)
	{
		return false;
	}
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(
		FMath::Max(0.0f, RadialDistance - SafeBeamStartRadius),
		AxialDistance));

	if (IsWorldPositionInsideUmbrellaLightShade(ReceiverPosition))
	{
		return false;
	}

	if (bRequireLineOfSight &&
		!HasLineOfSightFrom(
			ReceiverObject,
			ReflectionOrigin,
			ReceiverPosition,
			OutBlockingHit,
			SurfaceComponent->GetOwner(),
			SurfaceComponent))
	{
		return false;
	}

	const float DistanceFactor = bUseDistanceFalloff
		? 1.0f - FMath::Clamp(Distance / SurfaceComponent->ReflectionRange, 0.0f, 1.0f)
		: 1.0f;
	const float AngleFactor = bUseAngleFalloff ? CalculateConeFactor(Angle, SurfaceComponent->ReflectionConeAngle) : 1.0f;
	const float FinalIntensity = SurfaceIntensity *
		SurfaceComponent->ReflectionIntensityMultiplier *
		FMath::Clamp(DistanceFactor, 0.0f, 1.0f) *
		FMath::Clamp(AngleFactor, 0.0f, 1.0f);
	if (FinalIntensity <= 0.0f)
	{
		return false;
	}

	OutExposureData = FUOULightExposureData(
		const_cast<UUOULightExposureSourceComponent*>(this),
		ReflectionOrigin,
		ReceiverPosition,
		DirectionToReceiver,
		Distance,
		FinalIntensity,
		DistanceFactor,
		AngleFactor,
		DeltaTime);

	return true;
}

float UUOULightExposureSourceComponent::CalculateIntensity(
	float Distance,
	float Angle,
	float& OutDistanceFactor,
	float& OutAngleFactor) const
{
	const float ExposureRange = GetExposureRange();
	OutDistanceFactor = bUseDistanceFalloff && ExposureRange > 0.0f
		? 1.0f - FMath::Clamp(Distance / ExposureRange, 0.0f, 1.0f)
		: 1.0f;

	const float OuterConeAngle = GetEffectiveOuterConeAngle();
	const float InnerConeAngle = GetEffectiveInnerConeAngle(OuterConeAngle);
	if (bUseAngleFalloff)
	{
		const float AngleRange = FMath::Max(0.001f, OuterConeAngle - InnerConeAngle);
		OutAngleFactor = 1.0f - FMath::Clamp((Angle - InnerConeAngle) / AngleRange, 0.0f, 1.0f);
	}
	else
	{
		OutAngleFactor = 1.0f;
	}

	return Intensity * FMath::Clamp(OutDistanceFactor, 0.0f, 1.0f) * FMath::Clamp(OutAngleFactor, 0.0f, 1.0f);
}

float UUOULightExposureSourceComponent::CalculateConeFactor(float Angle, float ConeAngle) const
{
	const float SafeConeAngle = FMath::Max(0.001f, ConeAngle);
	return 1.0f - FMath::Clamp(Angle / SafeConeAngle, 0.0f, 1.0f);
}

void UUOULightExposureSourceComponent::DrawDebugSource() const
{
	if (!bDrawDebug || !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const FVector SourcePosition = GetSourceLocation();
	const FVector SourceForward = GetSourceForwardVector().GetSafeNormal();
	const float ExposureRange = GetExposureRange();
	if (SourceForward.IsNearlyZero() || ExposureRange <= 0.0f)
	{
		return;
	}

	const float OuterConeRadians = FMath::DegreesToRadians(GetEffectiveOuterConeAngle());
	const FColor SourceDebugColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::Puzzle, FColor::Cyan);
	const FColor ConeDebugColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::Puzzle, FColor::Yellow);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOULightSourceDebugTrace), false, GetOwner());
	if (bIgnoreOwner && GetOwner() != nullptr)
	{
		QueryParams.AddIgnoredActor(GetOwner());
	}

	const auto FindClippedRayEnd =
		[this, World, &QueryParams, &SourcePosition, ExposureRange](const FVector& RayDirection)
		{
			const FVector TraceEnd = SourcePosition + RayDirection * ExposureRange;
			FHitResult BlockingHit;
			return World->LineTraceSingleByChannel(
				BlockingHit,
				SourcePosition,
				TraceEnd,
				OcclusionTraceChannel,
				QueryParams)
				? BlockingHit.ImpactPoint
				: TraceEnd;
		};

	DrawDebugPoint(World, SourcePosition, 10.0f, SourceDebugColor, false, DebugDrawTime);
	DrawDebugLine(
		World,
		SourcePosition,
		FindClippedRayEnd(SourceForward),
		SourceDebugColor,
		false,
		DebugDrawTime,
		0,
		1.5f);

	FVector ConeAxisX = FVector::ZeroVector;
	FVector ConeAxisY = FVector::ZeroVector;
	SourceForward.FindBestAxisVectors(ConeAxisX, ConeAxisY);
	constexpr int32 ConeSegments = 24;
	const float ForwardScale = FMath::Cos(OuterConeRadians);
	const float RadiusScale = FMath::Sin(OuterConeRadians);
	for (int32 SegmentIndex = 0; SegmentIndex < ConeSegments; ++SegmentIndex)
	{
		const float Angle =
			UE_TWO_PI * static_cast<float>(SegmentIndex) / static_cast<float>(ConeSegments);
		const FVector RadiusDirection =
			ConeAxisX * FMath::Cos(Angle) + ConeAxisY * FMath::Sin(Angle);
		const FVector RayDirection =
			(SourceForward * ForwardScale + RadiusDirection * RadiusScale).GetSafeNormal();
		DrawDebugLine(
			World,
			SourcePosition,
			FindClippedRayEnd(RayDirection),
			ConeDebugColor,
			false,
			DebugDrawTime,
			0,
			1.0f);
	}
}

void UUOULightExposureSourceComponent::DrawDebugResult(const FUOULightExposureData& ExposureData, bool bLit) const
{
	if (!bDrawDebug || !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const FColor ResultColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::Puzzle, bLit ? FColor::Green : FColor::Red);
		DrawDebugLine(
			World,
			ExposureData.SourcePosition,
			ExposureData.ReceiverPosition,
			ResultColor,
			false,
			DebugDrawTime,
			0,
			2.0f);
	}
}

void UUOULightExposureSourceComponent::DrawDebugBlockedHit(const FVector& SourcePosition, const FHitResult& BlockingHit) const
{
	if (!bDrawDebug
		|| !BlockingHit.bBlockingHit
		|| !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const FColor BlockedColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::Puzzle, FColor::Red);
		DrawDebugLine(World, SourcePosition, BlockingHit.ImpactPoint, BlockedColor, false, DebugDrawTime, 0, 2.0f);
		DrawDebugPoint(World, BlockingHit.ImpactPoint, 8.0f, BlockedColor, false, DebugDrawTime);
	}
}

void UUOULightExposureSourceComponent::DrawDebugReflectionFrustum(
	const FVector& Start,
	const FVector& Direction,
	float Length,
	float ConeAngleDegrees,
	float StartRadius,
	const FColor& Color) const
{
	if (!bDrawDebug || !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	const FVector SafeDirection = Direction.GetSafeNormal();
	const float SafeLength = FMath::Max(0.0f, Length);
	if (SafeDirection.IsNearlyZero() || SafeLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const FVector End = Start + SafeDirection * SafeLength;
		const float ConeAngleRadians = FMath::DegreesToRadians(
			FMath::Clamp(ConeAngleDegrees, 1.0f, 89.0f));
		const float SafeStartRadius = FMath::Max(0.0f, StartRadius);
		const float EndRadius = SafeStartRadius + SafeLength * FMath::Tan(ConeAngleRadians);
		constexpr int32 ConeSegments = 24;
		FVector RadiusAxisX = FVector::ZeroVector;
		FVector RadiusAxisY = FVector::ZeroVector;
		SafeDirection.FindBestAxisVectors(RadiusAxisX, RadiusAxisY);

		DrawDebugLine(World, Start, End, Color, false, DebugDrawTime, 0, 1.5f);
		for (int32 SegmentIndex = 0; SegmentIndex < ConeSegments; ++SegmentIndex)
		{
			const float Angle = UE_TWO_PI * static_cast<float>(SegmentIndex) / static_cast<float>(ConeSegments);
			const float NextAngle =
				UE_TWO_PI * static_cast<float>(SegmentIndex + 1) / static_cast<float>(ConeSegments);
			const FVector RadiusDirection =
				RadiusAxisX * FMath::Cos(Angle) + RadiusAxisY * FMath::Sin(Angle);
			const FVector NextRadiusDirection =
				RadiusAxisX * FMath::Cos(NextAngle) + RadiusAxisY * FMath::Sin(NextAngle);
			const FVector StartPoint = Start + RadiusDirection * SafeStartRadius;
			const FVector NextStartPoint = Start + NextRadiusDirection * SafeStartRadius;
			const FVector EndPoint = End + RadiusDirection * EndRadius;
			const FVector NextEndPoint = End + NextRadiusDirection * EndRadius;

			DrawDebugLine(World, StartPoint, EndPoint, Color, false, DebugDrawTime, 0, 1.0f);
			DrawDebugLine(World, EndPoint, NextEndPoint, Color, false, DebugDrawTime, 0, 1.0f);
			if (SafeStartRadius > KINDA_SMALL_NUMBER)
			{
				DrawDebugLine(
					World,
					StartPoint,
					NextStartPoint,
					Color,
					false,
					DebugDrawTime,
					0,
					1.0f);
			}
		}
		DrawDebugPoint(World, Start, 8.0f, Color, false, DebugDrawTime);
	}
}

void UUOULightExposureSourceComponent::AddActorPrimitiveComponentsToIgnore(
	const AActor* Actor,
	FCollisionQueryParams& QueryParams,
	const UPrimitiveComponent* ComponentToKeep)
{
	if (Actor == nullptr)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Actor);
	for (const UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent != ComponentToKeep)
		{
			QueryParams.AddIgnoredComponent(PrimitiveComponent);
		}
	}
}

void UUOULightExposureSourceComponent::AddReceiverSelfComponentsToIgnore(
	UObject* ReceiverObject,
	FCollisionQueryParams& QueryParams)
{
	const AActor* ReceiverActor = ResolveReceiverActor(ReceiverObject);
	if (ReceiverActor == nullptr)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(ReceiverActor);
	for (const UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent == nullptr ||
			PrimitiveComponent->IsA<UUOULightInteractionSurfaceComponent>())
		{
			continue;
		}

		QueryParams.AddIgnoredComponent(PrimitiveComponent);
	}
}

AActor* UUOULightExposureSourceComponent::ResolveReceiverActor(UObject* ReceiverObject)
{
	if (AActor* ReceiverActor = Cast<AActor>(ReceiverObject))
	{
		return ReceiverActor;
	}

	if (const UActorComponent* ReceiverComponent = Cast<UActorComponent>(ReceiverObject))
	{
		return ReceiverComponent->GetOwner();
	}

	return nullptr;
}

FString UUOULightExposureSourceComponent::GetReceivableDebugName(UObject* ReceiverObject)
{
	if (const AActor* ReceiverActor = Cast<AActor>(ReceiverObject))
	{
		return ReceiverActor->GetName();
	}

	if (const UActorComponent* ReceiverComponent = Cast<UActorComponent>(ReceiverObject))
	{
		return FString::Printf(TEXT("%s.%s"), *GetNameSafe(ReceiverComponent->GetOwner()), *ReceiverComponent->GetName());
	}

	return GetNameSafe(ReceiverObject);
}
