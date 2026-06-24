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
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"

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
	RefreshMovementBaseReferenceTransform();
	if (AUOUFloorPlatformActor* BasePlatform = GetMovementBasePlatformActor())
	{
		AddTickPrerequisiteActor(BasePlatform);
	}
	ResetRuntimeStepIndex();
	PendingSequentialMoveCount = 0;
	SetActorTransform(ResolveAuthoredWorldTransformForMovementBase(StartTransform), false, nullptr, ETeleportType::TeleportPhysics);
	RefreshCurrentMovementBaseRelativeTransform();
	if (CarryComponent != nullptr)
	{
		CarryComponent->AttachPermanentCarriedActors();
	}

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
	ReleasePlayerInputBlockForMove();

	if (CarryComponent != nullptr)
	{
		CarryComponent->DetachCarriedActors();
		CarryComponent->DetachPermanentCarriedActors();
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
	RefreshMovementBaseReferenceTransform();

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
		ApplyMovementBaseIdleTransform();
		DrawRuntimeMoveDebug();
		return;
	}

	MoveElapsedTime += DeltaSeconds;

	const float SafeDuration = FMath::Max(UE_KINDA_SMALL_NUMBER, MoveDuration);
	const float RawAlpha = FMath::Clamp(MoveElapsedTime / SafeDuration, 0.0f, 1.0f);
	const float MoveAlpha = ResolveMoveAlpha(RawAlpha);

	const FTransform NextPlatformTransform = BuildActivePlatformTransformAtAlpha(MoveAlpha);
	SetActorTransform(NextPlatformTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CurrentMovementBaseRelativeTransform = ConvertWorldToMovementBaseRelativeTransform(NextPlatformTransform, false);
	MoveTargetTransform = ConvertMovementBaseRelativeToWorldTransform(MoveTargetMovementBaseRelativeTransform);
	DrawRuntimeMoveDebug();

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
	RefreshMovementBaseReferenceTransform();
	RefreshCurrentMovementBaseRelativeTransform();

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
	ReleasePlayerInputBlockForMove();

	EnsureStartTransform();
	RefreshMovementBaseReferenceTransform();

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
	ActiveMoveTargetIndex = INDEX_NONE;
	LastArrivedMoveStepIndex = INDEX_NONE;
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, false);
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Deactivate, true);
	ResetRuntimeStepIndex();
	if (StepComponent != nullptr)
	{
		StepComponent->ResetRuntimeState();
	}
	const FTransform ResetStartTransform = ResolveAuthoredWorldTransformForMovementBase(StartTransform);
	MoveStartTransform = ResetStartTransform;
	MoveTargetTransform = TargetTransform;
	MoveStartMovementBaseRelativeTransform = ConvertWorldToMovementBaseRelativeTransform(ResetStartTransform, false);
	MoveTargetMovementBaseRelativeTransform = ConvertWorldToMovementBaseRelativeTransform(TargetTransform, false);

#if WITH_EDITOR
	bIsApplyingEditorTransform = true;
#endif
	SetActorTransform(ResetStartTransform, false, nullptr, ETeleportType::TeleportPhysics);
	RefreshCurrentMovementBaseRelativeTransform();
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
	ReleasePlayerInputBlockForMove();

	EnsureStartTransform();
	RefreshMovementBaseReferenceTransform();

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

	RefreshCurrentMovementBaseRelativeTransform();
	AUOUFloorPlatformTargetActor* SnapTargetMarker = GetSequentialTargetMarkerAt(SnapTargetIndex);
	const FTransform ResolvedSnapTargetTransform = ResolveMoveTargetWorldTransform(SnapTargetTransform, SnapTargetMarker);

	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;
	PendingSequentialMoveCount = 0;
	ActiveMoveTargetMarker = nullptr;
	ActiveMoveTargetIndex = INDEX_NONE;
	LastArrivedMoveStepIndex = SnapTargetIndex;
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, true);
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Deactivate, false);
	if (StepComponent != nullptr)
	{
		StepComponent->ClearReturnToStartRequest();
		StepComponent->SetActiveTargetIndex(SnapTargetIndex);
	}
	MoveStartTransform = GetActorTransform();
	MoveTargetTransform = ResolvedSnapTargetTransform;
	MoveStartMovementBaseRelativeTransform = ConvertWorldToMovementBaseRelativeTransform(MoveStartTransform, false);
	MoveTargetMovementBaseRelativeTransform = ConvertWorldToMovementBaseRelativeTransform(MoveTargetTransform, false);

