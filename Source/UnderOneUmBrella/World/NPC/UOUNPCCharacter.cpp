// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/NPC/UOUNPCCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Debug/UOUDebugController.h"
#include "Debug/UOUDebugControllerComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "World/NPC/UOUNPCController.h"

namespace UOUNPCDebugPrivate
{
	FString GetActorDebugName(const AActor* Actor)
	{
		if (Actor == nullptr)
		{
			return TEXT("None");
		}

#if WITH_EDITOR
		return Actor->GetActorLabel();
#else
		return Actor->GetName();
#endif
	}

	FString GetActionTypeName(EUOUNPCActionType ActionType)
	{
		const UEnum* ActionTypeEnum = StaticEnum<EUOUNPCActionType>();
		return ActionTypeEnum != nullptr
			? ActionTypeEnum->GetNameStringByValue(static_cast<int64>(ActionType))
			: TEXT("Unknown");
	}

	FString GetMovementModeName(const UCharacterMovementComponent* MovementComponent)
	{
		if (MovementComponent == nullptr)
		{
			return TEXT("None");
		}

		switch (MovementComponent->MovementMode)
		{
		case MOVE_None:
			return TEXT("None");
		case MOVE_Walking:
			return TEXT("Walking");
		case MOVE_NavWalking:
			return TEXT("NavWalking");
		case MOVE_Falling:
			return TEXT("Falling");
		case MOVE_Swimming:
			return TEXT("Swimming");
		case MOVE_Flying:
			return TEXT("Flying");
		case MOVE_Custom:
			return TEXT("Custom");
		default:
			return TEXT("Unknown");
		}
	}

	FString GetPathFollowingStatusName(const UPathFollowingComponent* PathFollowingComponent)
	{
		if (PathFollowingComponent == nullptr)
		{
			return TEXT("No Controller");
		}

		switch (PathFollowingComponent->GetStatus())
		{
		case EPathFollowingStatus::Idle:
			return TEXT("Idle");
		case EPathFollowingStatus::Waiting:
			return TEXT("Waiting");
		case EPathFollowingStatus::Paused:
			return TEXT("Paused");
		case EPathFollowingStatus::Moving:
			return TEXT("Moving");
		default:
			return TEXT("Unknown");
		}
	}
}

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
	DrawNPCDebug();
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

void AUOUNPCCharacter::DrawNPCDebug()
{
	UWorld* World = GetWorld();
	if (World == nullptr
		|| !World->IsGameWorld()
		|| !UUOUDebugSubsystem::IsDebugCategoryEnabled(this, EUOUDebugCategory::NPC))
	{
		return;
	}

	UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>();
	const UUOUNPCDebugControllerComponent* NPCDebugController = DebugSubsystem != nullptr
		? Cast<UUOUNPCDebugControllerComponent>(DebugSubsystem->FindDebugControllerComponent(EUOUDebugCategory::NPC))
		: nullptr;
	if (NPCDebugController == nullptr)
	{
		return;
	}

	const AUOUDebugController* DebugController = DebugSubsystem->GetActiveDebugController();
	const FColor NPCLabelColor = DebugController != nullptr
		? DebugController->GetDebugCategoryColor(EUOUDebugCategory::NPC)
		: FColor::White;
	const FColor MoveTargetColor = DebugController != nullptr
		? DebugController->NPCMoveTargetColor
		: FColor::Green;
	const FColor PathColor = DebugController != nullptr
		? DebugController->NPCPathColor
		: FColor::Cyan;

	if (NPCDebugController->bShowWorldLabels)
	{
		const FVector LabelLocation = GetActorLocation() + FVector(0.0f, 0.0f, GetSimpleCollisionHalfHeight() + 60.0f);
		DrawDebugString(
			World,
			LabelLocation,
			BuildNPCDebugText(*NPCDebugController),
			nullptr,
			NPCLabelColor,
			0.0f,
			true,
			0.9f);
	}

	DrawNPCMoveTargetDebug(*NPCDebugController, MoveTargetColor);
	DrawNPCPathDebug(*NPCDebugController, PathColor);
}

