// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "UOUDevelopmentPuzzleCheatSubsystem.generated.h"

class AUOUPuzzleConditionGroupActor;
class SUOUDevelopmentPuzzleCheatHUD;
class UGameViewportClient;

// 현재 월드에서 발견한 퍼즐 치트 진행 단계 하나의 런타임 정보입니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLADEVTOOLS_API FUOUDevelopmentPuzzleCheatStep
{
	GENERATED_BODY()

	// UOU.PuzzleCheat.Step 태그에서 읽은 퍼즐 진행 순서입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat")
	int32 StepOrder = INDEX_NONE;

	// Label 태그 또는 액터 이름에서 만든 HUD 표시 이름입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat")
	FText DisplayName;

	// 이 단계가 완료될 때 만족 상태로 전환할 퍼즐 그룹입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat")
	TObjectPtr<AUOUPuzzleConditionGroupActor> PuzzleGroup = nullptr;

	// 이 단계를 실행한 뒤 다음 단계로 넘어가기 전에 기다릴 최소 시간입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat")
	float DelayAfterActivationSeconds = 0.25f;
};

// Development 및 Internal Shipping 빌드에서만 존재하는 퍼즐 순차 진행 도구입니다.
UCLASS()
class UNDERONEUMBRELLADEVTOOLS_API UUOUDevelopmentPuzzleCheatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// 현재 월드의 퍼즐 그룹 태그를 다시 읽고 StepOrder 순서로 캐시합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle Cheat")
	bool RefreshPuzzleSequence();

	// 앞에서부터 찾은 첫 번째 미완료 퍼즐 단계까지 진행합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle Cheat")
	bool AdvanceToNextPuzzle();

	// 지정한 StepOrder까지 앞선 미완료 퍼즐을 순서대로 진행합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle Cheat")
	bool AdvanceThroughStep(int32 TargetStepOrder);

	// 아직 실행되지 않은 예약 단계만 취소하고 이미 완료한 단계는 유지합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle Cheat")
	void CancelPendingSequence();

	// 현재 유효한 목록에서 첫 번째 미완료 퍼즐의 StepOrder를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle Cheat")
	int32 GetFirstIncompleteStepOrder() const;

	// 향후 HUD가 버튼 목록을 구성할 수 있도록 현재 퍼즐 단계 캐시를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle Cheat")
	TArray<FUOUDevelopmentPuzzleCheatStep> GetPuzzleSteps() const;

	// 순차 실행 중인지 반환하여 중복 요청과 HUD 입력을 제어합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle Cheat")
	bool IsSequenceRunning() const { return bSequenceRunning; }

	// 마지막 퍼즐 수집 결과가 중복 Step 없이 실행 가능한지 반환합니다.
	bool IsPuzzleSequenceValid() const { return bPuzzleSequenceValid; }

	// Viewport에 생성된 퍼즐 치트 HUD 패널의 확장 상태를 전환합니다.
	void ToggleCheatHUD();

	// 퍼즐 치트 HUD 패널이 현재 확장되어 있는지 반환합니다.
	bool IsCheatHUDExpanded() const;

	// 가장 최근 수집, 실행, 완료 또는 실패 결과입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Runtime")
	FString LastStatusMessage;

private:
	bool BuildActivationQueue(int32 TargetStepOrder);
	void ExecuteNextQueuedStep();
	float GetDelayBeforeNextStep(const FUOUDevelopmentPuzzleCheatStep& Step) const;
	void FinishSequence();
	void EnsureCheatHUDCreated();
	void RemoveCheatHUD();

	// 현재 월드에서 Step 태그로 발견해 진행 순서대로 정렬한 런타임 캐시입니다.
	UPROPERTY(Transient)
	TArray<FUOUDevelopmentPuzzleCheatStep> PuzzleSteps;

	// 현재 요청에서 아직 실행할 단계들의 PuzzleSteps 인덱스입니다.
	UPROPERTY(Transient)
	TArray<int32> PendingStepIndices;

	// PendingStepIndices에서 다음으로 실행할 위치입니다.
	UPROPERTY(Transient)
	int32 PendingQueuePosition = 0;

	// 중복 실행 요청을 차단하고 HUD 버튼 상태를 결정하는 런타임 값입니다.
	UPROPERTY(Transient)
	bool bSequenceRunning = false;

	// 마지막 월드 스캔 결과가 중복 Step 번호 없이 실행 가능한지 저장합니다.
	UPROPERTY(Transient)
	bool bPuzzleSequenceValid = false;

	// 다음 단계 예약을 취소하거나 월드 종료 시 정리하기 위한 타이머 핸들입니다.
	FTimerHandle SequenceTimerHandle;

	// 현재 GameViewport에 추가한 개발 전용 Slate HUD입니다.
	TSharedPtr<SUOUDevelopmentPuzzleCheatHUD> CheatHUDWidget;

	// HUD를 정확한 Viewport에서 제거하기 위해 생성 시점의 Viewport를 보관합니다.
	TWeakObjectPtr<UGameViewportClient> CheatHUDViewport;
};
