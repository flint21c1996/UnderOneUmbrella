// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformStepComponent.h"

#include "World/Stage/UOUFloorPlatformTargetActor.h"

UUOUFloorPlatformStepComponent::UUOUFloorPlatformStepComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUFloorPlatformStepComponent::ResetRuntimeState()
{
	ActiveTargetIndex = INDEX_NONE;
	bNextMoveReturnsToStart = false;
}

void UUOUFloorPlatformStepComponent::ClearReturnToStartRequest()
{
	bNextMoveReturnsToStart = false;
}

void UUOUFloorPlatformStepComponent::SetActiveTargetIndex(int32 TargetIndex)
{
	ActiveTargetIndex = TargetIndex;
}

bool UUOUFloorPlatformStepComponent::ShouldUseMoveSteps(
	bool bUseMoveSteps,
	const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
	int32 CurrentTargetIndex) const
{
	return bUseMoveSteps && GetCurrentTargetMarker(bUseMoveSteps, TargetMarkers, CurrentTargetIndex) != nullptr;
}

AUOUFloorPlatformTargetActor* UUOUFloorPlatformStepComponent::GetTargetMarkerAt(
	const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
	int32 TargetIndex) const
{
	if (!TargetMarkers.IsValidIndex(TargetIndex))
	{
		return nullptr;
	}

	return TargetMarkers[TargetIndex].Get();
}

AUOUFloorPlatformTargetActor* UUOUFloorPlatformStepComponent::GetCurrentTargetMarker(
	bool bUseMoveSteps,
	const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
	int32 CurrentTargetIndex) const
{
	if (!bUseMoveSteps || TargetMarkers.Num() == 0)
	{
		return nullptr;
	}

	const int32 ClampedIndex = FMath::Clamp(CurrentTargetIndex, 0, TargetMarkers.Num() - 1);
	if (AUOUFloorPlatformTargetActor* DirectTargetMarker = GetTargetMarkerAt(TargetMarkers, ClampedIndex))
	{
		return DirectTargetMarker;
	}

	for (int32 TargetIndex = 0; TargetIndex < TargetMarkers.Num(); ++TargetIndex)
	{
		if (AUOUFloorPlatformTargetActor* TargetMarker = GetTargetMarkerAt(TargetMarkers, TargetIndex))
		{
			return TargetMarker;
		}
	}

	return nullptr;
}

bool UUOUFloorPlatformStepComponent::ResolveNextTargetTransform(
	const FTransform& StartTransform,
	bool bUseMoveSteps,
	const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
	int32 CurrentTargetIndex,
	FTransform& OutTargetTransform,
	int32& OutTargetIndex) const
{
	OutTargetTransform = FTransform::Identity;
	OutTargetIndex = INDEX_NONE;

	if (bNextMoveReturnsToStart)
	{
		OutTargetTransform = StartTransform;
		return true;
	}

	if (!bUseMoveSteps || TargetMarkers.Num() == 0)
	{
		return false;
	}

	const int32 ClampedIndex = FMath::Clamp(CurrentTargetIndex, 0, TargetMarkers.Num() - 1);
	if (AUOUFloorPlatformTargetActor* DirectTargetMarker = GetTargetMarkerAt(TargetMarkers, ClampedIndex))
	{
		OutTargetTransform = DirectTargetMarker->GetActorTransform();
		OutTargetIndex = ClampedIndex;
		return true;
	}

	for (int32 TargetIndex = 0; TargetIndex < TargetMarkers.Num(); ++TargetIndex)
	{
		if (AUOUFloorPlatformTargetActor* TargetMarker = GetTargetMarkerAt(TargetMarkers, TargetIndex))
		{
			OutTargetTransform = TargetMarker->GetActorTransform();
			OutTargetIndex = TargetIndex;
			return true;
		}
	}

	return false;
}

void UUOUFloorPlatformStepComponent::AdvanceTargetIndex(
	bool bUseMoveSteps,
	const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
	bool bLoopMoveSteps,
	bool bLoopThroughStart,
	bool bPingPongMoveSteps,
	int32& InOutMoveDirection,
	int32& InOutCurrentTargetIndex)
{
	if (!bUseMoveSteps || ActiveTargetIndex == INDEX_NONE || TargetMarkers.Num() == 0)
	{
		ActiveTargetIndex = INDEX_NONE;
		return;
	}

	const int32 TargetCount = TargetMarkers.Num();
	const int32 MoveDirection = InOutMoveDirection >= 0 ? 1 : -1;

	auto TryAdvanceInDirection = [this, &TargetMarkers, TargetCount, &InOutCurrentTargetIndex](int32 Direction) -> bool
	{
		for (int32 CandidateIndex = ActiveTargetIndex + Direction;
			CandidateIndex >= 0 && CandidateIndex < TargetCount;
			CandidateIndex += Direction)
		{
			if (GetTargetMarkerAt(TargetMarkers, CandidateIndex) != nullptr)
			{
				InOutCurrentTargetIndex = CandidateIndex;
				return true;
			}
		}
		return false;
	};

	if (TryAdvanceInDirection(MoveDirection))
	{
		ActiveTargetIndex = INDEX_NONE;
		return;
	}

	if (bPingPongMoveSteps)
	{
		const int32 ReversedMoveDirection = -MoveDirection;
		if (TryAdvanceInDirection(ReversedMoveDirection))
		{
			InOutMoveDirection = ReversedMoveDirection;
			ActiveTargetIndex = INDEX_NONE;
			return;
		}

		InOutCurrentTargetIndex = FMath::Clamp(ActiveTargetIndex, 0, TargetCount - 1);
		ActiveTargetIndex = INDEX_NONE;
		return;
	}

	if (bLoopMoveSteps)
	{
		for (int32 CandidateIndex = 0; CandidateIndex < TargetCount; ++CandidateIndex)
		{
			if (GetTargetMarkerAt(TargetMarkers, CandidateIndex) != nullptr)
			{
				InOutCurrentTargetIndex = CandidateIndex;
				bNextMoveReturnsToStart = bLoopThroughStart;
				ActiveTargetIndex = INDEX_NONE;
				return;
			}
		}
	}

	InOutCurrentTargetIndex = FMath::Clamp(ActiveTargetIndex, 0, TargetCount - 1);
	ActiveTargetIndex = INDEX_NONE;
}
