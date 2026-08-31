// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Reward/UOURewardCollectedConditionComponent.h"

#include "World/Rewards/UOURewardActor.h"

UUOURewardCollectedConditionComponent::UUOURewardCollectedConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOURewardCollectedConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolvedReward = ResolveTargetReward();
	SetSatisfiedState(
		ResolvedReward != nullptr && ResolvedReward->IsCollectionCompleted(),
		false);
	SubscribeRewardCollection();
}

void UUOURewardCollectedConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeRewardCollection();
	ResolvedReward = nullptr;

	Super::EndPlay(EndPlayReason);
}

FText UUOURewardCollectedConditionComponent::GetDebugSummaryText_Implementation() const
{
	const TArray<FString> DebugLines = {
		FString::Printf(
			TEXT("Reward Collected Condition: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(TEXT("Reward: %s"), *GetNameSafe(ResolvedReward.Get()))
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOURewardCollectedConditionComponent::GetPuzzleDebugInputActors_Implementation(
	TArray<AActor*>& OutInputActors) const
{
	if (ResolvedReward != nullptr)
	{
		OutInputActors.AddUnique(ResolvedReward.Get());
	}
}

void UUOURewardCollectedConditionComponent::HandleRewardCollected(
	AUOURewardActor* RewardActor,
	FName RewardId,
	AActor* Collector)
{
	if (RewardActor != nullptr && RewardActor == ResolvedReward)
	{
		// Reward는 한 번만 수집되므로 만족 상태도 해제하지 않는 latch 조건으로 유지합니다.
		SetSatisfiedState(true, true);
	}
}

AUOURewardActor* UUOURewardCollectedConditionComponent::ResolveTargetReward() const
{
	if (TargetReward != nullptr)
	{
		return TargetReward.Get();
	}

	return Cast<AUOURewardActor>(GetOwner());
}

void UUOURewardCollectedConditionComponent::SubscribeRewardCollection()
{
	UnsubscribeRewardCollection();

	if (ResolvedReward != nullptr)
	{
		ResolvedReward->OnRewardCollected.RemoveDynamic(
			this,
			&UUOURewardCollectedConditionComponent::HandleRewardCollected);
		ResolvedReward->OnRewardCollected.AddDynamic(
			this,
			&UUOURewardCollectedConditionComponent::HandleRewardCollected);
	}
}

void UUOURewardCollectedConditionComponent::UnsubscribeRewardCollection()
{
	if (ResolvedReward != nullptr)
	{
		ResolvedReward->OnRewardCollected.RemoveDynamic(
			this,
			&UUOURewardCollectedConditionComponent::HandleRewardCollected);
	}
}
