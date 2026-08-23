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
	bool bWaitUntilDeactivated = false;
};

// 현재 NPC 액션에 설정된 애니메이션을 재생하는 Behavior Tree 태스크입니다.
UCLASS(meta = (DisplayName = "UOU Play NPC Animation", ToolTip = "현재 NPC 액션에 설정된 애니메이션을 재생합니다."))
class UBTTask_UOUPlayNPCAnimation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UOUPlayNPCAnimation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "NPC|Animation", meta = (ToolTip = "몽타주 재생 시간이 끝날 때까지 태스크를 실행 상태로 유지합니다."))
	bool bWaitForCompletion = true;

	UPROPERTY(EditAnywhere, Category = "NPC|Animation", meta = (ToolTip = "태스크가 끝날 때 현재 NPC 액션을 비웁니다. 시퀀스 요청은 자동으로 정리됩니다."))
	bool bClearActionOnFinish = false;
};
