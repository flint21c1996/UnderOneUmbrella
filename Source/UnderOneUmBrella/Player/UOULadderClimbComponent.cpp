// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOULadderClimbComponent.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/UOUCharacter.h"
#include "World/Traversal/UOULadderActor.h"

namespace
{
	constexpr float LadderInputDeadZone = 0.1f;
}

UUOULadderClimbComponent::UUOULadderClimbComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOULadderClimbComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AUOUCharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (UCapsuleComponent* CapsuleComponent = OwnerCharacter->GetCapsuleComponent())
	{
		CapsuleComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &UUOULadderClimbComponent::HandleCapsuleBeginOverlap);
		CapsuleComponent->OnComponentEndOverlap.AddUniqueDynamic(this, &UUOULadderClimbComponent::HandleCapsuleEndOverlap);

		TArray<AActor*> InitialOverlaps;
		CapsuleComponent->GetOverlappingActors(InitialOverlaps, AUOULadderActor::StaticClass());
		for (AActor* OverlappingActor : InitialOverlaps)
		{
			if (AUOULadderActor* Ladder = Cast<AUOULadderActor>(OverlappingActor))
			{
				NearbyLadders.AddUnique(Ladder);
			}
		}
	}

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		// Ladder velocity must be prepared before CharacterMovement consumes it.
		MovementComponent->AddTickPrerequisiteComponent(this);
	}
}

void UUOULadderClimbComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerCharacter != nullptr)
	{
		if (UCapsuleComponent* CapsuleComponent = OwnerCharacter->GetCapsuleComponent())
		{
			CapsuleComponent->OnComponentBeginOverlap.RemoveDynamic(this, &UUOULadderClimbComponent::HandleCapsuleBeginOverlap);
			CapsuleComponent->OnComponentEndOverlap.RemoveDynamic(this, &UUOULadderClimbComponent::HandleCapsuleEndOverlap);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UUOULadderClimbComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	NearbyLadders.RemoveAll([](const TObjectPtr<AUOULadderActor>& Ladder)
	{
		return !IsValid(Ladder.Get());
	});

	if (!IsClimbing())
	{
		return;
	}

	if (!IsValid(OwnerCharacter) || !IsValid(CurrentLadder))
	{
		RestoreMovementMode(true);
		CurrentLadder = nullptr;
		SetClimbState(EUOULadderClimbState::None);
		return;
	}

	switch (ClimbState)
	{
	case EUOULadderClimbState::EnteringFromBottom:
	case EUOULadderClimbState::EnteringFromTop:
	case EUOULadderClimbState::ExitingAtBottom:
	case EUOULadderClimbState::ExitingAtTop:
		UpdateTransition(DeltaTime);
		break;

	case EUOULadderClimbState::Climbing:
		UpdateClimbing(DeltaTime);
		break;

	default:
		break;
	}
}

bool UUOULadderClimbComponent::HandleMoveInput(const FVector2D& MovementVector, float MovementYaw)
{
	const float InputMagnitude = FMath::Clamp(MovementVector.Size(), 0.0f, 1.0f);

	if (IsClimbing())
	{
		if (ClimbState == EUOULadderClimbState::EnteringFromTop || bWaitForTopEntryInputRelease)
		{
			if (InputMagnitude <= LadderInputDeadZone)
			{
				bWaitForTopEntryInputRelease = false;
				CurrentClimbInput = 0.0f;
			}
			else
			{
				CurrentClimbInput = -InputMagnitude;
			}
		}
		else if (ClimbState == EUOULadderClimbState::EnteringFromBottom)
		{
			// Entry alignment is automatic, but climbing itself must follow the live W/S input.
			CurrentClimbInput = FMath::Clamp(MovementVector.Y, -1.0f, 1.0f);
		}
		else if (ClimbState == EUOULadderClimbState::Climbing)
		{
			CurrentClimbInput = FMath::Clamp(MovementVector.Y, -1.0f, 1.0f);
		}

		return true;
	}

	if (OwnerCharacter == nullptr || InputMagnitude <= LadderInputDeadZone)
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (MovementComponent == nullptr || MovementComponent->IsFalling() || MovementComponent->IsSwimming())
	{
		return false;
	}

	const FRotator YawRotation(0.0f, MovementYaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector DesiredWorldDirection =
		(ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X).GetSafeNormal();

	bool bEnterFromTop = false;
	if (AUOULadderActor* EntryLadder = FindEntryLadder(DesiredWorldDirection, bEnterFromTop))
	{
		BeginClimbing(EntryLadder, bEnterFromTop, InputMagnitude);
		return true;
	}

	return false;
}

bool UUOULadderClimbComponent::HandleJumpInput()
{
	if (!IsClimbing())
	{
		return false;
	}

	JumpOffLadder();
	return true;
}

void UUOULadderClimbComponent::HandleCapsuleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (AUOULadderActor* Ladder = Cast<AUOULadderActor>(OtherActor))
	{
		if (Ladder->IsEntryVolume(OtherComp))
		{
			NearbyLadders.AddUnique(Ladder);
		}
	}
}

void UUOULadderClimbComponent::HandleCapsuleEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (AUOULadderActor* Ladder = Cast<AUOULadderActor>(OtherActor))
	{
		if (Ladder->IsEntryVolume(OtherComp))
		{
			const UCapsuleComponent* CapsuleComponent = OwnerCharacter != nullptr
				? OwnerCharacter->GetCapsuleComponent()
				: nullptr;
			const bool bStillOverlappingEntryVolume = CapsuleComponent != nullptr &&
				(CapsuleComponent->IsOverlappingComponent(Ladder->GetBottomEntryVolume()) ||
				 CapsuleComponent->IsOverlappingComponent(Ladder->GetTopEntryVolume()));
			if (!bStillOverlappingEntryVolume)
			{
				NearbyLadders.Remove(Ladder);
			}
		}
	}
}

