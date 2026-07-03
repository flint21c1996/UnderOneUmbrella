// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/PushPull/UOUPushPullBlockActor.h"

#include "Components/StaticMeshComponent.h"
#include "Player/UOUWaterContainerComponent.h"
#include "Puzzle/PushPull/UOUPushPullObjectComponent.h"
#include "Puzzle/Weight/UOUPuzzleWeightComponent.h"

AUOUPushPullBlockActor::AUOUPushPullBlockActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BlockVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockVisual"));
	SetRootComponent(BlockVisual);
	BlockVisual->SetSimulatePhysics(true);
	BlockVisual->SetEnableGravity(true);
	BlockVisual->SetCollisionProfileName(TEXT("PhysicsActor"));
	BlockVisual->SetGenerateOverlapEvents(true);
	ApplyBlockRotationLocks(false);
	BlockVisual->SetLinearDamping(6.0f);
	BlockVisual->SetAngularDamping(12.0f);

	PushPullObjectComponent = CreateDefaultSubobject<UUOUPushPullObjectComponent>(TEXT("PushPullObjectComponent"));
	PushPullObjectComponent->TargetPrimitive = BlockVisual;
	PushPullObjectComponent->MaxHorizontalSpeed = 300.0f;
	PushPullObjectComponent->StepAssistProbeDistance = 20.0f;
	PushPullObjectComponent->StepAssistLiftSpeed = 240.0f;

	WaterContainerComponent = CreateDefaultSubobject<UUOUWaterContainerComponent>(TEXT("WaterContainerComponent"));
	WaterContainerComponent->MaxAmount = 5.0f;
	WaterContainerComponent->InitialAmount = 0.0f;
	WaterContainerComponent->WeightMultiplier = 1.0f;
	WaterContainerComponent->MaterialVisualComponent = BlockVisual;
	WaterContainerComponent->bUpdateMaterialVisual = true;

	PuzzleWeightComponent = CreateDefaultSubobject<UUOUPuzzleWeightComponent>(TEXT("PuzzleWeightComponent"));
	PuzzleWeightComponent->BaseWeight = 10.0f;
	PuzzleWeightComponent->WaterContainer = WaterContainerComponent;
}

void AUOUPushPullBlockActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyBlockRotationLocks(true);
	BlockVisual->SetGenerateOverlapEvents(true);
}

void AUOUPushPullBlockActor::ApplyBlockRotationLocks(bool bRecreatePhysicsState)
{
	if (BlockVisual == nullptr)
	{
		return;
	}

	BlockVisual->SetConstraintMode(EDOFMode::SixDOF);
	BlockVisual->BodyInstance.bLockXRotation = true;
	BlockVisual->BodyInstance.bLockYRotation = true;
	BlockVisual->BodyInstance.bLockZRotation = true;

	if (bRecreatePhysicsState && BlockVisual->IsRegistered())
	{
		BlockVisual->RecreatePhysicsState();
	}
}
