// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUEnvironmentVisualComponent.h"

#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

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

	FVector GetEnvironmentVisualComponentSafeEffectScale(const UNiagaraComponent* Effect)
	{
		if (Effect == nullptr)
		{
			return FVector::OneVector;
		}

		const FVector AbsScale = Effect->GetComponentTransform().GetScale3D().GetAbs();
		return FVector(
			FMath::Max(AbsScale.X, UE_KINDA_SMALL_NUMBER),
			FMath::Max(AbsScale.Y, UE_KINDA_SMALL_NUMBER),
			FMath::Max(AbsScale.Z, UE_KINDA_SMALL_NUMBER));
	}
}

UUOUEnvironmentVisualComponent::UUOUEnvironmentVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUEnvironmentVisualComponent::SetEffectComponents(UNiagaraComponent* NewPrimaryEffect, UNiagaraComponent* NewSecondaryEffect)
{
	PrimaryEffect = NewPrimaryEffect;
	SecondaryEffect = NewSecondaryEffect;

	EnsureInternalEffectComponents();

	UNiagaraComponent* ActivePrimaryEffect = GetPrimaryEffectComponent();
	UNiagaraComponent* ActiveSecondaryEffect = GetSecondaryEffectComponent();

	if (ActivePrimaryEffect != nullptr)
	{
		ActivePrimaryEffect->SetAutoActivate(false);
	}

	if (ActiveSecondaryEffect != nullptr)
	{
		ActiveSecondaryEffect->SetAutoActivate(false);
	}

	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}

void UUOUEnvironmentVisualComponent::ConfigureRainVisual(
	const FVector& RainLocalPosition,
	const FVector& GroundSplashLocalPosition,
	const FRotator& EffectLocalRotation,
	const FVector2D& AreaSize,
	const FVector& RainKillVolumeLocalCenter,
	const FVector& RainKillVolumeSize)
{
	CachedPrimaryLocalPosition = RainLocalPosition;
	CachedSecondaryLocalPosition = GroundSplashLocalPosition;
	CachedEffectLocalRotation = EffectLocalRotation;
	CachedAreaSize = AreaSize;
	CachedRainKillVolumeLocalCenter = RainKillVolumeLocalCenter;
	CachedRainKillVolumeSize = RainKillVolumeSize;

	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
}

void UUOUEnvironmentVisualComponent::SetRainBlockerData(
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

void UUOUEnvironmentVisualComponent::SetVisualIntensities(float PrimaryIntensity, float SecondaryIntensity)
{
	CachedPrimaryIntensity = FMath::Clamp(PrimaryIntensity, 0.0f, 1.0f);
	CachedSecondaryIntensity = FMath::Clamp(SecondaryIntensity, 0.0f, 1.0f);

	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}

void UUOUEnvironmentVisualComponent::SetRainSpawnRate(float NewRainSpawnRate)
{
	CachedRainSpawnRate = FMath::Max(0.0f, NewRainSpawnRate);

	ApplyNiagaraParameters();
}

void UUOUEnvironmentVisualComponent::SetRainBlockerKillRadiusPadding(float NewKillRadiusPadding)
{
	RainBlockerKillRadiusPadding = FMath::Max(0.0f, NewKillRadiusPadding);

	ApplyNiagaraParameters();
}

void UUOUEnvironmentVisualComponent::SetVisualsEnabled(bool bNewEnabled)
{
	bEnableVisuals = bNewEnabled;
	RefreshNiagaraActivation();
}

void UUOUEnvironmentVisualComponent::OnRegister()
{
	Super::OnRegister();

	EnsureInternalEffectComponents();
	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}

void UUOUEnvironmentVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureInternalEffectComponents();
	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}

#if WITH_EDITOR
void UUOUEnvironmentVisualComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.Property != nullptr
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	const bool bPrimarySystemChanged = PropertyName == GET_MEMBER_NAME_CHECKED(UUOUEnvironmentVisualComponent, PrimarySystem);
	const bool bSecondarySystemChanged = PropertyName == GET_MEMBER_NAME_CHECKED(UUOUEnvironmentVisualComponent, SecondarySystem);

	EnsureInternalEffectComponents();
	ApplyVisualEffectSettings(bPrimarySystemChanged, bSecondarySystemChanged);

	Super::PostEditChangeProperty(PropertyChangedEvent);

	EnsureInternalEffectComponents();
	ApplyVisualEffectSettings(bPrimarySystemChanged, bSecondarySystemChanged);
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}
#endif