#if WITH_EDITOR
	bIsApplyingEditorTransform = true;
#endif
	SetActorTransform(ResolvedSnapTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	RefreshCurrentMovementBaseRelativeTransform();
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

int32 AUOUFloorPlatformActor::GetLastArrivedMoveStepIndex() const
{
	return LastArrivedMoveStepIndex;
}

bool AUOUFloorPlatformActor::IsAtMoveStepIndex(int32 StepIndex, bool bRequireNotMoving) const
{
	if (StepIndex == INDEX_NONE || LastArrivedMoveStepIndex != StepIndex)
	{
		return false;
	}

	return !bRequireNotMoving || !bIsMoving;
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

bool AUOUFloorPlatformActor::IsPuzzleResultCompleted_Implementation(EOUUPuzzleResultAction Action) const
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		return bActivateResultCompleted;
	case EOUUPuzzleResultAction::Deactivate:
		return bDeactivateResultCompleted;
	case EOUUPuzzleResultAction::Toggle:
		return bActivateResultCompleted || bDeactivateResultCompleted;
	case EOUUPuzzleResultAction::None:
	case EOUUPuzzleResultAction::Pause:
	case EOUUPuzzleResultAction::Resume:
	default:
		return false;
	}
}

FOnUOUPuzzleResultCompletionStateChangedNativeSignature*
AUOUFloorPlatformActor::GetPuzzleResultCompletionStateChangedEvent()
{
	return &OnPuzzleResultCompletionStateChanged;
}

void AUOUFloorPlatformActor::SetPuzzleResultCompletionState(
	EOUUPuzzleResultAction Action,
	bool bNewCompleted)
{
	bool* TargetState = nullptr;

	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		TargetState = &bActivateResultCompleted;
		break;
	case EOUUPuzzleResultAction::Deactivate:
		TargetState = &bDeactivateResultCompleted;
		break;
	case EOUUPuzzleResultAction::None:
	case EOUUPuzzleResultAction::Pause:
	case EOUUPuzzleResultAction::Resume:
	case EOUUPuzzleResultAction::Toggle:
	default:
		return;
	}

	if (*TargetState == bNewCompleted)
	{
		return;
	}

	*TargetState = bNewCompleted;
	OnPuzzleResultCompletionStateChanged.Broadcast(Action, bNewCompleted);
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
	TargetTransform = ResolveAuthoredWorldTransformForMovementBase(TargetTransform);
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
	LastArrivedMoveStepIndex = ActiveMoveTargetIndex;
	ActiveMoveTargetIndex = INDEX_NONE;

	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, true);
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Deactivate, false);

#if WITH_EDITOR
	bIsApplyingEditorTransform = true;
#endif
	SetActorTransform(MoveTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	RefreshCurrentMovementBaseRelativeTransform();
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
	const bool bStartedNextMove = TryStartQueuedSequentialMove();
	if (!bStartedNextMove && !bIsMoving && PendingSequentialMoveCount <= 0)
	{
		ReleasePlayerInputBlockForMove();
	}
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

bool AUOUFloorPlatformActor::BeginMoveToTransform(
	const FTransform& InTargetTransform,
	AUOUFloorPlatformTargetActor* TargetMarker,
	int32 TargetStepIndex)
{
	if (bIsMoving)
	{
		return false;
	}

	CacheActiveMoveSettings(TargetMarker);
	ActiveMoveTargetMarker = TargetMarker;
	ActiveMoveTargetIndex = TargetStepIndex;

	RefreshCurrentMovementBaseRelativeTransform();
	MoveStartTransform = GetActorTransform();
	MoveStartMovementBaseRelativeTransform = CurrentMovementBaseRelativeTransform;
	MoveTargetMovementBaseRelativeTransform = ResolveMoveTargetRelativeTransform(InTargetTransform, TargetMarker);
	MoveTargetTransform = ConvertMovementBaseRelativeToWorldTransform(MoveTargetMovementBaseRelativeTransform);

	bIsMoving = true;
	bIsAtTarget = false;
	MoveElapsedTime = 0.0f;
	RequestPlayerInputBlockForMove();
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, false);
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Deactivate, false);
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

	AUOUFloorPlatformTargetActor* NextTargetMarker = GetSequentialTargetMarkerAt(NextTargetIndex);
	RefreshCurrentMovementBaseRelativeTransform();
	if (GetActorTransform().Equals(ResolveMoveTargetWorldTransform(NextTargetTransform, NextTargetMarker)))
	{
		PendingSequentialMoveCount = 0;
		LastArrivedMoveStepIndex = NextTargetIndex;
		SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, true);
		OnMoveFinished.Broadcast(this);
		return false;
	}

	if (StepComponent != nullptr)
	{
		StepComponent->ClearReturnToStartRequest();
		StepComponent->SetActiveTargetIndex(NextTargetIndex);
	}

	--PendingSequentialMoveCount;
	return BeginMoveToTransform(NextTargetTransform, NextTargetMarker, NextTargetIndex);
}

