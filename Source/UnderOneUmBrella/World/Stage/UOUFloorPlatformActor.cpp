// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformActor.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
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
	CarryDetectionBox->SetupAttachment(RootScene);
	CarryDetectionBox->SetMobility(EComponentMobility::Movable);
	CarryDetectionBox->SetBoxExtent(FVector(300.0f, 300.0f, 160.0f));
	CarryDetectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 170.0f));
	CarryDetectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CarryDetectionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	CarryDetectionBox->SetGenerateOverlapEvents(true);

	RotationPivot = CreateDefaultSubobject<UArrowComponent>(TEXT("RotationPivot"));
	RotationPivot->SetupAttachment(RootScene);
	RotationPivot->SetMobility(EComponentMobility::Movable);
	RotationPivot->SetRelativeLocation(FVector::ZeroVector);
	RotationPivot->SetRelativeRotation(FRotator::ZeroRotator);
	// 플랫폼 크기 조절과 상관없이 회전축 표시가 과하게 커지지 않도록 화살표 표시 스케일만 고정합니다.
	RotationPivot->SetUsingAbsoluteScale(true);
	RotationPivot->SetRelativeScale3D(FVector::OneVector);
	RotationPivot->SetArrowLength(140.0f);
	RotationPivot->SetArrowSize(0.45f);
	RotationPivot->SetIsScreenSizeScaled(true);
	RotationPivot->SetScreenSize(0.0025f);
	RotationPivot->SetHiddenInGame(true);
	RotationPivot->SetArrowFColor(FColor::Red);

	RotationPivotMarker = CreateDefaultSubobject<USphereComponent>(TEXT("RotationPivotMarker"));
	RotationPivotMarker->SetupAttachment(RotationPivot);
	RotationPivotMarker->SetMobility(EComponentMobility::Movable);
	RotationPivotMarker->SetUsingAbsoluteScale(true);
	RotationPivotMarker->SetSphereRadius(28.0f);
	RotationPivotMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RotationPivotMarker->SetHiddenInGame(true);
	RotationPivotMarker->ShapeColor = FColor::Red;

	MovePreviewArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("MovePreviewArrow"));
	MovePreviewArrow->SetupAttachment(RootScene);
	MovePreviewArrow->SetMobility(EComponentMobility::Movable);
	MovePreviewArrow->SetUsingAbsoluteScale(true);
	MovePreviewArrow->SetArrowSize(0.35f);
	MovePreviewArrow->SetArrowLength(120.0f);
	MovePreviewArrow->SetHiddenInGame(true);
	MovePreviewArrow->SetArrowFColor(FColor::Cyan);

	TransformPreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TransformPreviewMesh"));
	TransformPreviewMesh->SetupAttachment(RootScene);
	TransformPreviewMesh->SetMobility(EComponentMobility::Movable);
	TransformPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TransformPreviewMesh->SetGenerateOverlapEvents(false);
	TransformPreviewMesh->SetHiddenInGame(true);
	TransformPreviewMesh->SetCastShadow(false);
	TransformPreviewMesh->SetRenderCustomDepth(true);
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

	if (!bIsMoving)
	{
		return;
	}

	MoveElapsedTime += DeltaSeconds;

	const float SafeDuration = FMath::Max(UE_KINDA_SMALL_NUMBER, MoveDuration);
	const float RawAlpha = FMath::Clamp(MoveElapsedTime / SafeDuration, 0.0f, 1.0f);
	const float MoveAlpha = ResolveMoveAlpha(RawAlpha);

	SetActorTransform(BuildPlatformTransformAtAlpha(MoveAlpha), false, nullptr, ETeleportType::TeleportPhysics);

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
	AttachCarriedActors();
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

	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;

	SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyTargetCollisionState();
	CacheLastMovedActors();
	DetachCarriedActors();
	UpdateEditorPreviewVisuals();
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
	TargetTransform = BuildPlatformTransformAtAlpha(1.0f);
}

