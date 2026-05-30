// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformActor.h"

#include "World/Stage/UOUFloorPlatformCarryComponent.h"
#include "World/Stage/UOUFloorPlatformStepComponent.h"
#include "World/Stage/UOUFloorPlatformTargetActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"

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

	CarryDetectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CarryDetectionBox"));
	CarryDetectionBox->bEditableWhenInherited = false;
	CarryDetectionBox->SetupAttachment(RootScene);
	CarryDetectionBox->SetMobility(EComponentMobility::Movable);
	CarryDetectionBox->SetBoxExtent(FVector(300.0f, 300.0f, 160.0f));
	CarryDetectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 170.0f));
	CarryDetectionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarryDetectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CarryDetectionBox->SetGenerateOverlapEvents(false);

	CarryComponent = CreateDefaultSubobject<UUOUFloorPlatformCarryComponent>(TEXT("CarryComponent"));
	CarryComponent->SetDetectionBox(CarryDetectionBox);

	StepComponent = CreateDefaultSubobject<UUOUFloorPlatformStepComponent>(TEXT("StepComponent"));

	MovePreviewPath = CreateDefaultSubobject<USplineComponent>(TEXT("MovePreviewPath"));
	MovePreviewPath->bEditableWhenInherited = false;
	MovePreviewPath->SetupAttachment(RootScene);
	MovePreviewPath->SetMobility(EComponentMobility::Movable);
	MovePreviewPath->SetUsingAbsoluteLocation(true);
	MovePreviewPath->SetUsingAbsoluteRotation(true);
	MovePreviewPath->SetUsingAbsoluteScale(true);
	MovePreviewPath->SetHiddenInGame(true);
	MovePreviewPath->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MovePreviewPath->ClearSplinePoints(false);
	MovePreviewPath->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
	MovePreviewPath->AddSplinePoint(FVector::ForwardVector * 100.0f, ESplineCoordinateSpace::Local, false);
	MovePreviewPath->SetSplinePointType(0, ESplinePointType::Linear, false);
	MovePreviewPath->SetSplinePointType(1, ESplinePointType::Linear, false);
#if WITH_EDITORONLY_DATA
	MovePreviewPath->EditorUnselectedSplineSegmentColor = FLinearColor::FromSRGBColor(FColor::Cyan);
	MovePreviewPath->EditorSelectedSplineSegmentColor = FLinearColor::FromSRGBColor(FColor::Cyan);
	MovePreviewPath->EditorTangentColor = FLinearColor::FromSRGBColor(FColor::Cyan);
	MovePreviewPath->bShouldVisualizeScale = true;
	MovePreviewPath->ScaleVisualizationWidth = 8.0f;
#endif
	MovePreviewPath->UpdateSpline();

}

void AUOUFloorPlatformActor::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	EnsureStartTransform();
	ResetRuntimeStepIndex();

	// 맵을 다시 열 때만 저장된 시작 위치로 복구합니다.
	// OnConstruction에서 처리하면 에디터 수동 배치까지 막히기 때문에 로드 시점으로 제한합니다.
	if (bKeepActorAtSavedStartInEditor && bUseSavedStartTransform)
	{
		bIsApplyingEditorTransform = true;
		SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
		bIsApplyingEditorTransform = false;
	}
#endif
}

void AUOUFloorPlatformActor::BeginPlay()
{
	Super::BeginPlay();

	if (CarryComponent != nullptr)
	{
		CarryDetectionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CarryDetectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		CarryDetectionBox->SetGenerateOverlapEvents(false);
		CarryComponent->SetDetectionBox(CarryDetectionBox);
	}

	EnsureStartTransform();
	ResetRuntimeStepIndex();
	PendingSequentialMoveCount = 0;
	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);

	const bool bShouldStartAtTarget = bStartAtTarget && !ShouldUseSequentialTargetMarkers();
	if (bShouldStartAtTarget)
	{
		SnapToTarget();
	}
	else
	{
		bIsAtTarget = false;
		ApplyTargetCollisionState();
	}
}

void AUOUFloorPlatformActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CarryComponent != nullptr)
	{
		CarryComponent->DetachCarriedActors();
	}

	Super::EndPlay(EndPlayReason);
}

void AUOUFloorPlatformActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (CarryComponent != nullptr)
	{
		CarryDetectionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CarryDetectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		CarryDetectionBox->SetGenerateOverlapEvents(false);
		CarryComponent->SetDetectionBox(CarryDetectionBox);
	}

	EnsureStartTransform();

	RefreshTargetTransforms();
	UpdateEditorPreviewVisuals();
}

void AUOUFloorPlatformActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITOR
	if (!bIsMoving && GetWorld() != nullptr && !GetWorld()->IsGameWorld() && ShouldUseSequentialTargetMarkers())
	{
		RefreshTargetTransforms();
		UpdateEditorPreviewVisuals();
		return;
	}
#endif

	if (!bIsMoving)
	{
		return;
	}

	MoveElapsedTime += DeltaSeconds;

	const float SafeDuration = FMath::Max(UE_KINDA_SMALL_NUMBER, MoveDuration);
	const float RawAlpha = FMath::Clamp(MoveElapsedTime / SafeDuration, 0.0f, 1.0f);
	const float MoveAlpha = ResolveMoveAlpha(RawAlpha);

	SetActorTransform(BuildActivePlatformTransformAtAlpha(MoveAlpha), false, nullptr, ETeleportType::TeleportPhysics);

	if (RawAlpha >= 1.0f)
	{
		FinishMoveToTarget();
	}
}

void AUOUFloorPlatformActor::CaptureCurrentAsStart()
{
	StartTransform = GetActorTransform();
	SavedStartTransform = StartTransform;
	bUseSavedStartTransform = true;
	bHasCapturedStartTransform = true;
	ResetRuntimeStepIndex();

	RefreshTargetTransforms();
	UpdateEditorPreviewVisuals();
}

void AUOUFloorPlatformActor::MoveToTarget()
{
	RequestSequentialMoveSteps(MoveStepCountPerActivate);
}

void AUOUFloorPlatformActor::MoveToNextSequentialTarget()
{
	RequestSequentialMoveSteps(1);
}

void AUOUFloorPlatformActor::ResetPlatform()
{
	EnsureStartTransform();

	RefreshTargetTransforms();
	if (CarryComponent != nullptr)
	{
		CarryComponent->AttachLastMovedActors();
		if (!CarryComponent->HasCarriedActors())
		{
			CarryComponent->AttachCarriedActors();
		}
	}

	bIsMoving = false;
	bIsAtTarget = false;
	MoveElapsedTime = 0.0f;
	PendingSequentialMoveCount = 0;
	ActiveMoveTargetMarker = nullptr;
	ResetRuntimeStepIndex();
	if (StepComponent != nullptr)
	{
		StepComponent->ResetRuntimeState();
	}
	MoveStartTransform = StartTransform;
	MoveTargetTransform = TargetTransform;

#if WITH_EDITOR
	bIsApplyingEditorTransform = true;
#endif
	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
#if WITH_EDITOR
	bIsApplyingEditorTransform = false;
#endif
	ApplyTargetCollisionState();
	if (CarryComponent != nullptr)
	{
		CarryComponent->CacheLastMovedActors();
		CarryComponent->DetachCarriedActors();
	}
	UpdateEditorPreviewVisuals();
}

