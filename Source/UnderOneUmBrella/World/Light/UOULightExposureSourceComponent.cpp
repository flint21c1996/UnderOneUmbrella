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
#include "UObject/UObjectIterator.h"
#include "World/Light/UOULightReceivableInterface.h"

namespace
{
	FString BuildReflectionPathTopologyKey(const FUOULightReflectionPathData& PathData)
	{
		FString Key;
		for (const FUOULightReflectionSegmentData& SegmentData : PathData.Segments)
		{
			if (!Key.IsEmpty())
			{
				Key += TEXT(" -> ");
			}

			Key += GetPathNameSafe(SegmentData.Reflector);
		}

		return Key;
	}

	bool AreDirectionsNearlyEqual(const FVector& A, const FVector& B, float AngleToleranceDegrees)
	{
		const FVector SafeA = A.GetSafeNormal();
		const FVector SafeB = B.GetSafeNormal();
		if (SafeA.IsNearlyZero() || SafeB.IsNearlyZero())
		{
			return SafeA.IsNearlyZero() && SafeB.IsNearlyZero();
		}

		const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(
			FMath::Clamp(AngleToleranceDegrees, 0.0f, 180.0f)));
		return FVector::DotProduct(SafeA, SafeB) >= MinimumDot;
	}

	bool HaveSameReceivers(
		const TArray<TObjectPtr<UObject>>& A,
		const TArray<TObjectPtr<UObject>>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}

		for (const TObjectPtr<UObject>& Receiver : A)
		{
			if (!B.Contains(Receiver))
			{
				return false;
			}
		}

		return true;
	}

	bool IsBlockedByActiveUmbrellaShade(
		const FHitResult& Hit,
		const AActor* CandidateOwner)
	{
		if (!Hit.bBlockingHit || Hit.GetActor() == CandidateOwner)
		{
			return false;
		}

		if (const UUOUUmbrellaLightShadeVolumeComponent* HitShade =
			Cast<UUOUUmbrellaLightShadeVolumeComponent>(Hit.GetComponent()))
		{
			return HitShade->CanShadeLight();
		}

		const AActor* HitActor = Hit.GetActor();
		const UUOUUmbrellaLightShadeVolumeComponent* ActorShade = HitActor != nullptr
			? HitActor->FindComponentByClass<UUOUUmbrellaLightShadeVolumeComponent>()
			: nullptr;
		return ActorShade != nullptr && ActorShade->CanShadeLight();
	}
}

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

	if (!bEmitLight)
	{
		PendingDeltaTime = 0.0f;
		if (!ReflectionPaths.IsEmpty() || !LastPublishedReflectionPaths.IsEmpty() ||
			!LightPaths.IsEmpty() || !LastPublishedLightPaths.IsEmpty())
		{
			ReflectionPaths.Reset();
			PublishComputedPaths(false);
		}
		return;
	}

	if (DeltaTime <= 0.0f)
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
		FString::Printf(
			TEXT("Beam Shape: %s"),
			BeamShape == EUOULightBeamShape::Cylinder ? TEXT("Cylinder") : TEXT("Cone")),
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
	const float ReceiverSearchRadius = GetReceiverSearchRadius();
	if (World == nullptr || DeltaTime <= 0.0f || ExposureRange <= 0.0f || Intensity <= 0.0f)
	{
		PublishComputedPaths();
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
		FCollisionShape::MakeSphere(ReceiverSearchRadius),
		QueryParams);

	if (!bHasOverlaps)
	{
		PublishComputedPaths();
		return;
	}

	TSet<UObject*> ProcessedReceivers;
	FPendingExposureMap PendingExposures;
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
				RecordExposureCandidate(
					ReceiverObject,
					ExposureData,
					false,
					TEXT("Direct"),
					PendingExposures);
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
		DeliverPendingExposures(PendingExposures);
		PublishComputedPaths();
		return;
	}

	TArray<UUOULightInteractionSurfaceComponent*> CandidateSurfaces;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		TArray<UUOULightInteractionSurfaceComponent*> InteractionSurfaces;
		AppendLightInteractionSurfaces(
			OverlapResult.GetActor(),
			OverlapResult.GetComponent(),
			InteractionSurfaces);
		for (UUOULightInteractionSurfaceComponent* InteractionSurface : InteractionSurfaces)
		{
			CandidateSurfaces.AddUnique(InteractionSurface);
		}
	}

	const FVector SourceLocation = GetSourceLocation();
	CandidateSurfaces.Sort(
		[&SourceLocation](
			const UUOULightInteractionSurfaceComponent& A,
			const UUOULightInteractionSurfaceComponent& B)
		{
			const float DistanceA = FVector::DistSquared(SourceLocation, A.GetComponentLocation());
			const float DistanceB = FVector::DistSquared(SourceLocation, B.GetComponentLocation());
			if (!FMath::IsNearlyEqual(DistanceA, DistanceB, 1.0f))
			{
				return DistanceA < DistanceB;
			}

			return GetPathNameSafe(&A) < GetPathNameSafe(&B);
		});

	int32 ReflectedSurfaceCount = 0;
	for (UUOULightInteractionSurfaceComponent* InteractionSurface : CandidateSurfaces)
	{
		if (ReflectedSurfaceCount >= MaxReflectionSurfacesPerTick)
		{
			break;
		}

		if (InteractionSurface == nullptr)
		{
			continue;
		}

		FHitResult SurfaceHit;
		if (!TryBuildLightInteractionSurfaceHit(InteractionSurface, SurfaceHit))
		{
			continue;
		}

		++ReflectedSurfaceCount;
		LastReflectorName = GetNameSafe(InteractionSurface);
		EmitReflectedLightFromSurface(
			InteractionSurface,
			SurfaceHit,
			DeltaTime,
			PendingExposures);
	}

	DeliverPendingExposures(PendingExposures);
	PublishComputedPaths();
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
	FallbackRange = FMath::Max(0.0f, FallbackRange);
	CylinderRadius = FMath::Max(0.0f, CylinderRadius);
	CylinderLength = FMath::Max(0.0f, CylinderLength);
	CylinderInnerRadiusRatio = FMath::Clamp(CylinderInnerRadiusRatio, 0.0f, 1.0f);
	Intensity = FMath::Max(0.0f, Intensity);
	SampleInterval = FMath::Max(0.0f, SampleInterval);
	MaxReflectionSurfacesPerTick = FMath::Max(0, MaxReflectionSurfacesPerTick);
	MaxReflectionBouncesPerPath = FMath::Clamp(MaxReflectionBouncesPerPath, 1, 16);
	MinimumReflectedIntensity = FMath::Max(0.0f, MinimumReflectedIntensity);
	ReflectionPathPositionTolerance = FMath::Max(0.0f, ReflectionPathPositionTolerance);
	ReflectionPathDirectionToleranceDegrees = FMath::Clamp(
		ReflectionPathDirectionToleranceDegrees,
		0.0f,
		180.0f);
	ReflectionPathIntensityTolerance = FMath::Max(0.0f, ReflectionPathIntensityTolerance);
	ReflectionPathLossGraceTime = FMath::Max(0.0f, ReflectionPathLossGraceTime);
	DebugSamplePointSize = FMath::Max(1.0f, DebugSamplePointSize);
	DebugDrawTime = FMath::Max(0.0f, DebugDrawTime);
}

void UUOULightExposureSourceComponent::PublishComputedPaths(bool bAllowLossGrace)
{
	NotifyReflectionPathsUpdatedIfChanged(bAllowLossGrace);
	RebuildLightPaths();
	NotifyLightPathsUpdatedIfChanged();
}