void AUOUFloorPlatformActor::FinishMoveToTarget()
{
	bIsMoving = false;
	bIsAtTarget = true;
	MoveElapsedTime = MoveDuration;

	SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyTargetCollisionState();
	CacheLastMovedActors();
	DetachCarriedActors();

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

FTransform AUOUFloorPlatformActor::BuildPlatformTransformAtAlpha(float Alpha) const
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const FVector MoveOffsetWorld = StartTransform.TransformVectorNoScale(TargetLocalOffset) * SafeAlpha;

	FQuat RotationDelta = FQuat::Identity;
	FVector RotationLocationOffset = FVector::ZeroVector;

	if (bUseTargetRotation && !FMath::IsNearlyZero(TargetRotationAngleDegrees))
	{
		const FVector RotationAxisWorld = GetRotationAxisWorldDirection();
		if (!RotationAxisWorld.IsNearlyZero())
		{
			const FVector PivotWorldLocation = GetRotationPivotWorldLocation();
			const float RotationRadians = FMath::DegreesToRadians(TargetRotationAngleDegrees * SafeAlpha);

			RotationDelta = FQuat(RotationAxisWorld, RotationRadians);
			RotationLocationOffset = PivotWorldLocation
				+ RotationDelta.RotateVector(StartTransform.GetLocation() - PivotWorldLocation)
				- StartTransform.GetLocation();
		}
	}

	const FVector NewLocation = StartTransform.GetLocation() + MoveOffsetWorld + RotationLocationOffset;
	const FQuat NewRotation = RotationDelta * StartTransform.GetRotation();

	return FTransform(NewRotation, NewLocation, StartTransform.GetScale3D());
}

FTransform AUOUFloorPlatformActor::BuildPreviewMeshWorldTransform(float Alpha) const
{
	const FTransform PreviewActorTransform = BuildPlatformTransformAtAlpha(Alpha);
	if (PlatformMesh == nullptr)
	{
		return PreviewActorTransform;
	}

	return PlatformMesh->GetRelativeTransform() * PreviewActorTransform;
}

void AUOUFloorPlatformActor::UpdateEditorPreviewVisuals()
{
	if (RotationPivotMarker != nullptr)
	{
		RotationPivotMarker->SetVisibility(bUseTargetRotation, true);
	}

	if (MovePreviewArrow != nullptr)
	{
		const bool bHasMoveOffset = !TargetLocalOffset.IsNearlyZero();
		MovePreviewArrow->SetVisibility(bShowMovePreviewArrow && bHasMoveOffset, true);

		if (bHasMoveOffset)
		{
			constexpr float PreviewArrowLength = 160.0f;
			const FVector MoveDirection = TargetLocalOffset.GetSafeNormal();

			// 긴 막대가 아니라 목표 끝점을 찍는 짧은 화살표로 보여줍니다.
			MovePreviewArrow->SetRelativeLocation(TargetLocalOffset - MoveDirection * PreviewArrowLength);
			MovePreviewArrow->SetRelativeRotation(MoveDirection.Rotation());
			MovePreviewArrow->SetArrowLength(PreviewArrowLength);
		}
	}

	if (TransformPreviewMesh == nullptr)
	{
		return;
	}

	const bool bHasPreviewMesh = PlatformMesh != nullptr && PlatformMesh->GetStaticMesh() != nullptr;
	const bool bShouldShowPreview = bShowTransformPreview && bHasPreviewMesh;
	TransformPreviewMesh->SetVisibility(bShouldShowPreview, true);

	if (!bShouldShowPreview)
	{
		return;
	}

	SyncTransformPreviewMesh();
	TransformPreviewMesh->SetWorldTransform(BuildPreviewMeshWorldTransform(PreviewAlpha), false, nullptr, ETeleportType::TeleportPhysics);
}

void AUOUFloorPlatformActor::SyncTransformPreviewMesh()
{
	if (PlatformMesh == nullptr || TransformPreviewMesh == nullptr)
	{
		return;
	}

	TransformPreviewMesh->SetStaticMesh(PlatformMesh->GetStaticMesh());
	TransformPreviewMesh->SetRelativeScale3D(PlatformMesh->GetRelativeScale3D());

	const int32 MaterialCount = PlatformMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		TransformPreviewMesh->SetMaterial(MaterialIndex, PlatformMesh->GetMaterial(MaterialIndex));
	}
}

FVector AUOUFloorPlatformActor::GetRotationPivotWorldLocation() const
{
	if (RotationPivot == nullptr)
	{
		return StartTransform.GetLocation();
	}

	return StartTransform.TransformPosition(RotationPivot->GetRelativeLocation());
}

FVector AUOUFloorPlatformActor::GetRotationAxisWorldDirection() const
{
	if (RotationPivot == nullptr)
	{
		return StartTransform.GetRotation().GetAxisX();
	}

	const FVector PivotAxisLocal = RotationPivot->GetRelativeTransform().TransformVectorNoScale(FVector::ForwardVector);
	return StartTransform.TransformVectorNoScale(PivotAxisLocal).GetSafeNormal();
}

float AUOUFloorPlatformActor::ResolveMoveAlpha(float RawAlpha) const
{
	if (MoveCurve == nullptr)
	{
		return RawAlpha;
	}

	return FMath::Clamp(MoveCurve->GetFloatValue(RawAlpha), 0.0f, 1.0f);
}
