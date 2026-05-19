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

	UWorld* World = GetWorld();
	const float ExposureRange = GetExposureRange();
	if (World == nullptr || DeltaTime <= 0.0f || ExposureRange <= 0.0f || Intensity <= 0.0f)
	{
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
	DebugDrawTime = FMath::Max(0.0f, DebugDrawTime);
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

	return OutSurfaceHit.GetComponent() == SurfaceComponent;
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
	const FVector IncomingDirection = (SurfaceHit.ImpactPoint - SourcePosition).GetSafeNormal();
	const FVector ReflectedDirection = SurfaceComponent->GetReflectionDirection(IncomingDirection, SurfaceHit.ImpactNormal);
	if (IncomingDirection.IsNearlyZero() || ReflectedDirection.IsNearlyZero())
	{
		return;
	}

	const FVector SourceForward = GetSourceForwardVector().GetSafeNormal();
	const float SurfaceDistance = FVector::Dist(SourcePosition, SurfaceHit.ImpactPoint);
	const float Dot = FMath::Clamp(FVector::DotProduct(SourceForward, IncomingDirection), -1.0f, 1.0f);
	const float SurfaceAngle = FMath::RadiansToDegrees(FMath::Acos(Dot));

	float SourceDistanceFactor = 0.0f;
	float SourceAngleFactor = 0.0f;
	const float SurfaceIntensity = CalculateIntensity(SurfaceDistance, SurfaceAngle, SourceDistanceFactor, SourceAngleFactor);
	if (SurfaceIntensity <= 0.0f)
	{
		return;
	}

	const FVector ReflectionOrigin = SurfaceHit.ImpactPoint + ReflectedDirection * SurfaceComponent->ReflectionStartPadding;
	const FVector DebugReflectionEnd = ReflectionOrigin + ReflectedDirection * SurfaceComponent->ReflectionRange;
	DrawDebugReflectionRay(ReflectionOrigin, DebugReflectionEnd);

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
				SurfaceIntensity,
				DeltaTime,
				ExposureData,
				BlockingHit))
			{
				IUOULightReceivableInterface::Execute_ReceiveLightExposure(ReceiverObject, ExposureData);
				LitReceivers.Add(ReceiverObject);
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

bool UUOULightExposureSourceComponent::TryBuildReflectedExposureData(
	UObject* ReceiverObject,
	const UUOULightInteractionSurfaceComponent* SurfaceComponent,
	const FVector& ReflectionOrigin,
	const FVector& ReflectedDirection,
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

	const FVector DirectionToReceiver = ToReceiver / Distance;
	const float Dot = FMath::Clamp(FVector::DotProduct(ReflectedDirection.GetSafeNormal(), DirectionToReceiver), -1.0f, 1.0f);
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));
	if (Angle > SurfaceComponent->ReflectionConeAngle)
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
	if (!bDrawDebug || !UUOUDebugSubsystem::IsDebugCategoryEnabled(this, EUOUDebugCategory::Puzzle))
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
	if (ExposureRange <= 0.0f)
	{
		return;
	}

	const float OuterConeRadians = FMath::DegreesToRadians(GetEffectiveOuterConeAngle());
	DrawDebugPoint(World, SourcePosition, 10.0f, FColor::Cyan, false, DebugDrawTime);
	DrawDebugLine(
		World,
		SourcePosition,
		SourcePosition + SourceForward * ExposureRange,
		FColor::Cyan,
		false,
		DebugDrawTime,
		0,
		1.5f);
	DrawDebugCone(
		World,
		SourcePosition,
		SourceForward,
		ExposureRange,
		OuterConeRadians,
		OuterConeRadians,
		24,
		FColor::Yellow,
		false,
		DebugDrawTime,
		0,
		1.0f);
}

void UUOULightExposureSourceComponent::DrawDebugResult(const FUOULightExposureData& ExposureData, bool bLit) const
{
	if (!bDrawDebug || !UUOUDebugSubsystem::IsDebugCategoryEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugLine(
			World,
			ExposureData.SourcePosition,
			ExposureData.ReceiverPosition,
			bLit ? FColor::Green : FColor::Red,
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
		|| !UUOUDebugSubsystem::IsDebugCategoryEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugLine(World, SourcePosition, BlockingHit.ImpactPoint, FColor::Red, false, DebugDrawTime, 0, 2.0f);
		DrawDebugPoint(World, BlockingHit.ImpactPoint, 8.0f, FColor::Red, false, DebugDrawTime);
	}
}

void UUOULightExposureSourceComponent::DrawDebugReflectionRay(const FVector& Start, const FVector& End) const
{
	if (!bDrawDebug || !UUOUDebugSubsystem::IsDebugCategoryEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugLine(World, Start, End, FColor::Blue, false, DebugDrawTime, 0, 1.5f);
		DrawDebugPoint(World, Start, 8.0f, FColor::Blue, false, DebugDrawTime);
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
