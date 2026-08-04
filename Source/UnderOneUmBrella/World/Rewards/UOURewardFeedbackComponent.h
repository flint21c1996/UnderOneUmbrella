// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "UOURewardFeedbackComponent.generated.h"

class AUOUCharacter;
class UAnimMontage;
class UNiagaraSystem;
class UUOUCameraControllerComponent;
class UUOUPlayerInteractionExecutorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOURewardFeedbackFinishedSignature);

// 보상 수집 시 파티클, 입력 잠금, 카메라 줌을 하나의 짧은 연출 수명 주기로 관리합니다.
UCLASS(ClassGroup=(Reward), meta=(BlueprintSpawnableComponent, DisplayName="UOU Reward Feedback"))
class UNDERONEUMBRELLA_API UUOURewardFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOURewardFeedbackComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback")
	bool bFeedbackEnabled = true;

	// 하나의 Motion 시간축에서 실행할 Feedback 및 Presentation 요청 목록입니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Timeline",
		meta = (DisplayName = "Cue Requests"))
	TArray<FUOURewardPresentationCue> CueRequests;

	// MotionComponent가 수집 시작 시 복사할 Cue 요청 목록을 제공합니다.
	const TArray<FUOURewardPresentationCue>& GetCueRequests() const;

#if WITH_EDITOR
	// Motion 타임라인 편집기가 CueRequest의 내부 식별자를 준비할 때만 사용합니다.
	TArray<FUOURewardPresentationCue>& GetMutableCueRequestsForEditor()
	{
		return CueRequests;
	}
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Effect")
	TObjectPtr<UNiagaraSystem> CollectionEffect = nullptr;

	// true면 플레이어 위치, false면 수집되기 직전 보상 위치를 파티클 기준점으로 사용합니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Effect",
		meta = (EditCondition = "CollectionEffect != nullptr", EditConditionHides))
	bool bSpawnEffectAtCollector = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Effect",
		meta = (EditCondition = "CollectionEffect != nullptr", EditConditionHides))
	FVector EffectLocationOffset = FVector(0.0f, 0.0f, 100.0f);

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Effect",
		meta = (EditCondition = "CollectionEffect != nullptr", EditConditionHides))
	FVector EffectScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Input")
	bool bBlockPlayerInput = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Input",
		meta = (EditCondition = "bBlockPlayerInput", EditConditionHides))
	bool bStopMovementImmediately = true;

	// 플레이어가 보상을 획득했음을 몸동작으로 보여줄 전용 몽타주입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Animation")
	TObjectPtr<UAnimMontage> CollectionMontage = nullptr;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Animation",
		meta = (
			ClampMin = "0.01",
			EditCondition = "CollectionMontage != nullptr",
			EditConditionHides))
	float MontagePlayRate = 1.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Animation",
		meta = (EditCondition = "CollectionMontage != nullptr", EditConditionHides))
	FName MontageStartSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Camera")
	bool bUseTemporaryCameraZoom = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Camera",
		meta = (
			ClampMin = "0.0",
			EditCondition = "bUseTemporaryCameraZoom",
			EditConditionHides))
	float CameraTargetDistance = 300.0f;

	// 프로젝트의 기본 직교 카메라에서는 값이 작을수록 플레이어가 크게 보입니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Camera",
		meta = (
			ClampMin = "1.0",
			EditCondition = "bUseTemporaryCameraZoom",
			EditConditionHides))
	float CameraTargetOrthoWidth = 500.0f;

	// 임시 줌 중 평상시 주시점에서 이동할 캐릭터 기준 오프셋입니다. Z를 높이면 상체가 화면 중심에 가까워집니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Feedback|Camera",
		meta = (
			EditCondition = "bUseTemporaryCameraZoom",
			EditConditionHides,
			ToolTip = "임시 줌 중 평상시 주시점에서 이동할 캐릭터 기준 오프셋입니다. X는 앞, Y는 오른쪽, Z는 위쪽이며 Z를 높이면 캐릭터 상체가 화면 중심에 가까워집니다."))
	FVector CameraFocusOffset = FVector(0.0f, 0.0f, 80.0f);

	UPROPERTY(BlueprintAssignable, Category = "Reward|Feedback|Events")
	FUOURewardFeedbackFinishedSignature OnFeedbackFinished;

	// 개별 피드백 동작을 실행하기 전에 플레이어 참조와 입력 차단 상태를 준비합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Feedback")
	bool BeginFeedback(AUOUCharacter* Collector, FVector RewardWorldLocation);

	// Feedback Cue 목록에서 선택한 동작 하나만 실행합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Feedback|Cue")
	bool ExecuteFeedbackCue(EUOURewardFeedbackCueAction FeedbackAction);

	// 예약된 개별 피드백 Cue가 모두 전달되었음을 알려 종료 조건 평가를 허용합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Feedback")
	void CompleteFeedbackSequence();

	UFUNCTION(BlueprintCallable, Category = "Reward|Feedback")
	void FinishFeedback();

	UFUNCTION(BlueprintPure, Category = "Reward|Feedback")
	bool IsFeedbackPlaying() const;

	// UI Presentation이 끝날 때까지 현재 임시 카메라 요청을 유지합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Feedback|Camera")
	bool BeginPresentationCameraHold();

	// UI Presentation 종료 후 Hold를 해제합니다. Feedback도 끝났다면 이때 카메라가 복귀합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Feedback|Camera")
	void EndPresentationCameraHold();

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Reward|Feedback|Runtime")
	bool bFeedbackPlaying = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Reward|Feedback|Runtime")
	bool bPresentationCameraHoldActive = false;

private:
	// ExecuteFeedbackCue가 선택한 개별 피드백 동작을 실제로 실행합니다.
	bool PlayPlayerAnimationFeedback();
	bool SpawnNiagaraFeedback();
	bool StartCameraFeedback();

	void ApplyPlayerInputBlock(AUOUCharacter* Collector);
	bool StartCollectionMontage();
	void ReleasePlayerFeedback();
	void ReleaseCameraFeedback();
	void FinishFeedbackInternal(bool bBroadcastFinished);
	void TryFinishFeedback();

	UFUNCTION()
	void HandlePlayerInteractionFinished(UObject* InteractionSource, bool bInterrupted);

	// Motion 타임라인이 종료되어 예약된 Cue가 모두 전달되었는지 나타냅니다.
	bool bFeedbackSequenceCompleted = false;
	bool bCollectionMontagePlaying = false;
	// Niagara를 Reward 위치에 생성할 때 사용하는 수집 시작 시점의 위치입니다.
	FVector ActiveRewardWorldLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<AUOUCharacter> ActiveCollector = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUCameraControllerComponent> ActiveCameraController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUPlayerInteractionExecutorComponent> ActiveInputExecutor = nullptr;
};
