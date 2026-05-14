// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_UOUJumpMoveToTarget.generated.h"

class AUOUNPCCharacter;

struct FUOUJumpMoveToTargetTaskMemory
{
	TWeakObjectPtr<AUOUNPCCharacter> NPCCharacter;
	FVector TargetLocation = FVector::ZeroVector;
	float AcceptanceRadius = 50.0f;
	float ElapsedTime = 0.0f;
	bool bObservedAirborne = false;
};

// 현재 액션 타겟을 향해 NPC를 점프 이동시키는 Behavior Tree 태스크입니다.
UCLASS(meta = (DisplayName = "UOU Jump Move To Target", ToolTip = "현재 액션 타겟을 향해 NPC를 점프 이동시킵니다."))
class UBTTask_UOUJumpMoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UOUJumpMoveToTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "NPC|Jump", meta = (ClampMin = "0.1", ToolTip = "점프 완료로 처리하기 전까지 기다릴 최대 시간입니다."))
	float MaxJumpDuration = 2.0f;

	bool ResolveBlackboardTargetLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutTargetLocation) const;
};
