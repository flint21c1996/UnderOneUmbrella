// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Interaction/UOUInteractionConditionComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"

UUOUInteractionConditionComponent::UUOUInteractionConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUInteractionConditionComponent::BeginPlay()
{
	Super::BeginPlay();
	SetSatisfiedState(bInitialSatisfied, false);
}

void UUOUInteractionConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PulseTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UUOUInteractionConditionComponent::Interact_Implementation(AActor* Interactor)
{
	TriggerInteraction(Interactor);
}

FText UUOUInteractionConditionComponent::GetDebugSummaryText_Implementation() const
{
	const TArray<FString> DebugLines = {
		FString::Printf(TEXT("Interaction Condition: %s"), IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(TEXT("Mode: %s"), *StaticEnum<EUOUInteractionConditionMode>()->GetNameStringByValue(static_cast<int64>(InteractionMode))),
		FString::Printf(TEXT("Last Interactor: %s"), *GetNameSafe(LastInteractor))
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOUInteractionConditionComponent::GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const
{
	if (AActor* Owner = GetOwner())
	{
		OutInputActors.AddUnique(Owner);
	}

	if (LastInteractor != nullptr)
	{
		OutInputActors.AddUnique(LastInteractor);
	}
}

void UUOUInteractionConditionComponent::TriggerInteraction(AActor* Interactor)
{
	LastInteractor = Interactor;
	OnInteracted.Broadcast(Interactor);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PulseTimerHandle);
	}

	switch (InteractionMode)
	{
	case EUOUInteractionConditionMode::SetSatisfied:
		SetSatisfiedState(true, true);
		break;

	case EUOUInteractionConditionMode::SetUnsatisfied:
		SetSatisfiedState(false, true);
		break;

	case EUOUInteractionConditionMode::Toggle:
		SetSatisfiedState(!IsSatisfied(), true);
		break;

	case EUOUInteractionConditionMode::Pulse:
	default:
		if (IsSatisfied())
		{
			SetSatisfiedState(false, true);
		}

		SetSatisfiedState(true, true);

		if (PulseDuration <= 0.0f)
		{
			FinishPulse();
		}
		else if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				PulseTimerHandle,
				this,
				&UUOUInteractionConditionComponent::FinishPulse,
				PulseDuration,
				false);
		}
		break;
	}
}

void UUOUInteractionConditionComponent::SetInteractionSatisfied(bool bNewSatisfied)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PulseTimerHandle);
	}

	SetSatisfiedState(bNewSatisfied, true);
}

void UUOUInteractionConditionComponent::FinishPulse()
{
	SetSatisfiedState(false, true);
}
