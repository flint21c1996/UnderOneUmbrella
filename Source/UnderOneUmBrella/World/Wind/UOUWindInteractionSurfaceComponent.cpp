// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Wind/UOUWindInteractionSurfaceComponent.h"

#include "GameFramework/Actor.h"

UUOUWindInteractionSurfaceComponent::UUOUWindInteractionSurfaceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitBoxExtent(FVector(100.0f, 100.0f, 8.0f));
	SetHiddenInGame(true);
	ApplyCollisionSettings();
}

void UUOUWindInteractionSurfaceComponent::OnRegister()
{
	Super::OnRegister();
	ValidateSettings();
	ApplyCollisionSettings();
}

#if WITH_EDITOR
void UUOUWindInteractionSurfaceComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ValidateSettings();
	ApplyCollisionSettings();
}
#endif

void UUOUWindInteractionSurfaceComponent::SetInteractionMode(EUOUWindInteractionMode NewMode)
{
	if (InteractionMode == NewMode)
	{
		return;
	}

	InteractionMode = NewMode;
	ApplyCollisionSettings();
}

bool UUOUWindInteractionSurfaceComponent::CanReflectWind() const
{
	return (InteractionMode == EUOUWindInteractionMode::Reflecting
		|| InteractionMode == EUOUWindInteractionMode::Redirecting)
		&& StrengthRetention > 0.0f;
}

bool UUOUWindInteractionSurfaceComponent::CanBlockWind() const
{
	return InteractionMode != EUOUWindInteractionMode::Disabled;
}

FVector UUOUWindInteractionSurfaceComponent::GetOutgoingDirection(
	const FVector& IncomingDirection,
	const FVector& HitNormal) const
{
	if (InteractionMode == EUOUWindInteractionMode::Redirecting)
	{
		const AActor* OwnerActor = GetOwner();
		return RedirectDirectionMode == EUOUWindRedirectDirectionMode::OwnerForward
			&& OwnerActor != nullptr
			? OwnerActor->GetActorForwardVector().GetSafeNormal()
			: GetForwardVector().GetSafeNormal();
	}

	const FVector SafeIncomingDirection = IncomingDirection.GetSafeNormal();
	FVector ReflectionNormal = ResolveReflectionNormal(HitNormal);
	if (SafeIncomingDirection.IsNearlyZero() || ReflectionNormal.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	if (FVector::DotProduct(SafeIncomingDirection, ReflectionNormal) > 0.0f)
	{
		ReflectionNormal *= -1.0f;
	}

	return SafeIncomingDirection
		.MirrorByVector(ReflectionNormal)
		.GetSafeNormal();
}

void UUOUWindInteractionSurfaceComponent::ValidateSettings()
{
	StrengthRetention = FMath::Clamp(StrengthRetention, 0.0f, 1.0f);
	ReflectionStartPadding = FMath::Max(0.1f, ReflectionStartPadding);
}

void UUOUWindInteractionSurfaceComponent::ApplyCollisionSettings()
{
	SetCollisionEnabled(CanBlockWind() ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetGenerateOverlapEvents(false);
}

FVector UUOUWindInteractionSurfaceComponent::ResolveReflectionNormal(const FVector& HitNormal) const
{
	switch (ReflectionNormalMode)
	{
	case EUOUWindReflectionNormalMode::ComponentUp:
		return GetUpVector().GetSafeNormal();
	case EUOUWindReflectionNormalMode::ComponentForward:
		return GetForwardVector().GetSafeNormal();
	case EUOUWindReflectionNormalMode::HitNormal:
	default:
		return HitNormal.GetSafeNormal();
	}
}
