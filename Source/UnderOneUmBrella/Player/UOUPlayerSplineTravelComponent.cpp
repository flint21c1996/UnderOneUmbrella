// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUPlayerSplineTravelComponent.h"

#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"

UUOUPlayerSplineTravelComponent::UUOUPlayerSplineTravelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool UUOUPlayerSplineTravelComponent::StartTravel(
	USplineComponent* Path,
	float Speed,
	float AcceptanceRadius,
	UObject* RequestSource,
	bool bInAllowCameraRotation)
{
	if (IsTraveling()
		|| Path == nullptr
		|| Path->GetNumberOfSplinePoints() < 2
		|| Speed <= 0.0f
		|| Cast<AUOUCharacter>(GetOwner()) == nullptr)
	{
		return false;
	}

	ActivePath = Path;
	ActiveRequestSource = RequestSource;
	TravelSpeed = Speed;
	EntryAcceptanceRadius = FMath::Max(0.0f, AcceptanceRadius);
	CurrentDistanceAlongPath = 0.0f;
	bStartPending = true;
	bTravelActive = false;
	bMovingToPathStart = true;
	bAllowCameraRotation = bInAllowCameraRotation;
	SetComponentTickEnabled(true);
	return true;
}

void UUOUPlayerSplineTravelComponent::CancelTravel(UObject* RequestSource)
{
	if (!IsTraveling())
	{
		return;
	}

	if (RequestSource != nullptr && ActiveRequestSource.Get() != RequestSource)
	{
		return;
	}

	FinishTravel();
}

bool UUOUPlayerSplineTravelComponent::IsTraveling() const
{
	return bStartPending || bTravelActive;
}

void UUOUPlayerSplineTravelComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishTravel();
	Super::EndPlay(EndPlayReason);
}

void UUOUPlayerSplineTravelComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bStartPending && !BeginPendingTravel())
	{
		FinishTravel();
		return;
	}

	if (bTravelActive)
	{
		UpdateTravel(DeltaTime);
	}
}

bool UUOUPlayerSplineTravelComponent::BeginPendingTravel()
{
	bStartPending = false;

	AUOUCharacter* Character = Cast<AUOUCharacter>(GetOwner());
	USplineComponent* Path = ActivePath.Get();
	if (Character == nullptr
		|| Path == nullptr
		|| Path->GetNumberOfSplinePoints() < 2
		|| TravelSpeed <= 0.0f)
	{
		return false;
	}

	if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (UUOUPlayerInteractionExecutorComponent* InputExecutor =
		Character->GetInteractionExecutorComponent())
	{
		if (bAllowCameraRotation)
		{
			InputExecutor->RequestPlayerInputBlockAllowingCameraRotation(this, true);
		}
		else
		{
			InputExecutor->RequestPlayerInputBlock(this, true);
		}
		LockedInputExecutorComponent = InputExecutor;
	}

	bTravelActive = true;
	return true;
}

void UUOUPlayerSplineTravelComponent::UpdateTravel(float DeltaTime)
{
	AUOUCharacter* Character = Cast<AUOUCharacter>(GetOwner());
	USplineComponent* Path = ActivePath.Get();
	if (Character == nullptr
		|| Path == nullptr
		|| Path->GetNumberOfSplinePoints() < 2
		|| DeltaTime <= 0.0f
		|| TravelSpeed <= 0.0f)
	{
		if (Character == nullptr || Path == nullptr || Path->GetNumberOfSplinePoints() < 2)
		{
			FinishTravel();
		}
		return;
	}

	if (bMovingToPathStart)
	{
		const FVector PathStart =
			Path->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		const FVector ToPathStart = PathStart - Character->GetActorLocation();
		const float DistanceToPathStart = ToPathStart.Size();

		if (DistanceToPathStart <= EntryAcceptanceRadius || ToPathStart.IsNearlyZero())
		{
			Character->SetActorLocation(PathStart, true);
			bMovingToPathStart = false;
			CurrentDistanceAlongPath = 0.0f;
			return;
		}

		const float EntryMoveDistance =
			FMath::Min(TravelSpeed * DeltaTime, DistanceToPathStart);
		Character->SetActorLocation(
			Character->GetActorLocation()
				+ ToPathStart.GetSafeNormal() * EntryMoveDistance,
			true);
		return;
	}

	const float SplineLength = Path->GetSplineLength();
	const float NextDistance = FMath::Min(
		CurrentDistanceAlongPath + TravelSpeed * DeltaTime,
		SplineLength);
	const FVector NextLocation =
		Path->GetLocationAtDistanceAlongSpline(
			NextDistance,
			ESplineCoordinateSpace::World);

	FHitResult MoveHit;
	Character->SetActorLocation(NextLocation, true, &MoveHit);
	CurrentDistanceAlongPath = FMath::Lerp(
		CurrentDistanceAlongPath,
		NextDistance,
		MoveHit.bBlockingHit ? MoveHit.Time : 1.0f);

	if (CurrentDistanceAlongPath >= SplineLength - KINDA_SMALL_NUMBER)
	{
		FinishTravel();
	}
}

void UUOUPlayerSplineTravelComponent::FinishTravel()
{
	if (UUOUPlayerInteractionExecutorComponent* InputExecutor =
		LockedInputExecutorComponent.Get())
	{
		if (bAllowCameraRotation)
		{
			InputExecutor->ReleasePlayerInputBlockAllowingCameraRotation(this);
		}
		else
		{
			InputExecutor->ReleasePlayerInputBlock(this);
		}
	}

	if (bTravelActive)
	{
		if (AUOUCharacter* Character = Cast<AUOUCharacter>(GetOwner()))
		{
			if (UCharacterMovementComponent* MovementComponent =
				Character->GetCharacterMovement())
			{
				MovementComponent->StopMovementImmediately();
				MovementComponent->SetMovementMode(MOVE_Falling);
			}
		}
	}

	ResetTravelState();
}

void UUOUPlayerSplineTravelComponent::ResetTravelState()
{
	ActivePath.Reset();
	ActiveRequestSource.Reset();
	LockedInputExecutorComponent.Reset();
	TravelSpeed = 0.0f;
	EntryAcceptanceRadius = 0.0f;
	CurrentDistanceAlongPath = 0.0f;
	bStartPending = false;
	bTravelActive = false;
	bMovingToPathStart = false;
	bAllowCameraRotation = true;
	SetComponentTickEnabled(false);
}
