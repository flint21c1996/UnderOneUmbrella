// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"

AUOUFloorPlatformActor::AUOUFloorPlatformActor()
{
	// 플랫폼 이동은 시간 보간이 필요하므로 Tick을 사용합니다.
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	RootScene->SetMobility(EComponentMobility::Movable);

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	PlatformMesh->SetupAttachment(RootScene);
	PlatformMesh->SetMobility(EComponentMobility::Movable);
}

void AUOUFloorPlatformActor::BeginPlay()
{
	Super::BeginPlay();

	CaptureCurrentAsStart();

	if (bStartAtTarget)
	{
		SnapToTarget();
	}
	else
	{
		bIsAtTarget = false;
		ApplyTargetCollisionState();
	}
}

void AUOUFloorPlatformActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!bHasCapturedStartTransform)
	{
		StartTransform = GetActorTransform();
		bHasCapturedStartTransform = true;
	}

	RefreshTargetTransforms();
}

void AUOUFloorPlatformActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsMoving)
	{
		return;
	}

	MoveElapsedTime += DeltaSeconds;

	const float SafeDuration = FMath::Max(UE_KINDA_SMALL_NUMBER, MoveDuration);
	const float RawAlpha = FMath::Clamp(MoveElapsedTime / SafeDuration, 0.0f, 1.0f);
	const float MoveAlpha = ResolveMoveAlpha(RawAlpha);

	const FVector NewLocation = FMath::Lerp(StartTransform.GetLocation(), TargetTransform.GetLocation(), MoveAlpha);
	const FQuat NewRotation = FQuat::Slerp(StartTransform.GetRotation(), TargetTransform.GetRotation(), MoveAlpha);
	const FVector NewScale = FMath::Lerp(StartTransform.GetScale3D(), TargetTransform.GetScale3D(), MoveAlpha);

	SetActorTransform(FTransform(NewRotation, NewLocation, NewScale), false, nullptr, ETeleportType::TeleportPhysics);

	if (RawAlpha >= 1.0f)
	{
		FinishMoveToTarget();
	}
}

void AUOUFloorPlatformActor::CaptureCurrentAsStart()
{
	StartTransform = GetActorTransform();
	bHasCapturedStartTransform = true;

	RefreshTargetTransforms();
}

void AUOUFloorPlatformActor::MoveToTarget()
{
	if (bIsMoving || bIsAtTarget)
	{
		return;
	}

	if (!bHasCapturedStartTransform)
	{
		CaptureCurrentAsStart();
	}

	RefreshTargetTransforms();

	bIsMoving = true;
	bIsAtTarget = false;
	MoveElapsedTime = 0.0f;
	ApplyTargetCollisionState();

	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void AUOUFloorPlatformActor::ResetPlatform()
{
	if (!bHasCapturedStartTransform)
	{
		CaptureCurrentAsStart();
	}

	bIsMoving = false;
	bIsAtTarget = false;
	MoveElapsedTime = 0.0f;

	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyTargetCollisionState();
}

void AUOUFloorPlatformActor::SnapToTarget()
{
	if (!bHasCapturedStartTransform)
	{
		CaptureCurrentAsStart();
	}

	RefreshTargetTransforms();

	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;

	SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyTargetCollisionState();
}

bool AUOUFloorPlatformActor::IsMoving() const
{
	return bIsMoving;
}

bool AUOUFloorPlatformActor::IsAtTarget() const
{
	return bIsAtTarget;
}

void AUOUFloorPlatformActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	// 퍼즐 결과는 플랫폼 기준에서 필요한 동작으로만 해석합니다.
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		MoveToTarget();
		break;
	case EOUUPuzzleResultAction::Deactivate:
		ResetPlatform();
		break;
	case EOUUPuzzleResultAction::Toggle:
		if (bIsAtTarget)
		{
			ResetPlatform();
		}
		else
		{
			MoveToTarget();
		}
		break;
	case EOUUPuzzleResultAction::None:
	case EOUUPuzzleResultAction::Pause:
	case EOUUPuzzleResultAction::Resume:
	default:
		break;
	}
}

#if WITH_EDITOR
bool AUOUFloorPlatformActor::ShouldTickIfViewportsOnly() const
{
	return bIsMoving;
}
#endif

void AUOUFloorPlatformActor::RefreshTargetTransforms()
{
	if (!bHasCapturedStartTransform)
	{
		StartTransform = GetActorTransform();
		bHasCapturedStartTransform = true;
	}

	TargetTransform = StartTransform;
	const FVector TargetWorldOffset = StartTransform.TransformVectorNoScale(TargetLocalOffset);
	TargetTransform.SetLocation(StartTransform.GetLocation() + TargetWorldOffset);
}

void AUOUFloorPlatformActor::FinishMoveToTarget()
{
	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;

	SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyTargetCollisionState();

	OnMoveFinished.Broadcast(this);
}

void AUOUFloorPlatformActor::ApplyTargetCollisionState()
{
	if (!bDisableCollisionAtTarget)
	{
		SetActorEnableCollision(true);
		return;
	}

	SetActorEnableCollision(!bIsAtTarget);
}

float AUOUFloorPlatformActor::ResolveMoveAlpha(float RawAlpha) const
{
	if (MoveCurve == nullptr)
	{
		return RawAlpha;
	}

	return FMath::Clamp(MoveCurve->GetFloatValue(RawAlpha), 0.0f, 1.0f);
}
