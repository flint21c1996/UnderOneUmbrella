// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Weight/UOUWeightedButtonActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Puzzle/Results/UOUPuzzleMoverActor.h"
#include "Puzzle/Weight/UOUWeightedButtonComponent.h"
#include "Puzzle/Weight/UOUWeightSensorComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace UOUWeightedButtonActorPrivate
{
	constexpr float ButtonHalfExtentXY = 75.0f;
	constexpr float ButtonHalfHeight = 10.0f;
}

AUOUWeightedButtonActor::AUOUWeightedButtonActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootScene->SetMobility(EComponentMobility::Movable);
	SetRootComponent(RootScene);

	WeightSensorVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WeightSensorVolume"));
	WeightSensorVolume->SetMobility(EComponentMobility::Movable);
	WeightSensorVolume->SetupAttachment(RootScene);
	WeightSensorVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeightSensorVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeightSensorVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeightSensorVolume->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	WeightSensorVolume->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	WeightSensorVolume->SetGenerateOverlapEvents(true);
	WeightSensorVolume->SetBoxExtent(FVector(80.0f, 80.0f, 30.0f));
	WeightSensorVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));

	ButtonMotionRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ButtonMotionRoot"));
	ButtonMotionRoot->SetupAttachment(RootScene);
	ButtonMotionRoot->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));

	ButtonVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonVisual"));
	ButtonVisual->SetMobility(EComponentMobility::Movable);
	ButtonVisual->SetupAttachment(ButtonMotionRoot);
	ButtonVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ButtonVisual->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		ButtonVisual->SetStaticMesh(CubeMeshFinder.Object);
		ButtonVisual->SetRelativeScale3D(FVector(1.5f, 1.5f, 0.2f));
	}

	ButtonSurfaceCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ButtonSurfaceCollision"));
	ButtonSurfaceCollision->SetupAttachment(ButtonMotionRoot);
	ButtonSurfaceCollision->SetBoxExtent(FVector(
		UOUWeightedButtonActorPrivate::ButtonHalfExtentXY,
		UOUWeightedButtonActorPrivate::ButtonHalfExtentXY,
		UOUWeightedButtonActorPrivate::ButtonHalfHeight));
	ButtonSurfaceCollision->SetRelativeLocation(FVector::ZeroVector);

	ConfigureButtonCollisionLayout();
	ConfigureButtonCollision();

	ReleasedPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ReleasedPoint"));
	ReleasedPoint->SetMobility(EComponentMobility::Movable);
	ReleasedPoint->SetupAttachment(RootScene);
	ReleasedPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));

	PressedPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PressedPoint"));
	PressedPoint->SetMobility(EComponentMobility::Movable);
	PressedPoint->SetupAttachment(RootScene);
	PressedPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));

	ResultSinkPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ResultSinkPoint"));
	ResultSinkPoint->SetMobility(EComponentMobility::Movable);
	ResultSinkPoint->SetupAttachment(RootScene);
	ResultSinkPoint->SetRelativeLocation(FVector(0.0f, 0.0f, -60.0f));

	WeightSensorComponent = CreateDefaultSubobject<UUOUWeightSensorComponent>(TEXT("WeightSensorComponent"));
	WeightSensorComponent->bAutoFindSensorVolume = true;
	WeightSensorComponent->SensorVolume = WeightSensorVolume;

	WeightedButtonComponent = CreateDefaultSubobject<UUOUWeightedButtonComponent>(TEXT("WeightedButtonComponent"));
	WeightedButtonComponent->bAutoFindSensor = true;
	WeightedButtonComponent->Sensor = WeightSensorComponent;
	WeightedButtonComponent->bAutoFindMotionReferences = true;
	WeightedButtonComponent->ButtonVisual = ButtonVisual;
	WeightedButtonComponent->ReleasedPoint = ReleasedPoint;
	WeightedButtonComponent->PressedPoint = PressedPoint;
}

void AUOUWeightedButtonActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ConfigureWeightedButtonMotionReferences();
	ConfigureButtonCollisionLayout();
	ConfigureButtonCollision();
	SyncButtonCollisionToVisual();
}

void AUOUWeightedButtonActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ConfigureWeightedButtonMotionReferences();
	ConfigureButtonCollisionLayout();
	ConfigureButtonCollision();
	SyncButtonCollisionToVisual();
}

