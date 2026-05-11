// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Weight/UOUWeightedButtonActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Puzzle/Weight/UOUWeightedButtonComponent.h"
#include "Puzzle/Weight/UOUWeightSensorComponent.h"
#include "UObject/ConstructorHelpers.h"

AUOUWeightedButtonActor::AUOUWeightedButtonActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	WeightSensorVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WeightSensorVolume"));
	WeightSensorVolume->SetupAttachment(RootScene);
	WeightSensorVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeightSensorVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeightSensorVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeightSensorVolume->SetGenerateOverlapEvents(true);
	WeightSensorVolume->SetBoxExtent(FVector(80.0f, 80.0f, 30.0f));
	WeightSensorVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));

	ButtonVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonVisual"));
	ButtonVisual->SetupAttachment(RootScene);
	ButtonVisual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ButtonVisual->SetGenerateOverlapEvents(false);
	ButtonVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		ButtonVisual->SetStaticMesh(CubeMeshFinder.Object);
		ButtonVisual->SetRelativeScale3D(FVector(1.5f, 1.5f, 0.2f));
	}

	ReleasedPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ReleasedPoint"));
	ReleasedPoint->SetupAttachment(RootScene);
	ReleasedPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));

	PressedPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PressedPoint"));
	PressedPoint->SetupAttachment(RootScene);
	PressedPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));

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
