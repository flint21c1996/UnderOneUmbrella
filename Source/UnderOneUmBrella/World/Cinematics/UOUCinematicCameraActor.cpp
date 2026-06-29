// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Cinematics/UOUCinematicCameraActor.h"

#include "World/Cinematics/UOUCinematicCameraTargetActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"

AUOUCinematicCameraActor::AUOUCinematicCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	RootScene->SetMobility(EComponentMobility::Movable);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootScene);
	CameraComponent->SetMobility(EComponentMobility::Movable);
	CameraComponent->SetProjectionMode(ProjectionMode);
	CameraComponent->SetOrthoWidth(OrthographicWidth);
	CameraComponent->SetFieldOfView(FieldOfView);
	CameraComponent->AspectRatio = AspectRatio;
	CameraComponent->bConstrainAspectRatio = bConstrainAspectRatio;

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
	MovePreviewPath->EditorUnselectedSplineSegmentColor = FLinearColor::FromSRGBColor(FColor(0, 200, 120));
	MovePreviewPath->EditorSelectedSplineSegmentColor = FLinearColor::FromSRGBColor(FColor(0, 200, 120));
	MovePreviewPath->EditorTangentColor = FLinearColor::FromSRGBColor(FColor(0, 200, 120));
	MovePreviewPath->bShouldVisualizeScale = true;
	MovePreviewPath->ScaleVisualizationWidth = 8.0f;
#endif
	MovePreviewPath->UpdateSpline();
}

void AUOUCinematicCameraActor::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	EnsureStartTransform();
	ResetRuntimeStepIndex();

	if (bKeepActorAtSavedStartInEditor && bUseSavedStartTransform)
	{
		bIsApplyingEditorTransform = true;
		SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
		bIsApplyingEditorTransform = false;
	}

	ApplyDefaultCameraSettings();
#endif
}

void AUOUCinematicCameraActor::BeginPlay()
{
	Super::BeginPlay();

	EnsureStartTransform();
	ResetRuntimeStepIndex();
	PendingCameraMoveCount = 0;
	bNextMoveReturnsToStart = false;
	bIsMoving = false;
	bIsPaused = false;
	bIsAtTarget = false;
	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyDefaultCameraSettings();
}

void AUOUCinematicCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopActiveMove(false);
	FinishCameraViewTarget();

	Super::EndPlay(EndPlayReason);
}

void AUOUCinematicCameraActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	EnsureStartTransform();
	RefreshTargetTransforms();
	ApplyDefaultCameraSettings();
	UpdateEditorPreviewVisuals();
}

void AUOUCinematicCameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITOR
	if (!bIsMoving && GetWorld() != nullptr && !GetWorld()->IsGameWorld())
	{
		RefreshTargetTransforms();
		UpdateEditorPreviewVisuals();
		return;
	}
#endif

	if (!bIsMoving || bIsPaused)
	{
		return;
	}

	MoveElapsedTime += DeltaSeconds;

	const float SafeDuration = FMath::Max(UE_KINDA_SMALL_NUMBER, ActiveMoveDuration);
	const float RawAlpha = FMath::Clamp(MoveElapsedTime / SafeDuration, 0.0f, 1.0f);
	const float MoveAlpha = ResolveMoveAlpha(RawAlpha);

	const FTransform NextCameraTransform = BuildActiveCameraTransformAtAlpha(MoveAlpha);
	SetActorTransform(NextCameraTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyCameraSettingsAtAlpha(MoveAlpha);

	if (RawAlpha >= 1.0f)
	{
		FinishMoveToTarget();
	}
}

void AUOUCinematicCameraActor::CaptureCurrentAsStart()
{
	StartTransform = GetActorTransform();
	SavedStartTransform = StartTransform;
	bUseSavedStartTransform = true;
	bHasCapturedStartTransform = true;
	ResetRuntimeStepIndex();

	RefreshTargetTransforms();
	UpdateEditorPreviewVisuals();
}

void AUOUCinematicCameraActor::ApplyDefaultCameraSettings()
{
	ApplyCameraSettings(BuildDefaultCameraSettings());
	SyncStepMarkerPreviews();
}

void AUOUCinematicCameraActor::PlayCameraMove()
{
	RequestCameraMoveSteps(MoveStepCountPerActivate);
}

void AUOUCinematicCameraActor::MoveToNextCameraStep()
{
	RequestCameraMoveSteps(1);
}