AUOULadderActor* UUOULadderClimbComponent::FindEntryLadder(
	const FVector& DesiredWorldDirection,
	bool& bOutEnterFromTop) const
{
	bOutEnterFromTop = false;
	AUOULadderActor* BestLadder = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (AUOULadderActor* Ladder : NearbyLadders)
	{
		if (!IsValid(Ladder))
		{
			continue;
		}

		if (Ladder->GetTopClimbHeight() <= Ladder->GetBottomClimbHeight() + 1.0f)
		{
			continue;
		}

		const UCapsuleComponent* CapsuleComponent = OwnerCharacter->GetCapsuleComponent();
		const bool bNearBottom = CapsuleComponent != nullptr &&
			CapsuleComponent->IsOverlappingComponent(Ladder->GetBottomEntryVolume());
		const bool bNearTop = CapsuleComponent != nullptr &&
			CapsuleComponent->IsOverlappingComponent(Ladder->GetTopEntryVolume());
		if (!bNearBottom && !bNearTop)
		{
			continue;
		}

		const float BottomDistanceSquared = FVector::DistSquared(
			OwnerCharacter->GetActorLocation(),
			Ladder->GetBottomStandingLocation());
		const float TopDistanceSquared = FVector::DistSquared(
			OwnerCharacter->GetActorLocation(),
			Ladder->GetTopStandingLocation());
		const bool bUseTopEntry = bNearTop && (!bNearBottom || TopDistanceSquared < BottomDistanceSquared);
		const FVector RequiredEntryDirection = bUseTopEntry
			? Ladder->GetTopEntryDirection()
			: Ladder->GetBottomEntryDirection();
		if (FVector::DotProduct(DesiredWorldDirection, RequiredEntryDirection) < EntryDirectionDotThreshold)
		{
			continue;
		}

		const FVector EntryLocation = bUseTopEntry
			? Ladder->GetTopClimbLocation()
			: Ladder->GetBottomClimbLocation();
		const float DistanceSquared = FVector::DistSquared(OwnerCharacter->GetActorLocation(), EntryLocation);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestLadder = Ladder;
			bOutEnterFromTop = bUseTopEntry;
		}
	}

	return BestLadder;
}

