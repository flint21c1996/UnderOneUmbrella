// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/BTTask_UOUJumpMoveToTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/NPC/UOUNPCCharacter.h"

UBTTask_UOUJumpMoveToTarget::UBTTask_UOUJumpMoveToTarget()
{
	NodeName = TEXT("UOU Jump Move To Target");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_UOUJumpMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	AUOUNPCCharacter* NPCCharacter = AIController != nullptr ? Cast<AUOUNPCCharacter>(AIController->GetPawn()) : nullptr;
	if (NPCCharacter == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	FUOUJumpMoveToTargetTaskMemory* Memory = reinterpret_cast<FUOUJumpMoveToTargetTaskMemory*>(NodeMemory);
	Memory->NPCCharacter = NPCCharacter;
	Memory->ElapsedTime = 0.0f;
	Memory->bObservedAirborne = false;

	if (!NPCCharacter->JumpMoveToConfiguredTarget())
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_UOUJumpMoveToTarget::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	FUOUJumpMoveToTargetTaskMemory* Memory = reinterpret_cast<FUOUJumpMoveToTargetTaskMemory*>(NodeMemory);
	Memory->ElapsedTime += DeltaSeconds;

	AUOUNPCCharacter* NPCCharacter = Memory->NPCCharacter.Get();
	if (NPCCharacter == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const UCharacterMovementComponent* MovementComponent = NPCCharacter->GetCharacterMovement();
	const bool bIsFalling = MovementComponent != nullptr && MovementComponent->IsFalling();
	Memory->bObservedAirborne |= bIsFalling;

	if ((Memory->bObservedAirborne && !bIsFalling) || Memory->ElapsedTime >= MaxJumpDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UBTTask_UOUJumpMoveToTarget::GetInstanceMemorySize() const
{
	return sizeof(FUOUJumpMoveToTargetTaskMemory);
}
