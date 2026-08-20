// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/BTTask_UOUPlayNPCAnimation.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "World/NPC/UOUNPCCharacter.h"

UBTTask_UOUPlayNPCAnimation::UBTTask_UOUPlayNPCAnimation()
{
	NodeName = TEXT("UOU Play NPC Animation");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_UOUPlayNPCAnimation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	AUOUNPCCharacter* NPCCharacter = AIController != nullptr ? Cast<AUOUNPCCharacter>(AIController->GetPawn()) : nullptr;
	if (NPCCharacter == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	const float Duration = NPCCharacter->PlayActivationAnimation();
	if (Duration <= 0.0f)
	{
		return EBTNodeResult::Failed;
	}

	const bool bWaitUntilDeactivated = NPCCharacter->IsCurrentAnimationLoopingUntilDeactivated();
	if (!bWaitForCompletion && !bWaitUntilDeactivated)
	{
		if (bClearActionOnFinish || NPCCharacter->ShouldClearActiveActionOnFinish())
		{
			NPCCharacter->CompleteActiveNPCAction();
		}
		return EBTNodeResult::Succeeded;
	}

	FUOUPlayNPCAnimationTaskMemory* Memory = reinterpret_cast<FUOUPlayNPCAnimationTaskMemory*>(NodeMemory);
	Memory->NPCCharacter = NPCCharacter;
	Memory->ElapsedTime = 0.0f;
	Memory->Duration = Duration;
	Memory->bWaitUntilDeactivated = bWaitUntilDeactivated;
	return EBTNodeResult::InProgress;
}

void UBTTask_UOUPlayNPCAnimation::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	FUOUPlayNPCAnimationTaskMemory* Memory = reinterpret_cast<FUOUPlayNPCAnimationTaskMemory*>(NodeMemory);
	if (Memory->bWaitUntilDeactivated)
	{
		return;
	}

	Memory->ElapsedTime += DeltaSeconds;

	if (Memory->ElapsedTime < Memory->Duration)
	{
		return;
	}

	AUOUNPCCharacter* NPCCharacter = Memory->NPCCharacter.Get();
	const bool bShouldCompleteAction = bClearActionOnFinish ||
		(NPCCharacter != nullptr && NPCCharacter->ShouldClearActiveActionOnFinish());
	FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	if (bShouldCompleteAction && NPCCharacter != nullptr)
	{
		NPCCharacter->CompleteActiveNPCAction();
	}
}

uint16 UBTTask_UOUPlayNPCAnimation::GetInstanceMemorySize() const
{
	return sizeof(FUOUPlayNPCAnimationTaskMemory);
}
