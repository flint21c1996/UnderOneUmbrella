// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOURewardActor.generated.h"

class AActor;
class AUOURewardActor;
class UPrimitiveComponent;
class UNiagaraComponent;
class UUOURewardFeedbackComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

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

	// Blueprint 전용 피드백을 액터 내부에 추가할 수 있는 확장 지점입니다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Reward|Events")
	void ReceiveRewardCollected(FName CollectedRewardId, AActor* Collector);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<USphereComponent> CollectionTrigger = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	// 수집 전 보상이 목표임을 지속적으로 알려주는 상시 Niagara 표현입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<UNiagaraComponent> ObjectiveEffect = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Components")
	TObjectPtr<UUOURewardFeedbackComponent> RewardFeedbackComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Collection", meta = (ClampMin = "0.0"))
	float TriggerRadius = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Visual")
	FVector MeshRelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Visual")
	FRotator MeshRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Visual")
	FVector MeshRelativeScale = FVector::OneVector;

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
	void DisableAfterCollection();
	void CompleteCollection();
	bool IsValidCollector(const AActor* Candidate) const;

	UFUNCTION()
	void HandleRewardFeedbackFinished();

	UPROPERTY(Transient)
	TObjectPtr<AActor> PendingCollector = nullptr;
};