void UUOULightExposureSourceComponent::RebuildLightPaths()
{
	LightPaths.Reset();
	if (!bEmitLight || GetWorld() == nullptr || GetExposureRange() <= 0.0f || Intensity <= 0.0f)
	{
		return;
	}

	if (ReflectionPaths.IsEmpty())
	{
		FUOULightPathData DirectPath;
		DirectPath.PathIndex = 0;
		DirectPath.Segments.Add(BuildDirectLightPathSegment(nullptr));
		DirectPath.EndReason = DirectPath.Segments[0].EndReason;
		DirectPath.FinalIntensity = DirectPath.Segments[0].Intensity;
		LightPaths.Add(MoveTemp(DirectPath));
		return;
	}

	for (int32 ReflectionPathIndex = 0; ReflectionPathIndex < ReflectionPaths.Num(); ++ReflectionPathIndex)
	{
		const FUOULightReflectionPathData& ReflectionPath = ReflectionPaths[ReflectionPathIndex];
		if (ReflectionPath.Segments.IsEmpty())
		{
			continue;
		}

		FUOULightPathData LightPath;
		LightPath.PathIndex = LightPaths.Num();
		LightPath.EndReason = ReflectionPath.EndReason;
		LightPath.FinalIntensity = ReflectionPath.FinalIntensity;
		LightPath.Segments.Add(BuildDirectLightPathSegment(&ReflectionPath.Segments[0]));

		for (int32 SegmentIndex = 0; SegmentIndex < ReflectionPath.Segments.Num(); ++SegmentIndex)
		{
			const FUOULightReflectionSegmentData& ReflectionSegment = ReflectionPath.Segments[SegmentIndex];
			FUOULightPathSegmentData Segment;
			Segment.SegmentIndex = SegmentIndex + 1;
			Segment.bReflected = true;
			Segment.Start = ReflectionSegment.ReflectionStart;
			Segment.End = ReflectionSegment.SegmentEnd;
			Segment.Direction = ReflectionSegment.ReflectedDirection.GetSafeNormal();
			Segment.IncomingDirection = ReflectionSegment.IncomingDirection.GetSafeNormal();
			Segment.Length = ReflectionSegment.SegmentLength;
			Segment.StartRadius = ReflectionSegment.BeamStartRadius;
			Segment.EndRadius = ReflectionSegment.BeamEndRadius;
			Segment.ConeAngle = ReflectionSegment.BeamConeAngle;
			Segment.Intensity = ReflectionSegment.ReflectedIntensity;
			Segment.HitComponent = ReflectionSegment.BlockingComponent;
			Segment.InteractionSurface = ReflectionSegment.NextReflector;
			Segment.ReachedReceivers = ReflectionSegment.ReachedReceivers;

			if (ReflectionSegment.NextReflector != nullptr)
			{
				Segment.HitType = EUOULightPathHitType::ReflectingSurface;
				Segment.HitComponent = ReflectionSegment.NextReflector;
			}
			else if (ReflectionSegment.BlockingComponent != nullptr)
			{
				TArray<UObject*> HitReceivers;
				AppendReceivableObjects(
					ReflectionSegment.BlockingComponent->GetOwner(),
					ReflectionSegment.BlockingComponent,
					HitReceivers);
				for (UObject* Receiver : HitReceivers)
				{
					Segment.ReachedReceivers.AddUnique(Receiver);
				}
				Segment.HitType = HitReceivers.IsEmpty()
					? EUOULightPathHitType::BlockingSurface
					: EUOULightPathHitType::Receiver;
			}

			const bool bIsLastSegment = SegmentIndex == ReflectionPath.Segments.Num() - 1;
			Segment.EndReason = bIsLastSegment
				? ReflectionPath.EndReason
				: EUOULightReflectionPathEndReason::None;
			LightPath.Segments.Add(MoveTemp(Segment));
		}

		LightPaths.Add(MoveTemp(LightPath));
	}
}

FUOULightPathSegmentData UUOULightExposureSourceComponent::BuildDirectLightPathSegment(
	const FUOULightReflectionSegmentData* FirstReflectionSegment) const
{
	FUOULightPathSegmentData Segment;
	Segment.SegmentIndex = 0;
	Segment.bReflected = false;
	Segment.Start = GetSourceLocation();
	Segment.Direction = GetSourceForwardVector().GetSafeNormal();
	Segment.Intensity = Intensity;
	Segment.EndReason = EUOULightReflectionPathEndReason::RangeEnded;

	if (FirstReflectionSegment != nullptr)
	{
		Segment.End = FirstReflectionSegment->ImpactPoint;
		Segment.Direction = (Segment.End - Segment.Start).GetSafeNormal();
		Segment.Length = FVector::Distance(Segment.Start, Segment.End);
		Segment.Intensity = FirstReflectionSegment->IncomingIntensity;
		Segment.HitType = EUOULightPathHitType::ReflectingSurface;
		Segment.HitComponent = FirstReflectionSegment->Reflector;
		Segment.InteractionSurface = FirstReflectionSegment->Reflector;
		Segment.EndReason = EUOULightReflectionPathEndReason::None;
	}
	else
	{
		const float MaximumLength = GetExposureRange();
		Segment.End = Segment.Start + Segment.Direction * MaximumLength;
		Segment.Length = MaximumLength;

		if (UWorld* World = GetWorld())
		{
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOULightDirectPath), false, GetOwner());
			if (bIgnoreOwner && GetOwner() != nullptr)
			{
				QueryParams.AddIgnoredActor(GetOwner());
			}

			FHitResult Hit;
			if (TraceLightPathSingle(
				Hit,
				Segment.Start,
				Segment.End,
				QueryParams))
			{
				Segment.End = Hit.ImpactPoint;
				Segment.Length = Hit.Distance;
				Segment.HitComponent = Hit.GetComponent();
				UUOULightInteractionSurfaceComponent* HitInteractionSurface = nullptr;
				Segment.HitType = ClassifyLightPathHit(
					Hit,
					HitInteractionSurface,
					Segment.ReachedReceivers);
				Segment.InteractionSurface = HitInteractionSurface;
				Segment.EndReason = Segment.HitType == EUOULightPathHitType::ReflectingSurface
					? EUOULightReflectionPathEndReason::None
					: EUOULightReflectionPathEndReason::Blocked;
			}
		}
	}

	if (BeamShape == EUOULightBeamShape::Cylinder)
	{
		Segment.StartRadius = CylinderRadius;
		Segment.EndRadius = CylinderRadius;
		Segment.ConeAngle = 0.0f;
	}
	else
	{
		Segment.StartRadius = 0.0f;
		Segment.ConeAngle = GetEffectiveOuterConeAngle();
		Segment.EndRadius = Segment.Length * FMath::Tan(FMath::DegreesToRadians(Segment.ConeAngle));
	}

	return Segment;
}

EUOULightPathHitType UUOULightExposureSourceComponent::ClassifyLightPathHit(
	const FHitResult& Hit,
	UUOULightInteractionSurfaceComponent*& OutInteractionSurface,
	TArray<TObjectPtr<UObject>>& OutReachedReceivers) const
{
	OutInteractionSurface = Cast<UUOULightInteractionSurfaceComponent>(Hit.GetComponent());
	if (OutInteractionSurface == nullptr && Hit.GetActor() != nullptr)
	{
		OutInteractionSurface = Hit.GetActor()->FindComponentByClass<UUOULightInteractionSurfaceComponent>();
	}

	if (OutInteractionSurface != nullptr)
	{
		return OutInteractionSurface->CanReflectLight() &&
			OutInteractionSurface->CanReflectIncomingLight(
				GetSourceForwardVector(),
				Hit.ImpactNormal)
			? EUOULightPathHitType::ReflectingSurface
			: EUOULightPathHitType::BlockingSurface;
	}

	TArray<UObject*> Receivers;
	AppendReceivableObjects(Hit.GetActor(), Hit.GetComponent(), Receivers);
	for (UObject* Receiver : Receivers)
	{
		OutReachedReceivers.AddUnique(Receiver);
	}

	return Receivers.IsEmpty()
		? EUOULightPathHitType::BlockingSurface
		: EUOULightPathHitType::Receiver;
}

void UUOULightExposureSourceComponent::NotifyLightPathsUpdatedIfChanged()
{
	if (bHasPublishedLightPaths && AreLightPathsEquivalent(LastPublishedLightPaths, LightPaths))
	{
		LightPaths = LastPublishedLightPaths;
		return;
	}

	LastPublishedLightPaths = LightPaths;
	bHasPublishedLightPaths = true;
	OnLightPathsUpdated.Broadcast(LightPaths);
}

