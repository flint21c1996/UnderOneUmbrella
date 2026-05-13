// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/UOUNPCCharacter.h"

#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/NPC/UOUNPCController.h"

AUOUNPCCharacter::AUOUNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AUOUNPCController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
		MovementComponent->MaxWalkSpeed = 300.0f;
	}
}

void AUOUNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (bActivateOnBeginPlay)
	{
		Activate();
	}
}

void AUOUNPCCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (bPendingMoveAfterJumpLanding)
	{
		bPendingMoveAfterJumpLanding = false;
		if (bActivated && bMoveToTargetAfterJumpLanding)
		{
			MoveToConfiguredTarget();
		}
	}
}

void AUOUNPCCharacter::Activate()
{
	bActivated = true;
	if (!bHasActiveActionRequest)
	{
		ActiveActionRequest = BuildLegacyActionRequest();
		ActiveActionSource = this;
		bHasActiveActionRequest = true;
	}

	const bool bBehaviorTreeHandled = SyncActivationBlackboard();

	if (!bBehaviorTreeHandled)
	{
		ExecuteCurrentActionDirectly();
	}

	ReceiveNPCActivated();
}

void AUOUNPCCharacter::Deactivate()
{
	UAnimMontage* MontageToStop = ActiveActionRequest.AnimationMontage != nullptr
		? ActiveActionRequest.AnimationMontage.Get()
		: ActivationMontage.Get();

	bActivated = false;
	bPendingMoveAfterJumpLanding = false;
	bHasActiveActionRequest = false;
	ActiveActionRequest = FUOUNPCActionRequest();
	ActiveActionSource = nullptr;
	SyncActivationBlackboard();
	StopNPCMovement();
	StopAnimMontage(MontageToStop);
	ReceiveNPCDeactivated();
}

void AUOUNPCCharacter::Toggle()
{
	if (bActivated)
	{
		Deactivate();
		return;
	}

	Activate();
}

bool AUOUNPCCharacter::MoveToConfiguredTarget()
{
	const FUOUNPCActionRequest ActionRequest = GetCurrentActionRequest();
	AUOUNPCController* NPCController = GetNPCController();
	if (NPCController == nullptr)
	{
		return false;
	}

	if (ActionRequest.bUseTargetActor)
	{
		return NPCController->MoveToGoalActor(ActionRequest.TargetActor.Get(), ActionRequest.AcceptanceRadius);
	}

	return NPCController->MoveToGoalLocation(ActionRequest.TargetLocation, ActionRequest.AcceptanceRadius);
}

bool AUOUNPCCharacter::JumpMoveToConfiguredTarget()
{
	FVector TargetLocation;
	if (!GetConfiguredTargetLocation(TargetLocation))
	{
		return false;
	}

	return JumpMoveToTargetLocation(TargetLocation);
}

bool AUOUNPCCharacter::JumpMoveToTargetLocation(const FVector& TargetLocation)
{
	const FUOUNPCActionRequest ActionRequest = GetCurrentActionRequest();
	if (FVector::Dist2D(GetActorLocation(), TargetLocation) <= FMath::Max(0.0f, ActionRequest.AcceptanceRadius))
	{
		return true;
	}

	StopNPCMovement();
	bPendingMoveAfterJumpLanding = ActionRequest.bMoveToTargetAfterJumpLanding;
	LaunchCharacter(CalculateJumpLaunchVelocity(TargetLocation, ActionRequest.JumpTravelTime), true, true);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Falling);
	}
	return true;
}

float AUOUNPCCharacter::PlayActivationAnimation()
{
	const FUOUNPCActionRequest ActionRequest = GetCurrentActionRequest();
	UAnimMontage* MontageToPlay = ActionRequest.AnimationMontage != nullptr
		? ActionRequest.AnimationMontage.Get()
		: ActivationMontage.Get();
	if (MontageToPlay == nullptr)
	{
		return 0.0f;
	}

	const float PlayRate = FMath::Max(0.0f, ActionRequest.AnimationPlayRate);
	const FName StartSection = ActionRequest.AnimationMontage != nullptr
		? ActionRequest.AnimationStartSection
		: ActivationMontageStartSection;
	return PlayAnimMontage(MontageToPlay, PlayRate, StartSection);
}

void AUOUNPCCharacter::StopNPCMovement()
{
	if (AUOUNPCController* NPCController = GetNPCController())
	{
		NPCController->StopMovement();
	}
}

bool AUOUNPCCharacter::RequestNPCAction(UObject* ActionSource, const FUOUNPCActionRequest& ActionRequest)
{
	if (ActionRequest.ActionType == EUOUNPCActionType::None)
	{
		return false;
	}

	if (bHasActiveActionRequest)
	{
		StopNPCMovement();
	}

	ActiveActionRequest = ActionRequest;
	ActiveActionSource = ActionSource;
	bHasActiveActionRequest = true;
	bActivated = true;

	const bool bBehaviorTreeHandled = SyncActivationBlackboard();
	if (!bBehaviorTreeHandled)
	{
		ExecuteCurrentActionDirectly();
	}

	ReceiveNPCActivated();
	return true;
}

bool AUOUNPCCharacter::ClearNPCAction(UObject* ActionSource)
{
	if (!bHasActiveActionRequest)
	{
		return false;
	}

	if (ActionSource != nullptr && ActiveActionSource != nullptr && ActiveActionSource.Get() != ActionSource)
	{
		return false;
	}

	Deactivate();
	return true;
}

