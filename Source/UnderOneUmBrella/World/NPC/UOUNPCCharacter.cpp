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
	const bool bBehaviorTreeHandled = SyncActivationBlackboard();

	if (!bBehaviorTreeHandled)
	{
		switch (ActivationAction)
		{
		case EOUUNPCActivationAction::MoveToTarget:
			MoveToConfiguredTarget();
			break;
		case EOUUNPCActivationAction::PlayAnimation:
			PlayActivationAnimation();
			break;
		case EOUUNPCActivationAction::JumpMoveToTarget:
			JumpMoveToConfiguredTarget();
			break;
		default:
			break;
		}
	}

	ReceiveNPCActivated();
}

void AUOUNPCCharacter::Deactivate()
{
	bActivated = false;
	bPendingMoveAfterJumpLanding = false;
	SyncActivationBlackboard();
	StopNPCMovement();
	StopAnimMontage(ActivationMontage);
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
	AUOUNPCController* NPCController = GetNPCController();
	if (NPCController == nullptr)
	{
		return false;
	}

	if (bUseMoveTargetActor)
	{
		return NPCController->MoveToGoalActor(MoveTargetActor, AcceptanceRadius);
	}

	return NPCController->MoveToGoalLocation(MoveTargetLocation, AcceptanceRadius);
}

bool AUOUNPCCharacter::JumpMoveToConfiguredTarget()
{
	FVector TargetLocation;
	if (!GetConfiguredTargetLocation(TargetLocation))
	{
		return false;
	}

	if (FVector::Dist2D(GetActorLocation(), TargetLocation) <= FMath::Max(0.0f, AcceptanceRadius))
	{
		return true;
	}

	StopNPCMovement();
	bPendingMoveAfterJumpLanding = bMoveToTargetAfterJumpLanding;
	LaunchCharacter(CalculateJumpLaunchVelocity(TargetLocation), true, true);
	return true;
}

float AUOUNPCCharacter::PlayActivationAnimation()
{
	if (ActivationMontage == nullptr)
	{
		return 0.0f;
	}

	return PlayAnimMontage(ActivationMontage, ActivationMontagePlayRate, ActivationMontageStartSection);
}

void AUOUNPCCharacter::StopNPCMovement()
{
	if (AUOUNPCController* NPCController = GetNPCController())
	{
		NPCController->StopMovement();
	}
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

	FVector TargetLocation = MoveTargetLocation;
	if (bUseMoveTargetActor && MoveTargetActor != nullptr)
	{
		TargetLocation = MoveTargetActor->GetActorLocation();
	}

	return NPCController->SetActivationBlackboard(
		bActivated,
		bUseMoveTargetActor ? MoveTargetActor.Get() : nullptr,
		TargetLocation,
		static_cast<uint8>(ActivationAction));
}

bool AUOUNPCCharacter::GetConfiguredTargetLocation(FVector& OutTargetLocation) const
{
	if (bUseMoveTargetActor)
	{
		if (MoveTargetActor == nullptr)
		{
			return false;
		}

		OutTargetLocation = MoveTargetActor->GetActorLocation();
		return true;
	}

	OutTargetLocation = MoveTargetLocation;
	return true;
}

FVector AUOUNPCCharacter::CalculateJumpLaunchVelocity(const FVector& TargetLocation) const
{
	const float SafeTravelTime = FMath::Max(0.1f, JumpTravelTime);
	const FVector Delta = TargetLocation - GetActorLocation();
	const FVector HorizontalVelocity(Delta.X / SafeTravelTime, Delta.Y / SafeTravelTime, 0.0f);

	const UWorld* World = GetWorld();
	const float GravityZ = World != nullptr ? World->GetGravityZ() : -980.0f;
	const float VerticalVelocity = (Delta.Z - 0.5f * GravityZ * SafeTravelTime * SafeTravelTime) / SafeTravelTime;

	return HorizontalVelocity + FVector(0.0f, 0.0f, VerticalVelocity);
}
