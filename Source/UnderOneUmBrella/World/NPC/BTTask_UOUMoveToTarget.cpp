// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/BTTask_UOUMoveToTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "World/NPC/UOUNPCCharacter.h"

namespace
{
bool ProjectLocationToNavigation(
	const AAIController* AIController,
	const FVector& SourceLocation,
	const FVector& ProjectionExtent,
	FVector& OutLocation)
{
	OutLocation = SourceLocation;

	UWorld* World = AIController != nullptr ? AIController->GetWorld() : nullptr;
	if (World == nullptr)
	{
		return false;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (NavigationSystem == nullptr)
	{
		return false;
	}

	FNavLocation ProjectedLocation;
	if (!NavigationSystem->ProjectPointToNavigation(SourceLocation, ProjectedLocation, ProjectionExtent))
	{
		return false;
	}

	OutLocation = ProjectedLocation.Location;
	return true;
}

const TCHAR* MoveRequestResultToString(EPathFollowingRequestResult::Type MoveResult)
{
	switch (MoveResult)
	{
	case EPathFollowingRequestResult::AlreadyAtGoal:
		return TEXT("AlreadyAtGoal");
	case EPathFollowingRequestResult::RequestSuccessful:
		return TEXT("RequestSuccessful");
	case EPathFollowingRequestResult::Failed:
	default:
		return TEXT("Failed");
	}
}
}

UBTTask_UOUMoveToTarget::UBTTask_UOUMoveToTarget()
{
	NodeName = TEXT("UOU Move To Target");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_UOUMoveToTarget::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	AUOUNPCCharacter* NPCCharacter = AIController != nullptr ? Cast<AUOUNPCCharacter>(AIController->GetPawn()) : nullptr;
	if (NPCCharacter == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	FUOUMoveToTargetTaskMemory* Memory = reinterpret_cast<FUOUMoveToTargetTaskMemory*>(NodeMemory);
	Memory->NPCCharacter = NPCCharacter;
	Memory->ElapsedTime = 0.0f;
	Memory->TimeSinceLastMoveRequest = 0.0f;
	Memory->RequestedTargetLocation = NPCCharacter->GetActorLocation();
	Memory->bMoveRequestAccepted = false;
	Memory->bLoggedMoveFailure = false;
	Memory->bLoggedDirectMoveFallback = false;

	RequestMove(OwnerComp, *Memory);
	return EBTNodeResult::InProgress;
}

void UBTTask_UOUMoveToTarget::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	FUOUMoveToTargetTaskMemory* Memory = reinterpret_cast<FUOUMoveToTargetTaskMemory*>(NodeMemory);
	Memory->ElapsedTime += DeltaSeconds;

	AUOUNPCCharacter* NPCCharacter = Memory->NPCCharacter.Get();
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (NPCCharacter == nullptr || AIController == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector RawTargetLocation;
	if (!NPCCharacter->GetCurrentActionTargetLocation(RawTargetLocation))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector ProjectedTargetLocation;
	ProjectLocationToNavigation(
		AIController,
		RawTargetLocation,
		TargetProjectionExtent,
		ProjectedTargetLocation);
	Memory->RequestedTargetLocation = ProjectedTargetLocation;

	const bool bReachedTarget =
		FVector::Dist2D(NPCCharacter->GetActorLocation(), Memory->RequestedTargetLocation) <=
		GetCompletionRadius(*NPCCharacter);
	const bool bTimedOut = Memory->ElapsedTime >= MaxMoveDuration;
	if (!bReachedTarget && !bTimedOut)
	{
		if (!Memory->bMoveRequestAccepted)
		{
			Memory->TimeSinceLastMoveRequest += DeltaSeconds;
			if (Memory->TimeSinceLastMoveRequest >= MoveRequestRetryInterval)
			{
				RequestMove(OwnerComp, *Memory);
			}
		}

		if (Memory->bMoveRequestAccepted && Memory->ElapsedTime > MoveRequestRetryInterval &&
			AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			Memory->bMoveRequestAccepted = false;
			Memory->TimeSinceLastMoveRequest = MoveRequestRetryInterval;
		}

		return;
	}

	FinishLatentTask(OwnerComp, bReachedTarget ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	if (bReachedTarget && NPCCharacter->ShouldClearActiveActionOnFinish())
	{
		NPCCharacter->CompleteActiveNPCAction();
	}
}

uint16 UBTTask_UOUMoveToTarget::GetInstanceMemorySize() const
{
	return sizeof(FUOUMoveToTargetTaskMemory);
}

bool UBTTask_UOUMoveToTarget::RequestMove(
	UBehaviorTreeComponent& OwnerComp,
	FUOUMoveToTargetTaskMemory& Memory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	AUOUNPCCharacter* NPCCharacter = Memory.NPCCharacter.Get();
	if (AIController == nullptr || NPCCharacter == nullptr)
	{
		return false;
	}

	Memory.TimeSinceLastMoveRequest = 0.0f;

	FVector RawTargetLocation;
	if (!NPCCharacter->GetCurrentActionTargetLocation(RawTargetLocation))
	{
		return false;
	}

	FVector ProjectedStartLocation;
	const bool bProjectedStart = ProjectLocationToNavigation(
		AIController,
		NPCCharacter->GetActorLocation(),
		TargetProjectionExtent,
		ProjectedStartLocation);

	FVector ProjectedTargetLocation;
	const bool bProjectedTarget = ProjectLocationToNavigation(
		AIController,
		RawTargetLocation,
		TargetProjectionExtent,
		ProjectedTargetLocation);
	Memory.RequestedTargetLocation = ProjectedTargetLocation;

	const float AcceptanceRadius = NPCCharacter->GetCurrentActionAcceptanceRadius();
	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		ProjectedTargetLocation,
		AcceptanceRadius,
		true,
		true,
		false,
		true,
		nullptr,
		true);

	EPathFollowingRequestResult::Type DirectMoveResult = EPathFollowingRequestResult::Failed;
	if (MoveResult == EPathFollowingRequestResult::Failed && bAllowDirectMoveFallback)
	{
		DirectMoveResult = AIController->MoveToLocation(
			ProjectedTargetLocation,
			AcceptanceRadius,
			true,
			false,
			false,
			true,
			nullptr,
			true);
		MoveResult = DirectMoveResult;

		if (DirectMoveResult != EPathFollowingRequestResult::Failed && !Memory.bLoggedDirectMoveFallback)
		{
			Memory.bLoggedDirectMoveFallback = true;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("UOU Move To Target is using direct fallback. NPC=%s Target=%s AcceptanceRadius=%.1f"),
				*GetNameSafe(NPCCharacter),
				*ProjectedTargetLocation.ToCompactString(),
				AcceptanceRadius);
		}
	}

	Memory.bMoveRequestAccepted = MoveResult != EPathFollowingRequestResult::Failed;
	if (!Memory.bMoveRequestAccepted && !Memory.bLoggedMoveFailure)
	{
		Memory.bLoggedMoveFailure = true;
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UOU Move To Target failed. NPC=%s Start=%s ProjectedStart=%s bStartProjected=%s RawTarget=%s ProjectedTarget=%s bTargetProjected=%s AcceptanceRadius=%.1f PathResult=%s DirectResult=%s"),
			*GetNameSafe(NPCCharacter),
			*NPCCharacter->GetActorLocation().ToCompactString(),
			*ProjectedStartLocation.ToCompactString(),
			bProjectedStart ? TEXT("true") : TEXT("false"),
			*RawTargetLocation.ToCompactString(),
			*ProjectedTargetLocation.ToCompactString(),
			bProjectedTarget ? TEXT("true") : TEXT("false"),
			AcceptanceRadius,
			MoveRequestResultToString(MoveResult),
			MoveRequestResultToString(DirectMoveResult));
	}

	return Memory.bMoveRequestAccepted;
}

float UBTTask_UOUMoveToTarget::GetCompletionRadius(const AUOUNPCCharacter& NPCCharacter) const
{
	const UCapsuleComponent* CapsuleComponent = NPCCharacter.GetCapsuleComponent();
	const float CapsuleRadius = CapsuleComponent != nullptr ? CapsuleComponent->GetScaledCapsuleRadius() : 0.0f;
	return NPCCharacter.GetCurrentActionAcceptanceRadius() + CapsuleRadius + 5.0f;
}