void AUOUNPCCharacter::CompleteActiveNPCAction()
{
	if (!bHasActiveActionRequest)
	{
		return;
	}

	UObject* CompletedActionSource = ActiveActionSource.Get();
	bActivated = false;
	bPendingMoveAfterJumpLanding = false;
	bHasActiveActionRequest = false;
	ActiveActionRequest = FUOUNPCActionRequest();
	ActiveActionSource = nullptr;
	SyncActivationBlackboard();
	OnNPCActionCompleted.Broadcast(this, CompletedActionSource);
	ReceiveNPCActionCompleted(CompletedActionSource);
	ReceiveNPCDeactivated();
}

bool AUOUNPCCharacter::ShouldClearActiveActionOnFinish() const
{
	return bHasActiveActionRequest && ActiveActionRequest.bClearActionOnFinish;
}

bool AUOUNPCCharacter::GetCurrentActionTargetLocation(FVector& OutTargetLocation) const
{
	return GetTargetLocationFromActionRequest(GetCurrentActionRequest(), OutTargetLocation);
}

float AUOUNPCCharacter::GetCurrentActionAcceptanceRadius() const
{
	return FMath::Max(0.0f, GetCurrentActionRequest().AcceptanceRadius);
}

UBehaviorTree* AUOUNPCCharacter::GetBehaviorTree() const
{
	return BehaviorTree;
}

AUOUNPCController* AUOUNPCCharacter::GetNPCController()
{
	if (Controller == nullptr)
	{
		SpawnDefaultController();
	}

	return Cast<AUOUNPCController>(GetController());
}

bool AUOUNPCCharacter::SyncActivationBlackboard()
{
	AUOUNPCController* NPCController = GetNPCController();
	if (NPCController == nullptr)
	{
		return false;
	}

	return NPCController->SetActionBlackboard(bActivated && bHasActiveActionRequest, GetCurrentActionRequest());
}

void AUOUNPCCharacter::ExecuteCurrentActionDirectly()
{
	switch (GetCurrentActionRequest().ActionType)
	{
	case EUOUNPCActionType::MoveToTarget:
		MoveToConfiguredTarget();
		break;
	case EUOUNPCActionType::PlayAnimation:
		PlayActivationAnimation();
		break;
	case EUOUNPCActionType::JumpMoveToTarget:
		JumpMoveToConfiguredTarget();
		break;
	case EUOUNPCActionType::None:
	default:
		break;
	}
}

FUOUNPCActionRequest AUOUNPCCharacter::BuildLegacyActionRequest() const
{
	FUOUNPCActionRequest ActionRequest;
	switch (ActivationAction)
	{
	case EOUUNPCActivationAction::MoveToTarget:
		ActionRequest.ActionType = EUOUNPCActionType::MoveToTarget;
		break;
	case EOUUNPCActivationAction::PlayAnimation:
		ActionRequest.ActionType = EUOUNPCActionType::PlayAnimation;
		break;
	case EOUUNPCActivationAction::JumpMoveToTarget:
		ActionRequest.ActionType = EUOUNPCActionType::JumpMoveToTarget;
		break;
	default:
		ActionRequest.ActionType = EUOUNPCActionType::None;
		break;
	}

	ActionRequest.bUseTargetActor = bUseMoveTargetActor;
	ActionRequest.TargetActor = MoveTargetActor;
	ActionRequest.TargetLocation = MoveTargetLocation;
	ActionRequest.AcceptanceRadius = AcceptanceRadius;
	ActionRequest.JumpTravelTime = JumpTravelTime;
	ActionRequest.bMoveToTargetAfterJumpLanding = bMoveToTargetAfterJumpLanding;
	ActionRequest.AnimationMontage = ActivationMontage;
	ActionRequest.AnimationPlayRate = ActivationMontagePlayRate;
	ActionRequest.AnimationStartSection = ActivationMontageStartSection;
	return ActionRequest;
}

FUOUNPCActionRequest AUOUNPCCharacter::GetCurrentActionRequest() const
{
	return bHasActiveActionRequest ? ActiveActionRequest : BuildLegacyActionRequest();
}

bool AUOUNPCCharacter::GetTargetLocationFromActionRequest(
	const FUOUNPCActionRequest& ActionRequest,
	FVector& OutTargetLocation) const
{
	if (ActionRequest.bUseTargetActor)
	{
		if (ActionRequest.TargetActor == nullptr)
		{
			return false;
		}

		OutTargetLocation = ActionRequest.TargetActor->GetActorLocation();
		return true;
	}

	OutTargetLocation = ActionRequest.TargetLocation;
	return true;
}

bool AUOUNPCCharacter::GetConfiguredTargetLocation(FVector& OutTargetLocation) const
{
	return GetTargetLocationFromActionRequest(GetCurrentActionRequest(), OutTargetLocation);
}

FVector AUOUNPCCharacter::CalculateJumpLaunchVelocity(const FVector& TargetLocation, float TravelTime) const
{
	const float SafeTravelTime = FMath::Max(0.1f, TravelTime);
	const FVector Delta = TargetLocation - GetActorLocation();
	const FVector HorizontalVelocity(Delta.X / SafeTravelTime, Delta.Y / SafeTravelTime, 0.0f);

	const UWorld* World = GetWorld();
	const float GravityZ = World != nullptr ? World->GetGravityZ() : -980.0f;
	const float VerticalVelocity = (Delta.Z - 0.5f * GravityZ * SafeTravelTime * SafeTravelTime) / SafeTravelTime;

	return HorizontalVelocity + FVector(0.0f, 0.0f, VerticalVelocity);
}
