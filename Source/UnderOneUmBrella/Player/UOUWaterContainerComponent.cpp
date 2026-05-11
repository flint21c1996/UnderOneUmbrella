// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUWaterContainerComponent.h"

UUOUWaterContainerComponent::UUOUWaterContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUWaterContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxAmount = FMath::Max(0.0f, MaxAmount);
	WeightMultiplier = FMath::Max(0.0f, WeightMultiplier);
	SetAmount(InitialAmount);
}

float UUOUWaterContainerComponent::AddAmount(float AmountToAdd)
{
	if (AmountToAdd <= 0.0f)
	{
		return CurrentAmount;
	}

	SetAmount(CurrentAmount + AmountToAdd);
	return CurrentAmount;
}

float UUOUWaterContainerComponent::RemoveAmount(float AmountToRemove)
{
	if (AmountToRemove <= 0.0f)
	{
		return CurrentAmount;
	}

	SetAmount(CurrentAmount - AmountToRemove);
	return CurrentAmount;
}

void UUOUWaterContainerComponent::SetAmount(float NewAmount)
{
	const float ClampedAmount = FMath::Clamp(NewAmount, 0.0f, MaxAmount);
	if (FMath::IsNearlyEqual(CurrentAmount, ClampedAmount))
	{
		CurrentAmount = ClampedAmount;
		return;
	}

	CurrentAmount = ClampedAmount;
	BroadcastAmountChanged();
}

float UUOUWaterContainerComponent::GetFillRatio() const
{
	return MaxAmount > 0.0f ? CurrentAmount / MaxAmount : 0.0f;
}

float UUOUWaterContainerComponent::GetWeightContribution() const
{
	return CurrentAmount * WeightMultiplier;
}

void UUOUWaterContainerComponent::BroadcastAmountChanged()
{
	OnWaterAmountChanged.Broadcast(CurrentAmount, MaxAmount);
}
