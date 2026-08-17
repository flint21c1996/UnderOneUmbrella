// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUWaterAmountConditionComponent.h"

#include "GameFramework/Actor.h"
#include "Player/UOUWaterContainerComponent.h"

UUOUWaterAmountConditionComponent::UUOUWaterAmountConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUWaterAmountConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	RequiredAmount = FMath::Max(0.0f, RequiredAmount);
	ResolveWaterContainer();
	SubscribeWaterContainer();
	RefreshSatisfiedState();
}

void UUOUWaterAmountConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeWaterContainer();
	Super::EndPlay(EndPlayReason);
}

FText UUOUWaterAmountConditionComponent::GetDebugSummaryText_Implementation() const
{
	const float CurrentAmount = WaterContainer != nullptr ? WaterContainer->CurrentAmount : 0.0f;
	const TArray<FString> DebugLines = {
		FString::Printf(
			TEXT("Water Amount: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(
			TEXT("Water: %.1f / %.1f"),
			CurrentAmount,
			GetRequiredAmount()),
		FString::Printf(TEXT("Container: %s"), *GetNameSafe(WaterContainer))
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOUWaterAmountConditionComponent::HandleWaterAmountChanged(float NewAmount, float MaxAmount)
{
	RefreshSatisfiedState();
}

void UUOUWaterAmountConditionComponent::ResolveWaterContainer()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (UActorComponent* ReferencedComponent = WaterContainerReference.GetComponent(Owner))
	{
		WaterContainer = Cast<UUOUWaterContainerComponent>(ReferencedComponent);
	}

	if (WaterContainer == nullptr && bAutoFindWaterContainer)
	{
		WaterContainer = Owner->FindComponentByClass<UUOUWaterContainerComponent>();
	}
}

void UUOUWaterAmountConditionComponent::SubscribeWaterContainer()
{
	if (WaterContainer == nullptr)
	{
		return;
	}

	WaterContainer->OnWaterAmountChanged.RemoveDynamic(this, &UUOUWaterAmountConditionComponent::HandleWaterAmountChanged);
	WaterContainer->OnWaterAmountChanged.AddDynamic(this, &UUOUWaterAmountConditionComponent::HandleWaterAmountChanged);
}

void UUOUWaterAmountConditionComponent::UnsubscribeWaterContainer()
{
	if (WaterContainer != nullptr)
	{
		WaterContainer->OnWaterAmountChanged.RemoveDynamic(this, &UUOUWaterAmountConditionComponent::HandleWaterAmountChanged);
	}
}

void UUOUWaterAmountConditionComponent::RefreshSatisfiedState()
{
	const bool bNextSatisfied = WaterContainer != nullptr && WaterContainer->CurrentAmount >= GetRequiredAmount();
	SetSatisfiedState(bNextSatisfied, true);
}

float UUOUWaterAmountConditionComponent::GetRequiredAmount() const
{
	if (bUseContainerMaxAmountAsRequirement && WaterContainer != nullptr)
	{
		return WaterContainer->MaxAmount;
	}

	return RequiredAmount;
}