void AUOUFloorPlatformActor::SnapToTarget()
{
	EnsureStartTransform();

	RefreshTargetTransforms();

	if (CarryComponent != nullptr)
	{
		CarryComponent->AttachCarriedActors();
	}

	FTransform SnapTargetTransform = FTransform::Identity;
	int32 SnapTargetIndex = INDEX_NONE;
	if (!ResolveNextSequentialTargetTransform(SnapTargetTransform, SnapTargetIndex))
	{
		if (CarryComponent != nullptr)
		{
			CarryComponent->DetachCarriedActors();
		}
		return;
	}

	EUOUFloorPlatformRotationMode SnapRotationMode;
	EUOUFloorPlatformHingeEdge SnapHingeEdge;
	FVector SnapCustomHingeLocalOffset;
	ResolveMoveSettingsFromTarget(GetSequentialTargetMarkerAt(SnapTargetIndex), SnapRotationMode, SnapHingeEdge, SnapCustomHingeLocalOffset);

	const FTransform ResolvedSnapTargetTransform = BuildTransformBetween(
		GetActorTransform(),
		SnapTargetTransform,
		1.0f,
		SnapRotationMode,
		SnapHingeEdge,
		SnapCustomHingeLocalOffset);

	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;
	PendingSequentialMoveCount = 0;
	ActiveMoveTargetMarker = nullptr;
	if (StepComponent != nullptr)
	{
		StepComponent->ClearReturnToStartRequest();
		StepComponent->SetActiveTargetIndex(SnapTargetIndex);
	}
	MoveStartTransform = GetActorTransform();
	MoveTargetTransform = ResolvedSnapTargetTransform;

#if WITH_EDITOR
	bIsApplyingEditorTransform = true;
#endif
	SetActorTransform(ResolvedSnapTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
#if WITH_EDITOR
	bIsApplyingEditorTransform = false;
#endif
	ApplyTargetCollisionState();
	if (CarryComponent != nullptr)
	{
		CarryComponent->CacheLastMovedActors();
		CarryComponent->DetachCarriedActors();
	}
	AdvanceSequentialTargetIndex();
	UpdateEditorPreviewVisuals();
}

void AUOUFloorPlatformActor::SetTargetToStart()
{
	EnsureStartTransform();

	if (AUOUFloorPlatformTargetActor* SequentialTargetMarker = GetCurrentSequentialTargetMarker())
	{
		SequentialTargetMarker->SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
		SequentialTargetMarker->SyncPreviewFromMesh(PlatformMesh);
		SequentialTargetMarker->SetTargetPreviewMeshVisible(bShowTransformPreview);
	}

	RefreshTargetTransforms();
	UpdateEditorPreviewVisuals();
}

void AUOUFloorPlatformActor::SnapCurrentStepToHingeResult()
{
	EnsureStartTransform();

	RefreshTargetTransforms();

	AUOUFloorPlatformTargetActor* SequentialTargetMarker = GetCurrentSequentialTargetMarker();
	if (SequentialTargetMarker == nullptr)
	{
		return;
	}

	EUOUFloorPlatformRotationMode ResolvedRotationMode;
	EUOUFloorPlatformHingeEdge ResolvedHingeEdge;
	FVector ResolvedCustomHingeLocalOffset;
	ResolveMoveSettingsFromTarget(SequentialTargetMarker, ResolvedRotationMode, ResolvedHingeEdge, ResolvedCustomHingeLocalOffset);
	if (ResolvedRotationMode != EUOUFloorPlatformRotationMode::Hinge)
	{
		return;
	}

	// 목표 마커의 회전은 유지하고 위치만 힌지 접힘 결과에 맞춰서 보정합니다.
	const FTransform HingeTargetTransform = BuildHingeTransformBetween(
		GetActorTransform(),
		SequentialTargetMarker->GetActorTransform(),
		1.0f,
		ResolvedHingeEdge,
		ResolvedCustomHingeLocalOffset);
	SequentialTargetMarker->SetActorTransform(HingeTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	SequentialTargetMarker->SyncPreviewFromMesh(PlatformMesh);
	SequentialTargetMarker->SetTargetPreviewMeshVisible(bShowTransformPreview);

	RefreshTargetTransforms();
	UpdateEditorPreviewVisuals();
}

void AUOUFloorPlatformActor::AddSequentialTargetMarker()
{
#if WITH_EDITOR
	if (GetWorld() == nullptr)
	{
		return;
	}

	EnsureStartTransform();

	RefreshTargetTransforms();

	FTransform NewTargetTransform = TargetTransform;
	for (int32 MarkerIndex = SequentialTargetMarkers.Num() - 1; MarkerIndex >= 0; --MarkerIndex)
	{
		if (IsValid(SequentialTargetMarkers[MarkerIndex]))
		{
			NewTargetTransform = SequentialTargetMarkers[MarkerIndex]->GetActorTransform();
			break;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.Name = MakeUniqueObjectName(GetWorld()->GetCurrentLevel(), AUOUFloorPlatformTargetActor::StaticClass(), *FString::Printf(TEXT("%s_MoveStep"), *GetName()));

	AUOUFloorPlatformTargetActor* NewTargetMarker = GetWorld()->SpawnActor<AUOUFloorPlatformTargetActor>(
		AUOUFloorPlatformTargetActor::StaticClass(),
		NewTargetTransform,
		SpawnParameters);

	if (NewTargetMarker == nullptr)
	{
		return;
	}

	NewTargetMarker->SetActorLabel(FString::Printf(TEXT("%s_MoveStep_%d"), *GetActorLabel(), SequentialTargetMarkers.Num()));
	NewTargetMarker->SyncPreviewFromMesh(PlatformMesh);
	NewTargetMarker->SetTargetPreviewMeshVisible(bShowTransformPreview);

	SequentialTargetMarkers.Add(NewTargetMarker);
	bUseSequentialTargetMarkers = true;
	InitialSequentialTargetIndex = FMath::Clamp(InitialSequentialTargetIndex, 0, FMath::Max(0, SequentialTargetMarkers.Num() - 1));
	ResetRuntimeStepIndex();

	RefreshTargetTransforms();
	UpdateEditorPreviewVisuals();
#endif
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
		if (ShouldUseSequentialTargetMarkers())
		{
			MoveToNextSequentialTarget();
		}
		else if (bIsAtTarget)
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
	return bIsMoving || ShouldUseSequentialTargetMarkers();
}
#endif

void AUOUFloorPlatformActor::RefreshTargetTransforms()
{
	EnsureStartTransform();

	TargetTransform = StartTransform;
	if (AUOUFloorPlatformTargetActor* SequentialTargetMarker = GetCurrentSequentialTargetMarker())
	{
		TargetTransform = SequentialTargetMarker->GetActorTransform();
	}
}

void AUOUFloorPlatformActor::EnsureStartTransform()
{
	if (bUseSavedStartTransform)
	{
		StartTransform = SavedStartTransform;
		bHasCapturedStartTransform = true;
		return;
	}

	// 아직 시작 위치를 저장하지 않은 플랫폼은 현재 배치 위치를 임시 시작점으로 사용합니다.
	// 실제 저장 시작점은 CaptureCurrentAsStart 버튼을 눌렀을 때만 확정합니다.
	StartTransform = GetActorTransform();
	bHasCapturedStartTransform = true;
}

void AUOUFloorPlatformActor::ResetRuntimeStepIndex()
{
	const int32 MaxTargetIndex = FMath::Max(0, SequentialTargetMarkers.Num() - 1);
	RuntimeSequentialTargetIndex = FMath::Clamp(InitialSequentialTargetIndex, 0, MaxTargetIndex);
}

void AUOUFloorPlatformActor::FinishMoveToTarget()
{
	const bool bShouldContinueFromArrivedMarker = ActiveMoveTargetMarker != nullptr
		&& ActiveMoveTargetMarker->bContinueToNextStepOnArrival;
	ActiveMoveTargetMarker = nullptr;

	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;

#if WITH_EDITOR
	bIsApplyingEditorTransform = true;
#endif
	SetActorTransform(MoveTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
#if WITH_EDITOR
	bIsApplyingEditorTransform = false;
#endif
	ApplyTargetCollisionState();
	if (CarryComponent != nullptr)
	{
		CarryComponent->CacheLastMovedActors();
		CarryComponent->DetachCarriedActors();
	}
	AdvanceSequentialTargetIndex();

	OnMoveFinished.Broadcast(this);
	if (bShouldContinueFromArrivedMarker)
	{
		PendingSequentialMoveCount = FMath::Max(PendingSequentialMoveCount, 1);
	}
	TryStartQueuedSequentialMove();
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

bool AUOUFloorPlatformActor::ShouldUseSequentialTargetMarkers() const
{
	return StepComponent != nullptr
		&& StepComponent->ShouldUseMoveSteps(bUseSequentialTargetMarkers, SequentialTargetMarkers, RuntimeSequentialTargetIndex);
}

AUOUFloorPlatformTargetActor* AUOUFloorPlatformActor::GetSequentialTargetMarkerAt(int32 TargetIndex) const
{
	if (StepComponent == nullptr)
	{
		return nullptr;
	}

	return StepComponent->GetTargetMarkerAt(SequentialTargetMarkers, TargetIndex);
}

AUOUFloorPlatformTargetActor* AUOUFloorPlatformActor::GetCurrentSequentialTargetMarker() const
{
	if (StepComponent == nullptr)
	{
		return nullptr;
	}

	return StepComponent->GetCurrentTargetMarker(
		bUseSequentialTargetMarkers,
		SequentialTargetMarkers,
		RuntimeSequentialTargetIndex);
}

bool AUOUFloorPlatformActor::ResolveNextSequentialTargetTransform(FTransform& OutTargetTransform, int32& OutTargetIndex) const
{
	if (StepComponent == nullptr)
	{
		OutTargetTransform = FTransform::Identity;
		OutTargetIndex = INDEX_NONE;
		return false;
	}

	return StepComponent->ResolveNextTargetTransform(
		StartTransform,
		bUseSequentialTargetMarkers,
		SequentialTargetMarkers,
		RuntimeSequentialTargetIndex,
		OutTargetTransform,
		OutTargetIndex);
}

void AUOUFloorPlatformActor::AdvanceSequentialTargetIndex()
{
	if (StepComponent == nullptr)
	{
		return;
	}

	StepComponent->AdvanceTargetIndex(
		bUseSequentialTargetMarkers,
		SequentialTargetMarkers,
		bLoopSequentialTargetMarkers,
		bLoopMoveStepsThroughStart,
		RuntimeSequentialTargetIndex);
}

bool AUOUFloorPlatformActor::BeginMoveToTransform(const FTransform& InTargetTransform, AUOUFloorPlatformTargetActor* TargetMarker)
{
	if (bIsMoving)
	{
		return false;
	}

	CacheActiveMoveSettings(TargetMarker);
	ActiveMoveTargetMarker = TargetMarker;

	MoveStartTransform = GetActorTransform();
	MoveTargetTransform = BuildTransformBetween(
		MoveStartTransform,
		InTargetTransform,
		1.0f,
		ActiveMoveRotationMode,
		ActiveMoveHingeEdge,
		ActiveMoveCustomHingeLocalOffset);

	bIsMoving = true;
	bIsAtTarget = false;
	MoveElapsedTime = 0.0f;
	ApplyTargetCollisionState();
	if (CarryComponent != nullptr)
	{
		CarryComponent->AttachCarriedActors();
	}

	return true;
}

void AUOUFloorPlatformActor::RequestSequentialMoveSteps(int32 StepCount)
{
	const int32 SafeStepCount = FMath::Max(1, StepCount);
	PendingSequentialMoveCount += SafeStepCount;
	TryStartQueuedSequentialMove();
}

bool AUOUFloorPlatformActor::TryStartQueuedSequentialMove()
{
	if (bIsMoving || PendingSequentialMoveCount <= 0)
	{
		return false;
	}

	FTransform NextTargetTransform;
	int32 NextTargetIndex = INDEX_NONE;
	if (!ResolveNextSequentialTargetTransform(NextTargetTransform, NextTargetIndex))
	{
		PendingSequentialMoveCount = 0;
		return false;
	}

	if (GetActorTransform().Equals(NextTargetTransform))
	{
		PendingSequentialMoveCount = 0;
		return false;
	}

	AUOUFloorPlatformTargetActor* NextTargetMarker = GetSequentialTargetMarkerAt(NextTargetIndex);

	if (StepComponent != nullptr)
	{
		StepComponent->ClearReturnToStartRequest();
		StepComponent->SetActiveTargetIndex(NextTargetIndex);
	}

	--PendingSequentialMoveCount;
	return BeginMoveToTransform(NextTargetTransform, NextTargetMarker);
}

FTransform AUOUFloorPlatformActor::BuildActivePlatformTransformAtAlpha(float Alpha) const
{
	return BuildTransformBetween(
		MoveStartTransform,
		MoveTargetTransform,
		Alpha,
		ActiveMoveRotationMode,
		ActiveMoveHingeEdge,
		ActiveMoveCustomHingeLocalOffset);
}

FTransform AUOUFloorPlatformActor::BuildTransformBetween(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha) const
{
	return BuildTransformBetween(
		FromTransform,
		ToTransform,
		Alpha,
		RotationMode,
		HingeEdge,
		CustomHingeLocalOffset);
}

FTransform AUOUFloorPlatformActor::BuildTransformBetween(
	const FTransform& FromTransform,
	const FTransform& ToTransform,
	float Alpha,
	EUOUFloorPlatformRotationMode InRotationMode,
	EUOUFloorPlatformHingeEdge InHingeEdge,
	const FVector& InCustomHingeLocalOffset) const
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	if (InRotationMode == EUOUFloorPlatformRotationMode::Hinge)
	{
		return BuildHingeTransformBetween(FromTransform, ToTransform, SafeAlpha, InHingeEdge, InCustomHingeLocalOffset);
	}

	const FVector NewLocation = EvaluatePathLocation(FromTransform, ToTransform, SafeAlpha);
	const FQuat NewRotation = FQuat::Slerp(FromTransform.GetRotation(), ToTransform.GetRotation(), SafeAlpha);
	const FVector NewScale = FMath::Lerp(FromTransform.GetScale3D(), ToTransform.GetScale3D(), SafeAlpha);

	return FTransform(NewRotation, NewLocation, NewScale);
}

FTransform AUOUFloorPlatformActor::BuildHingeTransformBetween(
	const FTransform& FromTransform,
	const FTransform& ToTransform,
	float Alpha,
	EUOUFloorPlatformHingeEdge InHingeEdge,
	const FVector& InCustomHingeLocalOffset) const
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	FVector HingeLocalOffset = FVector::ZeroVector;
	if (!ResolveHingeLocalOffset(HingeLocalOffset, InHingeEdge, InCustomHingeLocalOffset))
	{
		const FVector NewLocation = EvaluatePathLocation(FromTransform, ToTransform, SafeAlpha);
		const FQuat NewRotation = FQuat::Slerp(FromTransform.GetRotation(), ToTransform.GetRotation(), SafeAlpha);
		const FVector NewScale = FMath::Lerp(FromTransform.GetScale3D(), ToTransform.GetScale3D(), SafeAlpha);
		return FTransform(NewRotation, NewLocation, NewScale);
	}

	const FQuat FromRotation = FromTransform.GetRotation();
	const FQuat NewRotation = FQuat::Slerp(FromRotation, ToTransform.GetRotation(), SafeAlpha).GetNormalized();
	const FQuat RotationDelta = NewRotation * FromRotation.Inverse();
	const FVector HingeWorldLocation = FromTransform.TransformPosition(HingeLocalOffset);
	const FVector FromLocationFromHinge = FromTransform.GetLocation() - HingeWorldLocation;
	const FVector NewLocation = HingeWorldLocation + RotationDelta.RotateVector(FromLocationFromHinge);
	const FVector NewScale = FMath::Lerp(FromTransform.GetScale3D(), ToTransform.GetScale3D(), SafeAlpha);

	return FTransform(NewRotation, NewLocation, NewScale);
}

bool AUOUFloorPlatformActor::ResolveHingeLocalOffset(
	FVector& OutHingeLocalOffset,
	EUOUFloorPlatformHingeEdge InHingeEdge,
	const FVector& InCustomHingeLocalOffset) const
{
	if (InHingeEdge == EUOUFloorPlatformHingeEdge::Custom)
	{
		OutHingeLocalOffset = InCustomHingeLocalOffset;
		return true;
	}

	if (PlatformMesh == nullptr || PlatformMesh->GetStaticMesh() == nullptr)
	{
		OutHingeLocalOffset = InCustomHingeLocalOffset;
		return !InCustomHingeLocalOffset.IsNearlyZero();
	}

	FVector MeshBoundsMin = FVector::ZeroVector;
	FVector MeshBoundsMax = FVector::ZeroVector;
	PlatformMesh->GetLocalBounds(MeshBoundsMin, MeshBoundsMax);

	FVector MeshLocalHingeOffset = (MeshBoundsMin + MeshBoundsMax) * 0.5f;
	switch (InHingeEdge)
	{
	case EUOUFloorPlatformHingeEdge::PositiveX:
		MeshLocalHingeOffset.X = MeshBoundsMax.X;
		break;
	case EUOUFloorPlatformHingeEdge::NegativeX:
		MeshLocalHingeOffset.X = MeshBoundsMin.X;
		break;
	case EUOUFloorPlatformHingeEdge::PositiveY:
		MeshLocalHingeOffset.Y = MeshBoundsMax.Y;
		break;
	case EUOUFloorPlatformHingeEdge::NegativeY:
		MeshLocalHingeOffset.Y = MeshBoundsMin.Y;
		break;
	case EUOUFloorPlatformHingeEdge::Custom:
	default:
		break;
	}

	// 메쉬가 루트에서 떨어져 있거나 스케일된 경우를 고려해 메쉬 로컬 위치를 액터 로컬 위치로 변환합니다.
	OutHingeLocalOffset = PlatformMesh->GetRelativeTransform().TransformPosition(MeshLocalHingeOffset);
	return true;
}

void AUOUFloorPlatformActor::ResolveMoveSettingsFromTarget(
	const AUOUFloorPlatformTargetActor* TargetMarker,
	EUOUFloorPlatformRotationMode& OutRotationMode,
	EUOUFloorPlatformHingeEdge& OutHingeEdge,
	FVector& OutCustomHingeLocalOffset) const
{
	OutRotationMode = RotationMode;
	OutHingeEdge = HingeEdge;
	OutCustomHingeLocalOffset = CustomHingeLocalOffset;

	if (TargetMarker == nullptr)
	{
		return;
	}

	OutRotationMode = TargetMarker->ResolveRotationMode(OutRotationMode);
	OutHingeEdge = TargetMarker->ResolveHingeEdge(OutHingeEdge);
	OutCustomHingeLocalOffset = TargetMarker->ResolveCustomHingeLocalOffset(OutCustomHingeLocalOffset);
}

void AUOUFloorPlatformActor::CacheActiveMoveSettings(const AUOUFloorPlatformTargetActor* TargetMarker)
{
	ResolveMoveSettingsFromTarget(
		TargetMarker,
		ActiveMoveRotationMode,
		ActiveMoveHingeEdge,
		ActiveMoveCustomHingeLocalOffset);
}

FVector AUOUFloorPlatformActor::EvaluatePathLocation(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha) const
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const FVector StartLocation = FromTransform.GetLocation();
	const FVector TargetLocation = ToTransform.GetLocation();

	if (PathMode != EUOUFloorPlatformPathMode::CubicBezier)
	{
		return FMath::Lerp(StartLocation, TargetLocation, SafeAlpha);
	}

	const FVector StartControlLocation = StartLocation + FromTransform.TransformVectorNoScale(BezierStartControlOffset);
	const FVector TargetControlLocation = TargetLocation + ToTransform.TransformVectorNoScale(BezierEndControlOffset);
	const float InverseAlpha = 1.0f - SafeAlpha;

	return (InverseAlpha * InverseAlpha * InverseAlpha * StartLocation)
		+ (3.0f * InverseAlpha * InverseAlpha * SafeAlpha * StartControlLocation)
		+ (3.0f * InverseAlpha * SafeAlpha * SafeAlpha * TargetControlLocation)
		+ (SafeAlpha * SafeAlpha * SafeAlpha * TargetLocation);
}

void AUOUFloorPlatformActor::SyncSequentialTargetMarkerPreviews()
{
	for (const TObjectPtr<AUOUFloorPlatformTargetActor>& SequentialTargetMarker : SequentialTargetMarkers)
	{
		if (!IsValid(SequentialTargetMarker))
		{
			continue;
		}

		SequentialTargetMarker->SyncPreviewFromMesh(PlatformMesh);
		SequentialTargetMarker->SetTargetPreviewMeshVisible(bShowTransformPreview);
	}
}

void AUOUFloorPlatformActor::UpdateEditorPreviewVisuals()
{
	SyncSequentialTargetMarkerPreviews();

	const FVector StartWorldLocation = StartTransform.GetLocation();
	TArray<FVector> PreviewPathPoints;
	PreviewPathPoints.Add(StartWorldLocation);

	auto AppendPreviewSegment = [this, &PreviewPathPoints](
		const FTransform& SegmentFromTransform,
		const FTransform& SegmentToTransform,
		const AUOUFloorPlatformTargetActor* TargetMarker) -> FTransform
	{
		EUOUFloorPlatformRotationMode ResolvedRotationMode;
		EUOUFloorPlatformHingeEdge ResolvedHingeEdge;
		FVector ResolvedCustomHingeLocalOffset;
		ResolveMoveSettingsFromTarget(TargetMarker, ResolvedRotationMode, ResolvedHingeEdge, ResolvedCustomHingeLocalOffset);

		constexpr int32 PreviewSampleCount = 8;
		const int32 SampleCount = (ResolvedRotationMode == EUOUFloorPlatformRotationMode::Hinge || PathMode == EUOUFloorPlatformPathMode::CubicBezier)
			? PreviewSampleCount
			: 1;

		for (int32 SampleIndex = 1; SampleIndex <= SampleCount; ++SampleIndex)
		{
			const float SampleAlpha = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
			PreviewPathPoints.Add(BuildTransformBetween(
				SegmentFromTransform,
				SegmentToTransform,
				SampleAlpha,
				ResolvedRotationMode,
				ResolvedHingeEdge,
				ResolvedCustomHingeLocalOffset).GetLocation());
		}

		return BuildTransformBetween(
			SegmentFromTransform,
			SegmentToTransform,
			1.0f,
			ResolvedRotationMode,
			ResolvedHingeEdge,
			ResolvedCustomHingeLocalOffset);
	};

	FTransform PreviewSegmentStartTransform = StartTransform;
	if (bUseSequentialTargetMarkers && SequentialTargetMarkers.Num() > 0)
	{
		for (const TObjectPtr<AUOUFloorPlatformTargetActor>& SequentialTargetMarker : SequentialTargetMarkers)
		{
			if (IsValid(SequentialTargetMarker))
			{
				const FTransform RawTargetTransform = SequentialTargetMarker->GetActorTransform();
				PreviewSegmentStartTransform = AppendPreviewSegment(PreviewSegmentStartTransform, RawTargetTransform, SequentialTargetMarker.Get());
			}
		}

		if (bLoopSequentialTargetMarkers && PreviewPathPoints.Num() > 1)
		{
			if (bLoopMoveStepsThroughStart)
			{
				AppendPreviewSegment(PreviewSegmentStartTransform, StartTransform, nullptr);
			}
			else
			{
				for (const TObjectPtr<AUOUFloorPlatformTargetActor>& SequentialTargetMarker : SequentialTargetMarkers)
				{
					if (IsValid(SequentialTargetMarker))
					{
						AppendPreviewSegment(PreviewSegmentStartTransform, SequentialTargetMarker->GetActorTransform(), SequentialTargetMarker.Get());
						break;
					}
				}
			}
		}
	}

	if (PreviewPathPoints.Num() == 1)
	{
		AppendPreviewSegment(StartTransform, TargetTransform, nullptr);
	}

	bool bHasMoveOffset = false;
	for (int32 PointIndex = 1; PointIndex < PreviewPathPoints.Num(); ++PointIndex)
	{
		if (!PreviewPathPoints[PointIndex].Equals(PreviewPathPoints[PointIndex - 1]))
		{
			bHasMoveOffset = true;
			break;
		}
	}

	if (MovePreviewPath != nullptr)
	{
		// 경로 미리보기는 플랫폼의 자식 컴포넌트지만 월드 기준으로 고정해 플랫폼 이동을 따라가지 않게 합니다.
		MovePreviewPath->SetWorldTransform(FTransform::Identity, false, nullptr, ETeleportType::TeleportPhysics);
		MovePreviewPath->SetVisibility(bShowMovePreviewPath && bHasMoveOffset, true);
		if (bHasMoveOffset)
		{
			// 시작점과 목표 지점을 실제 스플라인 선으로 이어서 경로를 명확하게 보여줍니다.
			MovePreviewPath->ClearSplinePoints(false);
			for (int32 PointIndex = 0; PointIndex < PreviewPathPoints.Num(); ++PointIndex)
			{
				MovePreviewPath->AddSplinePoint(PreviewPathPoints[PointIndex], ESplineCoordinateSpace::World, false);
				MovePreviewPath->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
			}
			MovePreviewPath->UpdateSpline();
		}
	}
}

float AUOUFloorPlatformActor::ResolveMoveAlpha(float RawAlpha) const
{
	const float SafeRawAlpha = FMath::Clamp(RawAlpha, 0.0f, 1.0f);

	if (MoveCurve == nullptr)
	{
		switch (EasingMode)
		{
		case EUOUFloorPlatformEasingMode::EaseIn:
			return FMath::InterpEaseIn(0.0f, 1.0f, SafeRawAlpha, EaseExponent);
		case EUOUFloorPlatformEasingMode::EaseOut:
			return FMath::InterpEaseOut(0.0f, 1.0f, SafeRawAlpha, EaseExponent);
		case EUOUFloorPlatformEasingMode::EaseInOut:
			return FMath::InterpEaseInOut(0.0f, 1.0f, SafeRawAlpha, EaseExponent);
		case EUOUFloorPlatformEasingMode::Linear:
		default:
			return SafeRawAlpha;
		}
	}

	return FMath::Clamp(MoveCurve->GetFloatValue(SafeRawAlpha), 0.0f, 1.0f);
}