void AUOUCinematicCameraActor::ResetCameraMove()
{
	const bool bWasActive = bIsMoving || bIsPaused || bViewTargetActive;
	StopActiveMove(bWasActive);

	EnsureStartTransform();
	RefreshTargetTransforms();

	bIsMoving = false;
	bIsPaused = false;
	bIsAtTarget = false;
	MoveElapsedTime = 0.0f;
	ActiveMoveDuration = 0.0f;
	PendingCameraMoveCount = 0;
	ActiveMoveTargetMarker = nullptr;
	ActiveMoveTargetIndex = INDEX_NONE;
	LastArrivedCameraStepIndex = INDEX_NONE;
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, false);
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Deactivate, true);
	ResetRuntimeStepIndex();

#if WITH_EDITOR
	bIsApplyingEditorTransform = true;
#endif
	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
#if WITH_EDITOR
	bIsApplyingEditorTransform = false;
#endif
	ApplyDefaultCameraSettings();
	UpdateEditorPreviewVisuals();
}

void AUOUCinematicCameraActor::SnapToTarget()
{
	StopActiveMove(false);

	EnsureStartTransform();
	RefreshTargetTransforms();

	FTransform SnapTargetTransform = FTransform::Identity;
	int32 SnapTargetIndex = INDEX_NONE;
	if (!ResolveNextCameraTargetTransform(SnapTargetTransform, SnapTargetIndex))
	{
		return;
	}

	AUOUCinematicCameraTargetActor* SnapTargetMarker = GetCameraStepMarkerAt(SnapTargetIndex);
	const FUOUCinematicCameraResolvedSettings SnapCameraSettings = ResolveCameraSettingsFromTarget(SnapTargetMarker);

	bIsMoving = false;
	bIsPaused = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;
	PendingCameraMoveCount = 0;
	ActiveMoveTargetMarker = nullptr;
	ActiveMoveTargetIndex = SnapTargetIndex;
	LastArrivedCameraStepIndex = SnapTargetIndex;
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, true);
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Deactivate, false);
	MoveStartTransform = GetActorTransform();
	MoveTargetTransform = SnapTargetTransform;
	MoveStartCameraSettings = ReadCurrentCameraSettings();
	MoveTargetCameraSettings = SnapCameraSettings;

#if WITH_EDITOR
	bIsApplyingEditorTransform = true;
#endif
	SetActorTransform(SnapTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
#if WITH_EDITOR
	bIsApplyingEditorTransform = false;
#endif
	ApplyCameraSettings(SnapCameraSettings);
	bNextMoveReturnsToStart = false;
	AdvanceCameraStepIndex();
	UpdateEditorPreviewVisuals();
}

void AUOUCinematicCameraActor::PauseCameraMove()
{
	if (!bIsMoving)
	{
		return;
	}

	bIsPaused = true;
}

void AUOUCinematicCameraActor::ResumeCameraMove()
{
	if (!bIsMoving)
	{
		return;
	}

	bIsPaused = false;
}

