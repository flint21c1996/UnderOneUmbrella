// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Results/UOUStageCompletionActor.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/UOUPlayerProgressSubsystem.h"

AUOUStageCompletionActor::AUOUStageCompletionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
}

void AUOUStageCompletionActor::ApplyPuzzleResult_Implementation(const EOUUPuzzleResultAction Action)
{
	if (Action == TriggerAction && Action != EOUUPuzzleResultAction::None)
	{
		CompleteStage();
	}
}

bool AUOUStageCompletionActor::CompleteStage()
{
	if (bHasCommitted)
	{
		return false;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	UUOUPlayerProgressSubsystem* ProgressSubsystem = GameInstance != nullptr
		? GameInstance->GetSubsystem<UUOUPlayerProgressSubsystem>()
		: nullptr;
	if (ProgressSubsystem == nullptr || !ProgressSubsystem->CommitCurrentStage())
	{
		return false;
	}

	bHasCommitted = true;
	return true;
}
