// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUEnvironmentVisualComponent.h"

#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraParameterStore.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "UObject/UnrealType.h"

namespace
{
	void ApplyEnvironmentVisualComponentNiagaraSystemSelection(
		UNiagaraComponent* NiagaraComponent,
		TObjectPtr<UNiagaraSystem>& StoredSystem,
		TObjectPtr<UNiagaraSystem>& LastAppliedSystem,
		bool bForceStoredSystem)
	{
		if (NiagaraComponent == nullptr || !NiagaraComponent->IsRegistered())
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

	const FNiagaraVariableWithOffset* FindEnvironmentVisualComponentNiagaraParameter(const UNiagaraComponent* Effect, FName ParameterName)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return nullptr;
		}

		const FNiagaraVariable QueryParameter(FNiagaraTypeDefinition::GetVec4Def(), ParameterName);
		if (const FNiagaraVariableWithOffset* OverrideParameter = Effect->GetOverrideParameters().FindParameterVariable(QueryParameter, true))
		{
			return OverrideParameter;
		}

		const UNiagaraSystem* NiagaraSystem = Effect->GetAsset();
		if (NiagaraSystem == nullptr)
		{
			return nullptr;
		}

		return NiagaraSystem->GetExposedParameters().FindParameterVariable(QueryParameter, true);
	}

	bool CanSetEnvironmentVisualComponentNiagaraParameter(const UNiagaraComponent* Effect, FName ParameterName, const FNiagaraTypeDefinition& ExpectedType)
	{
		const FNiagaraVariableWithOffset* ExistingParameter = FindEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName);
		return ExistingParameter == nullptr || ExistingParameter->GetType() == ExpectedType;
	}

	void SetEnvironmentVisualComponentNiagaraAreaSize(UNiagaraComponent* Effect, FName ParameterName, const FVector2D& AreaSize)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return;
		}

		const FNiagaraVariableWithOffset* ExistingParameter = FindEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName);
		if (ExistingParameter == nullptr || ExistingParameter->GetType() == FNiagaraTypeDefinition::GetVec2Def())
		{
			Effect->SetVariableVec2(ParameterName, AreaSize);
			return;
		}

		if (ExistingParameter->GetType() == FNiagaraTypeDefinition::GetVec3Def())
		{
			Effect->SetVariableVec3(ParameterName, FVector(AreaSize.X, AreaSize.Y, 0.0));
		}
	}

	void SetEnvironmentVisualComponentNiagaraVec3(UNiagaraComponent* Effect, FName ParameterName, const FVector& Value)
	{
		if (Effect != nullptr && !ParameterName.IsNone()
			&& CanSetEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName, FNiagaraTypeDefinition::GetVec3Def()))
		{
			Effect->SetVariableVec3(ParameterName, Value);
		}
	}

	void SetEnvironmentVisualComponentNiagaraFloat(UNiagaraComponent* Effect, FName ParameterName, float Value)
	{
		if (Effect != nullptr && !ParameterName.IsNone()
			&& CanSetEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName, FNiagaraTypeDefinition::GetFloatDef()))
		{
			Effect->SetVariableFloat(ParameterName, Value);
		}
	}

	void SetEnvironmentVisualComponentNiagaraBool(UNiagaraComponent* Effect, FName ParameterName, bool bValue)
	{
		if (Effect != nullptr && !ParameterName.IsNone()
			&& CanSetEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName, FNiagaraTypeDefinition::GetBoolDef()))
		{
			Effect->SetVariableBool(ParameterName, bValue);
		}
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
}

