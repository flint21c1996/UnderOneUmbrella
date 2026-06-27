// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Water/UOUWaterWheelRainConditionActor.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Puzzle/Water/UOUWaterWheelRainConditionComponent.h"
#include "Puzzle/Water/UOUWaterWheelSpeedConditionComponent.h"
#include "UObject/ConstructorHelpers.h"

AUOUWaterWheelRainConditionActor::AUOUWaterWheelRainConditionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	RootScene->SetMobility(EComponentMobility::Movable);

	RotationPivot = CreateDefaultSubobject<USceneComponent>(TEXT("RotationPivot"));
	RotationPivot->SetupAttachment(RootScene);
	RotationPivot->SetMobility(EComponentMobility::Movable);

	WheelVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelVisual"));
	WheelVisual->SetupAttachment(RotationPivot);
	WheelVisual->SetMobility(EComponentMobility::Movable);
	WheelVisual->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	WheelVisual->SetGenerateOverlapEvents(false);
	WheelVisual->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
	WheelVisual->SetRelativeScale3D(FVector(2.0f, 2.0f, 0.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		WheelVisual->SetStaticMesh(CylinderMeshFinder.Object);
	}

	CatchLeft = CreateDefaultSubobject<UArrowComponent>(TEXT("CatchLeft"));
	CatchLeft->SetupAttachment(RotationPivot);
	CatchLeft->SetMobility(EComponentMobility::Movable);
	CatchLeft->SetRelativeLocation(FVector(-150.0f, 0.0f, 0.0f));
	CatchLeft->SetArrowSize(0.75f);
	CatchLeft->SetArrowColor(FLinearColor::Blue);
	CatchLeft->SetHiddenInGame(true);

	CatchRight = CreateDefaultSubobject<UArrowComponent>(TEXT("CatchRight"));
	CatchRight->SetupAttachment(RotationPivot);
	CatchRight->SetMobility(EComponentMobility::Movable);
	CatchRight->SetRelativeLocation(FVector(150.0f, 0.0f, 0.0f));
	CatchRight->SetArrowSize(0.75f);
	CatchRight->SetArrowColor(FLinearColor::Green);
	CatchRight->SetHiddenInGame(true);

	WaterWheelCondition = CreateDefaultSubobject<UUOUWaterWheelRainConditionComponent>(TEXT("WaterWheelCondition"));
	WaterWheelCondition->RotationTargetComponent = RotationPivot;
	WaterWheelCondition->RotationCenterComponent = RotationPivot;
	WaterWheelCondition->bUseOwnerRootWhenTargetMissing = false;
	WaterWheelCondition->bUseOwnerLocationWhenNoCatchPoints = false;
	WaterWheelCondition->RotationAxis = FVector(0.0f, 1.0f, 0.0f);
	WaterWheelCondition->MaxRotationSpeedDegreesPerSecond = 90.0f;
	WaterWheelCondition->AccelerationDegreesPerSecond = 60.0f;
	WaterWheelCondition->DecelerationDegreesPerSecond = 30.0f;
	WaterWheelCondition->PouredWaterSpeedMultiplier = 2.0f;
	WaterWheelCondition->MaxBoostedRotationSpeedDegreesPerSecond = 180.0f;
	WaterWheelCondition->SetAutoActivate(true);

	FUOUWaterWheelRainCatchPoint LeftCatchPoint;
	LeftCatchPoint.LocalOffset = FVector(-150.0f, 0.0f, 0.0f);
	LeftCatchPoint.Weight = 0.65f;
	LeftCatchPoint.CoverageRadius = 90.0f;

	FUOUWaterWheelRainCatchPoint RightCatchPoint;
	RightCatchPoint.LocalOffset = FVector(150.0f, 0.0f, 0.0f);
	RightCatchPoint.Weight = 1.0f;
	RightCatchPoint.CoverageRadius = 90.0f;

	WaterWheelCondition->RainCatchPoints = { LeftCatchPoint, RightCatchPoint };

	FastSpeedCondition = CreateDefaultSubobject<UUOUWaterWheelSpeedConditionComponent>(TEXT("FastSpeedCondition"));
	FastSpeedCondition->ObservedWaterWheelCondition = WaterWheelCondition;
	FastSpeedCondition->RequiredSpeedDegreesPerSecond = 120.0f;
	FastSpeedCondition->MaximumSpeedDegreesPerSecond = 180.0f;
	FastSpeedCondition->SetAutoActivate(true);
}

void AUOUWaterWheelRainConditionActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	EnsureDefaultRainCatchPoints();
}

void AUOUWaterWheelRainConditionActor::BeginPlay()
{
	Super::BeginPlay();

	EnsureDefaultRainCatchPoints();
	if (WaterWheelCondition != nullptr)
	{
		WaterWheelCondition->Activate(true);
		WaterWheelCondition->SetComponentTickEnabled(true);
	}
	if (FastSpeedCondition != nullptr)
	{
		FastSpeedCondition->Activate(true);
		FastSpeedCondition->SetComponentTickEnabled(true);
	}
}

void AUOUWaterWheelRainConditionActor::ResetDefaultRainCatchPoints()
{
	if (CatchLeft != nullptr)
	{
		CatchLeft->SetRelativeLocation(FVector(-150.0f, 0.0f, 0.0f));
	}
	if (CatchRight != nullptr)
	{
		CatchRight->SetRelativeLocation(FVector(150.0f, 0.0f, 0.0f));
	}

	ApplyDefaultRainCatchPointsFromArrows();
}

void AUOUWaterWheelRainConditionActor::EnsureDefaultRainCatchPoints()
{
	if (!bAutoCreateDefaultRainCatchPoints || WaterWheelCondition == nullptr)
	{
		return;
	}

	ApplyDefaultRainCatchPointsFromArrows();
}

void AUOUWaterWheelRainConditionActor::ApplyDefaultRainCatchPointsFromArrows()
{
	if (WaterWheelCondition == nullptr)
	{
		return;
	}

	FUOUWaterWheelRainCatchPoint LeftCatchPoint;
	LeftCatchPoint.LocalOffset = CatchLeft != nullptr ? CatchLeft->GetRelativeLocation() : FVector(-150.0f, 0.0f, 0.0f);
	LeftCatchPoint.Weight = 0.65f;
	LeftCatchPoint.CoverageRadius = 90.0f;

	FUOUWaterWheelRainCatchPoint RightCatchPoint;
	RightCatchPoint.LocalOffset = CatchRight != nullptr ? CatchRight->GetRelativeLocation() : FVector(150.0f, 0.0f, 0.0f);
	RightCatchPoint.Weight = 1.0f;
	RightCatchPoint.CoverageRadius = 90.0f;

	WaterWheelCondition->RainCatchPoints = { LeftCatchPoint, RightCatchPoint };
}