void AUOUCinematicCameraActor::AddCameraStepMarker()
{
#if WITH_EDITOR
	if (GetWorld() == nullptr)
	{
		return;
	}

	EnsureStartTransform();
	RefreshTargetTransforms();

	FTransform NewTargetTransform = TargetTransform;
	for (int32 MarkerIndex = CameraStepMarkers.Num() - 1; MarkerIndex >= 0; --MarkerIndex)
	{
		if (IsValid(CameraStepMarkers[MarkerIndex]))
		{
			NewTargetTransform = CameraStepMarkers[MarkerIndex]->GetActorTransform();
			break;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.Name = MakeUniqueObjectName(
		GetWorld()->GetCurrentLevel(),
		AUOUCinematicCameraTargetActor::StaticClass(),
		*FString::Printf(TEXT("%s_CameraStep"), *GetName()));

	AUOUCinematicCameraTargetActor* NewTargetMarker = GetWorld()->SpawnActor<AUOUCinematicCameraTargetActor>(
		AUOUCinematicCameraTargetActor::StaticClass(),
		NewTargetTransform,
		SpawnParameters);

	if (NewTargetMarker == nullptr)
	{
		return;
	}

	NewTargetMarker->SetActorLabel(FString::Printf(TEXT("%s_CameraStep_%d"), *GetActorLabel(), CameraStepMarkers.Num()));
	const FUOUCinematicCameraResolvedSettings DefaultCameraSettings = BuildDefaultCameraSettings();
	NewTargetMarker->SyncPreviewCamera(
		DefaultCameraSettings.ProjectionMode,
		DefaultCameraSettings.OrthographicWidth,
		DefaultCameraSettings.FieldOfView,
		DefaultCameraSettings.AspectRatio,
		DefaultCameraSettings.bConstrainAspectRatio);
	NewTargetMarker->SetTargetCameraPreviewVisible(bShowTargetCameraPreviews);

	CameraStepMarkers.Add(NewTargetMarker);
	bUseCameraSteps = true;
	InitialCameraStepIndex = FMath::Clamp(InitialCameraStepIndex, 0, FMath::Max(0, CameraStepMarkers.Num() - 1));
	ResetRuntimeStepIndex();

	RefreshTargetTransforms();
	UpdateEditorPreviewVisuals();
#endif
}

void AUOUCinematicCameraActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		PlayCameraMove();
		break;
	case EOUUPuzzleResultAction::Deactivate:
		ResetCameraMove();
		break;
	case EOUUPuzzleResultAction::Pause:
		PauseCameraMove();
		break;
	case EOUUPuzzleResultAction::Resume:
		ResumeCameraMove();
		break;
	case EOUUPuzzleResultAction::Toggle:
		if (bIsMoving)
		{
			ResetCameraMove();
		}
		else if (bIsAtTarget)
		{
			ResetCameraMove();
		}
		else
		{
			PlayCameraMove();
		}
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

bool AUOUCinematicCameraActor::IsPuzzleResultCompleted_Implementation(EOUUPuzzleResultAction Action) const
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
AUOUCinematicCameraActor::GetPuzzleResultCompletionStateChangedEvent()
{
	return &OnPuzzleResultCompletionStateChanged;
}

#if WITH_EDITOR
bool AUOUCinematicCameraActor::ShouldTickIfViewportsOnly() const
{
	return bIsMoving || bUseCameraSteps;
}

void AUOUCinematicCameraActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!bIsMoving)
	{
		ApplyDefaultCameraSettings();
		RefreshTargetTransforms();
		UpdateEditorPreviewVisuals();
	}
}
#endif

void AUOUCinematicCameraActor::EnsureStartTransform()
{
	if (bUseSavedStartTransform)
	{
		StartTransform = SavedStartTransform;
		bHasCapturedStartTransform = true;
		return;
	}

	StartTransform = GetActorTransform();
	bHasCapturedStartTransform = true;
}

void AUOUCinematicCameraActor::RefreshTargetTransforms()
{
	EnsureStartTransform();

	TargetTransform = StartTransform;
	if (AUOUCinematicCameraTargetActor* CameraStepMarker = GetCurrentCameraStepMarker())
	{
		TargetTransform = CameraStepMarker->GetActorTransform();
	}
}

void AUOUCinematicCameraActor::ResetRuntimeStepIndex()
{
	const int32 MaxTargetIndex = FMath::Max(0, CameraStepMarkers.Num() - 1);
	RuntimeCameraStepIndex = FMath::Clamp(InitialCameraStepIndex, 0, MaxTargetIndex);
	bNextMoveReturnsToStart = false;
}

void AUOUCinematicCameraActor::FinishMoveToTarget()
{
	const bool bShouldContinueFromArrivedMarker = ActiveMoveTargetMarker != nullptr
		&& ActiveMoveTargetMarker->bContinueToNextStepOnArrival;
	const int32 ArrivedCameraStepIndex = ActiveMoveTargetIndex;

	ActiveMoveTargetMarker = nullptr;
	LastArrivedCameraStepIndex = ArrivedCameraStepIndex;

	bIsMoving = false;
	bIsPaused = false;
	bIsAtTarget = true;
	MoveElapsedTime = ActiveMoveDuration;
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, true);
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Deactivate, false);

#if WITH_EDITOR
	bIsApplyingEditorTransform = true;
#endif
	SetActorTransform(MoveTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
#if WITH_EDITOR
	bIsApplyingEditorTransform = false;
#endif
	ApplyCameraSettings(MoveTargetCameraSettings);
	AdvanceCameraStepIndex();

	OnCameraMoveFinished.Broadcast(this);
	if (bShouldContinueFromArrivedMarker)
	{
		PendingCameraMoveCount = FMath::Max(PendingCameraMoveCount, 1);
	}

	const bool bStartedNextMove = TryStartQueuedCameraMove();
	if (!bStartedNextMove && !bIsMoving && PendingCameraMoveCount <= 0)
	{
		FinishCameraViewTarget();
	}

	UpdateEditorPreviewVisuals();
}

