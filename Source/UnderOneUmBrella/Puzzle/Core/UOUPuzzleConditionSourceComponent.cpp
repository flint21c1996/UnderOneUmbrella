// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUPuzzleConditionSourceComponent.h"

UUOUPuzzleConditionSourceComponent::UUOUPuzzleConditionSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UUOUPuzzleConditionSourceComponent::IsSatisfied() const
{
	return bIsSatisfied;
}

bool UUOUPuzzleConditionSourceComponent::SetConditionSatisfied(bool bNewSatisfied)
{
	return SetSatisfiedState(bNewSatisfied, true);
}

#if UOU_WITH_PUZZLE_CHEATS
bool UUOUPuzzleConditionSourceComponent::TryResolveInputForCheat(AActor* InputActor)
{
	if (!IsValid(InputActor))
	{
		return false;
	}

	bCheatForceSatisfied = true;
	SetSatisfiedState(true, true);
	return IsSatisfied();
}
#endif

void UUOUPuzzleConditionSourceComponent::GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const
{
}

EUOUDebugCategory UUOUPuzzleConditionSourceComponent::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

FText UUOUPuzzleConditionSourceComponent::GetDebugSummaryText_Implementation() const
{
	return FText::FromString(FString::Printf(
		TEXT("Condition: %s"),
		bIsSatisfied ? TEXT("Satisfied") : TEXT("Unsatisfied")));
}

#if UOU_WITH_DEVELOPMENT_TOOLS
bool UUOUPuzzleConditionSourceComponent::ShouldDrawDevelopmentDebugLabel() const
{
	return false;
}
#endif

bool UUOUPuzzleConditionSourceComponent::SetSatisfiedState(bool bNewSatisfied, bool bBroadcastChange)
{
#if UOU_WITH_PUZZLE_CHEATS
	if (bCheatForceSatisfied && !bNewSatisfied)
	{
		return false;
	}
#endif

	if (bIsSatisfied == bNewSatisfied)
	{
		return false;
	}

	bIsSatisfied = bNewSatisfied;
	if (bBroadcastChange)
	{
		OnConditionChanged.Broadcast(bIsSatisfied);
	}

	return true;
}
