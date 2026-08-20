// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Puzzle/Core/UOUPuzzleResultCompletionState.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "World/NPC/UOUNPCActionTypes.h"
#include "UOUNPCCharacter.generated.h"

class AUOUNPCController;
class AUOUNPCCharacter;
class UBehaviorTree;
class UAnimMontage;
class UUOULightExposureReceiverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnUOUNPCActionCompletedSignature,
	AUOUNPCCharacter*, NPC,
	UObject*, ActionSource);

UENUM(BlueprintType)
enum class EOUUNPCActivationAction : uint8
{
	MoveToTarget UMETA(DisplayName = "Move To Target", ToolTip = "기존 직접 활성화 타겟으로 이동합니다."),
	PlayAnimation UMETA(DisplayName = "Play Animation", ToolTip = "기존 직접 활성화용 애니메이션 몽타주를 재생합니다."),
	JumpMoveToTarget UMETA(DisplayName = "Jump Move To Target", ToolTip = "기존 직접 활성화 타겟으로 점프 이동합니다.")
};

// 퍼즐에서 전달한 이동, 점프, 애니메이션 요청을 처리하는 NPC 베이스 캐릭터입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU NPC Character", ToolTip = "퍼즐 액션과 Behavior Tree 태스크로 제어되는 NPC 캐릭터입니다."))
class UNDERONEUMBRELLA_API AUOUNPCCharacter : public ACharacter, public IUOUPuzzleResultReceiver, public IUOUPuzzleResultCompletionState
{
	GENERATED_BODY()

public:
	AUOUNPCCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Activation", meta = (ToolTip = "명시적인 액션 요청이 없을 때 직접 Activate 호출에서 사용할 기본 액션입니다."))
	EOUUNPCActivationAction ActivationAction = EOUUNPCActivationAction::MoveToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Behavior", meta = (ToolTip = "이 NPC의 AI Controller가 실행할 Behavior Tree입니다."))
	TObjectPtr<UBehaviorTree> BehaviorTree = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ToolTip = "기존 직접 활성화 방식에서 Move Target Actor를 사용합니다. 끄면 Move Target Location을 사용합니다."))
	bool bUseMoveTargetActor = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement", meta = (EditCondition = "bUseMoveTargetActor", EditConditionHides, ToolTip = "NPC가 직접 활성화될 때 사용할 기존 방식의 타겟 액터입니다."))
	TObjectPtr<AActor> MoveTargetActor = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement", meta = (EditCondition = "!bUseMoveTargetActor", EditConditionHides, ToolTip = "Move Target Actor를 끈 경우 사용할 기존 방식의 월드 위치 타겟입니다."))
	FVector MoveTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "0.0", ToolTip = "기존 직접 활성화 액션의 도착 허용 거리입니다. 액션 요청에서 이 값을 덮어쓸 수 있습니다."))
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "0.1", ToolTip = "기존 직접 활성화 점프의 이동 시간입니다. 액션 요청에서 이 값을 덮어쓸 수 있습니다."))
	float JumpTravelTime = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ToolTip = "Jump Move 중에도 목표 방향을 향해 회전합니다. LaunchCharacter 이동은 기본 이동 회전이 잘 적용되지 않아 별도로 보정합니다."))
	bool bOrientRotationDuringJumpMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1440.0", EditCondition = "bOrientRotationDuringJumpMove", ToolTip = "Jump Move 중 목표 방향으로 회전하는 속도입니다. 0이면 즉시 회전합니다."))
	float JumpMoveRotationRate = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ToolTip = "점프 착지 후 같은 타겟으로 이동해 착지 오차를 보정합니다."))
	bool bMoveToTargetAfterJumpLanding = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation", meta = (ToolTip = "직접 Play Animation 활성화 또는 액션 기본값으로 사용할 기존 몽타주입니다."))
	TObjectPtr<UAnimMontage> ActivationMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation", meta = (ClampMin = "0.0", ToolTip = "기존 직접 활성화 몽타주의 재생 속도입니다."))
	float ActivationMontagePlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation", meta = (ToolTip = "기존 직접 활성화 몽타주에서 재생을 시작할 섹션 이름입니다."))
	FName ActivationMontageStartSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Activation", meta = (ToolTip = "플레이 시작 시 기존 직접 활성화 액션을 자동으로 실행합니다."))
	bool bActivateOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Activation", meta = (ToolTip = "켜져 있으면 기존 직접 Activate 액션의 Behavior Tree 태스크가 끝났을 때 결과 완료로 보고합니다."))
	bool bCompleteLegacyActivationOnFinish = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "Behavior Tree 블랙보드에서 사용하는 현재 활성화 상태입니다."))
	bool bActivated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "점프 착지 후 위치 보정을 기다리는 동안 true입니다."))
	bool bPendingMoveAfterJumpLanding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "액션 요청 액터나 시퀀스가 현재 NPC 액션을 소유하고 있으면 true입니다."))
	bool bHasActiveActionRequest = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "현재 실행 중인 액션 요청입니다."))
	FUOUNPCActionRequest ActiveActionRequest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "현재 액션을 요청한 오브젝트입니다. 액션 완료 시 시퀀스에 알릴 때 사용합니다."))
	TObjectPtr<UObject> ActiveActionSource = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	bool bHasCompletedResultSinceLastActivation = false;

	UPROPERTY(BlueprintAssignable, Category = "NPC|Action")
	FOnUOUNPCActionCompletedSignature OnNPCActionCompleted;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Activate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Toggle();

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual bool IsPuzzleResultCompleted_Implementation(EOUUPuzzleResultAction Action) const override;
	virtual FOnUOUPuzzleResultCompletionStateChangedNativeSignature* GetPuzzleResultCompletionStateChangedEvent() override;

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	bool MoveToConfiguredTarget();

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	bool JumpMoveToConfiguredTarget();

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	bool JumpMoveToTargetLocation(const FVector& TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "NPC|Animation")
	float PlayActivationAnimation();

	UFUNCTION(BlueprintPure, Category = "NPC|Animation")
	bool IsCurrentAnimationLoopingUntilDeactivated() const;

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	void StopNPCMovement();

	UFUNCTION(BlueprintCallable, Category = "NPC|Action")
	bool RequestNPCAction(UObject* ActionSource, const FUOUNPCActionRequest& ActionRequest);

	UFUNCTION(BlueprintCallable, Category = "NPC|Action")
	bool ClearNPCAction(UObject* ActionSource);

	UFUNCTION(BlueprintCallable, Category = "NPC|Action")
	void CompleteActiveNPCAction();

	UFUNCTION(BlueprintPure, Category = "NPC|Action")
	bool ShouldClearActiveActionOnFinish() const;

	UFUNCTION(BlueprintPure, Category = "NPC|Action")
	bool GetCurrentActionTargetLocation(FVector& OutTargetLocation) const;

	UFUNCTION(BlueprintPure, Category = "NPC|Action")
	float GetCurrentActionAcceptanceRadius() const;

	// 현재 활성 요청이 없으면 레거시 설정으로 만든 요청을 반환합니다.
	FUOUNPCActionRequest GetCurrentActionRequest() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC|Events")
	void ReceiveNPCActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC|Events")
	void ReceiveNPCDeactivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC|Events")
	void ReceiveNPCActionCompleted(UObject* ActionSource);

	UFUNCTION(BlueprintPure, Category = "NPC|Behavior")
	UBehaviorTree* GetBehaviorTree() const;

