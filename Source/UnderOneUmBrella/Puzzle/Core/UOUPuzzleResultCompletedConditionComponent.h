// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "Puzzle/Core/UOUPuzzleResultCompletionState.h"
#include "UOUPuzzleResultCompletedConditionComponent.generated.h"

class AActor;

// 다른 퍼즐 결과 액터의 완료 상태를 현재 퍼즐의 조건으로 변환하는 컴포넌트입니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Puzzle Result Completed Condition"))
class UNDERONEUMBRELLA_API UUOUPuzzleResultCompletedConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUPuzzleResultCompletedConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Result", meta = (ToolTip = "완료 여부를 확인할 이전 퍼즐의 결과 액터입니다."))
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Result", meta = (ToolTip = "완료되어야 하는 결과 액션입니다. 보통 이전 퍼즐의 Activate를 사용합니다."))
	EOUUPuzzleResultAction RequiredAction = EOUUPuzzleResultAction::Activate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime", meta = (ToolTip = "현재 완료 상태 변경 이벤트를 구독 중인 액터입니다."))
	TObjectPtr<AActor> SubscribedTargetActor = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Result")
	void RefreshConditionState();

protected:
	void SubscribeTargetCompletionState();
	void UnsubscribeTargetCompletionState();
	bool IsTargetResultCompleted() const;
	void HandleTargetCompletionStateChanged(EOUUPuzzleResultAction Action, bool bIsCompleted);

	FDelegateHandle CompletionStateChangedHandle;
};
