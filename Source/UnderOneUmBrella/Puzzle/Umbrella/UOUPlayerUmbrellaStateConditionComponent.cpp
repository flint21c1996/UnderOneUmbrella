// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Umbrella/UOUPlayerUmbrellaStateConditionComponent.h"

#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UUOUPlayerUmbrellaStateConditionComponent::UUOUPlayerUmbrellaStateConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUPlayerUmbrellaStateConditionComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshPlayerUmbrellaReference();
	RefreshConditionState();
}

void UUOUPlayerUmbrellaStateConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPlayerUmbrellaReference();
	Super::EndPlay(EndPlayReason);
}

void UUOUPlayerUmbrellaStateConditionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshPlayerUmbrellaReference();
	RefreshConditionState();
}

TArray<FString> UUOUPlayerUmbrellaStateConditionComponent::GetPuzzleDebugInfo_Implementation() const
{
	return {
		FString::Printf(
			TEXT("Player Umbrella Condition: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(
			TEXT("Required Umbrella: %s"),
			*StaticEnum<EUOUUmbrellaState>()->GetNameStringByValue(static_cast<int64>(RequiredUmbrellaState))),
		FString::Printf(TEXT("Player: %s"), *GetNameSafe(CachedPlayerPawn.Get())),
		FString::Printf(TEXT("Umbrella Component: %s"), *GetNameSafe(CachedUmbrellaComponent.Get()))
	};
}

void UUOUPlayerUmbrellaStateConditionComponent::GetPuzzleDebugInputActors_Implementation(
	TArray<AActor*>& OutInputActors) const
{
	if (CachedPlayerPawn != nullptr)
	{
		OutInputActors.AddUnique(CachedPlayerPawn.Get());
	}
}

void UUOUPlayerUmbrellaStateConditionComponent::RefreshPlayerUmbrellaReference()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, PlayerIndex);
	UUOUUmbrellaComponent* UmbrellaComponent =
		PlayerPawn != nullptr ? PlayerPawn->FindComponentByClass<UUOUUmbrellaComponent>() : nullptr;

	if (PlayerPawn == CachedPlayerPawn.Get() && UmbrellaComponent == CachedUmbrellaComponent.Get())
	{
		return;
	}

	ClearPlayerUmbrellaReference();

	CachedPlayerPawn = PlayerPawn;
	CachedUmbrellaComponent = UmbrellaComponent;

	if (CachedUmbrellaComponent != nullptr)
	{
		CachedUmbrellaComponent->OnUmbrellaStateChanged.AddUniqueDynamic(
			this,
			&UUOUPlayerUmbrellaStateConditionComponent::HandleUmbrellaStateChanged);
	}
}

void UUOUPlayerUmbrellaStateConditionComponent::HandleUmbrellaStateChanged(
	EUOUUmbrellaState NewState,
	bool bHasUmbrella)
{
	RefreshConditionState();
}

void UUOUPlayerUmbrellaStateConditionComponent::ClearPlayerUmbrellaReference()
{
	if (CachedUmbrellaComponent != nullptr)
	{
		CachedUmbrellaComponent->OnUmbrellaStateChanged.RemoveDynamic(
			this,
			&UUOUPlayerUmbrellaStateConditionComponent::HandleUmbrellaStateChanged);
	}

	CachedUmbrellaComponent = nullptr;
	CachedPlayerPawn = nullptr;
}

void UUOUPlayerUmbrellaStateConditionComponent::RefreshConditionState()
{
	SetSatisfiedState(DoesCurrentUmbrellaStateMatch(), true);
}

bool UUOUPlayerUmbrellaStateConditionComponent::DoesCurrentUmbrellaStateMatch() const
{
	if (CachedUmbrellaComponent == nullptr)
	{
		return false;
	}

	switch (RequiredUmbrellaState)
	{
	case EUOUUmbrellaState::Closed:
		return CachedUmbrellaComponent->IsClosed();

	case EUOUUmbrellaState::Open:
		return CachedUmbrellaComponent->IsOpen();

	case EUOUUmbrellaState::UpsideDown:
		return CachedUmbrellaComponent->IsUpsideDown();

	case EUOUUmbrellaState::Pouring:
		return CachedUmbrellaComponent->IsPouring();

	default:
		return false;
	}
}