void AUOUCinematicCameraActor::StopActiveMove(bool bBroadcastStopped)
{
	const bool bWasMoving = bIsMoving || bIsPaused;
	bIsMoving = false;
	bIsPaused = false;
	MoveElapsedTime = 0.0f;
	ActiveMoveDuration = 0.0f;
	PendingCameraMoveCount = 0;
	ActiveMoveTargetMarker = nullptr;
	ActiveMoveTargetIndex = INDEX_NONE;
	FinishCameraViewTarget();

	if (bBroadcastStopped && bWasMoving)
	{
		OnCameraMoveStopped.Broadcast(this);
	}
}

bool AUOUCinematicCameraActor::ShouldUseCameraSteps() const
{
	return bUseCameraSteps && GetCurrentCameraStepMarker() != nullptr;
}

AUOUCinematicCameraTargetActor* AUOUCinematicCameraActor::GetCameraStepMarkerAt(int32 StepIndex) const
{
	if (!CameraStepMarkers.IsValidIndex(StepIndex))
	{
		return nullptr;
	}

	return CameraStepMarkers[StepIndex].Get();
}

AUOUCinematicCameraTargetActor* AUOUCinematicCameraActor::GetCurrentCameraStepMarker() const
{
	if (!bUseCameraSteps || CameraStepMarkers.Num() == 0)
	{
		return nullptr;
	}

	const int32 ClampedIndex = FMath::Clamp(RuntimeCameraStepIndex, 0, CameraStepMarkers.Num() - 1);
	if (AUOUCinematicCameraTargetActor* DirectTargetMarker = GetCameraStepMarkerAt(ClampedIndex))
	{
		return DirectTargetMarker;
	}

	for (int32 TargetIndex = 0; TargetIndex < CameraStepMarkers.Num(); ++TargetIndex)
	{
		if (AUOUCinematicCameraTargetActor* TargetMarker = GetCameraStepMarkerAt(TargetIndex))
		{
			return TargetMarker;
		}
	}

	return nullptr;
}

bool AUOUCinematicCameraActor::ResolveNextCameraTargetTransform(FTransform& OutTargetTransform, int32& OutTargetIndex) const
{
	OutTargetTransform = FTransform::Identity;
	OutTargetIndex = INDEX_NONE;

	if (bNextMoveReturnsToStart)
	{
		OutTargetTransform = StartTransform;
		return true;
	}

	if (!ShouldUseCameraSteps())
	{
		return false;
	}

	const int32 ClampedIndex = FMath::Clamp(RuntimeCameraStepIndex, 0, CameraStepMarkers.Num() - 1);
	if (AUOUCinematicCameraTargetActor* DirectTargetMarker = GetCameraStepMarkerAt(ClampedIndex))
	{
		OutTargetTransform = DirectTargetMarker->GetActorTransform();
		OutTargetIndex = ClampedIndex;
		return true;
	}

	for (int32 TargetIndex = 0; TargetIndex < CameraStepMarkers.Num(); ++TargetIndex)
	{
		if (AUOUCinematicCameraTargetActor* TargetMarker = GetCameraStepMarkerAt(TargetIndex))
		{
			OutTargetTransform = TargetMarker->GetActorTransform();
			OutTargetIndex = TargetIndex;
			return true;
		}
	}

	return false;
}

void AUOUCinematicCameraActor::AdvanceCameraStepIndex()
{
	if (!bUseCameraSteps || ActiveMoveTargetIndex == INDEX_NONE || CameraStepMarkers.Num() == 0)
	{
		ActiveMoveTargetIndex = INDEX_NONE;
		return;
	}

	const int32 TargetCount = CameraStepMarkers.Num();
	for (int32 CandidateIndex = ActiveMoveTargetIndex + 1; CandidateIndex < TargetCount; ++CandidateIndex)
	{
		if (GetCameraStepMarkerAt(CandidateIndex) != nullptr)
		{
			RuntimeCameraStepIndex = CandidateIndex;
			ActiveMoveTargetIndex = INDEX_NONE;
			return;
		}
	}

	if (bLoopCameraSteps)
	{
		for (int32 CandidateIndex = 0; CandidateIndex < TargetCount; ++CandidateIndex)
		{
			if (GetCameraStepMarkerAt(CandidateIndex) != nullptr)
			{
				RuntimeCameraStepIndex = CandidateIndex;
				bNextMoveReturnsToStart = bLoopCameraStepsThroughStart;
				ActiveMoveTargetIndex = INDEX_NONE;
				return;
			}
		}
	}

	RuntimeCameraStepIndex = FMath::Clamp(ActiveMoveTargetIndex, 0, TargetCount - 1);
	ActiveMoveTargetIndex = INDEX_NONE;
}

