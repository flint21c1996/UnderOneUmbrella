// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Rewards/UOURewardFeedbackComponent.h"

#include "Engine/World.h"
#include "Interaction/UOUContextInteractionTypes.h"
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
	bPresentationCameraHoldActive = false;
	FinishFeedbackInternal(false);
	ReleaseCameraFeedback();
	Super::EndPlay(EndPlayReason);
}

bool UUOURewardFeedbackComponent::BeginFeedback(
	AUOUCharacter* Collector,
	FVector RewardWorldLocation)
{
	if (!bFeedbackEnabled
		|| Collector == nullptr
		|| bFeedbackPlaying
		|| bPresentationCameraHoldActive)
	{
		return false;
	}

	bFeedbackPlaying = true;
	bFeedbackDurationElapsed = false;
	bFeedbackSequenceCompleted = false;
	bCollectionMontagePlaying = false;
	ActiveCollector = Collector;
	ActiveRewardWorldLocation = RewardWorldLocation;

	ApplyPlayerInputBlock(Collector);
	BeginFeedbackDurationTimer();
	return true;
}

bool UUOURewardFeedbackComponent::ExecuteFeedbackCue(
	EUOURewardFeedbackCueAction FeedbackAction)
{
	switch (FeedbackAction)
	{
	case EUOURewardFeedbackCueAction::PlayPlayerAnimation:
		return PlayPlayerAnimationFeedback();

	case EUOURewardFeedbackCueAction::SpawnNiagara:
		return SpawnNiagaraFeedback();

	case EUOURewardFeedbackCueAction::StartCameraFocus:
		return StartCameraFeedback();
	}

	return false;
}

bool UUOURewardFeedbackComponent::PlayPlayerAnimationFeedback()
{
	if (!bFeedbackPlaying)
	{
		return false;
	}

	return StartCollectionMontage();
}

bool UUOURewardFeedbackComponent::SpawnNiagaraFeedback()
{
	UWorld* World = GetWorld();
	if (!bFeedbackPlaying || CollectionEffect == nullptr || World == nullptr)
	{
		return false;
	}

	const FVector BaseLocation =
		bSpawnEffectAtCollector && ActiveCollector != nullptr
			? ActiveCollector->GetActorLocation()
			: ActiveRewardWorldLocation;

	UNiagaraComponent* EffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		CollectionEffect,
		BaseLocation + EffectLocationOffset,
		FRotator::ZeroRotator,
		EffectScale,
		true,
		false);

	if (EffectComponent == nullptr)
	{
		return false;
	}

	EffectComponent->Activate(true);
	return true;
}

bool UUOURewardFeedbackComponent::StartCameraFeedback()
{
	if (!bFeedbackPlaying
		|| !bUseTemporaryCameraZoom
		|| ActiveCollector == nullptr)
	{
		return false;
	}

	if (ActiveCameraController == nullptr)
	{
		ActiveCameraController = ActiveCollector->GetCameraControllerComponent();
	}

	if (ActiveCameraController == nullptr)
	{
		return false;
	}

	ActiveCameraController->RequestTemporaryFocusZoom(
		this,
		CameraTargetDistance,
		CameraTargetOrthoWidth,
		CameraFocusOffset,
		true);
	return ActiveCameraController->IsTemporaryZoomRequestedBy(this);
}

void UUOURewardFeedbackComponent::CompleteFeedbackSequence()
{
	if (!bFeedbackPlaying || bFeedbackSequenceCompleted)
	{
		return;
	}

	bFeedbackSequenceCompleted = true;
	TryFinishFeedback();
}

void UUOURewardFeedbackComponent::BeginFeedbackDurationTimer()
{
	const float SafeDuration = FMath::Max(0.0f, FeedbackDuration);
	UWorld* World = GetWorld();
	if (SafeDuration <= KINDA_SMALL_NUMBER || World == nullptr)
	{
		bFeedbackDurationElapsed = true;
		TryFinishFeedback();
		return;
	}

	World->GetTimerManager().SetTimer(
		FeedbackTimerHandle,
		this,
		&UUOURewardFeedbackComponent::HandleFeedbackTimerFinished,
		SafeDuration,
		false);
}

void UUOURewardFeedbackComponent::FinishFeedback()
{
	FinishFeedbackInternal(true);
}

bool UUOURewardFeedbackComponent::IsFeedbackPlaying() const
{
	return bFeedbackPlaying;
}

bool UUOURewardFeedbackComponent::BeginPresentationCameraHold()
{
	if (!bFeedbackPlaying)
	{
		return false;
	}

	bPresentationCameraHoldActive = true;
	return true;
}

