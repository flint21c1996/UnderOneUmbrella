// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformActor.h"

#include "World/Stage/UOUFloorPlatformTargetActor.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

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
	CarryDetectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CarryDetectionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	CarryDetectionBox->SetGenerateOverlapEvents(true);

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

void AUOUFloorPlatformActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DetachCarriedActors();

	Super::EndPlay(EndPlayReason);
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
	bHasCapturedStartTransform = true;

	RefreshTargetTransforms();
	UpdateEditorPreviewVisuals();
}

void AUOUFloorPlatformActor::MoveToTarget()
{
	MoveToNextSequentialTarget();
}

void AUOUFloorPlatformActor::MoveToNextSequentialTarget()
{
	if (bIsMoving)
	{
		return;
	}

	FTransform NextTargetTransform;
	int32 NextTargetIndex = INDEX_NONE;
	if (!ResolveNextSequentialTargetTransform(NextTargetTransform, NextTargetIndex))
	{
		return;
	}

	bNextSequentialMoveReturnsToStart = false;
	ActiveSequentialTargetIndex = NextTargetIndex;
	BeginMoveToTransform(NextTargetTransform);
}

void AUOUFloorPlatformActor::ResetPlatform()
{
	if (!bHasCapturedStartTransform)
	{
		CaptureCurrentAsStart();
	}

	RefreshTargetTransforms();
	AttachLastMovedActors();
	if (CarriedActors.Num() == 0)
	{
		AttachCarriedActors();
	}

	bIsMoving = false;
	bIsAtTarget = false;
	MoveElapsedTime = 0.0f;
	CurrentSequentialTargetIndex = 0;
	ActiveSequentialTargetIndex = INDEX_NONE;
	bNextSequentialMoveReturnsToStart = false;
	MoveStartTransform = StartTransform;
	MoveTargetTransform = TargetTransform;

	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyTargetCollisionState();
	CacheLastMovedActors();
	DetachCarriedActors();
	UpdateEditorPreviewVisuals();
}