void AUOUWeightedButtonActor::BeginPlay()
{
	Super::BeginPlay();

	if (ButtonMotionRoot != nullptr)
	{
		ButtonMotionRoot->SetMobility(EComponentMobility::Movable);
	}

	if (ButtonVisual != nullptr)
	{
		ButtonVisual->SetMobility(EComponentMobility::Movable);
		ButtonVisual->SetSimulatePhysics(false);
	}

	if (!bEnableResultSinkMotion)
	{
		bResultSinkActive = false;
	}

	ConfigureWeightedButtonMotionReferences();
	RefreshButtonMotionTickState();

	if (bFollowSurfaceTarget && bCaptureSurfaceOffsetOnBeginPlay)
	{
		CaptureSurfaceOffsetFromCurrentLocation();
	}

	ConfigureButtonCollisionLayout();
	ConfigureButtonCollision();
	SyncButtonCollisionToVisual();
}

void AUOUWeightedButtonActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSurfaceFollow(DeltaSeconds);
	MoveResultSinkVisual(DeltaSeconds);
	SyncButtonCollisionToVisual();
}

void AUOUWeightedButtonActor::CaptureCurrentSurfaceOffset()
{
	CaptureSurfaceOffsetFromCurrentLocation();
}

void AUOUWeightedButtonActor::SetResultSinkEnabled(bool bNewEnabled)
{
	bEnableResultSinkMotion = bNewEnabled;

	if (!bEnableResultSinkMotion)
	{
		bResultSinkActive = false;
	}

	RefreshButtonMotionTickState();
}

void AUOUWeightedButtonActor::SetResultSinkActive(bool bNewActive)
{
	bResultSinkActive = bEnableResultSinkMotion && bNewActive;
	RefreshButtonMotionTickState();
}

void AUOUWeightedButtonActor::ActivateResultSink()
{
	SetResultSinkActive(true);
}

void AUOUWeightedButtonActor::DeactivateResultSink()
{
	SetResultSinkActive(false);
}

void AUOUWeightedButtonActor::ToggleResultSink()
{
	SetResultSinkActive(!bResultSinkActive);
}

void AUOUWeightedButtonActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		ActivateResultSink();
		break;
	case EOUUPuzzleResultAction::Deactivate:
		DeactivateResultSink();
		break;
	case EOUUPuzzleResultAction::Toggle:
		ToggleResultSink();
		break;
	case EOUUPuzzleResultAction::Pause:
	case EOUUPuzzleResultAction::Resume:
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}

	RefreshButtonMotionTickState();
}

void AUOUWeightedButtonActor::ConfigureWeightedButtonMotionReferences() const
{
	if (WeightedButtonComponent == nullptr)
	{
		return;
	}

	WeightedButtonComponent->ButtonVisual = ButtonVisual;
	WeightedButtonComponent->ReleasedPoint = ReleasedPoint;
	WeightedButtonComponent->PressedPoint = PressedPoint;
}

