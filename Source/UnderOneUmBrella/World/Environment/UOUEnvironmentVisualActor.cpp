// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUEnvironmentVisualActor.h"

#include "Components/SceneComponent.h"
#include "NiagaraComponent.h"

AUOUEnvironmentVisualActor::AUOUEnvironmentVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	PrimaryEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PrimaryEffect"));
	PrimaryEffect->SetupAttachment(RootScene);
	PrimaryEffect->SetAutoActivate(false);

	SecondaryEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SecondaryEffect"));
	SecondaryEffect->SetupAttachment(RootScene);
	SecondaryEffect->SetAutoActivate(false);
}
