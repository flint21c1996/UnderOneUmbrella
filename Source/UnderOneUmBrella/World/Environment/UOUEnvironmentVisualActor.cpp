// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUEnvironmentVisualActor.h"

#include "Components/SceneComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"
#include "UObject/UnrealType.h"

namespace
{
	void ApplyNiagaraSystemSelection(
		UNiagaraComponent* NiagaraComponent,
		TObjectPtr<UNiagaraSystem>& StoredSystem,
		TObjectPtr<UNiagaraSystem>& LastAppliedSystem,
		bool bForceStoredSystem)
	{
		if (NiagaraComponent == nullptr)
		{
			return;
		}

		UNiagaraSystem* DesiredSystem = StoredSystem.Get();
		UNiagaraSystem* ComponentSystem = NiagaraComponent->GetAsset();
		UNiagaraSystem* PreviousSystem = LastAppliedSystem.Get();

		if (bForceStoredSystem || DesiredSystem != PreviousSystem)
		{
			NiagaraComponent->SetAsset(DesiredSystem);
			LastAppliedSystem = DesiredSystem;
			return;
		}

		if (ComponentSystem != PreviousSystem)
		{
			StoredSystem = ComponentSystem;
			LastAppliedSystem = ComponentSystem;
			return;
		}

		if (ComponentSystem != DesiredSystem)
		{
			NiagaraComponent->SetAsset(DesiredSystem);
			LastAppliedSystem = DesiredSystem;
		}
	}
}

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

void AUOUEnvironmentVisualActor::SetRainBlockerData(
	bool bIsBlocking,
	const FVector& BlockerLocalPosition,
	float BlockerRadius,
	float BlockerIntensity)
{
	bCachedRainBlockerActive = bIsBlocking && BlockerRadius > 0.0f && BlockerIntensity > 0.0f;
	CachedRainBlockerLocalPosition = bCachedRainBlockerActive ? BlockerLocalPosition : FVector::ZeroVector;
	CachedRainBlockerRadius = bCachedRainBlockerActive ? FMath::Max(0.0f, BlockerRadius) : 0.0f;
	CachedRainBlockerIntensity = bCachedRainBlockerActive ? FMath::Clamp(BlockerIntensity, 0.0f, 1.0f) : 0.0f;

	ApplyNiagaraParameters();
}

void AUOUEnvironmentVisualActor::SetVisualIntensities(float PrimaryIntensity, float SecondaryIntensity)
{
	CachedPrimaryIntensity = FMath::Clamp(PrimaryIntensity, 0.0f, 1.0f);
	CachedSecondaryIntensity = FMath::Clamp(SecondaryIntensity, 0.0f, 1.0f);

	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}

void AUOUEnvironmentVisualActor::SetVisualsEnabled(bool bNewEnabled)
{
	bEnableVisuals = bNewEnabled;
	RefreshNiagaraActivation();
}

void AUOUEnvironmentVisualActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}

void AUOUEnvironmentVisualActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}

#if WITH_EDITOR
void AUOUEnvironmentVisualActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.Property != nullptr
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	const bool bPrimarySystemChanged = PropertyName == GET_MEMBER_NAME_CHECKED(AUOUEnvironmentVisualActor, PrimarySystem);
	const bool bSecondarySystemChanged = PropertyName == GET_MEMBER_NAME_CHECKED(AUOUEnvironmentVisualActor, SecondarySystem);

	ApplyVisualEffectSettings(bPrimarySystemChanged, bSecondarySystemChanged);

	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplyVisualEffectSettings(bPrimarySystemChanged, bSecondarySystemChanged);
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}
#endif

void AUOUEnvironmentVisualActor::ApplyVisualEffectSettings(bool bForcePrimarySystem, bool bForceSecondarySystem)
{
	ApplyNiagaraSystemSelection(PrimaryEffect, PrimarySystem, LastAppliedPrimarySystem, bForcePrimarySystem);
	ApplyNiagaraSystemSelection(SecondaryEffect, SecondarySystem, LastAppliedSecondarySystem, bForceSecondarySystem);
}

