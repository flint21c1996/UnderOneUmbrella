// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Core/UOUPuzzleResultCompletedConditionComponent.h"

#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogUOUPuzzleResultCondition, Log, All);

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
	const bool bWasSatisfied = IsSatisfied();
	const bool bTargetCompleted = IsTargetResultCompleted();
	UE_LOG(LogUOUPuzzleResultCondition, Log,
		TEXT("Refresh condition | Owner:%s Component:%s Target:%s RequiredAction:%s Previous:%s TargetCompleted:%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor.Get()),
		*StaticEnum<EOUUPuzzleResultAction>()->GetNameStringByValue(static_cast<int64>(RequiredAction)),
		bWasSatisfied ? TEXT("Satisfied") : TEXT("Unsatisfied"),
		bTargetCompleted ? TEXT("Yes") : TEXT("No"));
	SetSatisfiedState(bTargetCompleted, true);
}

void UUOUPuzzleResultCompletedConditionComponent::SubscribeTargetCompletionState()
{
	UnsubscribeTargetCompletionState();

	if (TargetActor == nullptr)
	{
		UE_LOG(LogUOUPuzzleResultCondition, Warning,
			TEXT("Completion subscription failed: TargetActor is null | Owner:%s Component:%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(this));
		return;
	}

	IUOUPuzzleResultCompletionState* CompletionState = Cast<IUOUPuzzleResultCompletionState>(TargetActor.Get());
	if (CompletionState == nullptr)
	{
		UE_LOG(LogUOUPuzzleResultCondition, Warning,
			TEXT("Completion subscription failed: target does not expose native completion state | Owner:%s Component:%s Target:%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor.Get()));
		return;
	}

	FOnUOUPuzzleResultCompletionStateChangedNativeSignature* CompletionStateChangedEvent =
		CompletionState->GetPuzzleResultCompletionStateChangedEvent();
	if (CompletionStateChangedEvent == nullptr)
	{
		UE_LOG(LogUOUPuzzleResultCondition, Warning,
			TEXT("Completion subscription failed: event is null | Owner:%s Component:%s Target:%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor.Get()));
		return;
	}

	SubscribedTargetActor = TargetActor;
	CompletionStateChangedHandle = CompletionStateChangedEvent->AddUObject(
		this,
		&UUOUPuzzleResultCompletedConditionComponent::HandleTargetCompletionStateChanged);
	UE_LOG(LogUOUPuzzleResultCondition, Log,
		TEXT("Completion subscription succeeded | Owner:%s Component:%s Target:%s RequiredAction:%s HandleValid:%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor.Get()),
		*StaticEnum<EOUUPuzzleResultAction>()->GetNameStringByValue(static_cast<int64>(RequiredAction)),
		CompletionStateChangedHandle.IsValid() ? TEXT("Yes") : TEXT("No"));
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
	UE_LOG(LogUOUPuzzleResultCondition, Log,
		TEXT("Completion event received | Owner:%s Component:%s Target:%s EventAction:%s RequiredAction:%s EventCompleted:%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor.Get()),
		*StaticEnum<EOUUPuzzleResultAction>()->GetNameStringByValue(static_cast<int64>(Action)),
		*StaticEnum<EOUUPuzzleResultAction>()->GetNameStringByValue(static_cast<int64>(RequiredAction)),
		bIsCompleted ? TEXT("Yes") : TEXT("No"));

	if (Action != RequiredAction)
	{
		return;
	}

	RefreshConditionState();
}
