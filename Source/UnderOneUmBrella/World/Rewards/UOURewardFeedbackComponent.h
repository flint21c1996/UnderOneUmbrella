// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOURewardFeedbackComponent.generated.h"

class AUOUCharacter;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback", meta = (ClampMin = "0.0"))
	float FeedbackDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Effect")
	TObjectPtr<UNiagaraSystem> CollectionEffect = nullptr;

	// true면 플레이어 위치, false면 수집되기 직전 보상 위치를 파티클 기준점으로 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Effect")
	bool bSpawnEffectAtCollector = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Effect")
	FVector EffectLocationOffset = FVector(0.0f, 0.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Effect")
	FVector EffectScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Input")
	bool bBlockPlayerInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Input")
	bool bStopMovementImmediately = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Camera")
	bool bUseTemporaryCameraZoom = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Camera", meta = (ClampMin = "0.0"))
	float CameraTargetDistance = 300.0f;

	// 프로젝트의 기본 직교 카메라에서는 값이 작을수록 플레이어가 크게 보입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Feedback|Camera", meta = (ClampMin = "1.0"))
	float CameraTargetOrthoWidth = 1100.0f;

	UPROPERTY(BlueprintAssignable, Category = "Reward|Feedback|Events")
	FUOURewardFeedbackFinishedSignature OnFeedbackFinished;

	UFUNCTION(BlueprintCallable, Category = "Reward|Feedback")
	bool StartFeedback(AUOUCharacter* Collector, FVector RewardWorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Reward|Feedback")
	void FinishFeedback();

	UFUNCTION(BlueprintPure, Category = "Reward|Feedback")
	bool IsFeedbackPlaying() const;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Reward|Feedback|Runtime")
	bool bFeedbackPlaying = false;

private:
	void SpawnCollectionEffect(const AUOUCharacter* Collector, const FVector& RewardWorldLocation) const;
	void ApplyPlayerFeedback(AUOUCharacter* Collector);
	void ReleasePlayerFeedback();
	void FinishFeedbackInternal(bool bBroadcastFinished);
	void HandleFeedbackTimerFinished();

	FTimerHandle FeedbackTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<AUOUCharacter> ActiveCollector = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUCameraControllerComponent> ActiveCameraController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUPlayerInteractionExecutorComponent> ActiveInputExecutor = nullptr;
};
