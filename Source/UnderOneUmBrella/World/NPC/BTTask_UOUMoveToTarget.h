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

// 현재 액션 타겟으로 NPC를 이동시키는 Behavior Tree 태스크입니다.
UCLASS(meta = (DisplayName = "UOU Move To Target", ToolTip = "현재 액션 타겟으로 NPC를 이동시키고 충분히 가까워지면 완료합니다."))
class UBTTask_UOUMoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UOUMoveToTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "NPC|Move", meta = (ClampMin = "0.1", ToolTip = "이동 태스크가 실패로 처리되기 전까지 기다릴 최대 시간입니다."))
	float MaxMoveDuration = 15.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Move", meta = (ClampMin = "0.1", ToolTip = "이동 요청이 받아들여지지 않았을 때 재시도하는 간격입니다."))
	float MoveRequestRetryInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "NPC|Move", meta = (ToolTip = "길찾기가 실패하면 직접 이동 요청을 시도합니다. 간단한 테스트 맵이나 일부만 깔린 NavMesh에서 유용합니다."))
	bool bAllowDirectMoveFallback = true;

	UPROPERTY(EditAnywhere, Category = "NPC|Move", meta = (ToolTip = "타겟 위치를 NavMesh에 투영할 때 사용하는 탐색 범위입니다. 타겟 액터가 이동 가능한 표면보다 위나 아래에 있으면 값을 키웁니다."))
	FVector TargetProjectionExtent = FVector(250.0f, 250.0f, 500.0f);

	bool RequestMove(UBehaviorTreeComponent& OwnerComp, FUOUMoveToTargetTaskMemory& Memory) const;
	float GetCompletionRadius(const AUOUNPCCharacter& NPCCharacter) const;
};
