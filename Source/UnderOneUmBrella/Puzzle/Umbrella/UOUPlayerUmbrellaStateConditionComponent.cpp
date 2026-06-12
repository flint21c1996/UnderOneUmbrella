// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Umbrella/UOUPlayerUmbrellaStateConditionComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	constexpr float PlayerResolveRetryInterval = 0.1f;
}

UUOUPlayerUmbrellaStateConditionComponent::UUOUPlayerUmbrellaStateConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUPlayerUmbrellaStateConditionComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshPlayerUmbrellaReference();
	if (CachedUmbrellaComponent == nullptr)
	{
		StartWaitingForPlayerSpawn();
		StartPlayerResolveRetry();
	}

	RefreshConditionState();
}

void UUOUPlayerUmbrellaStateConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopWaitingForPlayerSpawn();
	StopPlayerResolveRetry();
	ClearPlayerUmbrellaReference();
	Super::EndPlay(EndPlayReason);
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
		FString::Printf(
			TEXT("Required Umbrella Direction: %s"),
			*StaticEnum<EUOUUmbrellaDirectionRequirement>()->GetNameStringByValue(static_cast<int64>(RequiredUmbrellaDirection))),
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
		StopWaitingForPlayerSpawn();
		StopPlayerResolveRetry();
	}
}

void UUOUPlayerUmbrellaStateConditionComponent::HandleUmbrellaStateChanged(
	EUOUUmbrellaState NewState,
	bool bHasUmbrella)
{
	RefreshConditionState();
}

void UUOUPlayerUmbrellaStateConditionComponent::HandleActorSpawned(AActor* SpawnedActor)
{
	APawn* SpawnedPawn = Cast<APawn>(SpawnedActor);
	if (SpawnedPawn == nullptr || SpawnedPawn->FindComponentByClass<UUOUUmbrellaComponent>() == nullptr)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, PlayerIndex);
	if (PlayerPawn == nullptr)
	{
		StartPlayerResolveRetry();
		return;
	}

	if (SpawnedPawn != PlayerPawn)
	{
		return;
	}

	RefreshPlayerUmbrellaReference();
	RefreshConditionState();
}

void UUOUPlayerUmbrellaStateConditionComponent::StartWaitingForPlayerSpawn()
{
	if (ActorSpawnedHandle.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(
				this,
				&UUOUPlayerUmbrellaStateConditionComponent::HandleActorSpawned));
	}
}

void UUOUPlayerUmbrellaStateConditionComponent::StopWaitingForPlayerSpawn()
{
	if (!ActorSpawnedHandle.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}

	ActorSpawnedHandle.Reset();
}

void UUOUPlayerUmbrellaStateConditionComponent::StartPlayerResolveRetry()
{
	UWorld* World = GetWorld();
	if (World == nullptr || World->GetTimerManager().IsTimerActive(PlayerResolveRetryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		PlayerResolveRetryTimerHandle,
		this,
		&UUOUPlayerUmbrellaStateConditionComponent::RetryPlayerUmbrellaReference,
		PlayerResolveRetryInterval,
		true);
}

void UUOUPlayerUmbrellaStateConditionComponent::StopPlayerResolveRetry()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerResolveRetryTimerHandle);
	}
}

void UUOUPlayerUmbrellaStateConditionComponent::RetryPlayerUmbrellaReference()
{
	RefreshPlayerUmbrellaReference();
	RefreshConditionState();

	if (CachedUmbrellaComponent != nullptr)
	{
		StopPlayerResolveRetry();
	}
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

	if (!DoesCurrentUmbrellaDirectionMatch())
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

bool UUOUPlayerUmbrellaStateConditionComponent::DoesCurrentUmbrellaDirectionMatch() const
{
	if (CachedUmbrellaComponent == nullptr)
	{
		return false;
	}

	switch (RequiredUmbrellaDirection)
	{
	case EUOUUmbrellaDirectionRequirement::Any:
		return true;

	case EUOUUmbrellaDirectionRequirement::Normal:
		return CachedUmbrellaComponent->IsNormalDirection();

	case EUOUUmbrellaDirectionRequirement::Reversed:
		return CachedUmbrellaComponent->IsReversedDirection();

	default:
		return false;
	}
}
