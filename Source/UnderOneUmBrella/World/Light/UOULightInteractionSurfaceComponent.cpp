// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightInteractionSurfaceComponent.h"

#include "GameFramework/Actor.h"

UUOULightInteractionSurfaceComponent::UUOULightInteractionSurfaceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitBoxExtent(FVector(80.0f, 80.0f, 6.0f));
	SetHiddenInGame(true);
	ApplyCollisionSettings();
}

void UUOULightInteractionSurfaceComponent::OnRegister()
{
	Super::OnRegister();

	ValidateSettings();
	ApplyCollisionSettings();
}

#if WITH_EDITOR
void UUOULightInteractionSurfaceComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ValidateSettings();
	ApplyCollisionSettings();
}
#endif

void UUOULightInteractionSurfaceComponent::SetLightInteractionMode(EUOULightInteractionMode NewMode)
{
	LightInteractionMode = NewMode;
	ApplyCollisionSettings();
}

bool UUOULightInteractionSurfaceComponent::CanBlockLight() const
{
	return LightInteractionMode == EUOULightInteractionMode::Blocking ||
		LightInteractionMode == EUOULightInteractionMode::Reflecting;
}

bool UUOULightInteractionSurfaceComponent::CanReflectLight() const
{
	return LightInteractionMode == EUOULightInteractionMode::Reflecting &&
		ReflectionIntensityMultiplier > 0.0f &&
		(!bLimitReflectionBySurfaceAperture || GetReflectionApertureRadius() > KINDA_SMALL_NUMBER);
}

bool UUOULightInteractionSurfaceComponent::CanReflectIncomingLight(
	const FVector& IncomingDirection,
	const FVector& HitNormal) const
{
	return CanReflectIncomingLightWithMaximumAngle(
		IncomingDirection,
		HitNormal,
		MaximumReflectionIncidenceAngle);
}

bool UUOULightInteractionSurfaceComponent::CanReflectIncomingLightWithMaximumAngle(
	const FVector& IncomingDirection,
	const FVector& HitNormal,
	float MaximumIncidenceAngleDegrees) const
{
	if (!CanReflectLight())
	{
		return false;
	}

	const FVector SafeIncomingDirection = IncomingDirection.GetSafeNormal();
	if (SafeIncomingDirection.IsNearlyZero())
	{
		return false;
	}

	FVector FrontNormal = GetReflectionFrontNormal();
	if (FrontNormal.IsNearlyZero())
	{
		FrontNormal = HitNormal.GetSafeNormal();
	}
	if (FrontNormal.IsNearlyZero())
	{
		return false;
	}

	float FrontDot = FVector::DotProduct(-SafeIncomingDirection, FrontNormal);
	if (!bReflectFrontFaceOnly)
	{
		FrontDot = FMath::Abs(FrontDot);
	}

	const float SafeMaximumAngle = FMath::Clamp(
		MaximumIncidenceAngleDegrees,
		0.0f,
		180.0f);
	const float MinimumFrontDot = FMath::Cos(FMath::DegreesToRadians(SafeMaximumAngle));
	return FrontDot >= MinimumFrontDot - KINDA_SMALL_NUMBER;
}

bool UUOULightInteractionSurfaceComponent::ShouldPassThroughIncomingLight(
	const FVector& IncomingDirection,
	const FVector& HitNormal) const
{
	return ShouldPassThroughIncomingLightWithMaximumAngle(
		IncomingDirection,
		HitNormal,
		MaximumReflectionIncidenceAngle);
}

bool UUOULightInteractionSurfaceComponent::ShouldPassThroughIncomingLightWithMaximumAngle(
	const FVector& IncomingDirection,
	const FVector& HitNormal,
	float MaximumIncidenceAngleDegrees) const
{
	return LightInteractionMode == EUOULightInteractionMode::Reflecting &&
		bPassThroughWhenReflectionRejected &&
		CanReflectLight() &&
		!CanReflectIncomingLightWithMaximumAngle(
			IncomingDirection,
			HitNormal,
			MaximumIncidenceAngleDegrees);
}

