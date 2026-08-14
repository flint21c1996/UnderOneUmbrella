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

#if UOU_WITH_PUZZLE_CHEATS
bool UUOUPuzzleConditionSourceComponent::TryResolveInputForCheat(AActor* InputActor)
{
	if (!IsValid(InputActor))
	{
		return false;
	}

	SetSatisfiedState(true, true);
	return IsSatisfied();
}
#endif

TArray<FString> UUOUPuzzleConditionSourceComponent::GetPuzzleDebugInfo_Implementation() const
{
	return {
		FString::Printf(
			TEXT("Condition: %s"),
			bIsSatisfied ? TEXT("Satisfied") : TEXT("Unsatisfied"))
	};
}

void UUOUPuzzleConditionSourceComponent::GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const
{
}

bool UUOUPuzzleConditionSourceComponent::SetSatisfiedState(bool bNewSatisfied, bool bBroadcastChange)
{
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
