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

TArray<FString> UUOUPuzzleConditionSourceComponent::GetPuzzleDebugInfo_Implementation() const
{
	return {
		FString::Printf(
			TEXT("Condition: %s"),
			bIsSatisfied ? TEXT("Satisfied") : TEXT("Unsatisfied"))
	};
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
