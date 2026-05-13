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
	MoveToTarget,
	PlayAnimation,
	JumpMoveToTarget
};

UCLASS(Blueprintable, meta = (DisplayName = "UOU NPC Character"))
class AUOUNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AUOUNPCCharacter();

	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Activation")
	EOUUNPCActivationAction ActivationAction = EOUUNPCActivationAction::MoveToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTree = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement")
	bool bUseMoveTargetActor = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement", meta = (EditCondition = "bUseMoveTargetActor", EditConditionHides))
	TObjectPtr<AActor> MoveTargetActor = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement", meta = (EditCondition = "!bUseMoveTargetActor", EditConditionHides))
	FVector MoveTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "0.1"))
	float JumpTravelTime = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement")
	bool bMoveToTargetAfterJumpLanding = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation")
	TObjectPtr<UAnimMontage> ActivationMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation", meta = (ClampMin = "0.0"))
	float ActivationMontagePlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation")
	FName ActivationMontageStartSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Activation")
	bool bActivateOnBeginPlay = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	bool bActivated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	bool bPendingMoveAfterJumpLanding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	bool bHasActiveActionRequest = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	FUOUNPCActionRequest ActiveActionRequest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
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
