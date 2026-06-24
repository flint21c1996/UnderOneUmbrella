// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Results/UOUPuzzleLevelTransitionActor.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

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
	if (TransitionSubsystem == nullptr)
	{
		return false;
	}

	FUOULevelTransitionSettings Settings;
	Settings.FadeOutDuration = FadeOutDuration;
	Settings.BlackHoldDuration = BlackHoldDuration;
	Settings.FadeInDuration = FadeInDuration;
	Settings.FadeColor = FadeColor;
	Settings.bFadeAudio = bFadeAudio;
	Settings.bLockPlayerInputDuringTransition = bLockPlayerInputDuringTransition;

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
		bStarted = TransitionSubsystem->RestartCurrentLevel(Settings);
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
