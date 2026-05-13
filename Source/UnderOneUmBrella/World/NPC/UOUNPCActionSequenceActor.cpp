// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/UOUNPCActionSequenceActor.h"

#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "World/NPC/UOUNPCCharacter.h"

AUOUNPCActionSequenceActor::AUOUNPCActionSequenceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
}

void AUOUNPCActionSequenceActor::BeginPlay()
{
	Super::BeginPlay();

	BindToTargetNPC();

	if (bActivateOnBeginPlay)
	{
		Activate();
	}
}

void AUOUNPCActionSequenceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromTargetNPC();

	Super::EndPlay(EndPlayReason);
}

void AUOUNPCActionSequenceActor::Activate()
{
	bActivated = true;
	StartSequence(ActivateActions, false);
}

void AUOUNPCActionSequenceActor::Deactivate()
{
	bActivated = false;
	StartSequence(DeactivateActions, true);
}

void AUOUNPCActionSequenceActor::Toggle()
{
	if (bActivated)
	{
		Deactivate();
		return;
	}

	Activate();
}

void AUOUNPCActionSequenceActor::StopSequence()
{
	if (TargetNPC != nullptr)
	{
		TargetNPC->ClearNPCAction(nullptr);
	}

	bRunningSequence = false;
	bRunningDeactivateSequence = false;
	CurrentActionIndex = INDEX_NONE;
	CurrentSequenceActions.Reset();
}

void AUOUNPCActionSequenceActor::StartSequence(
	const TArray<FUOUNPCActionRequest>& Actions,
	bool bDeactivateSequence)
{
	BindToTargetNPC();

	if (TargetNPC != nullptr)
	{
		TargetNPC->ClearNPCAction(nullptr);
	}

	CurrentSequenceActions = Actions;
	CurrentActionIndex = 0;
	bRunningSequence = CurrentSequenceActions.Num() > 0;
	bRunningDeactivateSequence = bRunningSequence && bDeactivateSequence;

	if (!bRunningSequence)
	{
		FinishSequence();
		return;
	}

	RunCurrentAction();
}

void AUOUNPCActionSequenceActor::RunCurrentAction()
{
	if (!bRunningSequence || TargetNPC == nullptr)
	{
		FinishSequence();
		return;
	}

	if (!CurrentSequenceActions.IsValidIndex(CurrentActionIndex))
	{
		FinishSequence();
		return;
	}

	FUOUNPCActionRequest ActionRequest = CurrentSequenceActions[CurrentActionIndex];
	ActionRequest.bClearActionOnFinish = true;
	if (!TargetNPC->RequestNPCAction(this, ActionRequest))
	{
		++CurrentActionIndex;
		RunCurrentAction();
	}
}

void AUOUNPCActionSequenceActor::FinishSequence()
{
	bRunningSequence = false;
	bRunningDeactivateSequence = false;
	CurrentActionIndex = INDEX_NONE;
	CurrentSequenceActions.Reset();
}

void AUOUNPCActionSequenceActor::BindToTargetNPC()
{
	if (TargetNPC == nullptr)
	{
		return;
	}

	TargetNPC->OnNPCActionCompleted.RemoveDynamic(this, &AUOUNPCActionSequenceActor::HandleNPCActionCompleted);
	TargetNPC->OnNPCActionCompleted.AddDynamic(this, &AUOUNPCActionSequenceActor::HandleNPCActionCompleted);
}

void AUOUNPCActionSequenceActor::UnbindFromTargetNPC()
{
	if (TargetNPC == nullptr)
	{
		return;
	}

	TargetNPC->OnNPCActionCompleted.RemoveDynamic(this, &AUOUNPCActionSequenceActor::HandleNPCActionCompleted);
}

void AUOUNPCActionSequenceActor::HandleNPCActionCompleted(AUOUNPCCharacter* NPC, UObject* ActionSource)
{
	if (!bRunningSequence || NPC != TargetNPC || ActionSource != this)
	{
		return;
	}

	++CurrentActionIndex;
	FTimerDelegate RunNextActionDelegate;
	RunNextActionDelegate.BindUObject(this, &AUOUNPCActionSequenceActor::RunCurrentAction);
	GetWorldTimerManager().SetTimerForNextTick(RunNextActionDelegate);
}