bool UUOULightExposureSourceComponent::AreLightPathsEquivalent(
	const TArray<FUOULightPathData>& A,
	const TArray<FUOULightPathData>& B) const
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (int32 PathIndex = 0; PathIndex < A.Num(); ++PathIndex)
	{
		const FUOULightPathData& LeftPath = A[PathIndex];
		const FUOULightPathData& RightPath = B[PathIndex];
		if (LeftPath.PathIndex != RightPath.PathIndex ||
			LeftPath.EndReason != RightPath.EndReason ||
			!FMath::IsNearlyEqual(LeftPath.FinalIntensity, RightPath.FinalIntensity, ReflectionPathIntensityTolerance) ||
			LeftPath.Segments.Num() != RightPath.Segments.Num())
		{
			return false;
		}

		for (int32 SegmentIndex = 0; SegmentIndex < LeftPath.Segments.Num(); ++SegmentIndex)
		{
			const FUOULightPathSegmentData& LeftSegment = LeftPath.Segments[SegmentIndex];
			const FUOULightPathSegmentData& RightSegment = RightPath.Segments[SegmentIndex];
			if (LeftSegment.SegmentIndex != RightSegment.SegmentIndex ||
				LeftSegment.bReflected != RightSegment.bReflected ||
				!LeftSegment.Start.Equals(RightSegment.Start, ReflectionPathPositionTolerance) ||
				!LeftSegment.End.Equals(RightSegment.End, ReflectionPathPositionTolerance) ||
				!AreDirectionsNearlyEqual(
					LeftSegment.Direction,
					RightSegment.Direction,
					ReflectionPathDirectionToleranceDegrees) ||
				!AreDirectionsNearlyEqual(
					LeftSegment.IncomingDirection,
					RightSegment.IncomingDirection,
					ReflectionPathDirectionToleranceDegrees) ||
				!FMath::IsNearlyEqual(LeftSegment.Length, RightSegment.Length, ReflectionPathPositionTolerance) ||
				!FMath::IsNearlyEqual(LeftSegment.StartRadius, RightSegment.StartRadius, ReflectionPathPositionTolerance) ||
				!FMath::IsNearlyEqual(LeftSegment.EndRadius, RightSegment.EndRadius, ReflectionPathPositionTolerance) ||
				!FMath::IsNearlyEqual(LeftSegment.ConeAngle, RightSegment.ConeAngle, ReflectionPathDirectionToleranceDegrees) ||
				!FMath::IsNearlyEqual(LeftSegment.Intensity, RightSegment.Intensity, ReflectionPathIntensityTolerance) ||
				LeftSegment.HitType != RightSegment.HitType ||
				LeftSegment.HitComponent != RightSegment.HitComponent ||
				LeftSegment.InteractionSurface != RightSegment.InteractionSurface ||
				LeftSegment.EndReason != RightSegment.EndReason ||
				!HaveSameReceivers(LeftSegment.ReachedReceivers, RightSegment.ReachedReceivers))
			{
				return false;
			}
		}
	}

	return true;
}

void UUOULightExposureSourceComponent::NotifyReflectionPathsUpdatedIfChanged(bool bAllowLossGrace)
{
	NormalizeReflectionPathOrder();

	if (bHasPublishedReflectionPaths &&
		AreReflectionPathsEquivalent(LastPublishedReflectionPaths, ReflectionPaths))
	{
		ReflectionPaths = LastPublishedReflectionPaths;
		ReflectionPathLossStartWorldTime = -1.0f;
		return;
	}

	UWorld* World = GetWorld();
	const bool bHasTopologyLoss = bHasPublishedReflectionPaths &&
		HasReflectionPathTopologyLoss(LastPublishedReflectionPaths, ReflectionPaths);
	bool bOnlyLostExistingTopology = bHasTopologyLoss && !ReflectionPaths.IsEmpty();
	for (const FUOULightReflectionPathData& CurrentPath : ReflectionPaths)
	{
		const FString CurrentKey = BuildReflectionPathTopologyKey(CurrentPath);
		if (!LastPublishedReflectionPaths.ContainsByPredicate(
			[&CurrentKey](const FUOULightReflectionPathData& PreviousPath)
			{
				return BuildReflectionPathTopologyKey(PreviousPath) == CurrentKey;
			}))
		{
			bOnlyLostExistingTopology = false;
			break;
		}
	}
	if (bAllowLossGrace &&
		bOnlyLostExistingTopology &&
		!ReflectionPaths.IsEmpty() &&
		ReflectionPathLossGraceTime > 0.0f &&
		World != nullptr)
	{
		const float CurrentWorldTime = World->GetTimeSeconds();
		if (ReflectionPathLossStartWorldTime < 0.0f)
		{
			ReflectionPathLossStartWorldTime = CurrentWorldTime;
		}

		if (CurrentWorldTime - ReflectionPathLossStartWorldTime < ReflectionPathLossGraceTime)
		{
			ReflectionPaths = LastPublishedReflectionPaths;
			return;
		}
	}

	ReflectionPathLossStartWorldTime = -1.0f;

	LastPublishedReflectionPaths = ReflectionPaths;
	bHasPublishedReflectionPaths = true;
	OnReflectionPathsUpdated.Broadcast(ReflectionPaths);
}

void UUOULightExposureSourceComponent::NormalizeReflectionPathOrder()
{
	ReflectionPaths.Sort([](
		const FUOULightReflectionPathData& A,
		const FUOULightReflectionPathData& B)
	{
		return BuildReflectionPathTopologyKey(A) < BuildReflectionPathTopologyKey(B);
	});

	for (int32 PathIndex = 0; PathIndex < ReflectionPaths.Num(); ++PathIndex)
	{
		ReflectionPaths[PathIndex].PathIndex = PathIndex;
	}
}

bool UUOULightExposureSourceComponent::AreReflectionPathsEquivalent(
	const TArray<FUOULightReflectionPathData>& A,
	const TArray<FUOULightReflectionPathData>& B) const
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
			!LeftPath.SourcePosition.Equals(RightPath.SourcePosition, ReflectionPathPositionTolerance) ||
			LeftPath.EndReason != RightPath.EndReason ||
			!FMath::IsNearlyEqual(LeftPath.FinalIntensity, RightPath.FinalIntensity, ReflectionPathIntensityTolerance) ||
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
				!HaveSameReceivers(LeftSegment.ReachedReceivers, RightSegment.ReachedReceivers) ||
				!LeftSegment.IncomingStart.Equals(RightSegment.IncomingStart, ReflectionPathPositionTolerance) ||
				!LeftSegment.ImpactPoint.Equals(RightSegment.ImpactPoint, ReflectionPathPositionTolerance) ||
				!LeftSegment.ReflectionStart.Equals(RightSegment.ReflectionStart, ReflectionPathPositionTolerance) ||
				!LeftSegment.SegmentEnd.Equals(RightSegment.SegmentEnd, ReflectionPathPositionTolerance) ||
				!AreDirectionsNearlyEqual(LeftSegment.IncomingDirection, RightSegment.IncomingDirection, ReflectionPathDirectionToleranceDegrees) ||
				!AreDirectionsNearlyEqual(LeftSegment.ReflectedDirection, RightSegment.ReflectedDirection, ReflectionPathDirectionToleranceDegrees) ||
				!FMath::IsNearlyEqual(LeftSegment.SegmentLength, RightSegment.SegmentLength, ReflectionPathPositionTolerance) ||
				!FMath::IsNearlyEqual(LeftSegment.BeamStartRadius, RightSegment.BeamStartRadius, ReflectionPathPositionTolerance) ||
				!FMath::IsNearlyEqual(LeftSegment.BeamEndRadius, RightSegment.BeamEndRadius, ReflectionPathPositionTolerance) ||
				!FMath::IsNearlyEqual(LeftSegment.BeamConeAngle, RightSegment.BeamConeAngle, ReflectionPathDirectionToleranceDegrees) ||
				!FMath::IsNearlyEqual(LeftSegment.IncomingIntensity, RightSegment.IncomingIntensity, ReflectionPathIntensityTolerance) ||
				!FMath::IsNearlyEqual(LeftSegment.ReflectedIntensity, RightSegment.ReflectedIntensity, ReflectionPathIntensityTolerance))
			{
				return false;
			}
		}
	}

	return true;
}

