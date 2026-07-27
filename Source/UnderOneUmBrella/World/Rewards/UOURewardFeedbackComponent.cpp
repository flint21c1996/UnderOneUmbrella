// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Rewards/UOURewardFeedbackComponent.h"

#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"
#include "TimerManager.h"

UUOURewardFeedbackComponent::UUOURewardFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOURewardFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishFeedbackInternal(false);
	Super::EndPlay(EndPlayReason);
}

bool UUOURewardFeedbackComponent::StartFeedback(AUOUCharacter* Collector, FVector RewardWorldLocation)
{
	if (!bFeedbackEnabled || Collector == nullptr || bFeedbackPlaying)
	{
		return false;
	}

	bFeedbackPlaying = true;
	ActiveCollector = Collector;

	SpawnCollectionEffect(Collector, RewardWorldLocation);
	ApplyPlayerFeedback(Collector);

	const float SafeDuration = FMath::Max(0.0f, FeedbackDuration);
	UWorld* World = GetWorld();
	if (SafeDuration <= KINDA_SMALL_NUMBER || World == nullptr)
	{
		FinishFeedbackInternal(true);
		return true;
	}

	World->GetTimerManager().SetTimer(
		FeedbackTimerHandle,
		this,
		&UUOURewardFeedbackComponent::HandleFeedbackTimerFinished,
		SafeDuration,
		false);
	return true;
}

void UUOURewardFeedbackComponent::FinishFeedback()
{
	FinishFeedbackInternal(true);
}

bool UUOURewardFeedbackComponent::IsFeedbackPlaying() const
{
	return bFeedbackPlaying;
}

void UUOURewardFeedbackComponent::SpawnCollectionEffect(
	const AUOUCharacter* Collector,
	const FVector& RewardWorldLocation) const
{
	UWorld* World = GetWorld();
	if (CollectionEffect == nullptr || World == nullptr)
	{
		return;
	}

	const FVector BaseLocation = bSpawnEffectAtCollector && Collector != nullptr
		? Collector->GetActorLocation()
		: RewardWorldLocation;

	UNiagaraComponent* EffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		CollectionEffect,
		BaseLocation + EffectLocationOffset,
		FRotator::ZeroRotator,
		EffectScale,
		true,
		false);

	if (EffectComponent != nullptr)
	{
		EffectComponent->Activate(true);
	}
}

void UUOURewardFeedbackComponent::ApplyPlayerFeedback(AUOUCharacter* Collector)
{
	if (Collector == nullptr)
	{
		return;
	}

	if (bBlockPlayerInput)
	{
		ActiveInputExecutor = Collector->GetInteractionExecutorComponent();
		if (ActiveInputExecutor != nullptr)
		{
			ActiveInputExecutor->RequestPlayerInputBlock(this, bStopMovementImmediately);
		}
	}

	if (bUseTemporaryCameraZoom)
	{
		ActiveCameraController = Collector->GetCameraControllerComponent();
		if (ActiveCameraController != nullptr)
		{
			ActiveCameraController->RequestTemporaryZoom(
				this,
				CameraTargetDistance,
				CameraTargetOrthoWidth);
		}
	}
}

void UUOURewardFeedbackComponent::ReleasePlayerFeedback()
{
	if (ActiveInputExecutor != nullptr)
	{
		ActiveInputExecutor->ReleasePlayerInputBlock(this);
	}

	if (ActiveCameraController != nullptr)
	{
		ActiveCameraController->ReleaseTemporaryZoom(this);
	}

	ActiveInputExecutor = nullptr;
	ActiveCameraController = nullptr;
	ActiveCollector = nullptr;
}

void UUOURewardFeedbackComponent::FinishFeedbackInternal(bool bBroadcastFinished)
{
	if (!bFeedbackPlaying)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimerHandle);
	}

	bFeedbackPlaying = false;
	ReleasePlayerFeedback();

	if (bBroadcastFinished)
	{
		OnFeedbackFinished.Broadcast();
	}
}

void UUOURewardFeedbackComponent::HandleFeedbackTimerFinished()
{
	FinishFeedbackInternal(true);
}
