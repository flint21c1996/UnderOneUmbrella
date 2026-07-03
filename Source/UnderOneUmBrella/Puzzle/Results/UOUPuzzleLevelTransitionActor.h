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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (AllowedClasses = "/Script/Engine.World", ToolTip = "페이드 아웃 후 열 레벨입니다."))
	TSoftObjectPtr<UWorld> TargetLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition")
	EUOUPuzzleLevelTransitionMode TransitionMode = EUOUPuzzleLevelTransitionMode::OpenTargetLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ToolTip = "이 액션을 받았을 때 레벨 전환을 시작합니다. ConditionGroup의 SatisfiedAction 기본값은 Activate입니다."))
	EOUUPuzzleResultAction TriggerAction = EOUUPuzzleResultAction::Activate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ToolTip = "켜져 있으면 한 번 전환을 시작한 뒤 같은 액터가 다시 전환을 시작하지 않습니다."))
	bool bTriggerOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ClampMin = "0.0", ToolTip = "화면이 검은색으로 사라지는 시간입니다."))
	float FadeOutDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ClampMin = "0.0", ToolTip = "완전히 어두워진 뒤 레벨을 열기 전에 기다리는 시간입니다."))
	float BlackHoldDuration = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ClampMin = "0.0", ToolTip = "Time spent fading back in after the new level is loaded."))
	float FadeInDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ToolTip = "페이드 색상입니다."))
	FLinearColor FadeColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ToolTip = "켜져 있으면 카메라 페이드와 함께 오디오도 페이드 처리합니다."))
	bool bFadeAudio = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition", meta = (ToolTip = "If enabled, player move and look input are blocked while the transition is running."))
	bool bLockPlayerInputDuringTransition = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition|Message", meta = (DisplayName = "페이드 아웃 문구", ToolTip = "현재 레벨에서 화면이 검게 가려진 뒤 표시할 문구입니다. 비워두면 표시하지 않습니다."))
	FUOUTransitionMessageSettings FadeOutMessageSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Level Transition|Message", meta = (DisplayName = "페이드 인 문구", ToolTip = "새 레벨이 로드된 뒤 화면이 밝아지는 동안 표시할 문구입니다. 비워두면 표시하지 않습니다."))
	FUOUTransitionMessageSettings FadeInMessageSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bHasTriggered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bIsTransitioning = false;

private:
	UUOULevelTransitionSubsystem* GetTransitionSubsystem() const;
};
