// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Dialogue/UOUDialogueCompletedConditionComponent.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "UI/UOUDialogueSourceComponent.h"
#include "UI/UOUUISubsystem.h"

UUOUDialogueCompletedConditionComponent::UUOUDialogueCompletedConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUDialogueCompletedConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	SetSatisfiedState(bInitialSatisfied, false);
	ResolvedDialogueSource = ResolveTargetDialogueSource();
	SubscribeDialogueCompletion();
}

void UUOUDialogueCompletedConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeDialogueCompletion();
	ResolvedDialogueSource = nullptr;

	Super::EndPlay(EndPlayReason);
}

FText UUOUDialogueCompletedConditionComponent::GetDebugSummaryText_Implementation() const
{
	const TArray<FString> DebugLines = {
		FString::Printf(
			TEXT("Dialogue Completed Condition: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(TEXT("Dialogue Source: %s"), *GetNameSafe(ResolvedDialogueSource.Get()))
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOUDialogueCompletedConditionComponent::GetPuzzleDebugInputActors_Implementation(
	TArray<AActor*>& OutInputActors) const
{
	if (ResolvedDialogueSource != nullptr && ResolvedDialogueSource->GetOwner() != nullptr)
	{
		OutInputActors.AddUnique(ResolvedDialogueSource->GetOwner());
	}
}

void UUOUDialogueCompletedConditionComponent::ResetCompletion()
{
	SetSatisfiedState(false, true);
}

void UUOUDialogueCompletedConditionComponent::HandleDialogueCompleted(
	UUOUDialogueSourceComponent* CompletedDialogueSource)
{
	if (CompletedDialogueSource == nullptr || CompletedDialogueSource != ResolvedDialogueSource)
	{
		return;
	}

	SetSatisfiedState(true, true);
}

UUOUDialogueSourceComponent* UUOUDialogueCompletedConditionComponent::ResolveTargetDialogueSource() const
{
	if (TargetDialogueSource != nullptr)
	{
		return TargetDialogueSource.Get();
	}

	AActor* OwnerActor = GetOwner();
	return OwnerActor != nullptr
		? OwnerActor->FindComponentByClass<UUOUDialogueSourceComponent>()
		: nullptr;
}

UUOUUISubsystem* UUOUDialogueCompletedConditionComponent::ResolveUISubsystem() const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World != nullptr ? World->GetFirstPlayerController() : nullptr;
	ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UUOUUISubsystem>() : nullptr;
}

void UUOUDialogueCompletedConditionComponent::SubscribeDialogueCompletion()
{
	UnsubscribeDialogueCompletion();

	if (ResolvedDialogueSource == nullptr)
	{
		return;
	}

	SubscribedUISubsystem = ResolveUISubsystem();
	if (SubscribedUISubsystem != nullptr)
	{
		SubscribedUISubsystem->OnDialogueCompleted.RemoveDynamic(
			this,
			&UUOUDialogueCompletedConditionComponent::HandleDialogueCompleted);
		SubscribedUISubsystem->OnDialogueCompleted.AddDynamic(
			this,
			&UUOUDialogueCompletedConditionComponent::HandleDialogueCompleted);
	}
}

void UUOUDialogueCompletedConditionComponent::UnsubscribeDialogueCompletion()
{
	if (SubscribedUISubsystem != nullptr)
	{
		SubscribedUISubsystem->OnDialogueCompleted.RemoveDynamic(
			this,
			&UUOUDialogueCompletedConditionComponent::HandleDialogueCompleted);
		SubscribedUISubsystem = nullptr;
	}
}
