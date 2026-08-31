// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOURewardCollectedConditionComponent.generated.h"

class AActor;
class AUOURewardActor;

// 지정한 Reward의 수집 연출과 진행도 기록이 모두 완료된 상태를 퍼즐 조건으로 제공합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Reward Collected Condition"))
class UNDERONEUMBRELLA_API UUOURewardCollectedConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOURewardCollectedConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	// 비워두면 이 컴포넌트를 소유한 Reward Actor를 자동으로 관찰합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Reward")
	TObjectPtr<AUOURewardActor> TargetReward = nullptr;

	// 런타임에 실제로 수집 완료 이벤트를 관찰하는 Reward입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime")
	TObjectPtr<AUOURewardActor> ResolvedReward = nullptr;

private:
	UFUNCTION()
	void HandleRewardCollected(AUOURewardActor* RewardActor, FName RewardId, AActor* Collector);

	AUOURewardActor* ResolveTargetReward() const;
	void SubscribeRewardCollection();
	void UnsubscribeRewardCollection();
};
