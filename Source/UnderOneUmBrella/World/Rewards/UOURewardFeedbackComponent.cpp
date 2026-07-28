// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Rewards/UOURewardFeedbackComponent.h"

#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/UOUContextInteractionTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"
#include "TimerManager.h"
#include "UI/UOUUISubsystem.h"

UUOURewardFeedbackComponent::UUOURewardFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOURewardFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishFeedbackInternal(false);
	Super::EndPlay(EndPlayReason);
}

bool UUOURewardFeedbackComponent::StartFeedback(
	AUOUCharacter* Collector,
	FName RewardId,
	FVector RewardWorldLocation)
{
	if (!bFeedbackEnabled || Collector == nullptr || bFeedbackPlaying)
	{
		return false;
	}

	bFeedbackPlaying = true;
	bFeedbackDurationElapsed = false;
	bCollectionMontagePlaying = false;
	ActiveCollector = Collector;

	SpawnCollectionEffect(Collector, RewardWorldLocation);
	if (bRequestPresentationOnFeedbackStart)
	{
		RequestRewardUIPresentation(Collector, RewardId);
	}
	ApplyPlayerFeedback(Collector);
	StartCollectionMontage();

	const float SafeDuration = FMath::Max(0.0f, FeedbackDuration);
	UWorld* World = GetWorld();
	if (SafeDuration <= KINDA_SMALL_NUMBER || World == nullptr)
	{
		bFeedbackDurationElapsed = true;
		TryFinishFeedback();
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

void UUOURewardFeedbackComponent::RequestRewardUIPresentation(
	AUOUCharacter* Collector,
	FName RewardId) const
{
	if (!PresentationData.bShowResultUI || Collector == nullptr)
	{
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(Collector->GetController());
	ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
	UUOUUISubsystem* UISubsystem = LocalPlayer != nullptr
		? LocalPlayer->GetSubsystem<UUOUUISubsystem>()
		: nullptr;
	if (UISubsystem == nullptr)
	{
		return;
	}

	FUOURewardPresentationData ResolvedPresentationData = PresentationData;
	ResolvedPresentationData.RewardId = RewardId;
	UISubsystem->ShowRewardPresentation(ResolvedPresentationData);
}

void UUOURewardFeedbackComponent::ApplyPlayerFeedback(AUOUCharacter* Collector)
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

bool UUOURewardFeedbackComponent::StartCollectionMontage()
{
	if (CollectionMontage == nullptr || ActiveInputExecutor == nullptr)
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

	if (ActiveCameraController != nullptr)
	{
		ActiveCameraController->ReleaseTemporaryZoom(this);
	}

	ActiveInputExecutor = nullptr;
	ActiveCameraController = nullptr;
	ActiveCollector = nullptr;
	bCollectionMontagePlaying = false;
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

void UUOURewardFeedbackComponent::TryFinishFeedback()
{
	if (!bFeedbackPlaying || !bFeedbackDurationElapsed || bCollectionMontagePlaying)
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
