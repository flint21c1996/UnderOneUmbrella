// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_UOUPlayNPCAnimation.generated.h"

class AUOUNPCCharacter;

struct FUOUPlayNPCAnimationTaskMemory
{
	TWeakObjectPtr<AUOUNPCCharacter> NPCCharacter;
	float ElapsedTime = 0.0f;
	float Duration = 0.0f;
};

UCLASS(meta = (DisplayName = "UOU Play NPC Animation"))
class UBTTask_UOUPlayNPCAnimation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UOUPlayNPCAnimation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "NPC|Animation")
	bool bWaitForCompletion = true;

	UPROPERTY(EditAnywhere, Category = "NPC|Animation")
	bool bClearActionOnFinish = false;
};
