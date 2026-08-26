// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Rewards/UOURewardFeedbackComponent.h"

#include "Engine/World.h"
#include "Interaction/UOUContextInteractionTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"

#if WITH_EDITOR
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"
#include "World/Rewards/UOURewardAppearanceMotionComponent.h"
#include "World/Rewards/UOURewardCollectionMotionComponent.h"
#endif

UUOURewardFeedbackComponent::UUOURewardFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOURewardFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishFeedbackInternal(false);
	ReleaseCameraFeedback();
	Super::EndPlay(EndPlayReason);
}

const TArray<FUOURewardPresentationCue>&
UUOURewardFeedbackComponent::GetCueRequests() const
{
	return CueRequests;
}

const TArray<FUOURewardPresentationCue>&
UUOURewardFeedbackComponent::GetAppearanceCueRequests() const
{
	return AppearanceFeedback.CueRequests;
}

#if WITH_EDITOR
void UUOURewardFeedbackComponent::PostEditChangeChainProperty(
	FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	SynchronizeCueRequestsForEditor();
}

void UUOURewardFeedbackComponent::SynchronizeCueRequestsForEditor()
{
	const bool bCollectionRequestIdsChanged =
		NormalizeCueRequestIdsForEditor(CueRequests);
	const bool bAppearanceRequestIdsChanged =
		NormalizeCueRequestIdsForEditor(AppearanceFeedback.CueRequests);

	AActor* RewardOwner = GetOwner();
	if (RewardOwner == nullptr)
	{
		RewardOwner = GetTypedOuter<AActor>();
	}

	if (RewardOwner != nullptr)
	{
		if (UUOURewardCollectionMotionComponent* MotionComponent =
			RewardOwner->FindComponentByClass<UUOURewardCollectionMotionComponent>())
		{
			MotionComponent->SynchronizeCueTimelineForEditor(CueRequests);
		}
		if (UUOURewardAppearanceMotionComponent* MotionComponent =
			RewardOwner->FindComponentByClass<UUOURewardAppearanceMotionComponent>())
		{
			MotionComponent->SynchronizeCueTimelineForEditor(
				AppearanceFeedback.CueRequests);
		}
	}

	if (bCollectionRequestIdsChanged || bAppearanceRequestIdsChanged)
	{
		MarkPackageDirty();
	}
}

bool UUOURewardFeedbackComponent::NormalizeCueRequestIdsForEditor(
	TArray<FUOURewardPresentationCue>& Requests)
{
	TSet<FGuid> UsedRequestIds;
	bool bChanged = false;

	for (FUOURewardPresentationCue& CueRequest : Requests)
	{
		if (!CueRequest.RequestId.IsValid()
			|| UsedRequestIds.Contains(CueRequest.RequestId))
		{
			if (!bChanged)
			{
				Modify();
				bChanged = true;
			}

			do
			{
				CueRequest.RequestId = FGuid::NewGuid();
			}
			while (UsedRequestIds.Contains(CueRequest.RequestId));
		}

		UsedRequestIds.Add(CueRequest.RequestId);
	}

	return bChanged;
}
#endif

bool UUOURewardFeedbackComponent::BeginFeedback(
	AUOUCharacter* Collector,
	FVector RewardWorldLocation)
{
	return BeginFeedbackInternal(Collector, RewardWorldLocation, false);
}

bool UUOURewardFeedbackComponent::BeginAppearanceFeedback(
	AUOUCharacter* PlayerCharacter,
	FVector RewardWorldLocation)
{
	return BeginFeedbackInternal(PlayerCharacter, RewardWorldLocation, true);
}