void AUOUFloorPlatformActor::SnapToTarget()
{
	if (!bHasCapturedStartTransform)
	{
		CaptureCurrentAsStart();
	}

	RefreshTargetTransforms();

	AttachCarriedActors();

	FTransform SnapTargetTransform = FTransform::Identity;
	int32 SnapTargetIndex = INDEX_NONE;
	if (!ResolveNextSequentialTargetTransform(SnapTargetTransform, SnapTargetIndex))
	{
		DetachCarriedActors();
		return;
	}

	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;
	bNextSequentialMoveReturnsToStart = false;
	ActiveSequentialTargetIndex = SnapTargetIndex;
	MoveStartTransform = GetActorTransform();
	MoveTargetTransform = SnapTargetTransform;

	SetActorTransform(SnapTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyTargetCollisionState();
	CacheLastMovedActors();
	DetachCarriedActors();
	AdvanceSequentialTargetIndex();
	UpdateEditorPreviewVisuals();
}

void AUOUFloorPlatformActor::SetTargetToStart()
{
	if (!bHasCapturedStartTransform)
	{
		CaptureCurrentAsStart();
	}

	if (AUOUFloorPlatformTargetActor* SequentialTargetMarker = GetCurrentSequentialTargetMarker())
	{
		SequentialTargetMarker->SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
		SequentialTargetMarker->SyncPreviewFromMesh(PlatformMesh);
		SequentialTargetMarker->SetTargetPreviewMeshVisible(bShowTransformPreview);
	}

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

	if (!bHasCapturedStartTransform)
	{
		CaptureCurrentAsStart();
	}

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
	CurrentSequentialTargetIndex = FMath::Clamp(CurrentSequentialTargetIndex, 0, FMath::Max(0, SequentialTargetMarkers.Num() - 1));

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
	if (!bHasCapturedStartTransform)
	{
		StartTransform = GetActorTransform();
		bHasCapturedStartTransform = true;
	}

	TargetTransform = StartTransform;
	if (AUOUFloorPlatformTargetActor* SequentialTargetMarker = GetCurrentSequentialTargetMarker())
	{
		TargetTransform = SequentialTargetMarker->GetActorTransform();
	}
}

void AUOUFloorPlatformActor::FinishMoveToTarget()
{
	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;

	SetActorTransform(MoveTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyTargetCollisionState();
	CacheLastMovedActors();
	DetachCarriedActors();
	AdvanceSequentialTargetIndex();

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

bool AUOUFloorPlatformActor::ShouldUseSequentialTargetMarkers() const
{
	return bUseSequentialTargetMarkers && GetCurrentSequentialTargetMarker() != nullptr;
}

AUOUFloorPlatformTargetActor* AUOUFloorPlatformActor::GetSequentialTargetMarkerAt(int32 TargetIndex) const
{
	if (!SequentialTargetMarkers.IsValidIndex(TargetIndex))
	{
		return nullptr;
	}

	return SequentialTargetMarkers[TargetIndex].Get();
}

AUOUFloorPlatformTargetActor* AUOUFloorPlatformActor::GetCurrentSequentialTargetMarker() const
{
	if (!bUseSequentialTargetMarkers || SequentialTargetMarkers.Num() == 0)
	{
		return nullptr;
	}

	const int32 ClampedIndex = FMath::Clamp(CurrentSequentialTargetIndex, 0, SequentialTargetMarkers.Num() - 1);
	if (AUOUFloorPlatformTargetActor* DirectTargetMarker = GetSequentialTargetMarkerAt(ClampedIndex))
	{
		return DirectTargetMarker;
	}

	for (int32 TargetIndex = 0; TargetIndex < SequentialTargetMarkers.Num(); ++TargetIndex)
	{
		if (AUOUFloorPlatformTargetActor* SequentialTargetMarker = GetSequentialTargetMarkerAt(TargetIndex))
		{
			return SequentialTargetMarker;
		}
	}

	return nullptr;
}

bool AUOUFloorPlatformActor::ResolveNextSequentialTargetTransform(FTransform& OutTargetTransform, int32& OutTargetIndex) const
{
	OutTargetTransform = FTransform::Identity;
	OutTargetIndex = INDEX_NONE;

	if (bNextSequentialMoveReturnsToStart)
	{
		OutTargetTransform = StartTransform;
		return true;
	}

	if (!bUseSequentialTargetMarkers || SequentialTargetMarkers.Num() == 0)
	{
		return false;
	}

	const int32 ClampedIndex = FMath::Clamp(CurrentSequentialTargetIndex, 0, SequentialTargetMarkers.Num() - 1);
	if (AUOUFloorPlatformTargetActor* DirectTargetMarker = GetSequentialTargetMarkerAt(ClampedIndex))
	{
		OutTargetTransform = DirectTargetMarker->GetActorTransform();
		OutTargetIndex = ClampedIndex;
		return true;
	}

	for (int32 TargetIndex = 0; TargetIndex < SequentialTargetMarkers.Num(); ++TargetIndex)
	{
		if (AUOUFloorPlatformTargetActor* SequentialTargetMarker = GetSequentialTargetMarkerAt(TargetIndex))
		{
			OutTargetTransform = SequentialTargetMarker->GetActorTransform();
			OutTargetIndex = TargetIndex;
			return true;
		}
	}

	return false;
}

void AUOUFloorPlatformActor::AdvanceSequentialTargetIndex()
{
	if (!bUseSequentialTargetMarkers || ActiveSequentialTargetIndex == INDEX_NONE || SequentialTargetMarkers.Num() == 0)
	{
		ActiveSequentialTargetIndex = INDEX_NONE;
		return;
	}

	const int32 TargetCount = SequentialTargetMarkers.Num();

	for (int32 CandidateIndex = ActiveSequentialTargetIndex + 1; CandidateIndex < TargetCount; ++CandidateIndex)
	{
		if (GetSequentialTargetMarkerAt(CandidateIndex) != nullptr)
		{
			CurrentSequentialTargetIndex = CandidateIndex;
			ActiveSequentialTargetIndex = INDEX_NONE;
			return;
		}
	}

	if (bLoopSequentialTargetMarkers)
	{
		for (int32 CandidateIndex = 0; CandidateIndex < TargetCount; ++CandidateIndex)
		{
			if (GetSequentialTargetMarkerAt(CandidateIndex) != nullptr)
			{
				CurrentSequentialTargetIndex = CandidateIndex;
				bNextSequentialMoveReturnsToStart = bLoopMoveStepsThroughStart;
				ActiveSequentialTargetIndex = INDEX_NONE;
				return;
			}
		}
	}

	CurrentSequentialTargetIndex = FMath::Clamp(ActiveSequentialTargetIndex, 0, TargetCount - 1);
	ActiveSequentialTargetIndex = INDEX_NONE;
}

bool AUOUFloorPlatformActor::BeginMoveToTransform(const FTransform& InTargetTransform)
{
	if (bIsMoving)
	{
		return false;
	}

	MoveStartTransform = GetActorTransform();
	MoveTargetTransform = InTargetTransform;

	bIsMoving = true;
	bIsAtTarget = false;
	MoveElapsedTime = 0.0f;
	ApplyTargetCollisionState();
	AttachCarriedActors();

	return true;
}

void AUOUFloorPlatformActor::AttachCarriedActors()
{
	DetachCarriedActors();

	if (!bCarryActorsOnMove || CarryDetectionBox == nullptr)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	CollectCarryCandidateActors(OverlappingActors);

	for (AActor* CandidateActor : OverlappingActors)
	{
		if (!CanCarryActor(CandidateActor))
		{
			continue;
		}

		PrepareCarriedActorForAttach(CandidateActor);
		CandidateActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		CarriedActors.Add(CandidateActor);
	}
}

void AUOUFloorPlatformActor::AttachLastMovedActors()
{
	DetachCarriedActors();

	if (!bCarryActorsOnMove)
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& LastMovedActor : LastMovedActors)
	{
		AActor* CandidateActor = LastMovedActor.Get();
		if (!CanCarryActor(CandidateActor))
		{
			continue;
		}

		PrepareCarriedActorForAttach(CandidateActor);
		CandidateActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		CarriedActors.Add(CandidateActor);
	}
}

void AUOUFloorPlatformActor::CollectCarryCandidateActors(TArray<AActor*>& OutCandidateActors) const
{
	OutCandidateActors.Reset();

	if (CarryDetectionBox == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Destructible);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOUFloorPlatformCarryOverlap), false, this);
	QueryParams.AddIgnoredActor(this);

	TArray<FOverlapResult> OverlapResults;
	GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		CarryDetectionBox->GetComponentLocation(),
		CarryDetectionBox->GetComponentQuat(),
		ObjectQueryParams,
		FCollisionShape::MakeBox(CarryDetectionBox->GetScaledBoxExtent()),
		QueryParams);

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* CandidateActor = OverlapResult.GetActor();
		if (CandidateActor == nullptr || OutCandidateActors.Contains(CandidateActor))
		{
			continue;
		}

		OutCandidateActors.Add(CandidateActor);
	}
}

