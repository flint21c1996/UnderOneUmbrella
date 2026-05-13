// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "World/NPC/UOUNPCActionTypes.h"
#include "UOUNPCCharacter.generated.h"

class AUOUNPCController;
class AUOUNPCCharacter;
class UBehaviorTree;
class UAnimMontage;

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
class AUOUNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AUOUNPCCharacter();

	virtual void BeginPlay() override;
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

	UPROPERTY(BlueprintAssignable, Category = "NPC|Action")
	FOnUOUNPCActionCompletedSignature OnNPCActionCompleted;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Activate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Toggle();

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	bool MoveToConfiguredTarget();

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	bool JumpMoveToConfiguredTarget();

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	bool JumpMoveToTargetLocation(const FVector& TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "NPC|Animation")
	float PlayActivationAnimation();

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
	FUOUNPCActionRequest BuildLegacyActionRequest() const;
	FUOUNPCActionRequest GetCurrentActionRequest() const;
	bool GetTargetLocationFromActionRequest(const FUOUNPCActionRequest& ActionRequest, FVector& OutTargetLocation) const;
	bool GetConfiguredTargetLocation(FVector& OutTargetLocation) const;
	FVector CalculateJumpLaunchVelocity(const FVector& TargetLocation, float TravelTime) const;
};
