// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Game/UOULevelTransitionSubsystem.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUPuzzleLevelTransitionActor.generated.h"

class USceneComponent;
class UWorld;

UENUM(BlueprintType)
enum class EUOUPuzzleLevelTransitionMode : uint8
{
	OpenTargetLevel,
	RestartCurrentLevel,
	OpenNextLevel,
	OpenPreviousLevel
};

UCLASS(meta=(DisplayName="UOU Puzzle Level Transition Actor"))
class AUOUPuzzleLevelTransitionActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUPuzzleLevelTransitionActor();

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Level Transition")
	bool StartLevelTransition();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Level Transition")
	void ResetTransition();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (AllowedClasses = "/Script/Engine.World", ToolTip = "Transition Mode가 Open Target Level일 때 열 레벨입니다. Next/Previous 모드에서는 현재 맵의 UOU Level Transition Settings Actor를 사용합니다."))
	TSoftObjectPtr<UWorld> TargetLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition")
	EUOUPuzzleLevelTransitionMode TransitionMode = EUOUPuzzleLevelTransitionMode::OpenTargetLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ToolTip = "이 액션을 받았을 때 레벨 전환을 시작합니다. ConditionGroup의 SatisfiedAction 기본값은 Activate입니다."))
	EOUUPuzzleResultAction TriggerAction = EOUUPuzzleResultAction::Activate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ToolTip = "켜져 있으면 한 번 전환을 시작한 뒤 같은 액터가 다시 전환을 시작하지 않습니다."))
	bool bTriggerOnce = true;

	/** 퍼즐 완료로 다음/대상 맵에 이동할 때 현재 스테이지를 먼저 저장합니다. 재시작과 이전 맵 이동에는 적용하지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (DisplayName = "전환 전 스테이지 클리어 저장"))
	bool bCompleteStageBeforeTransition = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bHasTriggered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bIsTransitioning = false;

private:
	// 기존 맵에 저장된 전환 연출 값을 유지하기 위한 호환용 필드입니다.
	// 실제 페이드 시간, 색상, 문구는 각 맵의 UOU Level Transition Settings Actor가 담당합니다.
	UPROPERTY()
	float FadeOutDuration = 0.75f;

	UPROPERTY()
	float BlackHoldDuration = 0.1f;

	UPROPERTY()
	float FadeInDuration = 0.35f;

	UPROPERTY()
	FLinearColor FadeColor = FLinearColor::Black;

	UPROPERTY()
	bool bFadeAudio = false;

	UPROPERTY()
	bool bLockPlayerInputDuringTransition = true;

	UPROPERTY()
	FUOUTransitionMessageSettings FadeOutMessageSettings;

	UPROPERTY()
	FUOUTransitionMessageSettings FadeInMessageSettings;

	UUOULevelTransitionSubsystem* GetTransitionSubsystem() const;
};