void AUOUCinematicCameraActor::RequestCameraMoveSteps(int32 StepCount)
{
	const int32 SafeStepCount = FMath::Max(1, StepCount);
	PendingCameraMoveCount += SafeStepCount;
	TryStartQueuedCameraMove();
}

bool AUOUCinematicCameraActor::TryStartQueuedCameraMove()
{
	if (bIsMoving || PendingCameraMoveCount <= 0)
	{
		return false;
	}

	EnsureStartTransform();
	RefreshTargetTransforms();

	FTransform NextTargetTransform;
	int32 NextTargetIndex = INDEX_NONE;
	if (!ResolveNextCameraTargetTransform(NextTargetTransform, NextTargetIndex))
	{
		PendingCameraMoveCount = 0;
		return false;
	}

	AUOUCinematicCameraTargetActor* NextTargetMarker = GetCameraStepMarkerAt(NextTargetIndex);
	if (GetActorTransform().Equals(NextTargetTransform))
	{
		ApplyCameraSettings(ResolveCameraSettingsFromTarget(NextTargetMarker));
		PendingCameraMoveCount = 0;
		LastArrivedCameraStepIndex = NextTargetIndex;
		SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, true);
		OnCameraMoveFinished.Broadcast(this);
		return false;
	}

	bNextMoveReturnsToStart = false;
	--PendingCameraMoveCount;
	return BeginMoveToTransform(NextTargetTransform, NextTargetMarker, NextTargetIndex);
}

bool AUOUCinematicCameraActor::BeginMoveToTransform(
	const FTransform& InTargetTransform,
	AUOUCinematicCameraTargetActor* TargetMarker,
	int32 TargetStepIndex)
{
	if (bIsMoving)
	{
		return false;
	}

	BeginCameraViewTarget();

	ActiveMoveTargetMarker = TargetMarker;
	ActiveMoveTargetIndex = TargetStepIndex;
	MoveStartTransform = GetActorTransform();
	MoveTargetTransform = InTargetTransform;
	MoveStartCameraSettings = ReadCurrentCameraSettings();
	MoveTargetCameraSettings = ResolveCameraSettingsFromTarget(TargetMarker);
	ActiveMoveDuration = ResolveMoveDuration(TargetMarker);
	MoveElapsedTime = 0.0f;
	bIsMoving = true;
	bIsPaused = false;
	bIsAtTarget = false;
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Activate, false);
	SetPuzzleResultCompletionState(EOUUPuzzleResultAction::Deactivate, false);
	ApplyCameraSettingsAtAlpha(0.0f);
	OnCameraMoveStarted.Broadcast(this);

	if (ActiveMoveDuration <= UE_KINDA_SMALL_NUMBER)
	{
		FinishMoveToTarget();
	}

	return true;
}

FTransform AUOUCinematicCameraActor::BuildActiveCameraTransformAtAlpha(float Alpha) const
{
	return BuildTransformBetween(MoveStartTransform, MoveTargetTransform, Alpha);
}

FTransform AUOUCinematicCameraActor::BuildTransformBetween(
	const FTransform& FromTransform,
	const FTransform& ToTransform,
	float Alpha) const
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const FVector NewLocation = EvaluatePathLocation(FromTransform, ToTransform, SafeAlpha);
	const FQuat NewRotation = FQuat::Slerp(FromTransform.GetRotation(), ToTransform.GetRotation(), SafeAlpha);
	const FVector NewScale = FMath::Lerp(FromTransform.GetScale3D(), ToTransform.GetScale3D(), SafeAlpha);

	return FTransform(NewRotation, NewLocation, NewScale);
}

FVector AUOUCinematicCameraActor::EvaluatePathLocation(
	const FTransform& FromTransform,
	const FTransform& ToTransform,
	float Alpha) const
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const FVector StartLocation = FromTransform.GetLocation();
	const FVector TargetLocation = ToTransform.GetLocation();

	if (PathMode != EUOUCinematicCameraPathMode::CubicBezier)
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

