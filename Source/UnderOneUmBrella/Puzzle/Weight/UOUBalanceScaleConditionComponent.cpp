// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUBalanceScaleConditionComponent.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleWeightSource.h"

UUOUBalanceScaleConditionComponent::UUOUBalanceScaleConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UUOUBalanceScaleConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	AllowedDifference = FMath::Max(0.0f, AllowedDifference);
	MinimumTotalWeight = FMath::Max(0.0f, MinimumTotalWeight);
	ResolveWeightSources();
	RefreshBalanceState();
}

void UUOUBalanceScaleConditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshBalanceState();
}

FText UUOUBalanceScaleConditionComponent::GetDebugSummaryText_Implementation() const
{
	const TArray<FString> DebugLines = {
		FString::Printf(
			TEXT("Balance Scale: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(
			TEXT("Left / Right: %.1f / %.1f"),
			LeftWeight,
			RightWeight),
		FString::Printf(
			TEXT("Diff: %.1f / %.1f"),
			WeightDifference,
			AllowedDifference)
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOUBalanceScaleConditionComponent::ResolveWeightSources()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	ResolvedLeftWeightSource = LeftWeightSource.GetComponent(Owner);
	ResolvedRightWeightSource = RightWeightSource.GetComponent(Owner);
}

void UUOUBalanceScaleConditionComponent::RefreshBalanceState()
{
	LeftWeight = GetWeightFromComponent(ResolvedLeftWeightSource);
	RightWeight = GetWeightFromComponent(ResolvedRightWeightSource);
	WeightDifference = FMath::Abs(LeftWeight - RightWeight);

	const float TotalWeight = LeftWeight + RightWeight;
	const bool bHasEnoughWeight = TotalWeight >= MinimumTotalWeight;
	const bool bNextSatisfied = bHasEnoughWeight && WeightDifference <= AllowedDifference;
	SetSatisfiedState(bNextSatisfied, true);
}

float UUOUBalanceScaleConditionComponent::GetWeightFromComponent(const UActorComponent* Component) const
{
	if (const IUOUPuzzleWeightSource* WeightSource = Cast<IUOUPuzzleWeightSource>(Component))
	{
		return FMath::Max(0.0f, WeightSource->GetPuzzleWeight());
	}

	return 0.0f;
}
