// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Overlap/UOUOverlapConditionActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Puzzle/Overlap/UOUOverlapConditionComponent.h"

AUOUOverlapConditionActor::AUOUOverlapConditionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	OverlapVolume->SetupAttachment(RootScene);
	OverlapVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	OverlapVolume->SetGenerateOverlapEvents(true);
	OverlapVolume->SetBoxExtent(FVector(100.0f, 100.0f, 80.0f));
	OverlapVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	OverlapVolume->SetHiddenInGame(true);
	OverlapVolume->ShapeColor = FColor::Green;

	OverlapConditionComponent = CreateDefaultSubobject<UUOUOverlapConditionComponent>(TEXT("OverlapConditionComponent"));
	OverlapConditionComponent->bAutoFindOverlapVolume = true;
	OverlapConditionComponent->OverlapVolume = OverlapVolume;
}