void AUOUEnvironmentVisualActor::RefreshNiagaraActivation()
{
	const bool bShouldShowPrimary = bEnableVisuals
		&& PrimaryEffect != nullptr
		&& PrimaryEffect->GetAsset() != nullptr
		&& CachedPrimaryIntensity > UE_KINDA_SMALL_NUMBER;

	if (PrimaryEffect != nullptr)
	{
		const bool bWasPrimaryVisible = PrimaryEffect->IsVisible();
		PrimaryEffect->SetVisibility(bShouldShowPrimary, true);

		if (bShouldShowPrimary && (!PrimaryEffect->IsActive() || !bWasPrimaryVisible))
		{
			PrimaryEffect->Activate(true);
			ApplyNiagaraParameters();
		}
		else if (!bShouldShowPrimary && PrimaryEffect->IsActive())
		{
			PrimaryEffect->Deactivate();
		}
	}

	const bool bShouldShowSecondary = bEnableVisuals
		&& SecondaryEffect != nullptr
		&& SecondaryEffect->GetAsset() != nullptr
		&& CachedSecondaryIntensity > UE_KINDA_SMALL_NUMBER;

	if (SecondaryEffect != nullptr)
	{
		const bool bWasSecondaryVisible = SecondaryEffect->IsVisible();
		SecondaryEffect->SetVisibility(bShouldShowSecondary, true);

		if (bShouldShowSecondary && (!SecondaryEffect->IsActive() || !bWasSecondaryVisible))
		{
			SecondaryEffect->Activate(true);
			ApplyNiagaraParameters();
		}
		else if (!bShouldShowSecondary && SecondaryEffect->IsActive())
		{
			SecondaryEffect->Deactivate();
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

	auto ApplyRainBlockerParameters = [this](UNiagaraComponent* Effect)
	{
		if (Effect == nullptr)
		{
			return;
		}

		const FVector BlockerWorldPosition = GetActorTransform().TransformPosition(CachedRainBlockerLocalPosition);
		const FVector EffectLocalBlockerPosition = bCachedRainBlockerActive
			? Effect->GetComponentTransform().InverseTransformPosition(BlockerWorldPosition)
			: FVector::ZeroVector;
		const float EffectiveBlockerRadius = bCachedRainBlockerActive
			? CachedRainBlockerRadius + FMath::Max(0.0f, RainBlockerKillRadiusPadding)
			: 0.0f;

		if (!RainBlockerActiveParameterName.IsNone())
		{
			Effect->SetVariableBool(RainBlockerActiveParameterName, bCachedRainBlockerActive);
		}

		if (!RainBlockerLocalPositionParameterName.IsNone())
		{
			Effect->SetVariableVec3(RainBlockerLocalPositionParameterName, EffectLocalBlockerPosition);
		}

		if (!RainBlockerRadiusParameterName.IsNone())
		{
			Effect->SetVariableFloat(RainBlockerRadiusParameterName, EffectiveBlockerRadius);
		}

		if (!RainBlockerIntensityParameterName.IsNone())
		{
			Effect->SetVariableFloat(RainBlockerIntensityParameterName, CachedRainBlockerIntensity);
		}

		DrawRainBlockerNiagaraDebug(Effect, EffectLocalBlockerPosition, EffectiveBlockerRadius);
	};

	ApplyRainBlockerParameters(PrimaryEffect);
	ApplyRainBlockerParameters(SecondaryEffect);
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

void AUOUEnvironmentVisualActor::DrawRainBlockerNiagaraDebug(const UNiagaraComponent* Effect, const FVector& EffectLocalBlockerPosition, float EffectiveBlockerRadius) const
{
	if (!bDrawRainBlockerNiagaraDebug
		|| !bCachedRainBlockerActive
		|| !UUOUDebugSubsystem::IsDebugCategoryEnabled(this, EUOUDebugCategory::VFX)
		|| Effect == nullptr
		|| GetWorld() == nullptr
		|| EffectiveBlockerRadius <= 0.0f)
	{
		return;
	}

	const FVector DebugWorldLocation = Effect->GetComponentTransform().TransformPosition(EffectLocalBlockerPosition);
	const float CenterRadius = FMath::Max(6.0f, RainBlockerNiagaraDebugThickness * 2.0f);
	const FColor VFXDebugColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::VFX, FColor::Magenta);

	DrawDebugSphere(
		GetWorld(),
		DebugWorldLocation,
		EffectiveBlockerRadius,
		24,
		VFXDebugColor,
		false,
		0.0f,
		0,
		RainBlockerNiagaraDebugThickness);

	DrawDebugSphere(
		GetWorld(),
		DebugWorldLocation,
		CenterRadius,
		8,
		VFXDebugColor,
		false,
		0.0f,
		0,
		RainBlockerNiagaraDebugThickness);
}
