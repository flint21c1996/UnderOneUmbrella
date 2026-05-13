// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/BTTask_UOUJumpMoveToTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/NPC/UOUNPCCharacter.h"
#include "World/NPC/UOUNPCController.h"

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
	Memory->TargetLocation = NPCCharacter->GetActorLocation();
	Memory->AcceptanceRadius = NPCCharacter->GetCurrentActionAcceptanceRadius();
	Memory->ElapsedTime = 0.0f;
	Memory->bObservedAirborne = false;

	FVector TargetLocation = FVector::ZeroVector;
	if (!NPCCharacter->GetCurrentActionTargetLocation(TargetLocation) &&
		!ResolveBlackboardTargetLocation(OwnerComp, TargetLocation))
	{
		return EBTNodeResult::Failed;
	}

	Memory->TargetLocation = TargetLocation;
	if (FVector::Dist2D(NPCCharacter->GetActorLocation(), TargetLocation) <= Memory->AcceptanceRadius)
	{
		const bool bShouldCompleteAction = NPCCharacter->ShouldClearActiveActionOnFinish();
		if (bShouldCompleteAction)
		{
			NPCCharacter->CompleteActiveNPCAction();
		}
		return EBTNodeResult::Succeeded;
	}

	if (!NPCCharacter->JumpMoveToTargetLocation(TargetLocation))
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

	const bool bReachedTarget =
		FVector::Dist2D(NPCCharacter->GetActorLocation(), Memory->TargetLocation) <= Memory->AcceptanceRadius;
	if ((Memory->bObservedAirborne && !bIsFalling) || bReachedTarget || Memory->ElapsedTime >= MaxJumpDuration)
	{
		const bool bShouldCompleteAction = NPCCharacter->ShouldClearActiveActionOnFinish();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		if (bShouldCompleteAction)
		{
			NPCCharacter->CompleteActiveNPCAction();
		}
	}
}

uint16 UBTTask_UOUJumpMoveToTarget::GetInstanceMemorySize() const
{
	return sizeof(FUOUJumpMoveToTargetTaskMemory);
}

bool UBTTask_UOUJumpMoveToTarget::ResolveBlackboardTargetLocation(
	UBehaviorTreeComponent& OwnerComp,
	FVector& OutTargetLocation) const
{
	const UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (BlackboardComponent == nullptr)
	{
		return false;
	}

	if (AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(AUOUNPCController::TargetActorKeyName)))
	{
		OutTargetLocation = TargetActor->GetActorLocation();
		return true;
	}

	if (BlackboardComponent->IsVectorValueSet(AUOUNPCController::TargetLocationKeyName))
	{
		OutTargetLocation = BlackboardComponent->GetValueAsVector(AUOUNPCController::TargetLocationKeyName);
		return true;
	}

	return false;
}
