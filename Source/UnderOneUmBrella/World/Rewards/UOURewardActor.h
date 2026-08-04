// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "UOURewardActor.generated.h"

class AActor;
class AUOURewardActor;
class UPrimitiveComponent;
class UNiagaraComponent;
class UUOURewardCollectionMotionComponent;
class UUOURewardFeedbackComponent;
class USceneComponent;
class USphereComponent;
class USplineComponent;
class UStaticMeshComponent;
class UUOUUISubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUOURewardCollectedSignature,
	AUOURewardActor*, RewardActor,
	FName, RewardId,
	AActor*, Collector);

// 월드에 배치되어 회전·부유하다가 플레이어 접촉 시 한 번만 수집되는 보상 액터입니다.
UCLASS(meta=(DisplayName="UOU Reward"))
class UNDERONEUMBRELLA_API AUOURewardActor : public AActor
{
	GENERATED_BODY()

public:
	AUOURewardActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// 저장 데이터와 UI 표시 데이터가 동일한 보상을 식별할 때 사용하는 ID입니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Reward")
	FName RewardId = NAME_None;

	// 수집 피드백까지 끝난 뒤 RewardId와 수집자를 외부 시스템에 전달합니다.
	UPROPERTY(BlueprintAssignable, Category = "Reward|Events")
	FUOURewardCollectedSignature OnRewardCollected;

	// 오버랩 외의 연출이나 Blueprint에서도 동일한 중복 방지 경로로 수집을 요청할 수 있습니다.
	UFUNCTION(BlueprintCallable, Category = "Reward")
	bool TryCollectReward(AActor* Collector);

	UFUNCTION(BlueprintPure, Category = "Reward")
	bool IsCollected() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<USphereComponent> CollectionTrigger = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	// 블루프린트 뷰포트에서 수집 시 VisualMesh가 따라갈 경로를 편집합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<USplineComponent> CollectionMotionPath = nullptr;

	// 수집 전 보상이 목표임을 지속적으로 알려주는 상시 Niagara 표현입니다.
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Components",
		meta = (ToolTip = "움직이는 액터를 비주얼 이펙트가 따라가도록, 할당한 나이아가라 시스템의 모든 활성 이미터는 Local Space를 사용해야 합니다."))
	TObjectPtr<UNiagaraComponent> ObjectiveEffect = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<UUOURewardFeedbackComponent> RewardFeedbackComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<UUOURewardCollectionMotionComponent> RewardCollectionMotionComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Collection", meta = (ClampMin = "0.0"))
	float TriggerRadius = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Visual|Idle")
	bool bUseHoverMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Visual|Idle", meta = (ClampMin = "0.0"))
	float HoverAmplitude = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Visual|Idle", meta = (ClampMin = "0.0"))
	float HoverSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Visual|Idle")
	bool bUseRotationMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Visual|Idle")
	FRotator RotationSpeed = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reward|Runtime")
	bool bCollected = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reward|Runtime")
	bool bCollectionCompleted = false;

private:
	UFUNCTION()
	void HandleCollectionTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void ApplyComponentSettings();
	void DisableCollectionInteraction();
	void HideCollectedVisual();
	void BeginRewardFeedback();
	bool RoutePresentationCueToUI(const FUOURewardPresentationCue& Cue);
	void TryCompleteCollection();
	void CompleteCollection();
	bool IsValidCollector(const AActor* Candidate) const;
	void BindRewardPresentationFinished(UUOUUISubsystem* UISubsystem);
	void UnbindRewardPresentationFinished();

	UFUNCTION()
	void HandleRewardFeedbackFinished();

	UFUNCTION()
	void HandleCollectionMotionFinished();

	UFUNCTION()
	void HandleCollectionMotionCue(const FUOURewardPresentationCue& Cue);

	UFUNCTION()
	void HandleRewardPresentationFinished(FName FinishedRewardId);

	UPROPERTY(Transient)
	TObjectPtr<AActor> PendingCollector = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUUISubsystem> ActiveRewardUISubsystem = nullptr;

	bool bWaitingForRewardFeedback = false;
	bool bWaitingForCollectionMotion = false;
	bool bRewardPresentationActive = false;

	// 에디터에서 VisualMesh에 설정한 Transform을 idle 움직임의 기준으로 보존합니다.
	FVector BaseVisualRelativeLocation = FVector::ZeroVector;
	FRotator BaseVisualRelativeRotation = FRotator::ZeroRotator;
};