void AUOUFloorPlatformActor::RequestPlayerInputBlockForMove()
{
	if (LockedInputExecutorComponent != nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld())
	{
		return;
	}

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	if (PlayerPawn == nullptr)
	{
		return;
	}

	if (!ShouldBlockPlayerInputForMove(PlayerPawn))
	{
		return;
	}

	UUOUPlayerInteractionExecutorComponent* InputExecutor =
		UUOUPlayerInteractionExecutorComponent::FindLocalPlayerExecutor(this);
	if (InputExecutor == nullptr)
	{
		return;
	}

	InputExecutor->RequestPlayerInputBlock(this, true);
	LockedInputExecutorComponent = InputExecutor;
}

void AUOUFloorPlatformActor::ReleasePlayerInputBlockForMove()
{
	if (LockedInputExecutorComponent == nullptr)
	{
		return;
	}

	LockedInputExecutorComponent->ReleasePlayerInputBlock(this);
	LockedInputExecutorComponent = nullptr;
}

bool AUOUFloorPlatformActor::ShouldBlockPlayerInputForMove(const APawn* PlayerPawn) const
{
	switch (InputBlockPolicyDuringMove)
	{
	case EUOUFloorPlatformInputBlockPolicy::Always:
		return true;
	case EUOUFloorPlatformInputBlockPolicy::WhenPlayerOnPlatform:
		return IsPlayerOnPlatformForInputBlock(PlayerPawn);
	case EUOUFloorPlatformInputBlockPolicy::Never:
	default:
		return false;
	}
}

bool AUOUFloorPlatformActor::IsPlayerOnPlatformForInputBlock(const APawn* PlayerPawn) const
{
	if (PlayerPawn == nullptr || CarryDetectionBox == nullptr)
	{
		return false;
	}

	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	const FTransform BoxTransform = CarryDetectionBox->GetComponentTransform();
	const FVector LocalPlayerLocation = BoxTransform.InverseTransformPosition(PlayerLocation);
	const FVector BoxExtent = CarryDetectionBox->GetUnscaledBoxExtent();

	return FMath::Abs(LocalPlayerLocation.X) <= BoxExtent.X
		&& FMath::Abs(LocalPlayerLocation.Y) <= BoxExtent.Y
		&& FMath::Abs(LocalPlayerLocation.Z) <= BoxExtent.Z;
}

FTransform AUOUFloorPlatformActor::BuildActivePlatformTransformAtAlpha(float Alpha) const
{
	if (GetMovementBasePlatformActor() != nullptr)
	{
		const FTransform RelativeTransform = BuildTransformBetween(
			MoveStartMovementBaseRelativeTransform,
			MoveTargetMovementBaseRelativeTransform,
			Alpha,
			ActiveMoveRotationMode,
			ActiveMoveHingeEdge,
			ActiveMoveCustomHingeLocalOffset);
		return ConvertMovementBaseRelativeToWorldTransform(RelativeTransform);
	}

	return BuildTransformBetween(
		MoveStartTransform,
		MoveTargetTransform,
		Alpha,
		ActiveMoveRotationMode,
		ActiveMoveHingeEdge,
		ActiveMoveCustomHingeLocalOffset);
}

