// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUStageSelectNodeActor.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Game/UOUMapSelectPlayerController.h"

AUOUStageSelectNodeActor::AUOUStageSelectNodeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
}

bool AUOUStageSelectNodeActor::ActivateStage()
{
	const UWorld* World = GetWorld();
	AUOUMapSelectPlayerController* MapSelectController = World != nullptr
		? Cast<AUOUMapSelectPlayerController>(World->GetFirstPlayerController())
		: nullptr;
	return MapSelectController != nullptr && MapSelectController->EnterStageByIndex(StageIndex);
}