bool UUOURewardFeedbackComponent::BeginFeedbackInternal(
	AUOUCharacter* Collector,
	FVector RewardWorldLocation,
	bool bForAppearance)
{
	const bool bPhaseEnabled = bForAppearance
		? AppearanceFeedback.bEnabled
		: bFeedbackEnabled;
	if (!bPhaseEnabled
		|| Collector == nullptr
		|| bFeedbackPlaying)
	{
		return false;
	}

	bFeedbackPlaying = true;
	bFeedbackSequenceCompleted = false;
	bCollectionMontagePlaying = false;
	bAppearanceFeedbackActive = bForAppearance;
	ActiveCollector = Collector;
	ActiveRewardWorldLocation = RewardWorldLocation;

	ApplyPlayerInputBlock(Collector);
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
	UNiagaraSystem* Effect = bAppearanceFeedbackActive
		? AppearanceFeedback.NiagaraEffect.Get()
		: CollectionEffect.Get();
	if (!bFeedbackPlaying || Effect == nullptr || World == nullptr)
	{
		return false;
	}

	const FVector BaseLocation =
		(bAppearanceFeedbackActive
			? AppearanceFeedback.bSpawnEffectAtPlayer
			: bSpawnEffectAtCollector)
			&& ActiveCollector != nullptr
			? ActiveCollector->GetActorLocation()
			: ActiveRewardWorldLocation;
	const FVector LocationOffset = bAppearanceFeedbackActive
		? AppearanceFeedback.EffectLocationOffset
		: EffectLocationOffset;
	const FVector SpawnScale = bAppearanceFeedbackActive
		? AppearanceFeedback.EffectScale
		: EffectScale;

	UNiagaraComponent* EffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		Effect,
		BaseLocation + LocationOffset,
		FRotator::ZeroRotator,
		SpawnScale,
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
	const bool bUseZoom = bAppearanceFeedbackActive
		? AppearanceFeedback.bUseTemporaryCameraZoom
		: bUseTemporaryCameraZoom;
	if (!bFeedbackPlaying
		|| !bUseZoom
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
		bAppearanceFeedbackActive
			? AppearanceFeedback.CameraTargetDistance
			: CameraTargetDistance,
		bAppearanceFeedbackActive
			? AppearanceFeedback.CameraTargetOrthoWidth
			: CameraTargetOrthoWidth,
		bAppearanceFeedbackActive
			? AppearanceFeedback.CameraFocusOffset
			: CameraFocusOffset,
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

void UUOURewardFeedbackComponent::FinishFeedback()
{
	FinishFeedbackInternal(true);
}

void UUOURewardFeedbackComponent::CancelFeedback()
{
	FinishFeedbackInternal(false);
}

bool UUOURewardFeedbackComponent::IsFeedbackPlaying() const
{
	return bFeedbackPlaying;
}

void UUOURewardFeedbackComponent::ApplyPlayerInputBlock(AUOUCharacter* Collector)
{
	if (Collector == nullptr)
	{
		return;
	}

	ActiveInputExecutor = Collector->GetInteractionExecutorComponent();
	const bool bShouldBlockInput = bAppearanceFeedbackActive
		? AppearanceFeedback.bBlockPlayerInput
		: bBlockPlayerInput;
	const bool bShouldStopMovement = bAppearanceFeedbackActive
		? AppearanceFeedback.bStopMovementImmediately
		: bStopMovementImmediately;
	if (bShouldBlockInput && ActiveInputExecutor != nullptr)
	{
		ActiveInputExecutor->RequestPlayerInputBlock(this, bShouldStopMovement);
	}
}

bool UUOURewardFeedbackComponent::StartCollectionMontage()
{
	UAnimMontage* Montage = bAppearanceFeedbackActive
		? AppearanceFeedback.PlayerMontage.Get()
		: CollectionMontage.Get();
	if (bCollectionMontagePlaying
		|| Montage == nullptr
		|| ActiveInputExecutor == nullptr)
	{
		return false;
	}

	FUOUPlayerInteractionRequest AnimationRequest;
	AnimationRequest.PlayerMontage = Montage;
	AnimationRequest.MontagePlayRate = FMath::Max(
		0.01f,
		bAppearanceFeedbackActive
			? AppearanceFeedback.MontagePlayRate
			: MontagePlayRate);
	AnimationRequest.MontageStartSection = bAppearanceFeedbackActive
		? AppearanceFeedback.MontageStartSection
		: MontageStartSection;
	AnimationRequest.bHoldMontageLastPoseUntilInteractionEnds = true;

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
	ReleaseCameraFeedback();
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

	bFeedbackPlaying = false;
	bFeedbackSequenceCompleted = false;
	bAppearanceFeedbackActive = false;
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
		|| !bFeedbackSequenceCompleted)
	{
		return;
	}

	FinishFeedbackInternal(true);
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
