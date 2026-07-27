// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaLightShadeVolumeComponent.h"

UUOUUmbrellaLightShadeVolumeComponent::UUOUUmbrellaLightShadeVolumeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitBoxExtent(FVector(90.0f, 90.0f, 100.0f));
	SetHiddenInGame(true);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	ApplyCollisionSettings();
}

void UUOUUmbrellaLightShadeVolumeComponent::OnRegister()
{
	Super::OnRegister();
	ApplyCollisionSettings();
}

void UUOUUmbrellaLightShadeVolumeComponent::SetShadeEnabled(bool bNewShadeEnabled)
{
	if (bShadeEnabled == bNewShadeEnabled)
	{
		return;
	}

	bShadeEnabled = bNewShadeEnabled;
	ApplyCollisionSettings();
}

bool UUOUUmbrellaLightShadeVolumeComponent::CanShadeLight() const
{
	return bShadeEnabled && !GetUnscaledBoxExtent().IsNearlyZero();
}

bool UUOUUmbrellaLightShadeVolumeComponent::ContainsWorldPosition(const FVector& WorldPosition) const
{
	if (!CanShadeLight())
	{
		return false;
	}

	const FVector LocalPosition = GetComponentTransform().InverseTransformPosition(WorldPosition);
	const FVector ShadeBoxExtent = GetUnscaledBoxExtent();
	return FMath::Abs(LocalPosition.X) <= ShadeBoxExtent.X
		&& FMath::Abs(LocalPosition.Y) <= ShadeBoxExtent.Y
		&& FMath::Abs(LocalPosition.Z) <= ShadeBoxExtent.Z;
}

void UUOUUmbrellaLightShadeVolumeComponent::ApplyCollisionSettings()
{
	SetCollisionEnabled(CanShadeLight() ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetGenerateOverlapEvents(false);
}