void UUOUEnvironmentVisualComponent::SetEffectSystems(UNiagaraSystem* NewPrimarySystem, UNiagaraSystem* NewSecondarySystem)
{
	PrimarySystem = NewPrimarySystem;
	SecondarySystem = NewSecondarySystem;

	ApplyVisualEffectSettings(true, true);
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
	const FVector& BlockerHalfExtent,
	float BlockerIntensity)
{
	const FVector SafeHalfExtent(
		FMath::Max(0.0f, BlockerHalfExtent.X),
		FMath::Max(0.0f, BlockerHalfExtent.Y),
		FMath::Max(0.0f, BlockerHalfExtent.Z));

	bCachedRainBlockerActive = bIsBlocking && !SafeHalfExtent.IsNearlyZero() && BlockerIntensity > 0.0f;
	CachedRainBlockerLocalPosition = bCachedRainBlockerActive ? BlockerLocalPosition : FVector::ZeroVector;
	CachedRainBlockerHalfExtent = bCachedRainBlockerActive ? SafeHalfExtent : FVector::ZeroVector;
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

void UUOUEnvironmentVisualComponent::SetRainFallSpeed(float NewRainFallSpeed)
{
	CachedRainFallSpeed = NewRainFallSpeed;

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
	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
}

void UUOUEnvironmentVisualComponent::BeginPlay()
{
	Super::BeginPlay();
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
	ApplyVisualEffectSettings(bPrimarySystemChanged, bSecondarySystemChanged);

	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyVisualEffectSettings(bPrimarySystemChanged, bSecondarySystemChanged);
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();

	// 마우스 드래그를 끝냈거나 단순 수치 입력 완료 시에만 안전하게 이펙트 상태를 갱신합니다.
	RefreshNiagaraActivation();
}
#endif

UNiagaraComponent* UUOUEnvironmentVisualComponent::GetPrimaryEffectComponent() const
{
	return PrimaryEffect.Get();
}

UNiagaraComponent* UUOUEnvironmentVisualComponent::GetSecondaryEffectComponent() const
{
	return SecondaryEffect.Get();
}

void UUOUEnvironmentVisualComponent::ApplyVisualEffectSettings(bool bForcePrimarySystem, bool bForceSecondarySystem)
{
	if (!CanApplyNiagaraState())
	{
		return;
	}

	ApplyEnvironmentVisualComponentNiagaraSystemSelection(GetPrimaryEffectComponent(), PrimarySystem, LastAppliedPrimarySystem, bForcePrimarySystem);
	ApplyEnvironmentVisualComponentNiagaraSystemSelection(GetSecondaryEffectComponent(), SecondarySystem, LastAppliedSecondarySystem, bForceSecondarySystem);
}

void UUOUEnvironmentVisualComponent::RefreshNiagaraActivation()
{
	if (!CanApplyNiagaraState())
	{
		return;
	}

	const UWorld* World = GetWorld();
	const bool bIsGameWorld = World != nullptr && World->IsGameWorld();
	const bool bShouldAllowVisuals = bEnableVisuals && (bIsGameWorld || bEnableEditorPreview);
	UNiagaraComponent* ActivePrimaryEffect = GetPrimaryEffectComponent();
	UNiagaraComponent* ActiveSecondaryEffect = GetSecondaryEffectComponent();
	ActivePrimaryEffect = ActivePrimaryEffect != nullptr && ActivePrimaryEffect->IsRegistered() ? ActivePrimaryEffect : nullptr;
	ActiveSecondaryEffect = ActiveSecondaryEffect != nullptr && ActiveSecondaryEffect->IsRegistered() ? ActiveSecondaryEffect : nullptr;

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
	if (!CanApplyNiagaraState())
	{
		return;
	}

	UNiagaraComponent* ActivePrimaryEffect = GetPrimaryEffectComponent();
	UNiagaraComponent* ActiveSecondaryEffect = GetSecondaryEffectComponent();
	ActivePrimaryEffect = ActivePrimaryEffect != nullptr && ActivePrimaryEffect->IsRegistered() ? ActivePrimaryEffect : nullptr;
	ActiveSecondaryEffect = ActiveSecondaryEffect != nullptr && ActiveSecondaryEffect->IsRegistered() ? ActiveSecondaryEffect : nullptr;

	if (ActivePrimaryEffect != nullptr && !PrimaryAreaSizeParameterName.IsNone())
	{
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(ActivePrimaryEffect);
		const FVector2D EffectLocalAreaSize(
			CachedAreaSize.X / EffectScale.X,
			CachedAreaSize.Y / EffectScale.Y);
		SetEnvironmentVisualComponentNiagaraAreaSize(ActivePrimaryEffect, PrimaryAreaSizeParameterName, EffectLocalAreaSize);
	}

	if (ActivePrimaryEffect != nullptr && !RainKillVolumeCenterParameterName.IsNone())
	{
		const FVector KillVolumeWorldCenter = GetComponentTransform().TransformPosition(CachedRainKillVolumeLocalCenter);
		const FVector KillVolumeEffectLocalCenter = ActivePrimaryEffect->GetComponentTransform().InverseTransformPosition(KillVolumeWorldCenter);

		SetEnvironmentVisualComponentNiagaraVec3(ActivePrimaryEffect, RainKillVolumeCenterParameterName, KillVolumeEffectLocalCenter);
	}

	if (ActivePrimaryEffect != nullptr && !RainKillVolumeSizeParameterName.IsNone())
	{
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(ActivePrimaryEffect);
		const FVector EffectLocalKillVolumeSize(
			CachedRainKillVolumeSize.X / EffectScale.X,
			CachedRainKillVolumeSize.Y / EffectScale.Y,
			CachedRainKillVolumeSize.Z / EffectScale.Z);
		SetEnvironmentVisualComponentNiagaraVec3(ActivePrimaryEffect, RainKillVolumeSizeParameterName, EffectLocalKillVolumeSize);
	}

	SetEnvironmentVisualComponentNiagaraFloat(ActivePrimaryEffect, PrimaryIntensityParameterName, CachedPrimaryIntensity);
	SetEnvironmentVisualComponentNiagaraFloat(ActivePrimaryEffect, RainSpawnRateParameterName, CachedRainSpawnRate * CachedPrimaryIntensity);
	SetEnvironmentVisualComponentNiagaraFloat(ActivePrimaryEffect, RainFallSpeedParameterName, CachedRainFallSpeed);

	if (ActiveSecondaryEffect != nullptr && !SecondaryAreaSizeParameterName.IsNone())
	{
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(ActiveSecondaryEffect);
		const FVector2D EffectLocalAreaSize(
			CachedAreaSize.X / EffectScale.X,
			CachedAreaSize.Y / EffectScale.Y);
		SetEnvironmentVisualComponentNiagaraAreaSize(ActiveSecondaryEffect, SecondaryAreaSizeParameterName, EffectLocalAreaSize);
	}

	SetEnvironmentVisualComponentNiagaraFloat(ActiveSecondaryEffect, SecondaryIntensityParameterName, CachedSecondaryIntensity);

	auto ApplyRainBlockerParameters = [this](UNiagaraComponent* Effect)
		{
			if (Effect == nullptr)
			{
				return;
			}

			const FTransform EffectTransform = Effect->GetComponentTransform();
			const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(Effect);
			const FVector BlockerWorldPosition = GetComponentTransform().TransformPosition(CachedRainBlockerLocalPosition);
			const FVector EffectLocalBlockerPosition = bCachedRainBlockerActive
				? EffectTransform.InverseTransformPosition(BlockerWorldPosition)
				: FVector::ZeroVector;
			const FVector EffectLocalHalfExtent = bCachedRainBlockerActive
				? FVector(
					CachedRainBlockerHalfExtent.X / EffectScale.X,
					CachedRainBlockerHalfExtent.Y / EffectScale.Y,
					CachedRainBlockerHalfExtent.Z / EffectScale.Z)
				: FVector::ZeroVector;

			SetEnvironmentVisualComponentNiagaraBool(Effect, RainBlockerActiveParameterName, bCachedRainBlockerActive);
			SetEnvironmentVisualComponentNiagaraVec3(Effect, RainBlockerLocalPositionParameterName, EffectLocalBlockerPosition);
			SetEnvironmentVisualComponentNiagaraVec3(Effect, RainBlockerHalfExtentParameterName, EffectLocalHalfExtent);
			SetEnvironmentVisualComponentNiagaraFloat(Effect, RainBlockerIntensityParameterName, CachedRainBlockerIntensity);

			DrawRainBlockerNiagaraDebug(Effect, BlockerWorldPosition, CachedRainBlockerHalfExtent);
		};

	ApplyRainBlockerParameters(ActivePrimaryEffect);
	ApplyRainBlockerParameters(ActiveSecondaryEffect);
}

void UUOUEnvironmentVisualComponent::ApplyVisualEffectTransforms()
{
	if (!CanApplyNiagaraState())
	{
		return;
	}

	UNiagaraComponent* ActivePrimaryEffect = GetPrimaryEffectComponent();
	UNiagaraComponent* ActiveSecondaryEffect = GetSecondaryEffectComponent();
	ActivePrimaryEffect = ActivePrimaryEffect != nullptr && ActivePrimaryEffect->IsRegistered() ? ActivePrimaryEffect : nullptr;
	ActiveSecondaryEffect = ActiveSecondaryEffect != nullptr && ActiveSecondaryEffect->IsRegistered() ? ActiveSecondaryEffect : nullptr;

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

bool UUOUEnvironmentVisualComponent::CanApplyNiagaraState() const
{
	return IsRegistered() && !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject);
}

void UUOUEnvironmentVisualComponent::DrawRainBlockerNiagaraDebug(const UNiagaraComponent* Effect, const FVector& BlockerWorldCenter, const FVector& BlockerHalfExtent) const
{
	if (!bDrawRainBlockerNiagaraDebug
		|| !bCachedRainBlockerActive
		|| !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::VFX)
		|| Effect == nullptr
		|| GetWorld() == nullptr
		|| BlockerHalfExtent.IsNearlyZero())
	{
		return;
	}

	const float CenterRadius = FMath::Max(6.0f, RainBlockerNiagaraDebugThickness * 2.0f);
	const FColor VFXDebugColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::VFX, FColor::Magenta);

	DrawDebugBox(
		GetWorld(),
		BlockerWorldCenter,
		BlockerHalfExtent,
		Effect->GetComponentQuat(),
		VFXDebugColor,
		false,
		0.0f,
		0,
		RainBlockerNiagaraDebugThickness);

	DrawDebugSphere(
		GetWorld(),
		BlockerWorldCenter,
		CenterRadius,
		8,
		VFXDebugColor,
		false,
		0.0f,
		0,
		RainBlockerNiagaraDebugThickness);
}