protected:
	AUOUNPCController* GetNPCController();
	bool SyncActivationBlackboard();
	void ExecuteCurrentActionDirectly();
	void SetActivationResultCompleted(bool bNewCompleted);
	FUOUNPCActionRequest BuildLegacyActionRequest() const;
	bool GetTargetLocationFromActionRequest(const FUOUNPCActionRequest& ActionRequest, FVector& OutTargetLocation) const;
	bool GetConfiguredTargetLocation(FVector& OutTargetLocation) const;
	FVector CalculateJumpLaunchVelocity(const FVector& TargetLocation, float TravelTime) const;
	void UpdateJumpMoveRotation(float DeltaSeconds);
	UAnimMontage* ResolveCurrentAnimationMontage() const;
	float ResolveCurrentAnimationPlayRate() const;
	UUOULightExposureReceiverComponent* ResolveAnimationTemperatureReceiver() const;
	float CalculateTemperatureDrivenAnimationPlayRate(
		const FUOUNPCActionRequest& ActionRequest,
		float Temperature) const;
	void StartTemperatureDrivenAnimationTracking(UAnimMontage* Montage);
	void StopTemperatureDrivenAnimationTracking();
	void UpdateTemperatureDrivenAnimationPlayRate();

	UFUNCTION()
	void HandleAnimationTemperatureChanged(float NewTemperature, float PreviousTemperature);

	FOnUOUPuzzleResultCompletionStateChangedNativeSignature OnPuzzleResultCompletionStateChanged;

	bool bJumpMoveRotationActive = false;
	FVector JumpMoveRotationTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UUOULightExposureReceiverComponent> BoundAnimationTemperatureReceiver = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> TemperatureDrivenAnimationMontage = nullptr;
};