void UUOULadderClimbComponent::BeginClimbing(
	AUOULadderActor* Ladder,
	bool bEnterFromTop,
	float InputMagnitude)
{
	if (OwnerCharacter == nullptr || Ladder == nullptr)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (MovementComponent == nullptr)
	{
		return;
	}

	CurrentLadder = Ladder;
	SavedMovementMode = static_cast<uint8>(MovementComponent->MovementMode);
	SavedCustomMovementMode = MovementComponent->CustomMovementMode;
	bSavedOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
	MovementComponent->StopMovementImmediately();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->SetMovementMode(MOVE_Flying);

	TransitionStartLocation = OwnerCharacter->GetActorLocation();
	TransitionTargetLocation = bEnterFromTop
		? Ladder->GetTopClimbLocation()
		: Ladder->GetBottomClimbLocation();
	TransitionStartRotation = OwnerCharacter->GetActorRotation();
	TransitionTargetRotation = bEnterFromTop
		? Ladder->GetTopClimbRotation()
		: Ladder->GetBottomClimbRotation();
	TransitionElapsed = 0.0f;
	TransitionDuration = EntryTransitionDuration;
	// Do not carry the approach input into bottom climbing; subsequent input events update it.
	CurrentClimbInput = bEnterFromTop ? -InputMagnitude : 0.0f;
	bWaitForTopEntryInputRelease = bEnterFromTop;
	SetClimbState(bEnterFromTop
		? EUOULadderClimbState::EnteringFromTop
		: EUOULadderClimbState::EnteringFromBottom);
}

void UUOULadderClimbComponent::BeginExit(bool bExitAtTop)
{
	if (OwnerCharacter == nullptr || CurrentLadder == nullptr)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	TransitionStartLocation = OwnerCharacter->GetActorLocation();
	TransitionTargetLocation = bExitAtTop
		? CurrentLadder->GetTopExitLocation()
		: CurrentLadder->GetBottomExitLocation();
	TransitionStartRotation = OwnerCharacter->GetActorRotation();
	TransitionTargetRotation = bExitAtTop
		? CurrentLadder->GetTopExitRotation()
		: CurrentLadder->GetBottomExitRotation();
	TransitionElapsed = 0.0f;
	TransitionDuration = ExitTransitionDuration;
	CurrentClimbInput = 0.0f;
	SetClimbState(bExitAtTop
		? EUOULadderClimbState::ExitingAtTop
		: EUOULadderClimbState::ExitingAtBottom);
}

void UUOULadderClimbComponent::FinishExit()
{
	RestoreMovementMode(false);
	CurrentLadder = nullptr;
	CurrentClimbInput = 0.0f;
	NormalizedHeight = 0.0f;
	bWaitForTopEntryInputRelease = false;
	SetClimbState(EUOULadderClimbState::None);
}

void UUOULadderClimbComponent::JumpOffLadder()
{
	if (OwnerCharacter == nullptr || CurrentLadder == nullptr)
	{
		return;
	}

	const FVector JumpVelocity =
		CurrentLadder->GetOutwardNormal() * JumpOffHorizontalSpeed + FVector::UpVector * JumpOffVerticalSpeed;
	SetClimbState(EUOULadderClimbState::JumpingOff);
	RestoreMovementMode(true);
	OwnerCharacter->GetCharacterMovement()->Velocity = JumpVelocity;
	CurrentLadder = nullptr;
	CurrentClimbInput = 0.0f;
	bWaitForTopEntryInputRelease = false;
	SetClimbState(EUOULadderClimbState::None);
}

