// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "TimerManager.h"
#include "UOUTimerConditionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOUTimerConditionSignature);

// 지정한 시간이 지나면 만족 상태가 되는 퍼즐 조건 컴포넌트입니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Timer Condition"))
class UNDERONEUMBRELLA_API UUOUTimerConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUTimerConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FText GetDebugSummaryText_Implementation() const override;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Timer")
	FUOUTimerConditionSignature OnTimerStarted;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Timer")
	FUOUTimerConditionSignature OnTimerFinished;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Timer")
	FUOUTimerConditionSignature OnTimerStopped;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Timer", meta = (ToolTip = "게임 시작 시 조건의 초기 만족 상태입니다."))
	bool bInitialSatisfied = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Timer", meta = (ToolTip = "게임 시작 시 자동으로 타이머를 시작합니다."))
	bool bAutoStartOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Timer", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "조건이 만족되기까지 기다릴 시간입니다. 0이면 즉시 만족됩니다."))
	float TimerDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Timer", meta = (ToolTip = "타이머를 시작할 때 만족 상태를 먼저 해제합니다."))
	bool bResetSatisfiedOnStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Timer", meta = (ToolTip = "타이머를 중지하거나 리셋할 때 만족 상태를 해제합니다."))
	bool bSetUnsatisfiedOnStop = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bTimerRunning = false;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Timer")
	void StartTimer();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Timer")
	void RestartTimer();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Timer")
	void StopTimer();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Timer")
	void ResetTimerCondition();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Timer")
	void PauseTimer();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Timer")
	void ResumeTimer();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Timer")
	void FinishTimerImmediately();

	UFUNCTION(BlueprintPure, Category = "Puzzle|Timer")
	bool IsTimerActive() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Timer")
	float GetRemainingTime() const;

protected:
	void HandleTimerFinished();
	void ClearTimerHandle();

	FTimerHandle TimerHandle;
};
