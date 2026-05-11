// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Results/UOUPuzzleMoverActor.h"

#include "Components/SceneComponent.h"

AUOUPuzzleMoverActor::AUOUPuzzleMoverActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	MovingTarget = CreateDefaultSubobject<USceneComponent>(TEXT("MovingTarget"));
	MovingTarget->SetupAttachment(RootScene);

	InactivePoint = CreateDefaultSubobject<USceneComponent>(TEXT("InactivePoint"));
	InactivePoint->SetupAttachment(RootScene);

	ActivePoint = CreateDefaultSubobject<USceneComponent>(TEXT("ActivePoint"));
	ActivePoint->SetupAttachment(RootScene);
	ActivePoint->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
}

void AUOUPuzzleMoverActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPaused)
	{
		return;
	}

	MoveTarget(DeltaSeconds);
}

void AUOUPuzzleMoverActor::Activate()
{
	SetActivated(true);
}

void AUOUPuzzleMoverActor::Deactivate()
{
	SetActivated(false);
}

void AUOUPuzzleMoverActor::Pause()
{
	bPaused = true;
}

void AUOUPuzzleMoverActor::Resume()
{
	bPaused = false;
}

void AUOUPuzzleMoverActor::Toggle()
{
	SetActivated(!bActivated);
}

void AUOUPuzzleMoverActor::SetActivated(bool bNewActivated)
{
	bActivated = bNewActivated;
	bPaused = false;
}

void AUOUPuzzleMoverActor::MoveTarget(float DeltaSeconds)
{
	if (MovingTarget == nullptr)
	{
		return;
	}

	const USceneComponent* TargetPoint = bActivated ? ActivePoint.Get() : InactivePoint.Get();
	if (TargetPoint == nullptr)
	{
		return;
	}

	const FVector CurrentLocation = MovingTarget->GetComponentLocation();
	const FVector NextLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		TargetPoint->GetComponentLocation(),
		DeltaSeconds,
		FMath::Max(0.0f, MoveSpeed));

	MovingTarget->SetWorldLocation(NextLocation);
}
