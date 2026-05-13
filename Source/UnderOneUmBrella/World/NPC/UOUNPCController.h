// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "World/NPC/UOUNPCActionTypes.h"
#include "UOUNPCController.generated.h"

class UBehaviorTree;

UCLASS(Blueprintable, meta = (DisplayName = "UOU NPC Controller"))
class AUOUNPCController : public AAIController
{
	GENERATED_BODY()

public:
	AUOUNPCController();

	virtual void OnPossess(APawn* InPawn) override;

	static const FName ActivatedKeyName;
	static const FName TargetActorKeyName;
	static const FName TargetLocationKeyName;
	static const FName ActionTypeKeyName;
	static const FName SelfActorKeyName;

	UFUNCTION(BlueprintCallable, Category = "NPC|Behavior")
	bool RunAssignedBehaviorTree(UBehaviorTree* BehaviorTreeAsset);

	UFUNCTION(BlueprintCallable, Category = "NPC|Behavior")
	bool SetActivationBlackboard(bool bNewActivated, AActor* TargetActor, const FVector& TargetLocation, uint8 ActionTypeValue);

	UFUNCTION(BlueprintCallable, Category = "NPC|Behavior")
	bool SetActionBlackboard(bool bHasAction, const FUOUNPCActionRequest& ActionRequest);

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	bool MoveToGoalActor(AActor* GoalActor, float AcceptanceRadius);

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	bool MoveToGoalLocation(const FVector& GoalLocation, float AcceptanceRadius);
};