AUOUFloorPlatformActor* AUOUFloorPlatformActor::GetMovementBasePlatformActor() const
{
	AUOUFloorPlatformActor* BasePlatform = MovementBasePlatform.Get();
	return IsValid(BasePlatform) && BasePlatform != this ? BasePlatform : nullptr;
}

void AUOUFloorPlatformActor::RefreshMovementBaseReferenceTransform()
{
	AUOUFloorPlatformActor* BasePlatform = GetMovementBasePlatformActor();
	if (BasePlatform == nullptr)
	{
		bHasMovementBaseReferenceTransform = false;
		MovementBaseReferenceTransform = FTransform::Identity;
		CurrentMovementBaseRelativeTransform = GetActorTransform();
		return;
	}

	BasePlatform->EnsureStartTransform();
	MovementBaseReferenceTransform = BasePlatform->StartTransform;
	bHasMovementBaseReferenceTransform = true;
	CurrentMovementBaseRelativeTransform = ConvertWorldToMovementBaseRelativeTransform(GetActorTransform(), false);
}

void AUOUFloorPlatformActor::RefreshCurrentMovementBaseRelativeTransform()
{
	CurrentMovementBaseRelativeTransform = ConvertWorldToMovementBaseRelativeTransform(GetActorTransform(), false);
}

