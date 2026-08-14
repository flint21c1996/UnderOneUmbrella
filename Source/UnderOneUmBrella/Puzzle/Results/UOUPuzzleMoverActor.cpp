// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Results/UOUPuzzleMoverActor.h"

#include "Components/SceneComponent.h"

AUOUPuzzleMoverActor::AUOUPuzzleMoverActor()
{
	// 무버 액터는 상태에 따라 매 틱 위치를 보간해야 하므로 Tick을 사용합니다.
	PrimaryActorTick.bCanEverTick = true;

	// 이동 대상과 활성 비활성 기준점은 모두 같은 루트 아래에 배치합니다.
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

	// 일시정지 중에는 현재 위치를 유지하고 이동 갱신만 멈춥니다.
	if (bPaused)
	{
		return;
	}

	// 활성 상태를 보고 목표 지점 쪽으로 이동 대상을 조금씩 옮깁니다.
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

void AUOUPuzzleMoverActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	// 외부에서는 결과 액션 enum 하나만 전달하고
	// 실제 내부 동작은 기존 Activate 계열 함수로 다시 분기합니다.
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		Activate();
		break;
	case EOUUPuzzleResultAction::Deactivate:
		Deactivate();
		break;
	case EOUUPuzzleResultAction::Pause:
		Pause();
		break;
	case EOUUPuzzleResultAction::Resume:
		Resume();
		break;
	case EOUUPuzzleResultAction::Toggle:
		Toggle();
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

FText AUOUPuzzleMoverActor::GetDebugSummaryText_Implementation() const
{
	const FVector CurrentLocation = MovingTarget != nullptr ? MovingTarget->GetComponentLocation() : FVector::ZeroVector;
	const USceneComponent* TargetPoint = bActivated ? ActivePoint.Get() : InactivePoint.Get();
	const FVector TargetLocation = TargetPoint != nullptr ? TargetPoint->GetComponentLocation() : FVector::ZeroVector;
	const float DistanceToTarget = TargetPoint != nullptr && MovingTarget != nullptr
		? FVector::Distance(CurrentLocation, TargetLocation)
		: 0.0f;

	const TArray<FString> DebugLines = {
		FString::Printf(TEXT("Mover: %s"), bActivated ? TEXT("Active") : TEXT("Inactive")),
		FString::Printf(TEXT("Paused: %s"), bPaused ? TEXT("Yes") : TEXT("No")),
		FString::Printf(TEXT("Distance To Target: %.1f"), DistanceToTarget)
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

EUOUDebugCategory AUOUPuzzleMoverActor::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

void AUOUPuzzleMoverActor::MoveTarget(float DeltaSeconds)
{
	if (MovingTarget == nullptr)
	{
		return;
	}

	// 현재 활성 상태에 따라 도착해야 할 기준점을 고릅니다.
	const USceneComponent* TargetPoint = bActivated ? ActivePoint.Get() : InactivePoint.Get();
	if (TargetPoint == nullptr)
	{
		return;
	}

	// 현재 위치에서 목표 위치까지 일정 속도로 보간 이동합니다.
	const FVector CurrentLocation = MovingTarget->GetComponentLocation();
	const FVector NextLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		TargetPoint->GetComponentLocation(),
		DeltaSeconds,
		FMath::Max(0.0f, MoveSpeed));

	MovingTarget->SetWorldLocation(NextLocation);

	if (bApplyPointRotation)
	{
		// ActivePoint, InactivePoint의 회전을 목표 회전으로 보고 부드럽게 따라갑니다.
		const FRotator CurrentRotation = MovingTarget->GetComponentRotation();
		const FRotator NextRotation = FMath::RInterpConstantTo(
			CurrentRotation,
			TargetPoint->GetComponentRotation(),
			DeltaSeconds,
			FMath::Max(0.0f, RotationSpeed));

		MovingTarget->SetWorldRotation(NextRotation);
	}

	if (bApplyPointScale)
	{
		// 위치와 별개로 목표 포인트의 월드 스케일도 따라가게 해서
		// ActivePoint, InactivePoint에서 크기 변화까지 연출할 수 있게 합니다.
		const FVector CurrentScale = MovingTarget->GetComponentScale();
		const FVector NextScale = FMath::VInterpTo(
			CurrentScale,
			TargetPoint->GetComponentScale(),
			DeltaSeconds,
			FMath::Max(0.0f, ScaleSpeed));

		MovingTarget->SetWorldScale3D(NextScale);
	}
}
