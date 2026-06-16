// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Interaction/UOUInteractionConditionActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Puzzle/Interaction/UOUInteractionConditionComponent.h"

AUOUInteractionConditionActor::AUOUInteractionConditionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootScene);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionVolume->SetGenerateOverlapEvents(false);
	InteractionVolume->SetBoxExtent(FVector(60.0f, 60.0f, 80.0f));
	InteractionVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	InteractionVolume->SetHiddenInGame(true);
	InteractionVolume->ShapeColor = FColor::Yellow;

	InteractionConditionComponent = CreateDefaultSubobject<UUOUInteractionConditionComponent>(
		TEXT("InteractionConditionComponent"));
	InteractionConditionComponent->bInitialSatisfied = false;
	InteractionConditionComponent->InteractionMode = EUOUInteractionConditionMode::SetSatisfied;
}
