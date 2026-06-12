// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Umbrella/UOUUmbrellaInteractionConditionComponent.h"

#include "GameFramework/Actor.h"

UUOUUmbrellaInteractionConditionComponent::UUOUUmbrellaInteractionConditionComponent()
{
	InteractionMode = EUOUInteractionConditionMode::SetSatisfied;
}

TArray<FString> UUOUUmbrellaInteractionConditionComponent::GetPuzzleDebugInfo_Implementation() const
{
	TArray<FString> Lines = Super::GetPuzzleDebugInfo_Implementation();
	Lines.Add(FString::Printf(
		TEXT("Required Umbrella: %s"),
		*StaticEnum<EUOUUmbrellaState>()->GetNameStringByValue(static_cast<int64>(RequiredUmbrellaState))));
	Lines.Add(FString::Printf(TEXT("Last Rejected Interactor: %s"), *GetNameSafe(LastRejectedInteractor.Get())));
	return Lines;
}

void UUOUUmbrellaInteractionConditionComponent::GetPuzzleDebugInputActors_Implementation(
	TArray<AActor*>& OutInputActors) const
{
	Super::GetPuzzleDebugInputActors_Implementation(OutInputActors);

	if (LastRejectedInteractor != nullptr)
	{
		OutInputActors.AddUnique(LastRejectedInteractor.Get());
	}
}

bool UUOUUmbrellaInteractionConditionComponent::CanStartContextInteraction(AActor* Interactor) const
{
	return Super::CanStartContextInteraction(Interactor) && DoesInteractorUmbrellaStateMatch(Interactor);
}

void UUOUUmbrellaInteractionConditionComponent::HandleAcceptedContextInteraction(AActor* Interactor)
{
	LastRejectedInteractor = nullptr;
}

void UUOUUmbrellaInteractionConditionComponent::HandleRejectedContextInteraction(AActor* Interactor)
{
	LastRejectedInteractor = Interactor;
}

bool UUOUUmbrellaInteractionConditionComponent::DoesInteractorUmbrellaStateMatch(AActor* Interactor) const
{
	const UUOUUmbrellaComponent* UmbrellaComponent =
		Interactor != nullptr ? Interactor->FindComponentByClass<UUOUUmbrellaComponent>() : nullptr;
	if (UmbrellaComponent == nullptr)
	{
		return false;
	}

	switch (RequiredUmbrellaState)
	{
	case EUOUUmbrellaState::Closed:
		return UmbrellaComponent->IsClosed();

	case EUOUUmbrellaState::Open:
		return UmbrellaComponent->IsOpen();

	case EUOUUmbrellaState::UpsideDown:
		return UmbrellaComponent->IsUpsideDown();

	case EUOUUmbrellaState::Pouring:
		return UmbrellaComponent->IsPouring();

	default:
		return false;
	}
}
