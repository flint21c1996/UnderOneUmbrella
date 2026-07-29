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
	if (LightInteractionMode == NewMode)
	{
		return;
	}

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
		ReflectionRange > 0.0f &&
		(ReflectionConeAngleMode == EUOULightReflectionConeAngleMode::PreserveIncoming ||
			ReflectionConeAngle > 0.0f) &&
		ReflectionIntensityMultiplier > 0.0f &&
		(!bLimitReflectionBySurfaceAperture || GetReflectionApertureRadius() > KINDA_SMALL_NUMBER);
}

bool UUOULightInteractionSurfaceComponent::CanReflectIncomingLight(
	const FVector& IncomingDirection,
	const FVector& HitNormal) const
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
	if (bReflectFrontFaceOnly && FrontDot <= 0.0f)
	{
		return false;
	}
	if (!bReflectFrontFaceOnly)
	{
		FrontDot = FMath::Abs(FrontDot);
	}

	const float IncidenceAngle = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(FrontDot, 0.0f, 1.0f)));
	return IncidenceAngle <= MaximumReflectionIncidenceAngle;
}

float UUOULightInteractionSurfaceComponent::ClampReflectionBeamRadius(
	float IncomingBeamRadius,
	const FVector& IncomingDirection,
	const FVector& HitNormal) const
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
	return FMath::Min(SafeIncomingRadius, EffectiveApertureRadius);
}

float UUOULightInteractionSurfaceComponent::ResolveReflectionConeAngle(float IncomingConeAngle) const
{
	const float ConeAngle = ReflectionConeAngleMode == EUOULightReflectionConeAngleMode::PreserveIncoming
		? IncomingConeAngle
		: ReflectionConeAngle;
	return FMath::Clamp(ConeAngle, 1.0f, 89.0f);
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
	ReflectionRange = FMath::Max(0.0f, ReflectionRange);
	ReflectionConeAngle = FMath::Clamp(ReflectionConeAngle, 1.0f, 89.0f);
	ReflectionIntensityMultiplier = FMath::Max(0.0f, ReflectionIntensityMultiplier);
	ReflectionStartPadding = FMath::Max(0.0f, ReflectionStartPadding);
	MaximumReflectionIncidenceAngle = FMath::Clamp(MaximumReflectionIncidenceAngle, 0.0f, 89.0f);
	ReflectionApertureScale = FMath::Max(0.0f, ReflectionApertureScale);
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
