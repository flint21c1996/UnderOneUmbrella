// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "UOURewardActor.generated.h"

class AActor;
class AUOURewardActor;
class UPrimitiveComponent;
class UNiagaraComponent;
class UUOURewardCollectedConditionComponent;
class UUOURewardCollectionMotionComponent;
class UUOURewardFeedbackComponent;
class USceneComponent;
class USphereComponent;
class USplineComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUOURewardCollectedSignature,
	AUOURewardActor*, RewardActor,
	FName, RewardId,
	AActor*, Collector);

// 월드에 배치되어 회전·부유하다가 플레이어 접촉 시 한 번만 수집되는 보상 액터입니다.
UCLASS(meta=(DisplayName="UOU Reward"))
class UNDERONEUMBRELLA_API AUOURewardActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOURewardActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	// 저장 데이터와 UI 표시 데이터가 동일한 보상을 식별할 때 사용하는 ID입니다.
	UPROPERTY(
		EditInstanceOnly,
		BlueprintReadOnly,
		Category = "Reward",
		meta = (GetOptions = "GetAvailableRewardIds"))
	FName RewardId = NAME_None;

	/** 현재 Actor가 속한 레벨의 Stage Definition 행에서 선택 가능한 Reward ID를 반환합니다. */
	UFUNCTION()
	TArray<FName> GetAvailableRewardIds() const;

	// 수집 피드백까지 끝난 뒤 RewardId와 수집자를 외부 시스템에 전달합니다.
	UPROPERTY(BlueprintAssignable, Category = "Reward|Events")
	FUOURewardCollectedSignature OnRewardCollected;

	// 오버랩 외의 연출이나 Blueprint에서도 동일한 중복 방지 경로로 수집을 요청할 수 있습니다.
	UFUNCTION(BlueprintCallable, Category = "Reward")
	bool TryCollectReward(AActor* Collector);

	UFUNCTION(BlueprintPure, Category = "Reward")
	bool IsCollected() const;

	// 수집 연출과 진행도 기록까지 모두 끝나 퍼즐 조건으로 사용할 수 있는 완료 상태입니다.
	UFUNCTION(BlueprintPure, Category = "Reward")
	bool IsCollectionCompleted() const;

	// 퍼즐 결과에 따라 Reward의 표시, idle 움직임, 수집 상호작용을 함께 전환합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Activation")
	void SetRewardActive(bool bNewActive);

	UFUNCTION(BlueprintPure, Category = "Reward|Activation")
	bool IsRewardActive() const;

	UFUNCTION(BlueprintPure, Category = "Reward|Activation")
	bool IsRewardAppearanceInProgress() const;

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<USphereComponent> CollectionTrigger = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	// 블루프린트 뷰포트에서 활성화 시 VisualMesh가 최종 위치까지 따라올 등장 경로를 편집합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<USplineComponent> AppearanceMotionPath = nullptr;

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

	// 이 Reward의 최종 수집 완료 상태를 Puzzle Condition Group에 제공합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<UUOURewardCollectedConditionComponent> RewardCollectedConditionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<UUOURewardCollectionMotionComponent> RewardCollectionMotionComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Collection", meta = (ClampMin = "0.0"))
	float TriggerRadius = 75.0f;

	// 끄면 퍼즐 결과로 Activate를 받기 전까지 Reward를 숨기고 수집 충돌을 비활성화합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Activation")
	bool bStartActive = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reward|Activation|Runtime")
	bool bRewardActive = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reward|Activation|Runtime")
	bool bRewardAppearanceInProgress = false;

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
	void BeginRewardAppearance();
	void TryCompleteRewardAppearance();
	void CompleteRewardAppearance();
	void DisableCollectionInteraction();
	void HideCollectedVisual();
	void BeginRewardFeedback();
	bool RoutePresentationCueToUI(const FUOURewardPresentationCue& Cue);
	void ExecuteMotionCue(const FUOURewardPresentationCue& Cue);
	void TryCompleteCollection();
	void CompleteCollection();
	bool IsValidCollector(const AActor* Candidate) const;

	UFUNCTION()
	void HandleRewardFeedbackFinished();

	UFUNCTION()
	void HandleAppearanceMotionFinished();

	UFUNCTION()
	void HandleAppearanceMotionCue(const FUOURewardPresentationCue& Cue);

	UFUNCTION()
	void HandleCollectionMotionFinished();

	UFUNCTION()
	void HandleCollectionMotionCue(const FUOURewardPresentationCue& Cue);

	UPROPERTY(Transient)
	TObjectPtr<AActor> PendingCollector = nullptr;

	bool bWaitingForRewardFeedback = false;
	bool bWaitingForCollectionMotion = false;
	bool bWaitingForAppearanceFeedback = false;
	bool bWaitingForAppearanceMotion = false;

	// 에디터에서 VisualMesh에 설정한 Transform을 idle 움직임의 기준으로 보존합니다.
	FVector BaseVisualRelativeLocation = FVector::ZeroVector;
	FRotator BaseVisualRelativeRotation = FRotator::ZeroRotator;
	// 지연 등장 후에도 생성 이후 누적 시간이 한 번에 적용되지 않도록 idle 시작 시점을 분리합니다.
	float IdleMotionStartTime = 0.0f;
};
