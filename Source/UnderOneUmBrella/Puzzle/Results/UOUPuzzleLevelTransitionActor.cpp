// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Results/UOUPuzzleLevelTransitionActor.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/UOUPlayerProgressSubsystem.h"

AUOUPuzzleLevelTransitionActor::AUOUPuzzleLevelTransitionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
}

void AUOUPuzzleLevelTransitionActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	if (Action == TriggerAction && Action != EOUUPuzzleResultAction::None)
	{
		StartLevelTransition();
	}
}

bool AUOUPuzzleLevelTransitionActor::StartLevelTransition()
{
	if (bIsTransitioning || (bTriggerOnce && bHasTriggered))
	{
		return false;
	}

	UUOULevelTransitionSubsystem* TransitionSubsystem = GetTransitionSubsystem();
	if (TransitionSubsystem == nullptr || TransitionSubsystem->IsTransitioning())
	{
		return false;
	}
	if (TransitionMode == EUOUPuzzleLevelTransitionMode::OpenTargetLevel && TargetLevel.IsNull())
	{
		return false;
	}

	// 설정창 이동과 분리된 퍼즐 완료 경로에서만 저장합니다. 저장 실패 시 현재 맵에 남습니다.
	const bool bForwardTransition = TransitionMode == EUOUPuzzleLevelTransitionMode::OpenNextLevel
		|| TransitionMode == EUOUPuzzleLevelTransitionMode::OpenTargetLevel;
	if (bCompleteStageBeforeTransition && bForwardTransition)
	{
		UGameInstance* GameInstance = GetWorld()->GetGameInstance();
		UUOUPlayerProgressSubsystem* Progress = GameInstance != nullptr
			? GameInstance->GetSubsystem<UUOUPlayerProgressSubsystem>() : nullptr;
		if (Progress != nullptr && Progress->EnsureStageAttemptForWorld(GetWorld())
			&& !Progress->CommitCurrentStage())
		{
			UE_LOG(LogUOUPlayerProgress, Warning, TEXT("Stage completion save failed; level transition cancelled."));
			return false;
		}
	}

	// 전환 연출은 현재 맵과 도착 맵의 UOU Level Transition Settings Actor를 기준으로 적용합니다.
	FUOULevelTransitionSettings Settings;

	bool bStarted = false;
	switch (TransitionMode)
	{
	case EUOUPuzzleLevelTransitionMode::OpenTargetLevel:
		if (TargetLevel.IsNull())
		{
			UE_LOG(LogTemp, Warning, TEXT("%s cannot start level transition because TargetLevel is not set."), *GetName());
			return false;
		}
		bStarted = TransitionSubsystem->RequestLevelTransition(TargetLevel, Settings);
		break;
	case EUOUPuzzleLevelTransitionMode::RestartCurrentLevel:
		bStarted = TransitionSubsystem->RestartCurrentLevelFromWorld(GetWorld(), Settings);
		break;
	case EUOUPuzzleLevelTransitionMode::OpenNextLevel:
		bStarted = TransitionSubsystem->RequestNextLevelFromWorld(GetWorld(), Settings);
		break;
	case EUOUPuzzleLevelTransitionMode::OpenPreviousLevel:
		bStarted = TransitionSubsystem->RequestPreviousLevelFromWorld(GetWorld(), Settings);
		break;
	default:
		break;
	}

	if (bStarted)
	{
		bHasTriggered = true;
		bIsTransitioning = true;
	}

	return bStarted;
}

void AUOUPuzzleLevelTransitionActor::ResetTransition()
{
	bHasTriggered = false;
	bIsTransitioning = false;

	if (UUOULevelTransitionSubsystem* TransitionSubsystem = GetTransitionSubsystem())
	{
		TransitionSubsystem->CancelTransition();
	}
}

UUOULevelTransitionSubsystem* AUOUPuzzleLevelTransitionActor::GetTransitionSubsystem() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance != nullptr ? GameInstance->GetSubsystem<UUOULevelTransitionSubsystem>() : nullptr;
}
