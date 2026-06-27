// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOUTitleLevelTransitionActor.h"

#include "Components/SceneComponent.h"

AUOUTitleLevelTransitionActor::AUOUTitleLevelTransitionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
}