float AUOUCinematicCameraActor::ResolveMoveAlpha(float RawAlpha) const
{
	const float SafeRawAlpha = FMath::Clamp(RawAlpha, 0.0f, 1.0f);

	if (MoveCurve == nullptr)
	{
		switch (EasingMode)
		{
		case EUOUCinematicCameraEasingMode::EaseIn:
			return FMath::InterpEaseIn(0.0f, 1.0f, SafeRawAlpha, EaseExponent);
		case EUOUCinematicCameraEasingMode::EaseOut:
			return FMath::InterpEaseOut(0.0f, 1.0f, SafeRawAlpha, EaseExponent);
		case EUOUCinematicCameraEasingMode::EaseInOut:
			return FMath::InterpEaseInOut(0.0f, 1.0f, SafeRawAlpha, EaseExponent);
		case EUOUCinematicCameraEasingMode::Linear:
		default:
			return SafeRawAlpha;
		}
	}

	return FMath::Clamp(MoveCurve->GetFloatValue(SafeRawAlpha), 0.0f, 1.0f);
}

float AUOUCinematicCameraActor::ResolveMoveDuration(const AUOUCinematicCameraTargetActor* TargetMarker) const
{
	return TargetMarker != nullptr
		? TargetMarker->ResolveMoveDuration(MoveDuration)
		: FMath::Max(0.0f, MoveDuration);
}

FUOUCinematicCameraResolvedSettings AUOUCinematicCameraActor::BuildDefaultCameraSettings() const
{
	FUOUCinematicCameraResolvedSettings CameraSettings;
	if (CameraComponent != nullptr)
	{
		CameraSettings.ProjectionMode = CameraComponent->ProjectionMode.GetValue();
		CameraSettings.OrthographicWidth = FMath::Max(1.0f, CameraComponent->OrthoWidth);
		CameraSettings.FieldOfView = FMath::Clamp(CameraComponent->FieldOfView, 5.0f, 170.0f);
		CameraSettings.AspectRatio = FMath::Max(0.1f, CameraComponent->AspectRatio);
		CameraSettings.bConstrainAspectRatio = CameraComponent->bConstrainAspectRatio;
		return CameraSettings;
	}

	CameraSettings.ProjectionMode = ProjectionMode.GetValue();
	CameraSettings.OrthographicWidth = FMath::Max(1.0f, OrthographicWidth);
	CameraSettings.FieldOfView = FMath::Clamp(FieldOfView, 5.0f, 170.0f);
	CameraSettings.AspectRatio = FMath::Max(0.1f, AspectRatio);
	CameraSettings.bConstrainAspectRatio = bConstrainAspectRatio;
	return CameraSettings;
}

FUOUCinematicCameraResolvedSettings AUOUCinematicCameraActor::ReadCurrentCameraSettings() const
{
	if (CameraComponent == nullptr)
	{
		return BuildDefaultCameraSettings();
	}

	FUOUCinematicCameraResolvedSettings CameraSettings;
	CameraSettings.ProjectionMode = CameraComponent->ProjectionMode;
	CameraSettings.OrthographicWidth = FMath::Max(1.0f, CameraComponent->OrthoWidth);
	CameraSettings.FieldOfView = FMath::Clamp(CameraComponent->FieldOfView, 5.0f, 170.0f);
	CameraSettings.AspectRatio = FMath::Max(0.1f, CameraComponent->AspectRatio);
	CameraSettings.bConstrainAspectRatio = CameraComponent->bConstrainAspectRatio;
	return CameraSettings;
}

FUOUCinematicCameraResolvedSettings AUOUCinematicCameraActor::ResolveCameraSettingsFromTarget(
	const AUOUCinematicCameraTargetActor* TargetMarker) const
{
	FUOUCinematicCameraResolvedSettings CameraSettings = BuildDefaultCameraSettings();
	if (TargetMarker == nullptr)
	{
		return CameraSettings;
	}

	CameraSettings.ProjectionMode = TargetMarker->ResolveProjectionMode(CameraSettings.ProjectionMode);
	CameraSettings.OrthographicWidth = TargetMarker->ResolveOrthographicWidth(CameraSettings.OrthographicWidth);
	CameraSettings.FieldOfView = TargetMarker->ResolveFieldOfView(CameraSettings.FieldOfView);
	CameraSettings.AspectRatio = TargetMarker->ResolveAspectRatio(CameraSettings.AspectRatio);
	CameraSettings.bConstrainAspectRatio = TargetMarker->ResolveConstrainAspectRatio(CameraSettings.bConstrainAspectRatio);
	return CameraSettings;
}

