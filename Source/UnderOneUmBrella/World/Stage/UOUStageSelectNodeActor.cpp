// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUStageSelectNodeActor.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Game/UOUMapSelectPlayerController.h"
#include "Game/UOUStageSelectTypes.h"
#include "GameFramework/Pawn.h"
#include "World/Stage/UOUStageSelectAreaComponent.h"

AUOUStageSelectNodeActor::AUOUStageSelectNodeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	StageSelectArea = CreateDefaultSubobject<UUOUStageSelectAreaComponent>(TEXT("StageSelectArea"));
	StageSelectArea->SetupAttachment(RootScene);
}

void AUOUStageSelectNodeActor::BeginPlay()
{
	Super::BeginPlay();

	if (StageSelectArea != nullptr)
	{
		StageSelectArea->OnPlayerEntered.AddUniqueDynamic(
			this,
			&AUOUStageSelectNodeActor::HandlePlayerEnteredStageArea);
		StageSelectArea->OnPlayerExited.AddUniqueDynamic(
			this,
			&AUOUStageSelectNodeActor::HandlePlayerExitedStageArea);
	}
}

bool AUOUStageSelectNodeActor::GetStageDefinition(FUOUStageDefinition& OutStageDefinition) const
{
	OutStageDefinition = FUOUStageDefinition();

	if (StageRow.DataTable == nullptr || StageRow.RowName.IsNone())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Stage select node '%s' has no DataTable row configured."),
			*GetName());
		return false;
	}

	const FString Context = FString::Printf(TEXT("StageSelectNode:%s"), *GetName());
	const FUOUStageDefinitionRow* Row = StageRow.GetRow<FUOUStageDefinitionRow>(Context);
	if (Row == nullptr)
	{
		return false;
	}

	OutStageDefinition.StageId = StageRow.RowName;
	OutStageDefinition.DisplayName = Row->DisplayName;
	OutStageDefinition.Description = Row->Description;
	OutStageDefinition.Thumbnail = Row->Thumbnail;
	OutStageDefinition.Level = Row->Level;
	OutStageDefinition.RewardIds = Row->RewardIds;
	OutStageDefinition.TotalRewardCount = Row->RewardIds.Num();
	OutStageDefinition.CollectedRewardCount = 0;
	OutStageDefinition.MissingRewardCount = OutStageDefinition.TotalRewardCount;
	OutStageDefinition.bUnlocked = Row->bUnlocked;
	return true;
}

bool AUOUStageSelectNodeActor::ActivateStage()
{
	FUOUStageDefinition StageDefinition;
	if (!GetStageDefinition(StageDefinition))
	{
		return false;
	}

	const UWorld* World = GetWorld();
	AUOUMapSelectPlayerController* MapSelectController = World != nullptr
		? Cast<AUOUMapSelectPlayerController>(World->GetFirstPlayerController())
		: nullptr;
	return MapSelectController != nullptr && MapSelectController->EnterStage(StageDefinition);
}

void AUOUStageSelectNodeActor::HandlePlayerEnteredStageArea(APawn* PlayerPawn)
{
	AUOUMapSelectPlayerController* MapSelectController = PlayerPawn != nullptr
		? Cast<AUOUMapSelectPlayerController>(PlayerPawn->GetController())
		: nullptr;
	if (MapSelectController != nullptr)
	{
		MapSelectController->RegisterStageSelectNode(this);
	}
}

void AUOUStageSelectNodeActor::HandlePlayerExitedStageArea(APawn* PlayerPawn)
{
	AUOUMapSelectPlayerController* MapSelectController = PlayerPawn != nullptr
		? Cast<AUOUMapSelectPlayerController>(PlayerPawn->GetController())
		: nullptr;
	if (MapSelectController != nullptr)
	{
		MapSelectController->UnregisterStageSelectNode(this);
	}
}
