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
	ApplyNiagaraParameters();
}

void AUOUEnvironmentVisualActor::SetVisualIntensities(float PrimaryIntensity, float SecondaryIntensity)
{
	CachedPrimaryIntensity = FMath::Clamp(PrimaryIntensity, 0.0f, 1.0f);
	CachedSecondaryIntensity = FMath::Clamp(SecondaryIntensity, 0.0f, 1.0f);

	ApplyNiagaraParameters();
}

void AUOUEnvironmentVisualActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
}

void AUOUEnvironmentVisualActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
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

void AUOUEnvironmentVisualActor::ApplyNiagaraParameters()
{
	if (PrimaryEffect != nullptr && !PrimaryAreaSizeParameterName.IsNone())
	{
		PrimaryEffect->SetVariableVec2(PrimaryAreaSizeParameterName, CachedAreaSize);
	}

	if (PrimaryEffect != nullptr && !PrimaryIntensityParameterName.IsNone())
	{
		PrimaryEffect->SetVariableFloat(PrimaryIntensityParameterName, CachedPrimaryIntensity);
	}

	if (SecondaryEffect != nullptr && !SecondaryAreaSizeParameterName.IsNone())
	{
		SecondaryEffect->SetVariableVec2(SecondaryAreaSizeParameterName, CachedAreaSize);
	}

	if (SecondaryEffect != nullptr && !SecondaryIntensityParameterName.IsNone())
	{
		SecondaryEffect->SetVariableFloat(SecondaryIntensityParameterName, CachedSecondaryIntensity);
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
