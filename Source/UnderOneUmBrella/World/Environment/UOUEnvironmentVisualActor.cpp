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

void AUOUEnvironmentVisualActor::ConfigureRainVisual(
	const FVector& RainLocalPosition,
	const FVector& GroundSplashLocalPosition,
	const FRotator& EffectLocalRotation,
	const FVector2D& AreaSize)
{
	CachedPrimaryLocalPosition = RainLocalPosition;
	CachedSecondaryLocalPosition = GroundSplashLocalPosition;
	CachedEffectLocalRotation = EffectLocalRotation;
	CachedAreaSize = AreaSize;

	ApplyVisualEffectTransforms();
}

void AUOUEnvironmentVisualActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
}

void AUOUEnvironmentVisualActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
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

void AUOUEnvironmentVisualActor::ApplyVisualEffectTransforms()
{
	if (PrimaryEffect != nullptr)
	{
		PrimaryEffect->SetRelativeLocation(CachedPrimaryLocalPosition);
		PrimaryEffect->SetRelativeRotation(CachedEffectLocalRotation);
	}

	if (SecondaryEffect != nullptr)
	{
		SecondaryEffect->SetRelativeLocation(CachedSecondaryLocalPosition);
		SecondaryEffect->SetRelativeRotation(CachedEffectLocalRotation);
	}
}
