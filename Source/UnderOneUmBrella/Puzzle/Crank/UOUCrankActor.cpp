// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Crank/UOUCrankActor.h"

#include "Components/StaticMeshComponent.h"
#include "Puzzle/Crank/UOUCrankComponent.h"

AUOUCrankActor::AUOUCrankActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CrankVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrankVisual"));
	SetRootComponent(CrankVisual);
	CrankVisual->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	CrankComponent = CreateDefaultSubobject<UUOUCrankComponent>(TEXT("CrankComponent"));
	CrankComponent->RotationTargetComponent = CrankVisual;
}