void AUOUCinematicCameraActor::ApplyCameraSettings(const FUOUCinematicCameraResolvedSettings& CameraSettings) const
{
	if (CameraComponent == nullptr)
	{
		return;
	}

	CameraComponent->SetProjectionMode(CameraSettings.ProjectionMode);
	CameraComponent->SetOrthoWidth(FMath::Max(1.0f, CameraSettings.OrthographicWidth));
	CameraComponent->SetFieldOfView(FMath::Clamp(CameraSettings.FieldOfView, 5.0f, 170.0f));
	CameraComponent->AspectRatio = FMath::Max(0.1f, CameraSettings.AspectRatio);
	CameraComponent->bConstrainAspectRatio = CameraSettings.bConstrainAspectRatio;
}

void AUOUCinematicCameraActor::ApplyCameraSettingsAtAlpha(float Alpha)
{
	FUOUCinematicCameraResolvedSettings CameraSettings = MoveTargetCameraSettings;
	if (bInterpolateCameraSettings)
	{
		const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		CameraSettings.OrthographicWidth = FMath::Lerp(
			MoveStartCameraSettings.OrthographicWidth,
			MoveTargetCameraSettings.OrthographicWidth,
			SafeAlpha);
		CameraSettings.FieldOfView = FMath::Lerp(
			MoveStartCameraSettings.FieldOfView,
			MoveTargetCameraSettings.FieldOfView,
			SafeAlpha);
		CameraSettings.AspectRatio = FMath::Lerp(
			MoveStartCameraSettings.AspectRatio,
			MoveTargetCameraSettings.AspectRatio,
			SafeAlpha);
	}

	ApplyCameraSettings(CameraSettings);
}

void AUOUCinematicCameraActor::SyncStepMarkerPreviews()
{
	for (const TObjectPtr<AUOUCinematicCameraTargetActor>& CameraStepMarker : CameraStepMarkers)
	{
		if (!IsValid(CameraStepMarker))
		{
			continue;
		}

		const FUOUCinematicCameraResolvedSettings PreviewSettings = ResolveCameraSettingsFromTarget(CameraStepMarker.Get());
		CameraStepMarker->SyncPreviewCamera(
			PreviewSettings.ProjectionMode,
			PreviewSettings.OrthographicWidth,
			PreviewSettings.FieldOfView,
			PreviewSettings.AspectRatio,
			PreviewSettings.bConstrainAspectRatio);
		CameraStepMarker->SetTargetCameraPreviewVisible(bShowTargetCameraPreviews);
	}
}

