// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUPuzzleWeightComponent.h"

#include "GameFramework/Actor.h"
#include "Player/UOUWaterContainerComponent.h"

UUOUPuzzleWeightComponent::UUOUPuzzleWeightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUPuzzleWeightComponent::BeginPlay()
{
	Super::BeginPlay();

	BaseWeight = FMath::Max(0.0f, BaseWeight);
	if (WaterContainer == nullptr)
	{
		if (AActor* Owner = GetOwner())
		{
			WaterContainer = Owner->FindComponentByClass<UUOUWaterContainerComponent>();
		}
	}
}

float UUOUPuzzleWeightComponent::GetPuzzleWeight() const
{
	return BaseWeight + GetWaterWeightContribution();
}

float UUOUPuzzleWeightComponent::GetWaterWeightContribution() const
{
	if (!bIncludeStoredWaterWeight || WaterContainer == nullptr)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, WaterContainer->GetWeightContribution());
}