void AUOUFloorPlatformActor::DetachCarriedActors()
{
	for (AActor* CarriedActor : CarriedActors)
	{
		if (IsValid(CarriedActor) && CarriedActor->GetAttachParentActor() == this)
		{
			CarriedActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	CarriedActors.Reset();
	RestoreCarriedPhysicsStates();
}

bool AUOUFloorPlatformActor::CanCarryActor(AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor) || CandidateActor == this)
	{
		return false;
	}

	for (const TObjectPtr<AActor>& IgnoredActor : IgnoredCarryActors)
	{
		if (IgnoredActor.Get() == CandidateActor)
		{
			return false;
		}
	}

	if (CandidateActor->GetAttachParentActor() != nullptr && CandidateActor->GetAttachParentActor() != this)
	{
		return false;
	}

	const bool bIsCharacter = CandidateActor->IsA<ACharacter>();
	if (bIsCharacter && !bCarryPlayerCharacters)
	{
		return false;
	}

	if (!bCarryPhysicsSimulatingActors && HasSimulatingPhysicsComponent(CandidateActor))
	{
		return false;
	}

	return MatchesCarryFilters(CandidateActor);
}

void AUOUFloorPlatformActor::PrepareCarriedActorForAttach(AActor* CandidateActor)
{
	if (!bPauseCarriedPhysicsDuringMove || CandidateActor == nullptr)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(CandidateActor);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent == nullptr || !PrimitiveComponent->IsSimulatingPhysics())
		{
			continue;
		}

		FUOUFloorPlatformCarriedPhysicsState PhysicsState;
		PhysicsState.Component = PrimitiveComponent;
		PhysicsState.bWasSimulatingPhysics = true;
		CarriedPhysicsStates.Add(PhysicsState);

		PrimitiveComponent->SetSimulatePhysics(false);
	}
}