void UUOUEnvironmentVisualComponent::EnsureInternalEffectComponents()
{
	if (!bAutoCreateMissingEffectComponents
		|| HasAnyFlags(RF_ClassDefaultObject)
		|| GetOwner() == nullptr
		|| GetOwner()->HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	AActor* Owner = GetOwner();

	if (PrimaryEffect == nullptr && InternalPrimaryEffect == nullptr)
	{
		const FName ComponentName = MakeUniqueObjectName(Owner, UNiagaraComponent::StaticClass(), TEXT("RainVisualPrimaryEffect"));
		InternalPrimaryEffect = NewObject<UNiagaraComponent>(Owner, ComponentName, RF_Transient);
		if (InternalPrimaryEffect != nullptr)
		{
			InternalPrimaryEffect->CreationMethod = EComponentCreationMethod::Instance;
			InternalPrimaryEffect->SetAutoActivate(false);
			InternalPrimaryEffect->SetupAttachment(this);
			Owner->AddInstanceComponent(InternalPrimaryEffect);
			if (IsRegistered() && !InternalPrimaryEffect->IsRegistered())
			{
				InternalPrimaryEffect->RegisterComponent();
			}
		}
	}

	if (SecondaryEffect == nullptr && InternalSecondaryEffect == nullptr)
	{
		const FName ComponentName = MakeUniqueObjectName(Owner, UNiagaraComponent::StaticClass(), TEXT("RainVisualSecondaryEffect"));
		InternalSecondaryEffect = NewObject<UNiagaraComponent>(Owner, ComponentName, RF_Transient);
		if (InternalSecondaryEffect != nullptr)
		{
			InternalSecondaryEffect->CreationMethod = EComponentCreationMethod::Instance;
			InternalSecondaryEffect->SetAutoActivate(false);
			InternalSecondaryEffect->SetupAttachment(this);
			Owner->AddInstanceComponent(InternalSecondaryEffect);
			if (IsRegistered() && !InternalSecondaryEffect->IsRegistered())
			{
				InternalSecondaryEffect->RegisterComponent();
			}
		}
	}
}

UNiagaraComponent* UUOUEnvironmentVisualComponent::GetPrimaryEffectComponent() const
{
	return PrimaryEffect != nullptr
		? PrimaryEffect.Get()
		: (bAutoCreateMissingEffectComponents ? InternalPrimaryEffect.Get() : nullptr);
}

UNiagaraComponent* UUOUEnvironmentVisualComponent::GetSecondaryEffectComponent() const
{
	return SecondaryEffect != nullptr
		? SecondaryEffect.Get()
		: (bAutoCreateMissingEffectComponents ? InternalSecondaryEffect.Get() : nullptr);
}

void UUOUEnvironmentVisualComponent::ApplyVisualEffectSettings(bool bForcePrimarySystem, bool bForceSecondarySystem)
{
	ApplyNiagaraSystemSelection(GetPrimaryEffectComponent(), PrimarySystem, LastAppliedPrimarySystem, bForcePrimarySystem);
	ApplyNiagaraSystemSelection(GetSecondaryEffectComponent(), SecondarySystem, LastAppliedSecondarySystem, bForceSecondarySystem);
}

void UUOUEnvironmentVisualComponent::RefreshNiagaraActivation()
{
	const UWorld* World = GetWorld();
	const bool bIsGameWorld = World != nullptr && World->IsGameWorld();
	const bool bShouldAllowVisuals = bEnableVisuals && (bIsGameWorld || bEnableEditorPreview);
	UNiagaraComponent* ActivePrimaryEffect = GetPrimaryEffectComponent();
	UNiagaraComponent* ActiveSecondaryEffect = GetSecondaryEffectComponent();

	if ((PrimaryEffect != nullptr || !bAutoCreateMissingEffectComponents) && InternalPrimaryEffect != nullptr)
	{
		InternalPrimaryEffect->SetVisibility(false, true);
		InternalPrimaryEffect->Deactivate();
	}

	if ((SecondaryEffect != nullptr || !bAutoCreateMissingEffectComponents) && InternalSecondaryEffect != nullptr)
	{
		InternalSecondaryEffect->SetVisibility(false, true);
		InternalSecondaryEffect->Deactivate();
	}

	const bool bShouldShowPrimary = bShouldAllowVisuals
		&& ActivePrimaryEffect != nullptr
		&& ActivePrimaryEffect->GetAsset() != nullptr
		&& CachedPrimaryIntensity > UE_KINDA_SMALL_NUMBER;

	if (ActivePrimaryEffect != nullptr)
	{
		const bool bWasPrimaryVisible = ActivePrimaryEffect->IsVisible();
		ActivePrimaryEffect->SetVisibility(bShouldShowPrimary, true);

		if (bShouldShowPrimary && (!ActivePrimaryEffect->IsActive() || !bWasPrimaryVisible))
		{
			ActivePrimaryEffect->Activate(true);
			ApplyNiagaraParameters();
		}
		else if (!bShouldShowPrimary && ActivePrimaryEffect->IsActive())
		{
			ActivePrimaryEffect->Deactivate();
		}
	}

	const bool bShouldShowSecondary = bShouldAllowVisuals
		&& ActiveSecondaryEffect != nullptr
		&& ActiveSecondaryEffect->GetAsset() != nullptr
		&& CachedSecondaryIntensity > UE_KINDA_SMALL_NUMBER;

	if (ActiveSecondaryEffect != nullptr)
	{
		const bool bWasSecondaryVisible = ActiveSecondaryEffect->IsVisible();
		ActiveSecondaryEffect->SetVisibility(bShouldShowSecondary, true);

		if (bShouldShowSecondary && (!ActiveSecondaryEffect->IsActive() || !bWasSecondaryVisible))
		{
			ActiveSecondaryEffect->Activate(true);
			ApplyNiagaraParameters();
		}
		else if (!bShouldShowSecondary && ActiveSecondaryEffect->IsActive())
		{
			ActiveSecondaryEffect->Deactivate();
		}
	}
}

void UUOUEnvironmentVisualComponent::ApplyNiagaraParameters()
{
	UNiagaraComponent* ActivePrimaryEffect = GetPrimaryEffectComponent();
	UNiagaraComponent* ActiveSecondaryEffect = GetSecondaryEffectComponent();

	if (ActivePrimaryEffect != nullptr && !PrimaryAreaSizeParameterName.IsNone())
	{
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(ActivePrimaryEffect);
		const FVector2D EffectLocalAreaSize(
			CachedAreaSize.X / EffectScale.X,
			CachedAreaSize.Y / EffectScale.Y);
		ActivePrimaryEffect->SetVariableVec2(PrimaryAreaSizeParameterName, EffectLocalAreaSize);
	}

	if (ActivePrimaryEffect != nullptr && !RainKillVolumeCenterParameterName.IsNone())
	{
		const FVector KillVolumeWorldCenter = GetComponentTransform().TransformPosition(CachedRainKillVolumeLocalCenter);
		const FVector KillVolumeEffectLocalCenter = ActivePrimaryEffect->GetComponentTransform().InverseTransformPosition(KillVolumeWorldCenter);

		ActivePrimaryEffect->SetVariableVec3(RainKillVolumeCenterParameterName, KillVolumeEffectLocalCenter);
	}

	if (ActivePrimaryEffect != nullptr && !RainKillVolumeSizeParameterName.IsNone())
	{
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(ActivePrimaryEffect);
		const FVector EffectLocalKillVolumeSize(
			CachedRainKillVolumeSize.X / EffectScale.X,
			CachedRainKillVolumeSize.Y / EffectScale.Y,
			CachedRainKillVolumeSize.Z / EffectScale.Z);
		ActivePrimaryEffect->SetVariableVec3(RainKillVolumeSizeParameterName, EffectLocalKillVolumeSize);
	}

	if (ActivePrimaryEffect != nullptr && !PrimaryIntensityParameterName.IsNone())
	{
		ActivePrimaryEffect->SetVariableFloat(PrimaryIntensityParameterName, CachedPrimaryIntensity);
	}

	if (ActivePrimaryEffect != nullptr && !RainSpawnRateParameterName.IsNone())
	{
		ActivePrimaryEffect->SetVariableFloat(RainSpawnRateParameterName, CachedRainSpawnRate * CachedPrimaryIntensity);
	}

	if (ActiveSecondaryEffect != nullptr && !SecondaryAreaSizeParameterName.IsNone())
	{
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(ActiveSecondaryEffect);
		const FVector2D EffectLocalAreaSize(
			CachedAreaSize.X / EffectScale.X,
			CachedAreaSize.Y / EffectScale.Y);
		ActiveSecondaryEffect->SetVariableVec2(SecondaryAreaSizeParameterName, EffectLocalAreaSize);
	}

	if (ActiveSecondaryEffect != nullptr && !SecondaryIntensityParameterName.IsNone())
	{
		ActiveSecondaryEffect->SetVariableFloat(SecondaryIntensityParameterName, CachedSecondaryIntensity);
	}

	auto ApplyRainBlockerParameters = [this](UNiagaraComponent* Effect)
	{
		if (Effect == nullptr)
		{
			return;
		}

		const FVector BlockerWorldPosition = GetComponentTransform().TransformPosition(CachedRainBlockerLocalPosition);
		const FVector EffectLocalBlockerPosition = bCachedRainBlockerActive
			? Effect->GetComponentTransform().InverseTransformPosition(BlockerWorldPosition)
			: FVector::ZeroVector;
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(Effect);
		const float EffectHorizontalScale = FMath::Max(FMath::Min(EffectScale.X, EffectScale.Y), UE_KINDA_SMALL_NUMBER);
		const float EffectiveBlockerRadius = bCachedRainBlockerActive
			? (CachedRainBlockerRadius + FMath::Max(0.0f, RainBlockerKillRadiusPadding)) / EffectHorizontalScale
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

	ApplyRainBlockerParameters(ActivePrimaryEffect);
	ApplyRainBlockerParameters(ActiveSecondaryEffect);
}

void UUOUEnvironmentVisualComponent::ApplyVisualEffectTransforms()
{
	UNiagaraComponent* ActivePrimaryEffect = GetPrimaryEffectComponent();
	UNiagaraComponent* ActiveSecondaryEffect = GetSecondaryEffectComponent();

	if (ActivePrimaryEffect != nullptr)
	{
		ActivePrimaryEffect->SetRelativeLocation(CachedPrimaryLocalPosition);
		ActivePrimaryEffect->SetRelativeRotation(CachedEffectLocalRotation);
	}

	if (ActiveSecondaryEffect != nullptr)
	{
		ActiveSecondaryEffect->SetRelativeLocation(CachedSecondaryLocalPosition);
		ActiveSecondaryEffect->SetRelativeRotation(CachedEffectLocalRotation);
	}
}

void UUOUEnvironmentVisualComponent::DrawRainBlockerNiagaraDebug(const UNiagaraComponent* Effect, const FVector& EffectLocalBlockerPosition, float EffectLocalBlockerRadius) const
{
	if (!bDrawRainBlockerNiagaraDebug
		|| !bCachedRainBlockerActive
		|| !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::VFX)
		|| Effect == nullptr
		|| GetWorld() == nullptr
		|| EffectLocalBlockerRadius <= 0.0f)
	{
		return;
	}

	const FVector DebugWorldLocation = Effect->GetComponentTransform().TransformPosition(EffectLocalBlockerPosition);
	const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(Effect);
	const float EffectHorizontalScale = FMath::Max(FMath::Min(EffectScale.X, EffectScale.Y), UE_KINDA_SMALL_NUMBER);
	const float DebugWorldRadius = EffectLocalBlockerRadius * EffectHorizontalScale;
	const float CenterRadius = FMath::Max(6.0f, RainBlockerNiagaraDebugThickness * 2.0f);
	const FColor VFXDebugColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::VFX, FColor::Magenta);

	DrawDebugSphere(
		GetWorld(),
		DebugWorldLocation,
		DebugWorldRadius,
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