void UUOULadderClimbComponent::UpdateTransition(float DeltaTime)
{
	if (OwnerCharacter == nullptr || CurrentLadder == nullptr)
	{
		return;
	}

	TransitionElapsed += DeltaTime;
	const float LinearAlpha = FMath::Clamp(TransitionElapsed / FMath::Max(TransitionDuration, UE_SMALL_NUMBER), 0.0f, 1.0f);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, LinearAlpha, 2.0f);
	const FVector NewLocation = FMath::Lerp(TransitionStartLocation, TransitionTargetLocation, SmoothAlpha);
	const FRotator NewRotation = FQuat::Slerp(
		TransitionStartRotation.Quaternion(),
		TransitionTargetRotation.Quaternion(),
		SmoothAlpha).Rotator();
	OwnerCharacter->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::None);

	if (LinearAlpha < 1.0f)
	{
		return;
	}

	if (ClimbState == EUOULadderClimbState::ExitingAtBottom || ClimbState == EUOULadderClimbState::ExitingAtTop)
	{
		FinishExit();
	}
	else
	{
		SetClimbState(EUOULadderClimbState::Climbing);
	}
}

void UUOULadderClimbComponent::UpdateClimbing(float DeltaTime)
{
	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (MovementComponent == nullptr || CurrentLadder == nullptr)
	{
		return;
	}

	const FVector LocalCharacterLocation = CurrentLadder->GetActorTransform().InverseTransformPosition(OwnerCharacter->GetActorLocation());
	const float MinimumHeight = CurrentLadder->GetBottomClimbHeight();
	const float MaximumHeight = CurrentLadder->GetTopClimbHeight();
	NormalizedHeight = FMath::GetMappedRangeValueClamped(
		FVector2D(MinimumHeight, FMath::Max(MinimumHeight + 1.0f, MaximumHeight)),
		FVector2D(0.0f, 1.0f),
		LocalCharacterLocation.Z);

	if (CurrentClimbInput > LadderInputDeadZone && LocalCharacterLocation.Z >= CurrentLadder->GetTopExitHeight())
	{
		BeginExit(true);
		return;
	}

	if (CurrentClimbInput < -LadderInputDeadZone && LocalCharacterLocation.Z <= CurrentLadder->GetBottomExitHeight())
	{
		BeginExit(false);
		return;
	}

	const FVector AlignmentTarget = CurrentLadder->GetClimbLocationNear(OwnerCharacter->GetActorLocation());
	const FVector LadderUp = CurrentLadder->GetClimbDirection();
	const FVector AlignmentError = FVector::VectorPlaneProject(
		AlignmentTarget - OwnerCharacter->GetActorLocation(),
		LadderUp);
	const FVector AlignmentVelocity = (AlignmentError * AlignmentStrength).GetClampedToMaxSize(MaximumAlignmentSpeed);
	MovementComponent->Velocity =
		LadderUp * (CurrentClimbInput * CurrentLadder->GetClimbSpeed()) + AlignmentVelocity;

	const FRotator TargetRotation = CurrentLadder->GetClimbingRotationNear(OwnerCharacter->GetActorLocation());
	OwnerCharacter->SetActorRotation(FMath::RInterpTo(
		OwnerCharacter->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		RotationInterpSpeed));
}

void UUOULadderClimbComponent::SetClimbState(EUOULadderClimbState NewState)
{
	if (ClimbState == NewState)
	{
		return;
	}

	const EUOULadderClimbState PreviousState = ClimbState;
	ClimbState = NewState;
	OnLadderStateChanged.Broadcast(PreviousState, NewState);
}

void UUOULadderClimbComponent::RestoreMovementMode(bool bForceFalling)
{
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		if (bForceFalling)
		{
			MovementComponent->SetMovementMode(MOVE_Falling);
		}
		else
		{
			MovementComponent->SetMovementMode(
				static_cast<EMovementMode>(SavedMovementMode),
				SavedCustomMovementMode);
		}
	}
}