void AUOUFloorPlatformActor::RestoreCarriedPhysicsStates()
{
	for (const FUOUFloorPlatformCarriedPhysicsState& PhysicsState : CarriedPhysicsStates)
	{
		UPrimitiveComponent* PrimitiveComponent = PhysicsState.Component.Get();
		if (PrimitiveComponent != nullptr)
		{
			PrimitiveComponent->SetSimulatePhysics(PhysicsState.bWasSimulatingPhysics);
		}
	}

	CarriedPhysicsStates.Reset();
}

bool AUOUFloorPlatformActor::HasSimulatingPhysicsComponent(AActor* CandidateActor) const
{
	if (CandidateActor == nullptr)
	{
		return false;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(CandidateActor);

	for (const UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent->IsSimulatingPhysics())
		{
			return true;
		}
	}

	return false;
}

bool AUOUFloorPlatformActor::MatchesCarryFilters(AActor* CandidateActor) const
{
	if (CandidateActor == nullptr)
	{
		return false;
	}

	const bool bHasClassFilter = CarryActorClasses.Num() > 0;
	const bool bHasTagFilter = CarryActorTags.Num() > 0;

	if (!bHasClassFilter && !bHasTagFilter)
	{
		return true;
	}

	for (const TSubclassOf<AActor>& CarryActorClass : CarryActorClasses)
	{
		if (*CarryActorClass != nullptr && CandidateActor->IsA(CarryActorClass))
		{
			return true;
		}
	}

	for (const FName& CarryActorTag : CarryActorTags)
	{
		if (!CarryActorTag.IsNone() && CandidateActor->ActorHasTag(CarryActorTag))
		{
			return true;
		}
	}

	return false;
}

void AUOUFloorPlatformActor::CacheLastMovedActors()
{
	LastMovedActors.Reset();

	for (AActor* CarriedActor : CarriedActors)
	{
		if (IsValid(CarriedActor))
		{
			LastMovedActors.Add(CarriedActor);
		}
	}
}

FTransform AUOUFloorPlatformActor::BuildActivePlatformTransformAtAlpha(float Alpha) const
{
	return BuildTransformBetween(MoveStartTransform, MoveTargetTransform, Alpha);
}

FTransform AUOUFloorPlatformActor::BuildTransformBetween(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha) const
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const FVector NewLocation = EvaluatePathLocation(FromTransform, ToTransform, SafeAlpha);
	const FQuat NewRotation = FQuat::Slerp(FromTransform.GetRotation(), ToTransform.GetRotation(), SafeAlpha);
	const FVector NewScale = FMath::Lerp(FromTransform.GetScale3D(), ToTransform.GetScale3D(), SafeAlpha);

	return FTransform(NewRotation, NewLocation, NewScale);
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

	if (bUseSequentialTargetMarkers && SequentialTargetMarkers.Num() > 0)
	{
		for (const TObjectPtr<AUOUFloorPlatformTargetActor>& SequentialTargetMarker : SequentialTargetMarkers)
		{
			if (IsValid(SequentialTargetMarker))
			{
				PreviewPathPoints.Add(SequentialTargetMarker->GetActorLocation());
			}
		}

		if (bLoopSequentialTargetMarkers && PreviewPathPoints.Num() > 1)
		{
			PreviewPathPoints.Add(bLoopMoveStepsThroughStart ? StartWorldLocation : PreviewPathPoints[1]);
		}
	}

	if (PreviewPathPoints.Num() == 1)
	{
		PreviewPathPoints.Add(TargetTransform.GetLocation());
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