bool UUOULightExposureSourceComponent::HasReflectionPathTopologyLoss(
	const TArray<FUOULightReflectionPathData>& PreviousPaths,
	const TArray<FUOULightReflectionPathData>& CurrentPaths)
{
	if (CurrentPaths.Num() < PreviousPaths.Num())
	{
		return true;
	}

	for (const FUOULightReflectionPathData& PreviousPath : PreviousPaths)
	{
		const FString PreviousKey = BuildReflectionPathTopologyKey(PreviousPath);
		const bool bPathStillExists = CurrentPaths.ContainsByPredicate(
			[&PreviousKey](const FUOULightReflectionPathData& CurrentPath)
			{
				return BuildReflectionPathTopologyKey(CurrentPath) == PreviousKey;
			});
		if (!bPathStillExists)
		{
			return true;
		}
	}

	return false;
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
	if (BeamShape == EUOULightBeamShape::Cylinder)
	{
		return FMath::Max(0.0f, CylinderLength);
	}

	if (const ULocalLightComponent* SourceLight = GetSourceLocalLightComponent())
	{
		return FMath::Max(0.0f, SourceLight->AttenuationRadius);
	}

	return FMath::Max(0.0f, FallbackRange);
}

float UUOULightExposureSourceComponent::GetReceiverSearchRadius() const
{
	if (BeamShape == EUOULightBeamShape::Cylinder)
	{
		return FMath::Sqrt(
			FMath::Square(FMath::Max(0.0f, CylinderLength)) +
			FMath::Square(FMath::Max(0.0f, CylinderRadius)));
	}

	return GetExposureRange();
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

bool UUOULightExposureSourceComponent::TryEvaluateSourceBeamPoint(
	const FVector& WorldPosition,
	float& OutDistance,
	FVector& OutDirection,
	float& OutDistanceFactor,
	float& OutShapeFactor) const
{
	OutDistance = 0.0f;
	OutDirection = FVector::ZeroVector;
	OutDistanceFactor = 0.0f;
	OutShapeFactor = 0.0f;

	const FVector SourcePosition = GetSourceLocation();
	const FVector SourceForward = GetSourceForwardVector().GetSafeNormal();
	if (SourceForward.IsNearlyZero())
	{
		return false;
	}

	const FVector ToPoint = WorldPosition - SourcePosition;
	OutDistance = ToPoint.Size();
	if (BeamShape == EUOULightBeamShape::Cylinder)
	{
		const float SafeLength = FMath::Max(0.0f, CylinderLength);
		const float SafeRadius = FMath::Max(0.0f, CylinderRadius);
		const float AxialDistance = FVector::DotProduct(ToPoint, SourceForward);
		if (SafeLength <= 0.0f || SafeRadius <= 0.0f ||
			AxialDistance <= KINDA_SMALL_NUMBER || AxialDistance > SafeLength)
		{
			return false;
		}

		const float RadialDistance =
			(ToPoint - SourceForward * AxialDistance).Size();
		if (RadialDistance > SafeRadius)
		{
			return false;
		}

		OutDirection = SourceForward;
		OutDistanceFactor = bUseDistanceFalloff
			? 1.0f - FMath::Clamp(AxialDistance / SafeLength, 0.0f, 1.0f)
			: 1.0f;
		OutShapeFactor = bUseAngleFalloff
			? CalculateCylinderFactor(RadialDistance)
			: 1.0f;
		return OutDistanceFactor > 0.0f && OutShapeFactor > 0.0f;
	}

	const float ExposureRange = GetExposureRange();
	if (OutDistance <= KINDA_SMALL_NUMBER || OutDistance > ExposureRange)
	{
		return false;
	}

	OutDirection = ToPoint / OutDistance;
	const float Dot = FMath::Clamp(FVector::DotProduct(SourceForward, OutDirection), -1.0f, 1.0f);
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));
	if (Angle > GetEffectiveOuterConeAngle())
	{
		return false;
	}

	CalculateIntensity(OutDistance, Angle, OutDistanceFactor, OutShapeFactor);
	return OutDistanceFactor > 0.0f && OutShapeFactor > 0.0f;
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

	TArray<FVector> SamplePositions;
	int32 RequiredHits = 1;
	GetReceiverSamplePositions(
		ReceiverObject,
		GetSourceForwardVector(),
		SamplePositions,
		RequiredHits);

	int32 HitCount = 0;
	float BestIntensity = -1.0f;
	FHitResult FirstBlockingHit;
	for (const FVector& SamplePosition : SamplePositions)
	{
		FUOULightExposureData SampleExposureData;
		FHitResult SampleBlockingHit;
		if (TryBuildExposureDataAtPosition(
			ReceiverObject,
			SamplePosition,
			DeltaTime,
			SampleExposureData,
			SampleBlockingHit))
		{
			++HitCount;
			DrawDebugSamplePoint(SamplePosition, FColor::Green);
			if (SampleExposureData.Intensity > BestIntensity)
			{
				BestIntensity = SampleExposureData.Intensity;
				OutExposureData = SampleExposureData;
			}
		}
		else
		{
			DrawDebugSamplePoint(
				SamplePosition,
				SampleBlockingHit.bBlockingHit ? FColor::Red : FColor::Yellow);
			if (!FirstBlockingHit.bBlockingHit && SampleBlockingHit.bBlockingHit)
			{
				FirstBlockingHit = SampleBlockingHit;
			}
		}
	}

	const bool bAccepted = HitCount >= RequiredHits;
	const FVector SummaryPosition = SamplePositions.IsEmpty()
		? IUOULightReceivableInterface::Execute_GetLightReceiverPosition(ReceiverObject)
		: SamplePositions[0];
	DrawDebugSampleSummary(
		SummaryPosition,
		TEXT("Receiver"),
		HitCount,
		RequiredHits,
		bAccepted);
	if (!bAccepted)
	{
		OutExposureData = FUOULightExposureData();
		OutBlockingHit = FirstBlockingHit;
		return false;
	}

	return true;
}

bool UUOULightExposureSourceComponent::TryBuildExposureDataAtPosition(
	UObject* ReceiverObject,
	const FVector& ReceiverPosition,
	float DeltaTime,
	FUOULightExposureData& OutExposureData,
	FHitResult& OutBlockingHit) const
{
	OutExposureData = FUOULightExposureData();
	OutBlockingHit = FHitResult();

	const FVector SourcePosition = GetSourceLocation();
	float Distance = 0.0f;
	FVector Direction = FVector::ZeroVector;
	float DistanceFactor = 0.0f;
	float ShapeFactor = 0.0f;
	if (!TryEvaluateSourceBeamPoint(
		ReceiverPosition,
		Distance,
		Direction,
		DistanceFactor,
		ShapeFactor))
	{
		return false;
	}

	if (IsWorldPositionInsideUmbrellaLightShade(ReceiverPosition))
	{
		return false;
	}

	FVector RayStart = SourcePosition;
	if (BeamShape == EUOULightBeamShape::Cylinder)
	{
		const FVector SourceForward = GetSourceForwardVector().GetSafeNormal();
		const float AxialDistance = FVector::DotProduct(
			ReceiverPosition - SourcePosition,
			SourceForward);
		RayStart = ReceiverPosition - SourceForward * AxialDistance;
	}

	if (bRequireLineOfSight && !HasLineOfSight(ReceiverObject, RayStart, ReceiverPosition, OutBlockingHit))
	{
		return false;
	}

	const float FinalIntensity = Intensity *
		FMath::Clamp(DistanceFactor, 0.0f, 1.0f) *
		FMath::Clamp(ShapeFactor, 0.0f, 1.0f);
	if (FinalIntensity <= 0.0f)
	{
		return false;
	}

	OutExposureData = FUOULightExposureData(
		const_cast<UUOULightExposureSourceComponent*>(this),
		RayStart,
		ReceiverPosition,
		Direction,
		Distance,
		FinalIntensity,
		DistanceFactor,
		ShapeFactor,
		DeltaTime);

	return true;
}