void AUOUFloorPlatformActor::ApplyMovementBaseIdleTransform()
{
	if (GetMovementBasePlatformActor() == nullptr)
	{
		return;
	}

	SetActorTransform(
		ConvertMovementBaseRelativeToWorldTransform(CurrentMovementBaseRelativeTransform),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
}

FTransform AUOUFloorPlatformActor::ConvertWorldToMovementBaseRelativeTransform(
	const FTransform& WorldTransform,
	bool bUseReferenceTransform) const
{
	AUOUFloorPlatformActor* BasePlatform = GetMovementBasePlatformActor();
	if (BasePlatform == nullptr)
	{
		return WorldTransform;
	}

	const FTransform BaseTransform = bUseReferenceTransform && bHasMovementBaseReferenceTransform
		? MovementBaseReferenceTransform
		: BasePlatform->GetActorTransform();
	return WorldTransform.GetRelativeTransform(BaseTransform);
}

FTransform AUOUFloorPlatformActor::ConvertMovementBaseRelativeToWorldTransform(const FTransform& RelativeTransform) const
{
	AUOUFloorPlatformActor* BasePlatform = GetMovementBasePlatformActor();
	if (BasePlatform == nullptr)
	{
		return RelativeTransform;
	}

	return RelativeTransform * BasePlatform->GetActorTransform();
}

FTransform AUOUFloorPlatformActor::ResolveAuthoredWorldTransformForMovementBase(const FTransform& AuthoredWorldTransform) const
{
	if (GetMovementBasePlatformActor() == nullptr)
	{
		return AuthoredWorldTransform;
	}

	return ConvertMovementBaseRelativeToWorldTransform(
		ConvertWorldToMovementBaseRelativeTransform(AuthoredWorldTransform, true));
}

FTransform AUOUFloorPlatformActor::ResolveMoveTargetRelativeTransform(
	const FTransform& TargetWorldTransform,
	const AUOUFloorPlatformTargetActor* TargetMarker) const
{
	EUOUFloorPlatformRotationMode ResolvedRotationMode;
	EUOUFloorPlatformHingeEdge ResolvedHingeEdge;
	FVector ResolvedCustomHingeLocalOffset;
	ResolveMoveSettingsFromTarget(TargetMarker, ResolvedRotationMode, ResolvedHingeEdge, ResolvedCustomHingeLocalOffset);

	const FTransform TargetRelativeTransform = ConvertWorldToMovementBaseRelativeTransform(TargetWorldTransform, true);
	return BuildTransformBetween(
		CurrentMovementBaseRelativeTransform,
		TargetRelativeTransform,
		1.0f,
		ResolvedRotationMode,
		ResolvedHingeEdge,
		ResolvedCustomHingeLocalOffset);
}

FTransform AUOUFloorPlatformActor::ResolveMoveTargetWorldTransform(
	const FTransform& TargetWorldTransform,
	const AUOUFloorPlatformTargetActor* TargetMarker) const
{
	const FTransform ResolvedRelativeTransform = ResolveMoveTargetRelativeTransform(TargetWorldTransform, TargetMarker);
	return ConvertMovementBaseRelativeToWorldTransform(ResolvedRelativeTransform);
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

void AUOUFloorPlatformActor::DrawRuntimeMoveDebug() const
{
	if (!bDrawRuntimeMoveDebug)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld())
	{
		return;
	}

	const float DrawDuration = FMath::Max(0.0f, RuntimeDebugDrawDuration);
	constexpr float LineThickness = 3.0f;
	constexpr float MarkerRadius = 24.0f;
	const FVector DrawOffset = RuntimeDebugDrawOffset;
	const FVector PlatformDrawLocation = GetActorLocation() + DrawOffset;

	auto DrawMovePathFromRelative = [this, World, DrawDuration, DrawOffset](
		const FTransform& FromRelativeTransform,
		const FTransform& ToRelativeTransform,
		EUOUFloorPlatformRotationMode InRotationMode,
		EUOUFloorPlatformHingeEdge InHingeEdge,
		const FVector& InCustomHingeLocalOffset,
		const FColor& PathColor)
	{
		constexpr float PathThickness = 2.0f;
		const int32 SampleCount = (InRotationMode == EUOUFloorPlatformRotationMode::Hinge || PathMode == EUOUFloorPlatformPathMode::CubicBezier)
			? 12
			: 1;

		FVector PreviousPoint = ConvertMovementBaseRelativeToWorldTransform(FromRelativeTransform).GetLocation() + DrawOffset;
		for (int32 SampleIndex = 1; SampleIndex <= SampleCount; ++SampleIndex)
		{
			const float SampleAlpha = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
			const FTransform NextRelativeTransform = BuildTransformBetween(
				FromRelativeTransform,
				ToRelativeTransform,
				SampleAlpha,
				InRotationMode,
				InHingeEdge,
				InCustomHingeLocalOffset);
			const FVector NextPoint = ConvertMovementBaseRelativeToWorldTransform(NextRelativeTransform).GetLocation() + DrawOffset;

			DrawDebugLine(World, PreviousPoint, NextPoint, PathColor, false, DrawDuration, 0, PathThickness);
			PreviousPoint = NextPoint;
		}
	};

	DrawDebugSphere(World, PlatformDrawLocation, 18.0f, 12, FColor::Cyan, false, DrawDuration, 0, LineThickness);
	DrawDebugString(
		World,
		PlatformDrawLocation + FVector(0.0f, 0.0f, 70.0f),
		FString::Printf(
			TEXT("%s | Moving:%s Pending:%d RuntimeIndex:%d"),
			*GetNameSafe(this),
			bIsMoving ? TEXT("Y") : TEXT("N"),
			PendingSequentialMoveCount,
			RuntimeSequentialTargetIndex),
		nullptr,
		FColor::White,
		DrawDuration,
		true);

	const FTransform RuntimeStartTransform = ResolveAuthoredWorldTransformForMovementBase(StartTransform);
	DrawDebugSphere(World, RuntimeStartTransform.GetLocation() + DrawOffset, MarkerRadius, 12, FColor::Blue, false, DrawDuration, 0, LineThickness);
	DrawDebugString(
		World,
		RuntimeStartTransform.GetLocation() + DrawOffset + FVector(0.0f, 0.0f, 45.0f),
		TEXT("Start"),
		nullptr,
		FColor::Blue,
		DrawDuration,
		true);

	for (int32 TargetIndex = 0; TargetIndex < SequentialTargetMarkers.Num(); ++TargetIndex)
	{
		const AUOUFloorPlatformTargetActor* TargetMarker = SequentialTargetMarkers[TargetIndex].Get();
		if (!IsValid(TargetMarker))
		{
			continue;
		}

		const bool bIsActiveTarget = ActiveMoveTargetMarker.Get() == TargetMarker;
		const bool bIsRuntimeNextTarget = !bIsMoving && TargetIndex == RuntimeSequentialTargetIndex;
		const FColor MarkerColor = bIsActiveTarget
			? FColor::Green
			: bIsRuntimeNextTarget
				? FColor::Yellow
				: FColor(160, 160, 160);
		const FTransform MarkerDrawTransform = ResolveAuthoredWorldTransformForMovementBase(TargetMarker->GetActorTransform());
		const FVector MarkerLocation = MarkerDrawTransform.GetLocation();

		DrawDebugSphere(World, MarkerLocation + DrawOffset, MarkerRadius, 16, MarkerColor, false, DrawDuration, 0, LineThickness);
		DrawDebugCoordinateSystem(World, MarkerLocation, MarkerDrawTransform.GetRotation().Rotator(), 80.0f, false, DrawDuration, 0, 1.5f);
		DrawDebugString(
			World,
			MarkerLocation + DrawOffset + FVector(0.0f, 0.0f, 45.0f),
			FString::Printf(TEXT("Step[%d] %s"), TargetIndex, *GetNameSafe(TargetMarker)),
			nullptr,
			MarkerColor,
			DrawDuration,
			true);
	}

	if (bIsMoving)
	{
		DrawMovePathFromRelative(
			MoveStartMovementBaseRelativeTransform,
			MoveTargetMovementBaseRelativeTransform,
			ActiveMoveRotationMode,
			ActiveMoveHingeEdge,
			ActiveMoveCustomHingeLocalOffset,
			FColor::Green);

		const FVector TargetDrawLocation = MoveTargetTransform.GetLocation() + DrawOffset;
		const FString ActiveTargetLabel = ActiveMoveTargetMarker.Get() != nullptr
			? GetNameSafe(ActiveMoveTargetMarker.Get())
			: TEXT("Start");
		DrawDebugDirectionalArrow(World, PlatformDrawLocation, TargetDrawLocation, 100.0f, FColor::Orange, false, DrawDuration, 0, LineThickness);
		DrawDebugSphere(World, TargetDrawLocation, MarkerRadius * 1.25f, 16, FColor::Orange, false, DrawDuration, 0, LineThickness);
		DrawDebugString(
			World,
			TargetDrawLocation + FVector(0.0f, 0.0f, 80.0f),
			FString::Printf(TEXT("Active Target: %s"), *ActiveTargetLabel),
			nullptr,
			FColor::Orange,
			DrawDuration,
			true);
		return;
	}

	FTransform NextTargetTransform;
	int32 NextTargetIndex = INDEX_NONE;
	if (ResolveNextSequentialTargetTransform(NextTargetTransform, NextTargetIndex))
	{
		EUOUFloorPlatformRotationMode NextRotationMode;
		EUOUFloorPlatformHingeEdge NextHingeEdge;
		FVector NextCustomHingeLocalOffset;
		const AUOUFloorPlatformTargetActor* NextTargetMarker = GetSequentialTargetMarkerAt(NextTargetIndex);
		const FString NextTargetLabel = NextTargetMarker != nullptr
			? FString::Printf(TEXT("Step[%d] %s"), NextTargetIndex, *GetNameSafe(NextTargetMarker))
			: TEXT("Start");
		ResolveMoveSettingsFromTarget(NextTargetMarker, NextRotationMode, NextHingeEdge, NextCustomHingeLocalOffset);

		const FTransform CurrentTransform = GetActorTransform();
		const FTransform CurrentRelativeTransform = ConvertWorldToMovementBaseRelativeTransform(CurrentTransform, false);
		const FTransform NextTargetRelativeTransform = ResolveMoveTargetRelativeTransform(NextTargetTransform, NextTargetMarker);
		const FTransform ResolvedNextTransform = ConvertMovementBaseRelativeToWorldTransform(NextTargetRelativeTransform);

		DrawMovePathFromRelative(
			CurrentRelativeTransform,
			NextTargetRelativeTransform,
			NextRotationMode,
			NextHingeEdge,
			NextCustomHingeLocalOffset,
			FColor::Yellow);

		DrawDebugDirectionalArrow(
			World,
			PlatformDrawLocation,
			ResolvedNextTransform.GetLocation() + DrawOffset,
			100.0f,
			FColor::Yellow,
			false,
			DrawDuration,
			0,
			LineThickness);
		DrawDebugString(
			World,
			ResolvedNextTransform.GetLocation() + DrawOffset + FVector(0.0f, 0.0f, 80.0f),
			FString::Printf(TEXT("Next Target: %s"), *NextTargetLabel),
			nullptr,
			FColor::Yellow,
			DrawDuration,
			true);
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