void AUOUWeightedButtonActor::ConfigureButtonCollisionLayout() const
{
	if (ButtonSurfaceCollision != nullptr)
	{
		ButtonSurfaceCollision->SetBoxExtent(FVector(
			UOUWeightedButtonActorPrivate::ButtonHalfExtentXY,
			UOUWeightedButtonActorPrivate::ButtonHalfExtentXY,
			UOUWeightedButtonActorPrivate::ButtonHalfHeight));
		ButtonSurfaceCollision->SetRelativeLocation(FVector::ZeroVector);
		ButtonSurfaceCollision->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

void AUOUWeightedButtonActor::ConfigureButtonCollision() const
{
	if (WeightSensorVolume != nullptr)
	{
		WeightSensorVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		WeightSensorVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
		WeightSensorVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		WeightSensorVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
		WeightSensorVolume->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
		WeightSensorVolume->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
		WeightSensorVolume->SetGenerateOverlapEvents(true);
	}

	if (ButtonVisual != nullptr)
	{
		ButtonVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ButtonVisual->SetGenerateOverlapEvents(false);
	}

	ConfigureBlockingCollision(ButtonSurfaceCollision);
}

void AUOUWeightedButtonActor::ConfigureBlockingCollision(UBoxComponent* CollisionComponent) const
{
	if (CollisionComponent == nullptr)
	{
		return;
	}

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldStatic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	CollisionComponent->SetGenerateOverlapEvents(false);
	CollisionComponent->SetHiddenInGame(true);
	CollisionComponent->CanCharacterStepUpOn = ECB_Yes;
	CollisionComponent->SetCanEverAffectNavigation(false);
}

void AUOUWeightedButtonActor::SyncButtonCollisionToVisual() const
{
	if (ButtonSurfaceCollision == nullptr || ButtonVisual == nullptr)
	{
		return;
	}

	const USceneComponent* CollisionParent = ButtonSurfaceCollision->GetAttachParent();
	const FTransform ParentTransform = CollisionParent != nullptr
		? CollisionParent->GetComponentTransform()
		: FTransform::Identity;

	const FVector TargetRelativeLocation = ParentTransform.InverseTransformPosition(ButtonVisual->GetComponentLocation());
	ButtonSurfaceCollision->SetRelativeLocation(TargetRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AUOUWeightedButtonActor::MoveResultSinkVisual(float DeltaSeconds)
{
	USceneComponent* MovingButton = ButtonMotionRoot != nullptr ? ButtonMotionRoot : ButtonVisual;
	if (!bEnableResultSinkMotion || !bResultSinkActive || MovingButton == nullptr || ResultSinkPoint == nullptr)
	{
		return;
	}

	const FVector NextLocation = FMath::VInterpConstantTo(
		MovingButton->GetComponentLocation(),
		ResultSinkPoint->GetComponentLocation(),
		DeltaSeconds,
		FMath::Max(0.0f, ResultSinkMoveSpeed));

	MovingButton->SetWorldLocation(NextLocation);
}

void AUOUWeightedButtonActor::RefreshButtonMotionTickState() const
{
	if (WeightedButtonComponent == nullptr || !bPausePressMotionWhileSinking)
	{
		return;
	}

	// 침하 연출 중에는 WeightedButtonComponent가 ButtonVisual을 PressedPoint로 다시 끌어당기지 않게 합니다.
	const bool bShouldPausePressMotion = bEnableResultSinkMotion && bResultSinkActive;
	WeightedButtonComponent->SetComponentTickEnabled(!bShouldPausePressMotion);
}

USceneComponent* AUOUWeightedButtonActor::ResolveSurfaceTargetComponent() const
{
	if (SurfaceTargetActor == nullptr)
	{
		return nullptr;
	}

	if (const AUOUPuzzleMoverActor* MoverActor = Cast<AUOUPuzzleMoverActor>(SurfaceTargetActor))
	{
		if (SurfaceTargetComponentName.IsNone() || SurfaceTargetComponentName == TEXT("MovingTarget"))
		{
			return MoverActor->MovingTarget.Get();
		}
	}

	TArray<USceneComponent*> SceneComponents;
	SurfaceTargetActor->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent != nullptr && SceneComponent->GetFName() == SurfaceTargetComponentName)
		{
			return SceneComponent;
		}
	}

	return SurfaceTargetActor->GetRootComponent();
}

void AUOUWeightedButtonActor::CaptureSurfaceOffsetFromCurrentLocation()
{
	const USceneComponent* SurfaceComponent = ResolveSurfaceTargetComponent();
	if (SurfaceComponent == nullptr)
	{
		bSurfaceFollowOffsetCaptured = false;
		return;
	}

	// 현재 버튼 월드 위치를 표면 대상의 로컬 좌표로 저장해 둡니다.
	// 이후 대상이 이동하거나 스케일이 변해도 이 로컬 좌표를 다시 월드 위치로 바꿔 따라갑니다.
	SurfaceLocalOffset = SurfaceComponent->GetComponentTransform().InverseTransformPosition(GetActorLocation());
	bSurfaceFollowOffsetCaptured = true;
}

void AUOUWeightedButtonActor::UpdateSurfaceFollow(float DeltaSeconds)
{
	if (!bFollowSurfaceTarget)
	{
		return;
	}

	if (!bSurfaceFollowOffsetCaptured && bCaptureSurfaceOffsetOnBeginPlay)
	{
		CaptureSurfaceOffsetFromCurrentLocation();
	}

	if (!bSurfaceFollowOffsetCaptured)
	{
		return;
	}

	const USceneComponent* SurfaceComponent = ResolveSurfaceTargetComponent();
	if (SurfaceComponent == nullptr)
	{
		return;
	}

	const FVector TargetLocation = SurfaceComponent->GetComponentTransform().TransformPosition(SurfaceLocalOffset);
	if (SurfaceFollowSpeed <= 0.0f)
	{
		SetActorLocation(TargetLocation);
		return;
	}

	const FVector NextLocation = FMath::VInterpConstantTo(
		GetActorLocation(),
		TargetLocation,
		DeltaSeconds,
		SurfaceFollowSpeed);

	SetActorLocation(NextLocation);
}
