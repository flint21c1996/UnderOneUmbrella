// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_UOUMoveToTarget.generated.h"

class AUOUNPCCharacter;

struct FUOUMoveToTargetTaskMemory
{
	TWeakObjectPtr<AUOUNPCCharacter> NPCCharacter;
	float ElapsedTime = 0.0f;
	float TimeSinceLastMoveRequest = 0.0f;
	FVector RequestedTargetLocation = FVector::ZeroVector;
	bool bMoveRequestAccepted = false;
	bool bLoggedMoveFailure = false;
	bool bLoggedDirectMoveFallback = false;
};

UCLASS(meta = (DisplayName = "UOU Move To Target"))
class UBTTask_UOUMoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UOUMoveToTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "NPC|Move", meta = (ClampMin = "0.1"))
	float MaxMoveDuration = 15.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Move", meta = (ClampMin = "0.1"))
	float MoveRequestRetryInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "NPC|Move")
	bool bAllowDirectMoveFallback = true;

	UPROPERTY(EditAnywhere, Category = "NPC|Move")
	FVector TargetProjectionExtent = FVector(250.0f, 250.0f, 500.0f);

	bool RequestMove(UBehaviorTreeComponent& OwnerComp, FUOUMoveToTargetTaskMemory& Memory) const;
	float GetCompletionRadius(const AUOUNPCCharacter& NPCCharacter) const;
};
