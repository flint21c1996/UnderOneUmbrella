// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOULevelTransitionSettingsActor.h"

#include "Components/SceneComponent.h"

AUOULevelTransitionSettingsActor::AUOULevelTransitionSettingsActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
}
