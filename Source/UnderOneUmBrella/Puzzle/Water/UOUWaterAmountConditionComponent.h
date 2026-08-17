// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUWaterAmountConditionComponent.generated.h"

class UUOUWaterContainerComponent;

// 물 컨테이너의 현재 물 양이 목표 수치에 도달했는지 검사하는 조건 컴포넌트입니다.
// 우산 타겟이나 수조 퍼즐처럼 물 저장량으로 퍼즐을 푸는 경우에 사용합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUWaterAmountConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUWaterAmountConditionComponent();

	// 시작 시 물 컨테이너를 찾고 양 변화 이벤트를 구독합니다.
	virtual void BeginPlay() override;

	// 종료 시 구독한 물 컨테이너 이벤트를 정리합니다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FText GetDebugSummaryText_Implementation() const override;

	// 같은 액터 안의 물 컨테이너를 자동으로 찾을지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Water")
	bool bAutoFindWaterContainer = true;

	// 수동으로 연결할 물 컨테이너 참조입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Water")
	FComponentReference WaterContainerReference;

	// 컨테이너 최대량을 그대로 요구 수치로 사용할지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Water")
	bool bUseContainerMaxAmountAsRequirement = false;

	// 직접 지정하는 최소 요구 물 양입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Water", meta = (ClampMin = "0.0"))
	float RequiredAmount = 1.0f;

	// 실제로 연결된 물 컨테이너 런타임 참조입니다.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	TObjectPtr<UUOUWaterContainerComponent> WaterContainer = nullptr;

protected:
	// 물 양이 바뀔 때 조건 만족 여부를 다시 계산합니다.
	UFUNCTION()
	void HandleWaterAmountChanged(float NewAmount, float MaxAmount);

	// 참조나 자동 탐색을 통해 사용할 물 컨테이너를 결정합니다.
	void ResolveWaterContainer();

	// 연결된 물 컨테이너의 양 변화 이벤트를 구독합니다.
	void SubscribeWaterContainer();

	// 연결된 물 컨테이너 이벤트 구독을 해제합니다.
	void UnsubscribeWaterContainer();

	// 현재 물 양과 목표값을 비교해서 조건 상태를 갱신합니다.
	void RefreshSatisfiedState();

	// 설정에 따라 실제 요구 물 양을 계산합니다.
	float GetRequiredAmount() const;
};
