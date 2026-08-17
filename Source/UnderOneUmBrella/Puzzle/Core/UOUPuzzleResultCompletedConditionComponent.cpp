// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Core/UOUPuzzleResultCompletedConditionComponent.h"

#include "GameFramework/Actor.h"

UUOUPuzzleResultCompletedConditionComponent::UUOUPuzzleResultCompletedConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUPuzzleResultCompletedConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	SubscribeTargetCompletionState();
	RefreshConditionState();
}

void UUOUPuzzleResultCompletedConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeTargetCompletionState();

	Super::EndPlay(EndPlayReason);
}

FText UUOUPuzzleResultCompletedConditionComponent::GetDebugSummaryText_Implementation() const
{
	const TArray<FString> DebugLines = {
		FString::Printf(
			TEXT("Puzzle Result Completed Condition: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(TEXT("Target: %s"), *GetNameSafe(TargetActor.Get())),
		FString::Printf(
			TEXT("Required Action: %s"),
			*StaticEnum<EOUUPuzzleResultAction>()->GetNameStringByValue(static_cast<int64>(RequiredAction))),
		FString::Printf(TEXT("Subscribed Target: %s"), *GetNameSafe(SubscribedTargetActor.Get()))
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOUPuzzleResultCompletedConditionComponent::GetPuzzleDebugInputActors_Implementation(
	TArray<AActor*>& OutInputActors) const
{
	if (TargetActor != nullptr)
	{
		OutInputActors.AddUnique(TargetActor.Get());
	}
}

void UUOUPuzzleResultCompletedConditionComponent::RefreshConditionState()
{
	SetSatisfiedState(IsTargetResultCompleted(), true);
}

void UUOUPuzzleResultCompletedConditionComponent::SubscribeTargetCompletionState()
{
	UnsubscribeTargetCompletionState();

	if (TargetActor == nullptr)
	{
		return;
	}

	IUOUPuzzleResultCompletionState* CompletionState = Cast<IUOUPuzzleResultCompletionState>(TargetActor.Get());
	if (CompletionState == nullptr)
	{
		return;
	}

	FOnUOUPuzzleResultCompletionStateChangedNativeSignature* CompletionStateChangedEvent =
		CompletionState->GetPuzzleResultCompletionStateChangedEvent();
	if (CompletionStateChangedEvent == nullptr)
	{
		return;
	}

	SubscribedTargetActor = TargetActor;
	CompletionStateChangedHandle = CompletionStateChangedEvent->AddUObject(
		this,
		&UUOUPuzzleResultCompletedConditionComponent::HandleTargetCompletionStateChanged);
}

void UUOUPuzzleResultCompletedConditionComponent::UnsubscribeTargetCompletionState()
{
	if (!CompletionStateChangedHandle.IsValid() || SubscribedTargetActor == nullptr)
	{
		CompletionStateChangedHandle.Reset();
		SubscribedTargetActor = nullptr;
		return;
	}

	IUOUPuzzleResultCompletionState* CompletionState =
		Cast<IUOUPuzzleResultCompletionState>(SubscribedTargetActor.Get());
	if (CompletionState != nullptr)
	{
		if (FOnUOUPuzzleResultCompletionStateChangedNativeSignature* CompletionStateChangedEvent =
			CompletionState->GetPuzzleResultCompletionStateChangedEvent())
		{
			CompletionStateChangedEvent->Remove(CompletionStateChangedHandle);
		}
	}

	CompletionStateChangedHandle.Reset();
	SubscribedTargetActor = nullptr;
}

bool UUOUPuzzleResultCompletedConditionComponent::IsTargetResultCompleted() const
{
	if (TargetActor == nullptr || RequiredAction == EOUUPuzzleResultAction::None)
	{
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UUOUPuzzleResultCompletionState::StaticClass()))
	{
		return false;
	}

	return IUOUPuzzleResultCompletionState::Execute_IsPuzzleResultCompleted(TargetActor.Get(), RequiredAction);
}

void UUOUPuzzleResultCompletedConditionComponent::HandleTargetCompletionStateChanged(
	EOUUPuzzleResultAction Action,
	bool bIsCompleted)
{
	if (Action != RequiredAction)
	{
		return;
	}

	RefreshConditionState();
}
