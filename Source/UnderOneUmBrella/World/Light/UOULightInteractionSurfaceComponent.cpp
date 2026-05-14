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
		ReflectionConeAngle > 0.0f &&
		ReflectionIntensityMultiplier > 0.0f;
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
