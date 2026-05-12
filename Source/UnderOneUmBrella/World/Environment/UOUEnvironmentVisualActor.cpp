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

void AUOUEnvironmentVisualActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyVisualEffectSettings();
}

void AUOUEnvironmentVisualActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyVisualEffectSettings();
}

void AUOUEnvironmentVisualActor::ApplyVisualEffectSettings()
{
	if (PrimaryEffect != nullptr)
	{
		if (PrimarySystem != nullptr)
		{
			PrimaryEffect->SetAsset(PrimarySystem);
		}
		else if (PrimaryEffect->GetAsset() != nullptr)
		{
			PrimarySystem = PrimaryEffect->GetAsset();
		}
	}

	if (SecondaryEffect != nullptr)
	{
		if (SecondarySystem != nullptr)
		{
			SecondaryEffect->SetAsset(SecondarySystem);
		}
		else if (SecondaryEffect->GetAsset() != nullptr)
		{
			SecondarySystem = SecondaryEffect->GetAsset();
		}
	}
}