void UUOULightExposureSourceComponent::GetReceiverSamplePositions(
	UObject* ReceiverObject,
	const FVector& BeamDirection,
	TArray<FVector>& OutSamplePositions,
	int32& OutRequiredHits) const
{
	OutSamplePositions.Reset();
	OutRequiredHits = 1;

	if (const UUOULightExposureReceiverComponent* ReceiverComponent =
		Cast<UUOULightExposureReceiverComponent>(ReceiverObject))
	{
		ReceiverComponent->GetLightReceiverSamplePositions(BeamDirection, OutSamplePositions);
		OutRequiredHits = ReceiverComponent->GetRequiredLightSampleHits(OutSamplePositions.Num());
		return;
	}

	if (ReceiverObject != nullptr &&
		ReceiverObject->GetClass()->ImplementsInterface(UUOULightReceivableInterface::StaticClass()))
	{
		OutSamplePositions.Add(
			IUOULightReceivableInterface::Execute_GetLightReceiverPosition(ReceiverObject));
	}
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
	return !TraceLightPathSingle(OutBlockingHit, TraceStart, TraceEnd, QueryParams, IgnoredActor);
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

bool UUOULightExposureSourceComponent::TraceLightPathSingle(
	FHitResult& OutHit,
	const FVector& TraceStart,
	const FVector& TraceEnd,
	const FCollisionQueryParams& QueryParams,
	const AActor* IgnoredShadeOwner) const
{
	OutHit = FHitResult();
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FHitResult CollisionHit;
	const bool bHasCollisionHit = World->LineTraceSingleByChannel(
		CollisionHit,
		TraceStart,
		TraceEnd,
		OcclusionTraceChannel,
		QueryParams);

	FHitResult ShadeHit;
	const bool bHasShadeHit = FindNearestUmbrellaLightShadeHit(
		TraceStart,
		TraceEnd,
		ShadeHit,
		IgnoredShadeOwner);
	if (!bHasCollisionHit && !bHasShadeHit)
	{
		return false;
	}

	// 우산 반사판과 그늘 볼륨이 거의 같은 위치일 때는 실제 반사판 Hit를 우선한다.
	// 그늘이 명확히 앞에 있을 때만 뒤쪽 거울/수신 대상을 차단한다.
	constexpr float CoincidentHitTolerance = 1.0f;
	if (bHasShadeHit &&
		(!bHasCollisionHit || ShadeHit.Distance + CoincidentHitTolerance < CollisionHit.Distance))
	{
		OutHit = ShadeHit;
		return true;
	}

	OutHit = CollisionHit;
	return bHasCollisionHit;
}

bool UUOULightExposureSourceComponent::FindNearestUmbrellaLightShadeHit(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	FHitResult& OutHit,
	const AActor* IgnoredShadeOwner) const
{
	OutHit = FHitResult();
	UWorld* World = GetWorld();
	const FVector TraceDelta = TraceEnd - TraceStart;
	const float TraceLength = TraceDelta.Size();
	if (World == nullptr || TraceLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	float BestHitTime = TNumericLimits<float>::Max();
	for (TObjectIterator<UUOUUmbrellaLightShadeVolumeComponent> ShadeIt; ShadeIt; ++ShadeIt)
	{
		UUOUUmbrellaLightShadeVolumeComponent* ShadeVolume = *ShadeIt;
		if (!IsValid(ShadeVolume) ||
			ShadeVolume->GetWorld() != World ||
			ShadeVolume->GetOwner() == IgnoredShadeOwner ||
			!ShadeVolume->IsRegistered() ||
			!ShadeVolume->CanShadeLight() ||
			ShadeVolume->ContainsWorldPosition(TraceStart))
		{
			continue;
		}

		const FTransform ShadeTransform = ShadeVolume->GetComponentTransform();
		const FVector LocalStart = ShadeTransform.InverseTransformPosition(TraceStart);
		const FVector LocalEnd = ShadeTransform.InverseTransformPosition(TraceEnd);
		const FVector Extent = ShadeVolume->GetUnscaledBoxExtent();
		const FBox LocalBox(-Extent, Extent);

		FVector LocalHitLocation = FVector::ZeroVector;
		FVector LocalHitNormal = FVector::ZeroVector;
		float HitTime = 0.0f;
		if (!FMath::LineExtentBoxIntersection(
			LocalBox,
			LocalStart,
			LocalEnd,
			FVector::ZeroVector,
			LocalHitLocation,
			LocalHitNormal,
			HitTime) ||
			HitTime < 0.0f ||
			HitTime > 1.0f ||
			HitTime >= BestHitTime)
		{
			continue;
		}

		BestHitTime = HitTime;
		const FVector HitLocation = ShadeTransform.TransformPosition(LocalHitLocation);
		const FVector HitNormal = ShadeTransform.TransformVectorNoScale(LocalHitNormal).GetSafeNormal();
		OutHit = FHitResult(ShadeVolume->GetOwner(), ShadeVolume, HitLocation, HitNormal);
		OutHit.bBlockingHit = true;
		OutHit.Time = HitTime;
		OutHit.Distance = TraceLength * HitTime;
		OutHit.TraceStart = TraceStart;
		OutHit.TraceEnd = TraceEnd;
	}

	return OutHit.bBlockingHit;
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

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOULightInteractionSurfaceTrace), false, GetOwner());
	if (bIgnoreOwner && GetOwner() != nullptr)
	{
		QueryParams.AddIgnoredActor(GetOwner());
	}
	if (SurfaceComponent->GetOwner() != nullptr)
	{
		AddActorPrimitiveComponentsToIgnore(
			SurfaceComponent->GetOwner(),
			QueryParams,
			SurfaceComponent);
	}

	const FVector SourcePosition = GetSourceLocation();
	const FVector SourceForward = GetSourceForwardVector().GetSafeNormal();
	TArray<FVector> SamplePositions;
	SurfaceComponent->GetReflectionSamplePositions(SamplePositions);

	// 우산이 반사면 중심으로 향하는 광선을 막았다면, 가장자리 샘플만 우산 옆으로
	// 빠져나갔다는 이유로 뒤쪽 거울에서 고립된 반사광을 만들지 않습니다.
	float CenterDistance = 0.0f;
	FVector CenterDirection = FVector::ZeroVector;
	float CenterDistanceFactor = 0.0f;
	float CenterShapeFactor = 0.0f;
	const FVector SurfaceCenter = SurfaceComponent->GetComponentLocation();
	if (TryEvaluateSourceBeamPoint(
		SurfaceCenter,
		CenterDistance,
		CenterDirection,
		CenterDistanceFactor,
		CenterShapeFactor))
	{
		FVector CenterTraceStart = SourcePosition;
		if (BeamShape == EUOULightBeamShape::Cylinder)
		{
			const float AxialDistance = FVector::DotProduct(
				SurfaceCenter - SourcePosition,
				SourceForward);
			CenterTraceStart = SurfaceCenter - SourceForward * AxialDistance;
		}

		FHitResult CenterHit;
		const FVector CenterTraceEnd = SurfaceCenter +
			CenterDirection * SurfaceComponent->ReflectionStartPadding;
		TraceLightPathSingle(
			CenterHit,
			CenterTraceStart,
			CenterTraceEnd,
			QueryParams,
			SurfaceComponent->GetOwner());
		if (IsBlockedByActiveUmbrellaShade(CenterHit, SurfaceComponent->GetOwner()))
		{
			DrawDebugSamplePoint(CenterHit.ImpactPoint, FColor::Red);
			DrawDebugSampleSummary(
				SurfaceCenter,
				TEXT("Mirror"),
				0,
				1,
				false);
			return false;
		}
	}

	int32 HitCount = 0;
	float BestScore = -1.0f;

	// 고정 표면 샘플 사이로 빛 중심축이 지나가는 경우도 놓치지 않도록
	// 실제 빔 중심축과 반사면의 교차점을 가장 먼저 검사합니다.
	const float BeamRange = GetExposureRange();
	if (!SourceForward.IsNearlyZero() && BeamRange > KINDA_SMALL_NUMBER)
	{
		FHitResult AxisHit;
		const bool bAxisHitSurface = TraceLightPathSingle(
			AxisHit,
			SourcePosition,
			SourcePosition + SourceForward * BeamRange,
			QueryParams,
			SurfaceComponent->GetOwner()) &&
			AxisHit.GetComponent() == SurfaceComponent &&
			SurfaceComponent->CanReflectIncomingLight(
				SourceForward,
				AxisHit.ImpactNormal);
		if (bAxisHitSurface)
		{
			float AxisDistance = 0.0f;
			FVector AxisDirection = FVector::ZeroVector;
			float AxisDistanceFactor = 0.0f;
			float AxisShapeFactor = 0.0f;
			if (TryEvaluateSourceBeamPoint(
				AxisHit.ImpactPoint,
				AxisDistance,
				AxisDirection,
				AxisDistanceFactor,
				AxisShapeFactor))
			{
				++HitCount;
				BestScore = AxisDistanceFactor * AxisShapeFactor;
				OutSurfaceHit = AxisHit;
				DrawDebugSamplePoint(AxisHit.ImpactPoint, FColor::Cyan);
			}
		}
	}

	for (const FVector& SamplePosition : SamplePositions)
	{
		float SurfaceDistance = 0.0f;
		FVector DirectionToSurface = FVector::ZeroVector;
		float DistanceFactor = 0.0f;
		float ShapeFactor = 0.0f;
		if (!TryEvaluateSourceBeamPoint(
			SamplePosition,
			SurfaceDistance,
			DirectionToSurface,
			DistanceFactor,
			ShapeFactor))
		{
			DrawDebugSamplePoint(SamplePosition, FColor::Yellow);
			continue;
		}

		FVector TraceStart = SourcePosition;
		if (BeamShape == EUOULightBeamShape::Cylinder)
		{
			const float AxialDistance = FVector::DotProduct(
				SamplePosition - SourcePosition,
				SourceForward);
			TraceStart = SamplePosition - SourceForward * AxialDistance;
		}

		FHitResult SampleHit;
		const FVector TraceEnd =
			SamplePosition + DirectionToSurface * SurfaceComponent->ReflectionStartPadding;
		const bool bHitSurface = TraceLightPathSingle(
			SampleHit,
			TraceStart,
			TraceEnd,
			QueryParams,
			SurfaceComponent->GetOwner()) &&
			SampleHit.GetComponent() == SurfaceComponent &&
			SurfaceComponent->CanReflectIncomingLight(
				DirectionToSurface,
				SampleHit.ImpactNormal);
		if (!bHitSurface)
		{
			DrawDebugSamplePoint(
				SamplePosition,
				SampleHit.bBlockingHit ? FColor::Red : FColor::Yellow);
			continue;
		}

		++HitCount;
		DrawDebugSamplePoint(SampleHit.ImpactPoint, FColor::Cyan);
		const float SampleScore = DistanceFactor * ShapeFactor;
		if (SampleScore > BestScore)
		{
			BestScore = SampleScore;
			OutSurfaceHit = SampleHit;
		}
	}

	DrawDebugSampleSummary(
		SurfaceComponent->GetComponentLocation(),
		TEXT("Mirror"),
		HitCount,
		1,
		OutSurfaceHit.bBlockingHit);
	return OutSurfaceHit.bBlockingHit;
}

