// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/UOUNPCActionRequestActor.h"

#include "Components/SceneComponent.h"
#include "World/NPC/UOUNPCCharacter.h"

AUOUNPCActionRequestActor::AUOUNPCActionRequestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
}

void AUOUNPCActionRequestActor::BeginPlay()
{
	Super::BeginPlay();

	if (bActivateOnBeginPlay)
	{
		Activate();
	}
}

void AUOUNPCActionRequestActor::Activate()
{
	bActivated = true;

	if (TargetNPC != nullptr)
	{
		TargetNPC->RequestNPCAction(this, ActionRequest);
	}
}

void AUOUNPCActionRequestActor::Deactivate()
{
	bActivated = false;

	if (bClearNPCActionOnDeactivate && TargetNPC != nullptr)
	{
		TargetNPC->ClearNPCAction(this);
	}
}

void AUOUNPCActionRequestActor::Toggle()
{
	if (bActivated)
	{
		Deactivate();
		return;
	}

	Activate();
}

void AUOUNPCActionRequestActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		Activate();
		break;
	case EOUUPuzzleResultAction::Deactivate:
		Deactivate();
		break;
	case EOUUPuzzleResultAction::Toggle:
		Toggle();
		break;
	case EOUUPuzzleResultAction::None:
	case EOUUPuzzleResultAction::Pause:
	case EOUUPuzzleResultAction::Resume:
	default:
		break;
	}
}
