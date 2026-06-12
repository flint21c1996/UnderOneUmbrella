// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Interaction/UOUContextInteractionConditionComponent.h"

#include "GameFramework/Actor.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"

UUOUContextInteractionConditionComponent::UUOUContextInteractionConditionComponent()
{
	InteractionMode = EUOUInteractionConditionMode::SetSatisfied;
}

void UUOUContextInteractionConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPlayerInteractionWait();
	Super::EndPlay(EndPlayReason);
}

void UUOUContextInteractionConditionComponent::Interact_Implementation(AActor* Interactor)
{
	if (bWaitingForPlayerInteraction)
	{
		return;
	}

	if (bBlockInteractionAfterSatisfied && IsSatisfied())
	{
		return;
	}

	if (!CanStartContextInteraction(Interactor))
	{
		HandleRejectedContextInteraction(Interactor);
		return;
	}

	HandleAcceptedContextInteraction(Interactor);
	StartContextInteraction(Interactor);
}

TArray<FString> UUOUContextInteractionConditionComponent::GetPuzzleDebugInfo_Implementation() const
{
	TArray<FString> Lines = Super::GetPuzzleDebugInfo_Implementation();
	Lines.Add(FString::Printf(
		TEXT("Player Interaction: %s"),
		PlayerInteractionRequest.HasPlayerMontage() ? TEXT("Montage") : TEXT("Instant")));
	Lines.Add(FString::Printf(
		TEXT("Waiting Player Interaction: %s"),
		bWaitingForPlayerInteraction ? TEXT("Yes") : TEXT("No")));
	Lines.Add(FString::Printf(
		TEXT("Block After Satisfied: %s"),
		bBlockInteractionAfterSatisfied ? TEXT("Yes") : TEXT("No")));
	Lines.Add(FString::Printf(TEXT("Pending Interactor: %s"), *GetNameSafe(PendingInteractor.Get())));
	return Lines;
}

void UUOUContextInteractionConditionComponent::GetPuzzleDebugInputActors_Implementation(
	TArray<AActor*>& OutInputActors) const
{
	Super::GetPuzzleDebugInputActors_Implementation(OutInputActors);

	if (PendingInteractor != nullptr)
	{
		OutInputActors.AddUnique(PendingInteractor.Get());
	}
}

bool UUOUContextInteractionConditionComponent::CanStartContextInteraction(AActor* Interactor) const
{
	return Interactor != nullptr;
}

void UUOUContextInteractionConditionComponent::HandleAcceptedContextInteraction(AActor* Interactor)
{
}

void UUOUContextInteractionConditionComponent::HandleRejectedContextInteraction(AActor* Interactor)
{
}

void UUOUContextInteractionConditionComponent::StartContextInteraction(AActor* Interactor)
{
	if (!bWaitForPlayerInteraction || !PlayerInteractionRequest.HasPlayerMontage())
	{
		TriggerInteraction(Interactor);
		return;
	}

	UUOUPlayerInteractionExecutorComponent* Executor = FindPlayerInteractionExecutor(Interactor);
	if (Executor == nullptr)
	{
		return;
	}

	PendingInteractor = Interactor;
	PendingExecutor = Executor;
	bWaitingForPlayerInteraction = true;

	Executor->OnInteractionFinished.RemoveDynamic(
		this,
		&UUOUContextInteractionConditionComponent::HandlePlayerInteractionFinished);
	Executor->OnInteractionFinished.AddDynamic(
		this,
		&UUOUContextInteractionConditionComponent::HandlePlayerInteractionFinished);

	if (!Executor->TryStartInteraction(this, PlayerInteractionRequest))
	{
		ClearPlayerInteractionWait();
	}
}

void UUOUContextInteractionConditionComponent::FinishContextInteraction(bool bInterrupted)
{
	AActor* InteractorToApply = PendingInteractor.Get();
	ClearPlayerInteractionWait();

	if (!bInterrupted && InteractorToApply != nullptr)
	{
		TriggerInteraction(InteractorToApply);
	}
}

UUOUPlayerInteractionExecutorComponent* UUOUContextInteractionConditionComponent::FindPlayerInteractionExecutor(
	AActor* Interactor) const
{
	return Interactor != nullptr ? Interactor->FindComponentByClass<UUOUPlayerInteractionExecutorComponent>() : nullptr;
}

void UUOUContextInteractionConditionComponent::ClearPlayerInteractionWait()
{
	if (PendingExecutor != nullptr)
	{
		PendingExecutor->OnInteractionFinished.RemoveDynamic(
			this,
			&UUOUContextInteractionConditionComponent::HandlePlayerInteractionFinished);
	}

	PendingExecutor = nullptr;
	PendingInteractor = nullptr;
	bWaitingForPlayerInteraction = false;
}

void UUOUContextInteractionConditionComponent::HandlePlayerInteractionFinished(
	UObject* InteractionSource,
	bool bInterrupted)
{
	if (InteractionSource != this)
	{
		return;
	}

	FinishContextInteraction(bInterrupted);
}
