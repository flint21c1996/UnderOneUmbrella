// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Dialogue/UOUDialoguePuzzleStateComponent.h"

#include "GameFramework/Actor.h"
#include "UI/UOUDialogueSourceComponent.h"
#include "UI/UOUDialogueTriggerComponent.h"

UUOUDialoguePuzzleStateComponent::UUOUDialoguePuzzleStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUDialoguePuzzleStateComponent::BeginPlay()
{
	Super::BeginPlay();

	bPuzzleSolved = bStartSolved;
	if (bApplyInitialStateOnBeginPlay)
	{
		ApplyCurrentDialogueState();
	}
}

void UUOUDialoguePuzzleStateComponent::SetPuzzleSolved(bool bNewPuzzleSolved)
{
	const bool bStateChanged = bPuzzleSolved != bNewPuzzleSolved;
	bPuzzleSolved = bNewPuzzleSolved;

	UUOUDialogueSourceComponent* DialogueSource = ResolveDialogueSource();
	const FName TargetState = bPuzzleSolved ? SolvedDialogueState : UnsolvedDialogueState;
	if (DialogueSource != nullptr && !TargetState.IsNone())
	{
		DialogueSource->SetDialogueState(TargetState);
		if (bStateChanged && bResetDialoguePlaybackOnStateChange)
		{
			DialogueSource->ResetDialoguePlayback();
		}
	}

	if (UUOUDialogueTriggerComponent* DialogueTrigger = ResolveDialogueTrigger())
	{
		if (bStateChanged && bResetTriggerOnStateChange)
		{
			DialogueTrigger->ResetTrigger();
		}

		if (bRefreshOverlappingInteraction)
		{
			DialogueTrigger->RefreshOverlappingInteraction();
		}
	}

	OnDialoguePuzzleStateApplied.Broadcast(bPuzzleSolved, TargetState);
}

void UUOUDialoguePuzzleStateComponent::ApplyCurrentDialogueState()
{
	SetPuzzleSolved(bPuzzleSolved);
}

UUOUDialogueSourceComponent* UUOUDialoguePuzzleStateComponent::ResolveDialogueSource() const
{
	if (IsValid(TargetDialogueSource))
	{
		return TargetDialogueSource.Get();
	}

	AActor* DialogueActor = ResolveDialogueActor();
	return DialogueActor != nullptr
		? DialogueActor->FindComponentByClass<UUOUDialogueSourceComponent>()
		: nullptr;
}

UUOUDialogueTriggerComponent* UUOUDialoguePuzzleStateComponent::ResolveDialogueTrigger() const
{
	if (IsValid(TargetDialogueTrigger))
	{
		return TargetDialogueTrigger.Get();
	}

	AActor* DialogueActor = ResolveDialogueActor();
	return DialogueActor != nullptr
		? DialogueActor->FindComponentByClass<UUOUDialogueTriggerComponent>()
		: nullptr;
}

void UUOUDialoguePuzzleStateComponent::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		SetPuzzleSolved(true);
		break;
	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		SetPuzzleSolved(false);
		break;
	case EOUUPuzzleResultAction::Toggle:
		SetPuzzleSolved(!bPuzzleSolved);
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

AActor* UUOUDialoguePuzzleStateComponent::ResolveDialogueActor() const
{
	return IsValid(TargetDialogueActor) ? TargetDialogueActor.Get() : GetOwner();
}