void UUOULightInteractionSurfaceComponent::SetReflectionIncidenceAngles(
	float StartMaximumAngleDegrees,
	float RetainedMaximumAngleDegrees)
{
	MaximumReflectionIncidenceAngle = FMath::Clamp(
		StartMaximumAngleDegrees,
		0.0f,
		180.0f);
	RetainedMaximumReflectionIncidenceAngle = FMath::Clamp(
		RetainedMaximumAngleDegrees,
		MaximumReflectionIncidenceAngle,
		180.0f);
}

float UUOULightInteractionSurfaceComponent::ClampReflectionBeamRadius(
	float IncomingBeamRadius,
	const FVector& IncomingDirection,
	const FVector& HitNormal,
	const FVector& ImpactPoint) const
{
	const float SafeIncomingRadius = FMath::Max(0.0f, IncomingBeamRadius);
	if (!bLimitReflectionBySurfaceAperture)
	{
		return SafeIncomingRadius;
	}

	FVector FrontNormal = GetReflectionFrontNormal();
	if (FrontNormal.IsNearlyZero())
	{
		FrontNormal = HitNormal.GetSafeNormal();
	}
	const FVector SafeIncomingDirection = IncomingDirection.GetSafeNormal();
	const float IncidenceProjection = FrontNormal.IsNearlyZero() || SafeIncomingDirection.IsNearlyZero()
		? 1.0f
		: FMath::Abs(FVector::DotProduct(-SafeIncomingDirection, FrontNormal));
	const float EffectiveApertureRadius =
		GetReflectionApertureRadius() * FMath::Clamp(IncidenceProjection, 0.0f, 1.0f);
	float AvailableApertureRadius = EffectiveApertureRadius;
	if (bLimitReflectionByImpactOffset && !FrontNormal.IsNearlyZero())
	{
		const FVector CenterToImpact = ImpactPoint - GetComponentLocation();
		const float ImpactOffset = FVector::VectorPlaneProject(
			CenterToImpact,
			FrontNormal).Size();
		AvailableApertureRadius = FMath::Max(
			0.0f,
			EffectiveApertureRadius - ImpactOffset - ReflectionImpactEdgeInset);
	}

	return FMath::Min(SafeIncomingRadius, AvailableApertureRadius);
}

bool UUOULightInteractionSurfaceComponent::HasSufficientReflectionCoverage(
	float IncomingBeamRadius,
	const FVector& IncomingDirection,
	const FVector& HitNormal,
	const FVector& ImpactPoint) const
{
	return HasSufficientReflectionCoverageAtRatio(
		IncomingBeamRadius,
		IncomingDirection,
		HitNormal,
		ImpactPoint,
		GetStartingBeamFootprintCoverageRatio());
}

bool UUOULightInteractionSurfaceComponent::HasSufficientReflectionCoverageAtRatio(
	float IncomingBeamRadius,
	const FVector& IncomingDirection,
	const FVector& HitNormal,
	const FVector& ImpactPoint,
	float RequiredFootprintCoverageRatio) const
{
	const float SafeIncomingRadius = FMath::Max(0.0f, IncomingBeamRadius);
	const float SafeRequiredFootprintCoverageRatio = FMath::Clamp(
		RequiredFootprintCoverageRatio,
		0.0f,
		1.0f);
	if (!HasMinimumBeamFootprintCoverage(
		SafeIncomingRadius,
		IncomingDirection,
		HitNormal,
		ImpactPoint,
		SafeRequiredFootprintCoverageRatio))
	{
		return false;
	}
	if (!bLimitReflectionByImpactOffset || SafeIncomingRadius <= KINDA_SMALL_NUMBER)
	{
		return true;
	}

	const float ReflectedRadius = ClampReflectionBeamRadius(
		SafeIncomingRadius,
		IncomingDirection,
		HitNormal,
		ImpactPoint);
	const float CoverageRatio = ReflectedRadius / SafeIncomingRadius;
	return CoverageRatio >= FMath::Clamp(MinimumReflectionCoverageRatio, 0.0f, 1.0f);
}

bool UUOULightInteractionSurfaceComponent::ContainsFullBeamFootprint(
	float IncomingBeamRadius,
	const FVector& IncomingDirection,
	const FVector& HitNormal,
	const FVector& ImpactPoint) const
{
	return HasMinimumBeamFootprintCoverage(
		IncomingBeamRadius,
		IncomingDirection,
		HitNormal,
		ImpactPoint,
		GetStartingBeamFootprintCoverageRatio());
}