void UUOURewardFeedbackComponent::EndPresentationCameraHold()
{
	if (!bPresentationCameraHoldActive)
	{
		return;
	}

	bPresentationCameraHoldActive = false;
	if (bFeedbackPlaying)
	{
		TryFinishFeedback();
	}
	else
	{
		ReleaseCameraFeedback();
	}
}

void UUOURewardFeedbackComponent::ApplyPlayerInputBlock(AUOUCharacter* Collector)
{
	if (Collector == nullptr)
	{
		return;
	}

	ActiveInputExecutor = Collector->GetInteractionExecutorComponent();
	if (bBlockPlayerInput && ActiveInputExecutor != nullptr)
	{
		ActiveInputExecutor->RequestPlayerInputBlock(this, bStopMovementImmediately);
	}
}

bool UUOURewardFeedbackComponent::StartCollectionMontage()
{
	if (bCollectionMontagePlaying
		|| CollectionMontage == nullptr
		|| ActiveInputExecutor == nullptr)
	{
		return false;
	}

	FUOUPlayerInteractionRequest AnimationRequest;
	AnimationRequest.PlayerMontage = CollectionMontage;
	AnimationRequest.MontagePlayRate = FMath::Max(0.01f, MontagePlayRate);
	AnimationRequest.MontageStartSection = MontageStartSection;

	// 전체 피드백의 입력 잠금은 이 컴포넌트가 관리하므로 실행기는 몽타주 수명만 담당합니다.
	AnimationRequest.bBlockPlayerInputDuringInteraction = false;
	AnimationRequest.bStopMovementOnStart = false;

	ActiveInputExecutor->OnInteractionFinished.AddUniqueDynamic(
		this,
		&UUOURewardFeedbackComponent::HandlePlayerInteractionFinished);

	// 재생 실패 과정에서 종료 이벤트가 동기 호출되어도 상태가 올바르게 정리되도록 먼저 기록합니다.
	bCollectionMontagePlaying = true;
	if (ActiveInputExecutor->TryStartInteraction(this, AnimationRequest))
	{
		return true;
	}

	bCollectionMontagePlaying = false;
	ActiveInputExecutor->OnInteractionFinished.RemoveDynamic(
		this,
		&UUOURewardFeedbackComponent::HandlePlayerInteractionFinished);
	return false;
}

void UUOURewardFeedbackComponent::ReleasePlayerFeedback()
{
	if (ActiveInputExecutor != nullptr)
	{
		ActiveInputExecutor->OnInteractionFinished.RemoveDynamic(
			this,
			&UUOURewardFeedbackComponent::HandlePlayerInteractionFinished);

		if (ActiveInputExecutor->IsInteractionActiveFor(this))
		{
			ActiveInputExecutor->CancelActiveInteraction();
		}

		ActiveInputExecutor->ReleasePlayerInputBlock(this);
	}

	ActiveInputExecutor = nullptr;
	ActiveCollector = nullptr;
	bCollectionMontagePlaying = false;

	if (!bPresentationCameraHoldActive)
	{
		ReleaseCameraFeedback();
	}
}

void UUOURewardFeedbackComponent::ReleaseCameraFeedback()
{
	if (ActiveCameraController != nullptr)
	{
		ActiveCameraController->ReleaseTemporaryZoom(this);
	}

	ActiveCameraController = nullptr;
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
	bFeedbackSequenceCompleted = false;
	ActiveRewardWorldLocation = FVector::ZeroVector;
	ReleasePlayerFeedback();

	if (bBroadcastFinished)
	{
		OnFeedbackFinished.Broadcast();
	}
}

void UUOURewardFeedbackComponent::TryFinishFeedback()
{
	if (!bFeedbackPlaying
		|| !bFeedbackSequenceCompleted
		|| bPresentationCameraHoldActive
		|| !bFeedbackDurationElapsed
		|| bCollectionMontagePlaying)
	{
		return;
	}

	FinishFeedbackInternal(true);
}

void UUOURewardFeedbackComponent::HandleFeedbackTimerFinished()
{
	bFeedbackDurationElapsed = true;
	TryFinishFeedback();
}

void UUOURewardFeedbackComponent::HandlePlayerInteractionFinished(
	UObject* InteractionSource,
	bool bInterrupted)
{
	if (InteractionSource != this)
	{
		return;
	}

	if (ActiveInputExecutor != nullptr)
	{
		ActiveInputExecutor->OnInteractionFinished.RemoveDynamic(
			this,
			&UUOURewardFeedbackComponent::HandlePlayerInteractionFinished);
	}

	bCollectionMontagePlaying = false;
	TryFinishFeedback();
}
