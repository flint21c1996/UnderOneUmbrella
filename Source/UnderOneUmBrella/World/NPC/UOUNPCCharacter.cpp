// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/UOUNPCCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/NPC/UOUNPCController.h"

AUOUNPCCharacter::AUOUNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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

void AUOUNPCCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateJumpMoveRotation(DeltaSeconds);
}

void AUOUNPCCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	bJumpMoveRotationActive = false;

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
	SetActivationResultCompleted(false);
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
	SetActivationResultCompleted(false);
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

void AUOUNPCCharacter::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		Activate();
		break;
	case EOUUPuzzleResultAction::Deactivate:
		Deactivate();
		break;
	case EOUUPuzzleResultAction::Pause:
		StopNPCMovement();
		break;
	case EOUUPuzzleResultAction::Resume:
		if (bActivated)
		{
			const bool bBehaviorTreeHandled = SyncActivationBlackboard();
			if (!bBehaviorTreeHandled)
			{
				ExecuteCurrentActionDirectly();
			}
		}
		break;
	case EOUUPuzzleResultAction::Toggle:
		Toggle();
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

bool AUOUNPCCharacter::IsPuzzleResultCompleted_Implementation(EOUUPuzzleResultAction Action) const
{
	return Action == EOUUPuzzleResultAction::Activate && bHasCompletedResultSinceLastActivation;
}

FOnUOUPuzzleResultCompletionStateChangedNativeSignature*
AUOUNPCCharacter::GetPuzzleResultCompletionStateChangedEvent()
{
	return &OnPuzzleResultCompletionStateChanged;
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
	bJumpMoveRotationActive = bOrientRotationDuringJumpMove;
	JumpMoveRotationTargetLocation = TargetLocation;
	LaunchCharacter(CalculateJumpLaunchVelocity(TargetLocation, ActionRequest.JumpTravelTime), true, true);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// 낙하 상태 갱신이 다음 프레임으로 밀려도 점프 태스크 완료 판정이 안정적으로 동작하게 합니다.
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
	bJumpMoveRotationActive = false;

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
	SetActivationResultCompleted(false);

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
	SetActivationResultCompleted(true);
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

void AUOUNPCCharacter::SetActivationResultCompleted(bool bNewCompleted)
{
	if (bHasCompletedResultSinceLastActivation == bNewCompleted)
	{
		return;
	}

	bHasCompletedResultSinceLastActivation = bNewCompleted;
	OnPuzzleResultCompletionStateChanged.Broadcast(EOUUPuzzleResultAction::Activate, bNewCompleted);
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
	ActionRequest.bClearActionOnFinish = bCompleteLegacyActivationOnFinish;
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

void AUOUNPCCharacter::UpdateJumpMoveRotation(float DeltaSeconds)
{
	if (!bJumpMoveRotationActive || !bOrientRotationDuringJumpMove)
	{
		return;
	}

	FVector DesiredDirection = JumpMoveRotationTargetLocation - GetActorLocation();
	DesiredDirection.Z = 0.0f;

	if (DesiredDirection.IsNearlyZero())
	{
		DesiredDirection = GetVelocity();
		DesiredDirection.Z = 0.0f;
	}

	if (DesiredDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator DesiredRotation(0.0f, DesiredDirection.Rotation().Yaw, 0.0f);
	const float SafeRotationRate = FMath::Max(0.0f, JumpMoveRotationRate);
	const FRotator NextRotation = SafeRotationRate <= KINDA_SMALL_NUMBER
		? DesiredRotation
		: FMath::RInterpConstantTo(
			FRotator(0.0f, CurrentRotation.Yaw, 0.0f),
			DesiredRotation,
			DeltaSeconds,
			SafeRotationRate);

	SetActorRotation(NextRotation);
}