void UUOULightExposureSourceComponent::RecordExposureCandidate(
	UObject* ReceiverObject,
	const FUOULightExposureData& ExposureData,
	bool bReflected,
	const FString& StablePathKey,
	FPendingExposureMap& PendingExposures) const
{
	if (ReceiverObject == nullptr || ExposureData.Intensity <= 0.0f)
	{
		return;
	}

	FPendingExposureCandidate* ExistingCandidate = PendingExposures.Find(ReceiverObject);
	const bool bHasStrongerIntensity = ExistingCandidate == nullptr ||
		ExposureData.Intensity > ExistingCandidate->ExposureData.Intensity + KINDA_SMALL_NUMBER;
	const bool bHasEqualIntensity = ExistingCandidate != nullptr &&
		FMath::IsNearlyEqual(
			ExposureData.Intensity,
			ExistingCandidate->ExposureData.Intensity,
			KINDA_SMALL_NUMBER);
	const bool bPreferDirectPath = bHasEqualIntensity &&
		ExistingCandidate->bReflected &&
		!bReflected;
	const bool bHasStableTieBreak = bHasEqualIntensity &&
		ExistingCandidate->bReflected == bReflected &&
		StablePathKey < ExistingCandidate->StablePathKey;
	if (!bHasStrongerIntensity && !bPreferDirectPath && !bHasStableTieBreak)
	{
		return;
	}

	FPendingExposureCandidate& Candidate = PendingExposures.FindOrAdd(ReceiverObject);
	Candidate.ExposureData = ExposureData;
	Candidate.StablePathKey = StablePathKey;
	Candidate.bReflected = bReflected;
}

void UUOULightExposureSourceComponent::DeliverPendingExposures(
	const FPendingExposureMap& PendingExposures)
{
	TArray<UObject*> Receivers;
	PendingExposures.GenerateKeyArray(Receivers);
	Receivers.Sort(
		[](const UObject& A, const UObject& B)
		{
			return GetPathNameSafe(&A) < GetPathNameSafe(&B);
		});

	for (UObject* ReceiverObject : Receivers)
	{
		const FPendingExposureCandidate* Candidate = PendingExposures.Find(ReceiverObject);
		if (!IsValid(ReceiverObject) || Candidate == nullptr)
		{
			continue;
		}

		IUOULightReceivableInterface::Execute_ReceiveLightExposure(
			ReceiverObject,
			Candidate->ExposureData);
		++LastLitCount;
		if (Candidate->bReflected)
		{
			++LastReflectedCount;
		}
		LastLitTargetName = GetReceivableDebugName(ReceiverObject);
		DrawDebugResult(Candidate->ExposureData, true);
	}
}