float UUOULightInteractionSurfaceComponent::CalculateBeamFootprintCoverageRatio(
	float IncomingBeamRadius,
	const FVector& IncomingDirection,
	const FVector& HitNormal,
	const FVector& ImpactPoint) const
{
	if (!bRequireFullBeamFootprint || IncomingBeamRadius <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	const FVector SafeIncomingDirection = IncomingDirection.GetSafeNormal();
	FVector SurfaceNormal = GetReflectionFrontNormal();
	if (SurfaceNormal.IsNearlyZero())
	{
		SurfaceNormal = HitNormal.GetSafeNormal();
	}

	const float ProjectionDenominator = FVector::DotProduct(
		SafeIncomingDirection,
		SurfaceNormal);
	if (SafeIncomingDirection.IsNearlyZero() ||
		SurfaceNormal.IsNearlyZero() ||
		FMath::Abs(ProjectionDenominator) <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	FVector BeamAxisA = FVector::ZeroVector;
	FVector BeamAxisB = FVector::ZeroVector;
	SafeIncomingDirection.FindBestAxisVectors(BeamAxisA, BeamAxisB);

	const FTransform SurfaceTransform = GetComponentTransform();
	const FVector LocalNormal = SurfaceTransform.InverseTransformVectorNoScale(
		SurfaceNormal).GetAbs();
	int32 NormalAxis = 0;
	if (LocalNormal.Y > LocalNormal.X)
	{
		NormalAxis = 1;
	}
	if (LocalNormal.Z > LocalNormal[NormalAxis])
	{
		NormalAxis = 2;
	}

	const FVector LocalExtent = GetUnscaledBoxExtent().GetAbs();
	const FVector AbsoluteScale = GetComponentScale().GetAbs();
	constexpr int32 RadialSampleCount = 8;
	constexpr int32 AngularSampleCount = 32;
	constexpr int32 FootprintSampleCount = RadialSampleCount * AngularSampleCount;
	int32 ContainedSampleCount = 0;
	for (int32 RadialIndex = 0; RadialIndex < RadialSampleCount; ++RadialIndex)
	{
		// sqrt 분포를 사용하면 각 고리가 같은 면적을 대표합니다.
		const float RadiusFraction = FMath::Sqrt(
			(static_cast<float>(RadialIndex) + 0.5f) /
			static_cast<float>(RadialSampleCount));
		const float AngularOffset = (RadialIndex % 2 == 0)
			? 0.0f
			: PI / static_cast<float>(AngularSampleCount);
		for (int32 AngularIndex = 0; AngularIndex < AngularSampleCount; ++AngularIndex)
		{
			const float SampleAngle = AngularOffset + 2.0f * PI *
				static_cast<float>(AngularIndex) / static_cast<float>(AngularSampleCount);
			const FVector BeamPoint = ImpactPoint + IncomingBeamRadius * RadiusFraction *
				(BeamAxisA * FMath::Cos(SampleAngle) + BeamAxisB * FMath::Sin(SampleAngle));
			const float ProjectionDistance = FVector::DotProduct(
				ImpactPoint - BeamPoint,
				SurfaceNormal) / ProjectionDenominator;
			const FVector ProjectedPoint = BeamPoint +
				SafeIncomingDirection * ProjectionDistance;
			const FVector LocalPoint = SurfaceTransform.InverseTransformPosition(ProjectedPoint);
			bool bContained = true;
			for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
			{
				if (AxisIndex == NormalAxis)
				{
					continue;
				}

				const float SafeAxisScale = FMath::Max(AbsoluteScale[AxisIndex], KINDA_SMALL_NUMBER);
				const float LocalInset = FullBeamFootprintEdgeInset / SafeAxisScale;
				const float AvailableExtent = FMath::Max(0.0f, LocalExtent[AxisIndex] - LocalInset);
				if (FMath::Abs(LocalPoint[AxisIndex]) > AvailableExtent)
				{
					bContained = false;
					break;
				}
			}

			if (bContained)
			{
				++ContainedSampleCount;
			}
		}
	}

	return static_cast<float>(ContainedSampleCount) /
		static_cast<float>(FootprintSampleCount);
}

bool UUOULightInteractionSurfaceComponent::HasMinimumBeamFootprintCoverage(
	float IncomingBeamRadius,
	const FVector& IncomingDirection,
	const FVector& HitNormal,
	const FVector& ImpactPoint,
	float RequiredCoverageRatio) const
{
	return CalculateBeamFootprintCoverageRatio(
		IncomingBeamRadius,
		IncomingDirection,
		HitNormal,
		ImpactPoint) >= FMath::Clamp(RequiredCoverageRatio, 0.0f, 1.0f);
}

float UUOULightInteractionSurfaceComponent::GetStartingBeamFootprintCoverageRatio() const
{
	return 1.0f - FMath::Clamp(BeamFootprintOverflowAllowancePercent, 0.0f, 100.0f) * 0.01f;
}

float UUOULightInteractionSurfaceComponent::GetRetainedBeamFootprintCoverageRatio() const
{
	constexpr float RetainedOverflowBonusPercent = 10.0f;
	const float RetainedOverflowPercent = FMath::Clamp(
		BeamFootprintOverflowAllowancePercent + RetainedOverflowBonusPercent,
		0.0f,
		100.0f);
	return 1.0f - RetainedOverflowPercent * 0.01f;
}

void UUOULightInteractionSurfaceComponent::SetBeamFootprintOverflowAllowance(
	float OverflowAllowancePercent)
{
	BeamFootprintOverflowAllowancePercent = FMath::Clamp(
		OverflowAllowancePercent,
		0.0f,
		100.0f);
}

float UUOULightInteractionSurfaceComponent::ResolveReflectionConeAngle(float IncomingConeAngle) const
{
	const float ConeAngle = ReflectionConeAngleMode == EUOULightReflectionConeAngleMode::PreserveIncoming
		? IncomingConeAngle
		: ReflectionConeAngle;
	return FMath::Clamp(ConeAngle, 0.0f, 89.0f);
}

void UUOULightInteractionSurfaceComponent::GetReflectionSamplePositions(
	TArray<FVector>& OutSamplePositions) const
{
	OutSamplePositions.Reset();
	OutSamplePositions.Add(GetComponentLocation());
	if (!bUseSurfaceAreaSampling)
	{
		return;
	}

	const FVector Extent = GetScaledBoxExtent().GetAbs();
	TArray<TPair<float, FVector>, TInlineAllocator<3>> LocalAxes;
	LocalAxes.Emplace(Extent.X, FVector::ForwardVector);
	LocalAxes.Emplace(Extent.Y, FVector::RightVector);
	LocalAxes.Emplace(Extent.Z, FVector::UpVector);
	LocalAxes.Sort([](const TPair<float, FVector>& A, const TPair<float, FVector>& B)
	{
		return A.Key > B.Key;
	});

	const float SafeInset = FMath::Clamp(SurfaceSampleInset, 0.0f, 1.0f);
	for (int32 AxisIndex = 0; AxisIndex < 2; ++AxisIndex)
	{
		const float SampleDistance = LocalAxes[AxisIndex].Key * SafeInset;
		if (SampleDistance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector WorldOffset = GetComponentTransform().TransformVectorNoScale(
			LocalAxes[AxisIndex].Value * SampleDistance);
		OutSamplePositions.Add(GetComponentLocation() + WorldOffset);
		OutSamplePositions.Add(GetComponentLocation() - WorldOffset);
	}
}

FVector UUOULightInteractionSurfaceComponent::GetReflectionDirection(
	const FVector& IncomingDirection,
	const FVector& HitNormal) const
{
	if (ReflectionDirectionMode == EUOULightReflectionDirectionMode::ComponentForward)
	{
		return GetForwardVector().GetSafeNormal();
	}

	if (ReflectionDirectionMode == EUOULightReflectionDirectionMode::OwnerForward)
	{
		const AActor* OwnerActor = GetOwner();
		return OwnerActor != nullptr
			? OwnerActor->GetActorForwardVector().GetSafeNormal()
			: GetForwardVector().GetSafeNormal();
	}

	const FVector SafeIncomingDirection = IncomingDirection.GetSafeNormal();
	FVector ReflectionNormal = GetReflectionNormal(HitNormal);
	if (SafeIncomingDirection.IsNearlyZero() || ReflectionNormal.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	if (FVector::DotProduct(SafeIncomingDirection, ReflectionNormal) > 0.0f)
	{
		ReflectionNormal *= -1.0f;
	}

	return (SafeIncomingDirection - 2.0f * FVector::DotProduct(SafeIncomingDirection, ReflectionNormal) * ReflectionNormal).GetSafeNormal();
}

void UUOULightInteractionSurfaceComponent::ValidateSettings()
{
	ReflectionConeAngle = FMath::Clamp(ReflectionConeAngle, 0.0f, 89.0f);
	ReflectionIntensityMultiplier = FMath::Max(0.0f, ReflectionIntensityMultiplier);
	ReflectionStartPadding = FMath::Max(0.0f, ReflectionStartPadding);
	ReflectionApertureScale = FMath::Max(0.0f, ReflectionApertureScale);
	ReflectionImpactEdgeInset = FMath::Max(0.0f, ReflectionImpactEdgeInset);
	MinimumReflectionCoverageRatio = FMath::Clamp(MinimumReflectionCoverageRatio, 0.0f, 1.0f);
	MaximumReflectionIncidenceAngle = FMath::Clamp(
		MaximumReflectionIncidenceAngle,
		0.0f,
		180.0f);
	RetainedMaximumReflectionIncidenceAngle = FMath::Clamp(
		RetainedMaximumReflectionIncidenceAngle,
		MaximumReflectionIncidenceAngle,
		180.0f);
	FullBeamFootprintEdgeInset = FMath::Max(0.0f, FullBeamFootprintEdgeInset);
	BeamFootprintOverflowAllowancePercent = FMath::Clamp(
		BeamFootprintOverflowAllowancePercent,
		0.0f,
		100.0f);
	SurfaceSampleInset = FMath::Clamp(SurfaceSampleInset, 0.0f, 1.0f);
}

void UUOULightInteractionSurfaceComponent::ApplyCollisionSettings()
{
	const bool bEnableLightCollision = CanBlockLight() || CanReflectLight();

	SetCollisionEnabled(bEnableLightCollision ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetGenerateOverlapEvents(false);
}

FVector UUOULightInteractionSurfaceComponent::GetReflectionNormal(const FVector& HitNormal) const
{
	switch (ReflectionNormalMode)
	{
	case EUOULightReflectionNormalMode::ComponentUp:
		return GetUpVector().GetSafeNormal();
	case EUOULightReflectionNormalMode::ComponentForward:
		return GetForwardVector().GetSafeNormal();
	case EUOULightReflectionNormalMode::HitNormal:
	default:
		return HitNormal.GetSafeNormal();
	}
}

FVector UUOULightInteractionSurfaceComponent::GetReflectionFrontNormal() const
{
	switch (ReflectionFrontNormalMode)
	{
	case EUOULightReflectionFrontNormalMode::ComponentUp:
		return GetUpVector().GetSafeNormal();
	case EUOULightReflectionFrontNormalMode::OwnerForward:
		if (const AActor* OwnerActor = GetOwner())
		{
			return OwnerActor->GetActorForwardVector().GetSafeNormal();
		}
		return GetForwardVector().GetSafeNormal();
	case EUOULightReflectionFrontNormalMode::ComponentForward:
	default:
		return GetForwardVector().GetSafeNormal();
	}
}

float UUOULightInteractionSurfaceComponent::GetReflectionApertureRadius() const
{
	const FVector Extent = GetScaledBoxExtent().GetAbs();
	const float SmallestExtent = FMath::Min3(Extent.X, Extent.Y, Extent.Z);
	const float LargestExtent = FMath::Max3(Extent.X, Extent.Y, Extent.Z);
	const float MiddleExtent = Extent.X + Extent.Y + Extent.Z - SmallestExtent - LargestExtent;
	return FMath::Max(0.0f, MiddleExtent * ReflectionApertureScale);
}
