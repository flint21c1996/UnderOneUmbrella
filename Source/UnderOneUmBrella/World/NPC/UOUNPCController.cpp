// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/UOUNPCController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "World/NPC/UOUNPCCharacter.h"

const FName AUOUNPCController::ActivatedKeyName(TEXT("bActivated"));
const FName AUOUNPCController::TargetActorKeyName(TEXT("TargetActor"));
const FName AUOUNPCController::TargetLocationKeyName(TEXT("TargetLocation"));
const FName AUOUNPCController::ActionTypeKeyName(TEXT("ActionType"));
const FName AUOUNPCController::SelfActorKeyName(TEXT("SelfActor"));

AUOUNPCController::AUOUNPCController()
{
	bAttachToPawn = true;
}

void AUOUNPCController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AUOUNPCCharacter* NPCCharacter = Cast<AUOUNPCCharacter>(InPawn))
	{
		RunAssignedBehaviorTree(NPCCharacter->GetBehaviorTree());

		FVector TargetLocation = NPCCharacter->MoveTargetLocation;
		if (NPCCharacter->bUseMoveTargetActor && NPCCharacter->MoveTargetActor != nullptr)
		{
			TargetLocation = NPCCharacter->MoveTargetActor->GetActorLocation();
		}

		SetActivationBlackboard(
			NPCCharacter->bActivated,
			NPCCharacter->bUseMoveTargetActor ? NPCCharacter->MoveTargetActor.Get() : nullptr,
			TargetLocation,
			static_cast<uint8>(NPCCharacter->ActivationAction));
	}
}

bool AUOUNPCController::RunAssignedBehaviorTree(UBehaviorTree* BehaviorTreeAsset)
{
	if (BehaviorTreeAsset == nullptr)
	{
		return false;
	}

	return RunBehaviorTree(BehaviorTreeAsset);
}

bool AUOUNPCController::SetActivationBlackboard(
	bool bNewActivated,
	AActor* TargetActor,
	const FVector& TargetLocation,
	uint8 ActionTypeValue)
{
	if (GetBlackboardComponent() == nullptr)
	{
		const AUOUNPCCharacter* NPCCharacter = Cast<AUOUNPCCharacter>(GetPawn());
		if (NPCCharacter == nullptr || !RunAssignedBehaviorTree(NPCCharacter->GetBehaviorTree()))
		{
			return false;
		}
	}

	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (BlackboardComponent == nullptr)
	{
		return false;
	}

	BlackboardComponent->SetValueAsObject(SelfActorKeyName, GetPawn());
	BlackboardComponent->SetValueAsBool(ActivatedKeyName, bNewActivated);

	if (!bNewActivated)
	{
		BlackboardComponent->ClearValue(TargetActorKeyName);
		BlackboardComponent->ClearValue(TargetLocationKeyName);
		BlackboardComponent->ClearValue(ActionTypeKeyName);
		return true;
	}

	if (TargetActor != nullptr)
	{
		BlackboardComponent->SetValueAsObject(TargetActorKeyName, TargetActor);
	}
	else
	{
		BlackboardComponent->ClearValue(TargetActorKeyName);
	}

	BlackboardComponent->SetValueAsVector(TargetLocationKeyName, TargetLocation);
	BlackboardComponent->SetValueAsEnum(ActionTypeKeyName, ActionTypeValue);
	return true;
}

bool AUOUNPCController::SetActionBlackboard(bool bHasAction, const FUOUNPCActionRequest& ActionRequest)
{
	FVector TargetLocation = ActionRequest.TargetLocation;
	AActor* TargetActor = nullptr;
	if (ActionRequest.bUseTargetActor && ActionRequest.TargetActor != nullptr)
	{
		TargetActor = ActionRequest.TargetActor.Get();
		TargetLocation = TargetActor->GetActorLocation();
	}

	return SetActivationBlackboard(
		bHasAction,
		TargetActor,
		TargetLocation,
		static_cast<uint8>(ActionRequest.ActionType));
}

bool AUOUNPCController::MoveToGoalActor(AActor* GoalActor, float AcceptanceRadius)
{
	if (GoalActor == nullptr)
	{
		return false;
	}

	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(
		GoalActor,
		FMath::Max(0.0f, AcceptanceRadius),
		true,
		true,
		true,
		nullptr,
		true);

	return MoveResult == EPathFollowingRequestResult::RequestSuccessful ||
		MoveResult == EPathFollowingRequestResult::AlreadyAtGoal;
}

bool AUOUNPCController::MoveToGoalLocation(const FVector& GoalLocation, float AcceptanceRadius)
{
	const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(
		GoalLocation,
		FMath::Max(0.0f, AcceptanceRadius),
		true,
		true,
		true,
		true,
		nullptr,
		true);

	return MoveResult == EPathFollowingRequestResult::RequestSuccessful ||
		MoveResult == EPathFollowingRequestResult::AlreadyAtGoal;
}