void UUOULightExposureSourceComponent::EmitReflectedLightFromSurface(
	UUOULightInteractionSurfaceComponent* SurfaceComponent,
	const FHitResult& SurfaceHit,
	float DeltaTime,
	FPendingExposureMap& PendingExposures)
{
	UWorld* World = GetWorld();
	if (World == nullptr || SurfaceComponent == nullptr || DeltaTime <= 0.0f)
	{
		return;
	}

	const FVector SourcePosition = GetSourceLocation();
	const FVector SourceForward = GetSourceForwardVector().GetSafeNormal();
	const FVector InitialIncomingDirection = BeamShape == EUOULightBeamShape::Cylinder
		? SourceForward
		: (SurfaceHit.ImpactPoint - SourcePosition).GetSafeNormal();
	if (InitialIncomingDirection.IsNearlyZero())
	{
		return;
	}

	float SurfaceDistance = 0.0f;
	FVector EvaluatedDirection = FVector::ZeroVector;
	float SourceDistanceFactor = 0.0f;
	float SourceShapeFactor = 0.0f;
	if (!TryEvaluateSourceBeamPoint(
		SurfaceHit.ImpactPoint,
		SurfaceDistance,
		EvaluatedDirection,
		SourceDistanceFactor,
		SourceShapeFactor))
	{
		return;
	}

	float IncomingSurfaceIntensity = Intensity *
		FMath::Clamp(SourceDistanceFactor, 0.0f, 1.0f) *
		FMath::Clamp(SourceShapeFactor, 0.0f, 1.0f);
	if (IncomingSurfaceIntensity <= 0.0f)
	{
		return;
	}

	UUOULightInteractionSurfaceComponent* CurrentSurface = SurfaceComponent;
	FHitResult CurrentSurfaceHit = SurfaceHit;
	FVector CurrentRayOrigin = SourcePosition;
	if (BeamShape == EUOULightBeamShape::Cylinder)
	{
		const float AxialDistance = FVector::DotProduct(
			SurfaceHit.ImpactPoint - SourcePosition,
			SourceForward);
		CurrentRayOrigin = SurfaceHit.ImpactPoint - SourceForward * AxialDistance;
	}
	TSet<const UUOULightInteractionSurfaceComponent*> VisitedSurfaces;
	TArray<FString> ReflectionPath;
	FUOULightReflectionPathData PathData;
	PathData.PathIndex = ReflectionPaths.Num();
	PathData.SourcePosition = SourcePosition;
	int32 ReflectionDepth = 0;
	float IncomingBeamRadius = BeamShape == EUOULightBeamShape::Cylinder
		? FMath::Max(0.0f, CylinderRadius)
		: SurfaceDistance * FMath::Tan(
			FMath::DegreesToRadians(FMath::Clamp(GetEffectiveOuterConeAngle(), 1.0f, 89.0f)));
	float CurrentBeamConeAngle = BeamShape == EUOULightBeamShape::Cylinder
		? 0.0f
		: GetEffectiveOuterConeAngle();

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
		const float OutgoingBeamConeAngle =
			CurrentSurface->ResolveReflectionConeAngle(CurrentBeamConeAngle);

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
				OutgoingBeamConeAngle,
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
			if (TraceLightPathSingle(
				SegmentBlockingHit,
				ReflectionOrigin,
				SegmentTraceEnd,
				SegmentTraceQueryParams,
				CurrentSurface->GetOwner()))
			{
				SegmentEnd = SegmentBlockingHit.ImpactPoint;
			}
		}

		const float SegmentLength = FVector::Dist(ReflectionOrigin, SegmentEnd);
		const float ReflectedIntensity = CalculateReflectedSegmentIntensity(
			CurrentSurface,
			IncomingSurfaceIntensity,
			0.0f,
			0.0f,
			OutgoingBeamConeAngle);
		const float BeamEndRadius = CurrentBeamStartRadius + SegmentLength * FMath::Tan(
			FMath::DegreesToRadians(OutgoingBeamConeAngle));

		const float NextSurfaceIntensity = bHasNextSurface
			? CalculateReflectedSegmentIntensity(
				CurrentSurface,
				IncomingSurfaceIntensity,
				NextSurfaceDistance,
				NextSurfaceAngle,
				OutgoingBeamConeAngle)
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
			OutgoingBeamConeAngle,
			CurrentBeamStartRadius,
			ReflectionDebugColor);

		TArray<TObjectPtr<UObject>> ReachedReceivers;
		EmitReflectedLightToReceivers(
			CurrentSurface,
			ReflectionOrigin,
			ReflectedDirection,
			CurrentBeamStartRadius,
			OutgoingBeamConeAngle,
			IncomingSurfaceIntensity,
			DeltaTime,
			FString::Join(ReflectionPath, TEXT(" -> ")),
			PendingExposures,
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
		SegmentData.BeamConeAngle = OutgoingBeamConeAngle;
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
			FMath::DegreesToRadians(OutgoingBeamConeAngle));
		CurrentBeamConeAngle = OutgoingBeamConeAngle;
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
	float BeamConeAngle,
	float SurfaceIntensity,
	float DeltaTime,
	const FString& StablePathKey,
	FPendingExposureMap& PendingExposures,
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
				BeamConeAngle,
				SurfaceIntensity,
				DeltaTime,
				ExposureData,
				BlockingHit))
			{
				RecordExposureCandidate(
					ReceiverObject,
					ExposureData,
					true,
					StablePathKey,
					PendingExposures);
				OutReachedReceivers.AddUnique(ReceiverObject);
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
	float BeamConeAngle,
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
			if (CandidateSurface->GetOwner() != nullptr)
			{
				AddActorPrimitiveComponentsToIgnore(
					CandidateSurface->GetOwner(),
					TraceQueryParams,
					CandidateSurface);
			}

			TArray<FVector> CandidateSamplePositions;
			CandidateSurface->GetReflectionSamplePositions(CandidateSamplePositions);
			int32 CandidateHitCount = 0;

			// 반사 빔의 중심축이 표면 샘플 사이를 통과해도 다음 거울을 검출합니다.
			FHitResult AxisHit;
			const bool bAxisHitCandidate = TraceLightPathSingle(
				AxisHit,
				ReflectionOrigin,
				ReflectionOrigin + SafeReflectedDirection * CurrentSurface->ReflectionRange,
				TraceQueryParams,
				CurrentSurface->GetOwner()) &&
				AxisHit.GetComponent() == CandidateSurface &&
				CandidateSurface->CanReflectIncomingLight(
					SafeReflectedDirection,
					AxisHit.ImpactNormal);
			if (bAxisHitCandidate)
			{
				++CandidateHitCount;
				DrawDebugSamplePoint(AxisHit.ImpactPoint, FColor::Cyan);
				const float AxisHitDistance = FVector::Dist(ReflectionOrigin, AxisHit.ImpactPoint);
				if (AxisHitDistance < ClosestHitDistance)
				{
					ClosestHitDistance = AxisHitDistance;
					OutSurface = CandidateSurface;
					OutSurfaceHit = AxisHit;
					OutDistance = AxisHitDistance;
					OutAngle = 0.0f;
				}
			}

			for (const FVector& CandidateSamplePosition : CandidateSamplePositions)
			{
				const FVector ToCandidate = CandidateSamplePosition - ReflectionOrigin;
				const float CandidateDistance = ToCandidate.Size();
				if (CandidateDistance <= KINDA_SMALL_NUMBER ||
					CandidateDistance > CurrentSurface->ReflectionRange)
				{
					DrawDebugSamplePoint(CandidateSamplePosition, FColor::Yellow);
					continue;
				}

				const FVector DirectionToCandidate = ToCandidate / CandidateDistance;
				const float AxialDistance = FVector::DotProduct(ToCandidate, SafeReflectedDirection);
				const float RadialDistance =
					(ToCandidate - SafeReflectedDirection * AxialDistance).Size();
				const float ConeAngleRadians = FMath::DegreesToRadians(
					FMath::Clamp(BeamConeAngle, 0.0f, 89.0f));
				const float SafeBeamStartRadius = FMath::Max(0.0f, BeamStartRadius);
				const float MaximumBeamRadius = AxialDistance > 0.0f
					? SafeBeamStartRadius + AxialDistance * FMath::Tan(ConeAngleRadians)
					: 0.0f;
				if (AxialDistance <= KINDA_SMALL_NUMBER || RadialDistance > MaximumBeamRadius)
				{
					DrawDebugSamplePoint(CandidateSamplePosition, FColor::Yellow);
					continue;
				}

				const float CandidateAngle = FMath::RadiansToDegrees(FMath::Atan2(
					FMath::Max(0.0f, RadialDistance - SafeBeamStartRadius),
					AxialDistance));
				FHitResult CandidateHit;
				const FVector TraceEnd = CandidateSamplePosition +
					DirectionToCandidate * CandidateSurface->ReflectionStartPadding;
				const bool bHitCandidate = TraceLightPathSingle(
					CandidateHit,
					ReflectionOrigin,
					TraceEnd,
					TraceQueryParams,
					CurrentSurface->GetOwner()) &&
					CandidateHit.GetComponent() == CandidateSurface &&
					CandidateSurface->CanReflectIncomingLight(
						SafeReflectedDirection,
						CandidateHit.ImpactNormal);
				if (!bHitCandidate)
				{
					DrawDebugSamplePoint(
						CandidateSamplePosition,
						CandidateHit.bBlockingHit ? FColor::Red : FColor::Yellow);
					continue;
				}

				++CandidateHitCount;
				DrawDebugSamplePoint(CandidateHit.ImpactPoint, FColor::Cyan);
				const float HitDistance = FVector::Dist(ReflectionOrigin, CandidateHit.ImpactPoint);
				if (HitDistance < ClosestHitDistance)
				{
					ClosestHitDistance = HitDistance;
					OutSurface = CandidateSurface;
					OutSurfaceHit = CandidateHit;
					OutDistance = HitDistance;
					OutAngle = CandidateAngle;
				}
			}

			DrawDebugSampleSummary(
				CandidateSurface->GetComponentLocation(),
				TEXT("Mirror"),
				CandidateHitCount,
				1,
				CandidateHitCount > 0);
		}
	}

	return OutSurface != nullptr;
}