void AUOUCinematicCameraActor::UpdateEditorPreviewVisuals()
{
	SyncStepMarkerPreviews();

	TArray<FVector> PreviewPathPoints;
	PreviewPathPoints.Add(StartTransform.GetLocation());

	auto AppendPreviewSegment = [this, &PreviewPathPoints](
		const FTransform& SegmentFromTransform,
		const FTransform& SegmentToTransform) -> FTransform
	{
		constexpr int32 PreviewSampleCount = 8;
		const int32 SampleCount = PathMode == EUOUCinematicCameraPathMode::CubicBezier ? PreviewSampleCount : 1;

		for (int32 SampleIndex = 1; SampleIndex <= SampleCount; ++SampleIndex)
		{
			const float SampleAlpha = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
			PreviewPathPoints.Add(BuildTransformBetween(SegmentFromTransform, SegmentToTransform, SampleAlpha).GetLocation());
		}

		return BuildTransformBetween(SegmentFromTransform, SegmentToTransform, 1.0f);
	};

	FTransform PreviewSegmentStartTransform = StartTransform;
	if (bUseCameraSteps && CameraStepMarkers.Num() > 0)
	{
		for (const TObjectPtr<AUOUCinematicCameraTargetActor>& CameraStepMarker : CameraStepMarkers)
		{
			if (IsValid(CameraStepMarker))
			{
				PreviewSegmentStartTransform = AppendPreviewSegment(PreviewSegmentStartTransform, CameraStepMarker->GetActorTransform());
			}
		}

		if (bLoopCameraSteps && PreviewPathPoints.Num() > 1)
		{
			if (bLoopCameraStepsThroughStart)
			{
				AppendPreviewSegment(PreviewSegmentStartTransform, StartTransform);
			}
			else
			{
				for (const TObjectPtr<AUOUCinematicCameraTargetActor>& CameraStepMarker : CameraStepMarkers)
				{
					if (IsValid(CameraStepMarker))
					{
						AppendPreviewSegment(PreviewSegmentStartTransform, CameraStepMarker->GetActorTransform());
						break;
					}
				}
			}
		}
	}

	if (PreviewPathPoints.Num() == 1 && !TargetTransform.Equals(StartTransform))
	{
		AppendPreviewSegment(StartTransform, TargetTransform);
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

	if (MovePreviewPath == nullptr)
	{
		return;
	}

	MovePreviewPath->SetWorldTransform(FTransform::Identity, false, nullptr, ETeleportType::TeleportPhysics);
	MovePreviewPath->SetVisibility(bShowMovePreviewPath && bHasMoveOffset, true);
	if (!bHasMoveOffset)
	{
		return;
	}

	MovePreviewPath->ClearSplinePoints(false);
	for (int32 PointIndex = 0; PointIndex < PreviewPathPoints.Num(); ++PointIndex)
	{
		MovePreviewPath->AddSplinePoint(PreviewPathPoints[PointIndex], ESplineCoordinateSpace::World, false);
		MovePreviewPath->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
	}
	MovePreviewPath->UpdateSpline();
}

APlayerController* AUOUCinematicCameraActor::ResolvePlayerController() const
{
	UWorld* World = GetWorld();
	return World != nullptr ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
}

AActor* AUOUCinematicCameraActor::ResolveReturnViewTarget(APlayerController* PlayerController) const
{
	if (ReturnViewTargetOverride != nullptr)
	{
		return ReturnViewTargetOverride;
	}

	if (PreviousViewTarget != nullptr)
	{
		return PreviousViewTarget;
	}

	return PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
}

void AUOUCinematicCameraActor::BeginCameraViewTarget()
{
	if (bViewTargetActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld())
	{
		return;
	}

	if (!bSetViewTargetOnMove && !bUseCinematicMode && !bBlockPlayerInputDuringPlayback)
	{
		return;
	}

	APlayerController* PlayerController = ResolvePlayerController();
	if (PlayerController == nullptr)
	{
		return;
	}

	CachedPlayerController = PlayerController;
	PreviousViewTarget = PlayerController->GetViewTarget();
	EnterCinematicState(PlayerController);
	RequestPlayerInputBlockForPlayback();

	if (bSetViewTargetOnMove)
	{
		PlayerController->SetViewTargetWithBlend(this, FMath::Max(0.0f, BlendInTime));
	}

	bViewTargetActive = true;
}

void AUOUCinematicCameraActor::FinishCameraViewTarget()
{
	ReleasePlayerInputBlockForPlayback();

	if (!bViewTargetActive)
	{
		return;
	}

	APlayerController* PlayerController = CachedPlayerController != nullptr
		? CachedPlayerController.Get()
		: ResolvePlayerController();
	if (PlayerController != nullptr)
	{
		if (bSetViewTargetOnMove && bRestoreViewTargetOnFinish)
		{
			if (AActor* ReturnViewTarget = ResolveReturnViewTarget(PlayerController))
			{
				PlayerController->SetViewTargetWithBlend(ReturnViewTarget, FMath::Max(0.0f, BlendOutTime));
			}
		}

		ExitCinematicState(PlayerController);
	}

	bViewTargetActive = false;
	CachedPlayerController = nullptr;
	PreviousViewTarget = nullptr;
}

void AUOUCinematicCameraActor::RequestPlayerInputBlockForPlayback()
{
	if (!bBlockPlayerInputDuringPlayback || LockedInputExecutorComponent != nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld())
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

void AUOUCinematicCameraActor::ReleasePlayerInputBlockForPlayback()
{
	if (IsValid(LockedInputExecutorComponent))
	{
		LockedInputExecutorComponent->ReleasePlayerInputBlock(this);
	}

	LockedInputExecutorComponent = nullptr;
}

void AUOUCinematicCameraActor::EnterCinematicState(APlayerController* PlayerController)
{
	if (PlayerController == nullptr || !bUseCinematicMode)
	{
		return;
	}

	PlayerController->SetCinematicMode(
		true,
		bHidePlayerDuringPlayback,
		bHideHUDDuringPlayback,
		bDisableMovementDuringPlayback,
		bDisableTurningDuringPlayback);
}

void AUOUCinematicCameraActor::ExitCinematicState(APlayerController* PlayerController)
{
	if (PlayerController == nullptr || !bUseCinematicMode)
	{
		return;
	}

	PlayerController->SetCinematicMode(
		false,
		bHidePlayerDuringPlayback,
		bHideHUDDuringPlayback,
		bDisableMovementDuringPlayback,
		bDisableTurningDuringPlayback);
}

void AUOUCinematicCameraActor::SetPuzzleResultCompletionState(
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
