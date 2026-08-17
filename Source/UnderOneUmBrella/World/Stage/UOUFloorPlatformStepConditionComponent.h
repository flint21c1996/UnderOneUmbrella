// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "Puzzle/Core/UOUPuzzleResultCompletionState.h"
#include "UOUFloorPlatformStepConditionComponent.generated.h"

class AUOUFloorPlatformActor;

// 플랫폼이 지정한 MoveStep까지 도착했는지를 퍼즐 조건으로 바꾸는 컴포넌트입니다.
// 버튼 조건과 함께 ConditionGroup에 넣으면 특정 위치에 도착한 뒤 버튼을 눌러야 하는 흐름을 만들 수 있습니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Floor Platform Step Condition"))
class UNDERONEUMBRELLA_API UUOUFloorPlatformStepConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUFloorPlatformStepConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	// 도착 여부를 확인할 플랫폼입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Floor Step")
	TObjectPtr<AUOUFloorPlatformActor> TargetPlatform = nullptr;

	// 만족해야 하는 MoveStep 배열 인덱스입니다.
	// 에디터 배열 기준이라 첫 번째 MoveStep은 0, 두 번째 MoveStep은 1입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Floor Step", meta = (ClampMin = "0"))
	int32 RequiredStepIndex = 0;

	// 켜져 있으면 플랫폼이 이동 중일 때는 아직 조건을 만족하지 않은 것으로 봅니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Floor Step")
	bool bRequireNotMoving = true;

	// TargetPlatform이 비어 있으면 이 컴포넌트의 소유자를 플랫폼으로 자동 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Floor Step")
	bool bAutoResolveOwnerPlatform = true;

	// 현재 플랫폼 상태를 다시 읽어서 조건 만족 여부를 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Floor Step")
	void RefreshConditionState();

protected:
	// 플랫폼 이동 완료 이벤트가 오면 조건을 다시 계산합니다.
	UFUNCTION()
	void HandleTargetPlatformMoveFinished(AUOUFloorPlatformActor* Platform);

	void HandleTargetPlatformCompletionStateChanged(EOUUPuzzleResultAction Action, bool bIsCompleted);
	void ResolveTargetPlatform();
	void SubscribeTargetPlatform();
	void UnsubscribeTargetPlatform();

	// 현재 이동 완료 이벤트를 구독 중인 플랫폼입니다.
	UPROPERTY(Transient)
	TObjectPtr<AUOUFloorPlatformActor> SubscribedTargetPlatform = nullptr;

	FDelegateHandle CompletionStateChangedHandle;
};