float UUOULightExposureSourceComponent::CalculateReflectedSegmentIntensity(
	const UUOULightInteractionSurfaceComponent* SurfaceComponent,
	float IncomingIntensity,
	float Distance,
	float Angle,
	float BeamConeAngle) const
{
	if (SurfaceComponent == nullptr || IncomingIntensity <= 0.0f)
	{
		return 0.0f;
	}

	const float DistanceFactor = bUseDistanceFalloff && SurfaceComponent->ReflectionRange > 0.0f
		? 1.0f - FMath::Clamp(Distance / SurfaceComponent->ReflectionRange, 0.0f, 1.0f)
		: 1.0f;
	const float AngleFactor = bUseAngleFalloff
		? CalculateConeFactor(Angle, BeamConeAngle)
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
	float BeamConeAngle,
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

	TArray<FVector> SamplePositions;
	int32 RequiredHits = 1;
	GetReceiverSamplePositions(
		ReceiverObject,
		ReflectedDirection,
		SamplePositions,
		RequiredHits);

	int32 HitCount = 0;
	float BestIntensity = -1.0f;
	FHitResult FirstBlockingHit;
	for (const FVector& SamplePosition : SamplePositions)
	{
		FUOULightExposureData SampleExposureData;
		FHitResult SampleBlockingHit;
		if (TryBuildReflectedExposureDataAtPosition(
			ReceiverObject,
			SamplePosition,
			SurfaceComponent,
			ReflectionOrigin,
			ReflectedDirection,
			BeamStartRadius,
			BeamConeAngle,
			SurfaceIntensity,
			DeltaTime,
			SampleExposureData,
			SampleBlockingHit))
		{
			++HitCount;
			DrawDebugSamplePoint(SamplePosition, FColor::Green);
			if (SampleExposureData.Intensity > BestIntensity)
			{
				BestIntensity = SampleExposureData.Intensity;
				OutExposureData = SampleExposureData;
			}
		}
		else
		{
			DrawDebugSamplePoint(
				SamplePosition,
				SampleBlockingHit.bBlockingHit ? FColor::Red : FColor::Yellow);
			if (!FirstBlockingHit.bBlockingHit && SampleBlockingHit.bBlockingHit)
			{
				FirstBlockingHit = SampleBlockingHit;
			}
		}
	}

	const bool bAccepted = HitCount >= RequiredHits;
	const FVector SummaryPosition = SamplePositions.IsEmpty()
		? IUOULightReceivableInterface::Execute_GetLightReceiverPosition(ReceiverObject)
		: SamplePositions[0];
	DrawDebugSampleSummary(
		SummaryPosition,
		TEXT("Reflected Receiver"),
		HitCount,
		RequiredHits,
		bAccepted);
	if (!bAccepted)
	{
		OutExposureData = FUOULightExposureData();
		OutBlockingHit = FirstBlockingHit;
		return false;
	}

	return true;
}

bool UUOULightExposureSourceComponent::TryBuildReflectedExposureDataAtPosition(
	UObject* ReceiverObject,
	const FVector& ReceiverPosition,
	const UUOULightInteractionSurfaceComponent* SurfaceComponent,
	const FVector& ReflectionOrigin,
	const FVector& ReflectedDirection,
	float BeamStartRadius,
	float BeamConeAngle,
	float SurfaceIntensity,
	float DeltaTime,
	FUOULightExposureData& OutExposureData,
	FHitResult& OutBlockingHit) const
{
	OutExposureData = FUOULightExposureData();
	OutBlockingHit = FHitResult();

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
		FMath::Clamp(BeamConeAngle, 0.0f, 89.0f));
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
	const float AngleFactor = bUseAngleFalloff ? CalculateConeFactor(Angle, BeamConeAngle) : 1.0f;
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

float UUOULightExposureSourceComponent::CalculateCylinderFactor(float RadialDistance) const
{
	const float SafeRadius = FMath::Max(0.0f, CylinderRadius);
	if (SafeRadius <= KINDA_SMALL_NUMBER || RadialDistance >= SafeRadius)
	{
		return 0.0f;
	}

	const float InnerRadius = SafeRadius * FMath::Clamp(CylinderInnerRadiusRatio, 0.0f, 1.0f);
	if (RadialDistance <= InnerRadius)
	{
		return 1.0f;
	}

	const float FalloffWidth = FMath::Max(0.001f, SafeRadius - InnerRadius);
	return 1.0f - FMath::Clamp(
		(RadialDistance - InnerRadius) / FalloffWidth,
		0.0f,
		1.0f);
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

	if (BeamShape == EUOULightBeamShape::Cylinder)
	{
		constexpr int32 CylinderSegments = 24;
		const float SafeRadius = FMath::Max(0.0f, CylinderRadius);
		FVector RadiusAxisX = FVector::ZeroVector;
		FVector RadiusAxisY = FVector::ZeroVector;
		SourceForward.FindBestAxisVectors(RadiusAxisX, RadiusAxisY);
		TArray<FVector, TInlineAllocator<CylinderSegments>> StartPoints;
		TArray<FVector, TInlineAllocator<CylinderSegments>> EndPoints;
		StartPoints.Reserve(CylinderSegments);
		EndPoints.Reserve(CylinderSegments);

		for (int32 SegmentIndex = 0; SegmentIndex < CylinderSegments; ++SegmentIndex)
		{
			const float Angle =
				UE_TWO_PI * static_cast<float>(SegmentIndex) / static_cast<float>(CylinderSegments);
			const FVector RadiusDirection =
				RadiusAxisX * FMath::Cos(Angle) + RadiusAxisY * FMath::Sin(Angle);
			const FVector RayStart = SourcePosition + RadiusDirection * SafeRadius;
			const FVector TraceEnd = RayStart + SourceForward * ExposureRange;
			FHitResult BlockingHit;
			const FVector RayEnd = TraceLightPathSingle(
				BlockingHit,
				RayStart,
				TraceEnd,
				QueryParams)
				? BlockingHit.ImpactPoint
				: TraceEnd;
			StartPoints.Add(RayStart);
			EndPoints.Add(RayEnd);
		}

		DrawDebugPoint(World, SourcePosition, 10.0f, SourceDebugColor, false, DebugDrawTime);
		for (int32 SegmentIndex = 0; SegmentIndex < CylinderSegments; ++SegmentIndex)
		{
			const int32 NextSegmentIndex = (SegmentIndex + 1) % CylinderSegments;
			DrawDebugLine(
				World,
				StartPoints[SegmentIndex],
				EndPoints[SegmentIndex],
				ConeDebugColor,
				false,
				DebugDrawTime,
				0,
				1.0f);
			DrawDebugLine(
				World,
				StartPoints[SegmentIndex],
				StartPoints[NextSegmentIndex],
				ConeDebugColor,
				false,
				DebugDrawTime,
				0,
				1.0f);
			DrawDebugLine(
				World,
				EndPoints[SegmentIndex],
				EndPoints[NextSegmentIndex],
				ConeDebugColor,
				false,
				DebugDrawTime,
				0,
				1.0f);
		}

		return;
	}

	const auto FindClippedRayEnd =
		[this, World, &QueryParams, &SourcePosition, ExposureRange](const FVector& RayDirection)
		{
			const FVector TraceEnd = SourcePosition + RayDirection * ExposureRange;
			FHitResult BlockingHit;
			return TraceLightPathSingle(
				BlockingHit,
				SourcePosition,
				TraceEnd,
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

void UUOULightExposureSourceComponent::DrawDebugSamplePoint(
	const FVector& Position,
	const FColor& Color) const
{
	if (!bDrawDebug ||
		!bDrawSampleDebug ||
		!UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugPoint(
			World,
			Position,
			DebugSamplePointSize,
			Color,
			false,
			DebugDrawTime);
	}
}

void UUOULightExposureSourceComponent::DrawDebugSampleSummary(
	const FVector& Position,
	const TCHAR* SampleType,
	int32 HitCount,
	int32 RequiredHits,
	bool bAccepted) const
{
	if (!bDrawDebug ||
		!bDrawSampleDebug ||
		!UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugString(
			World,
			Position + FVector(0.0f, 0.0f, 20.0f),
			FString::Printf(
				TEXT("%s %d/%d %s"),
				SampleType,
				HitCount,
				RequiredHits,
				bAccepted ? TEXT("ON") : TEXT("OFF")),
			nullptr,
			bAccepted ? FColor::Green : FColor::Red,
			DebugDrawTime,
			false);
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
			FMath::Clamp(ConeAngleDegrees, 0.0f, 89.0f));
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