FString AUOUNPCCharacter::BuildNPCDebugText(const UUOUNPCDebugControllerComponent& DebugController) const
{
	TArray<FString> Lines;
	Lines.Add(UOUNPCDebugPrivate::GetActorDebugName(this));

	const FUOUNPCActionRequest ActionRequest = GetCurrentActionRequest();
	if (DebugController.bShowState)
	{
		FVector TargetLocation;
		const bool bHasTargetLocation = GetTargetLocationFromActionRequest(ActionRequest, TargetLocation);
		const AUOUNPCController* NPCController = Cast<AUOUNPCController>(Controller);
		const UPathFollowingComponent* PathFollowingComponent = NPCController != nullptr
			? NPCController->GetPathFollowingComponent()
			: nullptr;

		Lines.Add(FString::Printf(TEXT("Activated: %s"), bActivated ? TEXT("Yes") : TEXT("No")));
		Lines.Add(FString::Printf(TEXT("Action Request: %s"), bHasActiveActionRequest ? TEXT("Active") : TEXT("Legacy")));
		Lines.Add(FString::Printf(TEXT("Action: %s"), *UOUNPCDebugPrivate::GetActionTypeName(ActionRequest.ActionType)));
		Lines.Add(FString::Printf(TEXT("Source: %s"), *GetNameSafe(ActiveActionSource.Get())));
		Lines.Add(FString::Printf(TEXT("Movement: %s"), *UOUNPCDebugPrivate::GetMovementModeName(GetCharacterMovement())));
		Lines.Add(FString::Printf(TEXT("Path: %s"), *UOUNPCDebugPrivate::GetPathFollowingStatusName(PathFollowingComponent)));
		Lines.Add(FString::Printf(TEXT("Pending Jump Move: %s"), bPendingMoveAfterJumpLanding ? TEXT("Yes") : TEXT("No")));

		if (ActionRequest.bUseTargetActor)
		{
			Lines.Add(FString::Printf(TEXT("Target Actor: %s"), *UOUNPCDebugPrivate::GetActorDebugName(ActionRequest.TargetActor.Get())));
		}
		else
		{
			Lines.Add(FString::Printf(TEXT("Target Location: %s"), *ActionRequest.TargetLocation.ToCompactString()));
		}

		if (bHasTargetLocation)
		{
			Lines.Add(FString::Printf(
				TEXT("Distance: %.1f / %.1f"),
				FVector::Dist2D(GetActorLocation(), TargetLocation),
				GetCurrentActionAcceptanceRadius()));
		}
	}

	if (DebugController.bShowAnimation)
	{
		const UAnimMontage* RequestedMontage = ActionRequest.AnimationMontage != nullptr
			? ActionRequest.AnimationMontage.Get()
			: ActivationMontage.Get();
		const USkeletalMeshComponent* MeshComponent = GetMesh();
		UAnimInstance* AnimInstance = MeshComponent != nullptr ? MeshComponent->GetAnimInstance() : nullptr;
		const UAnimMontage* ActiveMontage = AnimInstance != nullptr ? AnimInstance->GetCurrentActiveMontage() : nullptr;

		Lines.Add(FString::Printf(TEXT("Requested Montage: %s"), *GetNameSafe(RequestedMontage)));
		Lines.Add(FString::Printf(TEXT("Active Montage: %s"), *GetNameSafe(ActiveMontage)));
		Lines.Add(FString::Printf(TEXT("Anim Rate: %.2f"), ActionRequest.AnimationPlayRate));
		Lines.Add(FString::Printf(TEXT("Anim Section: %s"), *ActionRequest.AnimationStartSection.ToString()));
	}

	return FString::Join(Lines, LINE_TERMINATOR);
}

void AUOUNPCCharacter::DrawNPCMoveTargetDebug(const UUOUNPCDebugControllerComponent& DebugController, FColor DebugColor) const
{
	if (!DebugController.bShowMoveTarget)
	{
		return;
	}

	UWorld* World = GetWorld();
	FVector TargetLocation;
	if (World == nullptr || !GetCurrentActionTargetLocation(TargetLocation))
	{
		return;
	}

	const FVector StartLocation = GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
	const FVector EndLocation = TargetLocation + FVector(0.0f, 0.0f, 40.0f);
	const float CurrentAcceptanceRadius = FMath::Max(0.0f, GetCurrentActionAcceptanceRadius());

	DrawDebugDirectionalArrow(World, StartLocation, EndLocation, 80.0f, DebugColor, false, 0.0f, 0, 2.0f);
	DrawDebugSphere(World, TargetLocation, 24.0f, 12, DebugColor, false, 0.0f, 0, 2.0f);

	if (CurrentAcceptanceRadius > 0.0f)
	{
		DrawDebugCylinder(
			World,
			TargetLocation,
			TargetLocation + FVector(0.0f, 0.0f, 80.0f),
			CurrentAcceptanceRadius,
			24,
			DebugColor,
			false,
			0.0f,
			0,
			1.0f);
	}
}

void AUOUNPCCharacter::DrawNPCPathDebug(const UUOUNPCDebugControllerComponent& DebugController, FColor DebugColor) const
{
	if (!DebugController.bShowPath)
	{
		return;
	}

	UWorld* World = GetWorld();
	AUOUNPCController* NPCController = Cast<AUOUNPCController>(Controller);
	UPathFollowingComponent* PathFollowingComponent = NPCController != nullptr
		? NPCController->GetPathFollowingComponent()
		: nullptr;
	if (World == nullptr || PathFollowingComponent == nullptr)
	{
		return;
	}

	const FNavPathSharedPtr Path = PathFollowingComponent->GetPath();
	if (!Path.IsValid())
	{
		return;
	}

	const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();
	for (int32 Index = 0; Index < PathPoints.Num(); ++Index)
	{
		const FVector PointLocation = PathPoints[Index].Location + FVector(0.0f, 0.0f, 20.0f);
		DrawDebugSphere(World, PointLocation, 12.0f, 8, DebugColor, false, 0.0f, 0, 1.0f);

		if (Index > 0)
		{
			const FVector PreviousLocation = PathPoints[Index - 1].Location + FVector(0.0f, 0.0f, 20.0f);
			DrawDebugLine(World, PreviousLocation, PointLocation, DebugColor, false, 0.0f, 0, 2.0f);
		}
	}
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
