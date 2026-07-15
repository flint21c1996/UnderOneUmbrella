// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUEnvironmentVisualComponent.h"

#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraParameterStore.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "UObject/UnrealType.h"

namespace
{
	bool ShouldLogEnvironmentVisualRainBlocker(const UObject* WorldContext, double& LastLogTime, double IntervalSeconds = 0.5)
	{
		const UWorld* World = WorldContext != nullptr ? WorldContext->GetWorld() : nullptr;
		const double CurrentTime = World != nullptr ? World->GetTimeSeconds() : FPlatformTime::Seconds();
		if (CurrentTime - LastLogTime < IntervalSeconds)
		{
			return false;
		}

		LastLogTime = CurrentTime;
		return true;
	}

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

	// Niagara의 User Parameter는 에셋 쪽에서는 User.RainAreaSize처럼 보일 수 있지만,
	// SetVariable 계열 함수에는 RainAreaSize처럼 User.를 뺀 이름을 넘기는 쪽이 안전합니다.
	FName NormalizeEnvironmentVisualComponentNiagaraParameterName(FName ParameterName)
	{
		if (ParameterName.IsNone())
		{
			return NAME_None;
		}

		FString ParameterNameString = ParameterName.ToString();
		if (ParameterNameString.StartsWith(TEXT("User.")))
		{
			ParameterNameString.RightChopInline(5);
			return FName(*ParameterNameString);
		}

		return ParameterName;
	}

	const FNiagaraVariableWithOffset* FindEnvironmentVisualComponentNiagaraParameterByName(const UNiagaraComponent* Effect, FName ParameterName)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return nullptr;
		}

		const UNiagaraSystem* NiagaraSystem = Effect->GetAsset();
		const FNiagaraTypeDefinition CandidateTypes[] =
		{
			FNiagaraTypeDefinition::GetVec2Def(),
			FNiagaraTypeDefinition::GetVec3Def(),
			FNiagaraTypeDefinition::GetPositionDef(),
			FNiagaraTypeDefinition::GetVec4Def(),
			FNiagaraTypeDefinition::GetFloatDef(),
			FNiagaraTypeDefinition::GetBoolDef(),
			FNiagaraTypeDefinition::GetIntDef()
		};

		if (NiagaraSystem != nullptr)
		{
			for (const FNiagaraTypeDefinition& CandidateType : CandidateTypes)
			{
				const FNiagaraVariable QueryParameter(CandidateType, ParameterName);
				if (const FNiagaraVariableWithOffset* ExposedParameter = NiagaraSystem->GetExposedParameters().FindParameterVariable(QueryParameter, false))
				{
					return ExposedParameter;
				}
			}
		}

		for (const FNiagaraTypeDefinition& CandidateType : CandidateTypes)
		{
			const FNiagaraVariable QueryParameter(CandidateType, ParameterName);
			if (const FNiagaraVariableWithOffset* OverrideParameter = Effect->GetOverrideParameters().FindParameterVariable(QueryParameter, false))
			{
				return OverrideParameter;
			}
		}

		return nullptr;
	}

	const FNiagaraVariableWithOffset* FindEnvironmentVisualComponentNiagaraParameter(const UNiagaraComponent* Effect, FName ParameterName)
	{
		if (const FNiagaraVariableWithOffset* ExistingParameter = FindEnvironmentVisualComponentNiagaraParameterByName(Effect, ParameterName))
		{
			return ExistingParameter;
		}

		const FName NormalizedParameterName = NormalizeEnvironmentVisualComponentNiagaraParameterName(ParameterName);
		if (NormalizedParameterName != ParameterName)
		{
			return FindEnvironmentVisualComponentNiagaraParameterByName(Effect, NormalizedParameterName);
		}

		return nullptr;
	}

	bool CanSetEnvironmentVisualComponentNiagaraParameter(const UNiagaraComponent* Effect, FName ParameterName, const FNiagaraTypeDefinition& ExpectedType)
	{
		const FNiagaraVariableWithOffset* ExistingParameter = FindEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName);
		return ExistingParameter == nullptr || ExistingParameter->GetType() == ExpectedType;
	}

	const TCHAR* GetEnvironmentVisualComponentNiagaraTypeDebugName(const FNiagaraTypeDefinition& Type)
	{
		if (Type == FNiagaraTypeDefinition::GetPositionDef())
		{
			return TEXT("Position");
		}

		if (Type == FNiagaraTypeDefinition::GetVec3Def())
		{
			return TEXT("Vec3");
		}

		if (Type == FNiagaraTypeDefinition::GetVec2Def())
		{
			return TEXT("Vec2");
		}

		if (Type == FNiagaraTypeDefinition::GetFloatDef())
		{
			return TEXT("Float");
		}

		if (Type == FNiagaraTypeDefinition::GetBoolDef())
		{
			return TEXT("Bool");
		}

		if (Type == FNiagaraTypeDefinition::GetIntDef())
		{
			return TEXT("Int");
		}

		return TEXT("Other");
	}

	const FNiagaraVariableWithOffset* FindEnvironmentVisualComponentNiagaraParameterInStore(
		const FNiagaraParameterStore& Store,
		FName ParameterName)
	{
		if (ParameterName.IsNone())
		{
			return nullptr;
		}

		const FNiagaraTypeDefinition CandidateTypes[] =
		{
			FNiagaraTypeDefinition::GetPositionDef(),
			FNiagaraTypeDefinition::GetVec3Def(),
			FNiagaraTypeDefinition::GetVec2Def(),
			FNiagaraTypeDefinition::GetFloatDef(),
			FNiagaraTypeDefinition::GetBoolDef(),
			FNiagaraTypeDefinition::GetIntDef()
		};

		for (const FNiagaraTypeDefinition& CandidateType : CandidateTypes)
		{
			const FNiagaraVariable QueryParameter(CandidateType, ParameterName);
			if (const FNiagaraVariableWithOffset* Parameter = Store.FindParameterVariable(QueryParameter, false))
			{
				return Parameter;
			}
		}

		return nullptr;
	}

	FString DescribeEnvironmentVisualComponentNiagaraParameterBinding(const UNiagaraComponent* Effect, FName ParameterName)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return TEXT("ParamDebug: invalid effect/name");
		}

		TArray<FName, TInlineAllocator<3>> NamesToCheck;
		NamesToCheck.AddUnique(ParameterName);

		const FName NormalizedParameterName = NormalizeEnvironmentVisualComponentNiagaraParameterName(ParameterName);
		NamesToCheck.AddUnique(NormalizedParameterName);

		const FString NormalizedParameterString = NormalizedParameterName.ToString();
		if (!NormalizedParameterString.StartsWith(TEXT("User.")))
		{
			NamesToCheck.AddUnique(FName(*FString::Printf(TEXT("User.%s"), *NormalizedParameterString)));
		}

		auto AppendMatchingParameters = [](FString& OutText, const TCHAR* Label, const FNiagaraParameterStore& Store)
			{
				TArray<FNiagaraVariable> Parameters;
				Store.GetParameters(Parameters);

				OutText += FString::Printf(TEXT("\n%s RainBlocker:"), Label);
				int32 MatchCount = 0;
				for (const FNiagaraVariable& Parameter : Parameters)
				{
					const FString ParameterNameString = Parameter.GetName().ToString();
					if (!ParameterNameString.Contains(TEXT("RainBlocker")))
					{
						continue;
					}

					++MatchCount;
					OutText += FString::Printf(
						TEXT(" %s:%s"),
						*ParameterNameString,
						GetEnvironmentVisualComponentNiagaraTypeDebugName(Parameter.GetType()));
				}

				if (MatchCount == 0)
				{
					OutText += TEXT(" none");
				}
			};

		FString Result = FString::Printf(TEXT("Param %s |"), *ParameterName.ToString());
		const UNiagaraSystem* NiagaraSystem = Effect->GetAsset();
		for (const FName NameToCheck : NamesToCheck)
		{
			const FNiagaraVariableWithOffset* SystemParameter = NiagaraSystem != nullptr
				? FindEnvironmentVisualComponentNiagaraParameterInStore(NiagaraSystem->GetExposedParameters(), NameToCheck)
				: nullptr;
			const FNiagaraVariableWithOffset* OverrideParameter = FindEnvironmentVisualComponentNiagaraParameterInStore(Effect->GetOverrideParameters(), NameToCheck);

			Result += FString::Printf(
				TEXT(" %s Sys:%s Ovr:%s |"),
				*NameToCheck.ToString(),
				SystemParameter != nullptr ? GetEnvironmentVisualComponentNiagaraTypeDebugName(SystemParameter->GetType()) : TEXT("-"),
				OverrideParameter != nullptr ? GetEnvironmentVisualComponentNiagaraTypeDebugName(OverrideParameter->GetType()) : TEXT("-"));
		}

		if (NiagaraSystem != nullptr)
		{
			AppendMatchingParameters(Result, TEXT("SysAll"), NiagaraSystem->GetExposedParameters());
		}
		AppendMatchingParameters(Result, TEXT("OvrAll"), Effect->GetOverrideParameters());

		return Result;
	}

	void SetEnvironmentVisualComponentNiagaraAreaSize(UNiagaraComponent* Effect, FName ParameterName, const FVector& VolumeSize)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return;
		}

		const FName RuntimeParameterName = NormalizeEnvironmentVisualComponentNiagaraParameterName(ParameterName);
		const FNiagaraVariableWithOffset* ExistingParameter = FindEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName);
		if (ExistingParameter == nullptr)
		{
			return;
		}

		if (ExistingParameter->GetType() == FNiagaraTypeDefinition::GetVec2Def())
		{
			Effect->SetVariableVec2(RuntimeParameterName, FVector2D(VolumeSize.X, VolumeSize.Y));
			return;
		}

		if (ExistingParameter->GetType() == FNiagaraTypeDefinition::GetVec3Def())
		{
			Effect->SetVariableVec3(RuntimeParameterName, VolumeSize);
		}
	}

	void SetEnvironmentVisualComponentNiagaraVec3(UNiagaraComponent* Effect, FName ParameterName, const FVector& Value)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return;
		}

		const FName RuntimeParameterName = NormalizeEnvironmentVisualComponentNiagaraParameterName(ParameterName);
		const FNiagaraVariableWithOffset* ExistingParameter = FindEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName);
		if (ExistingParameter == nullptr || ExistingParameter->GetType() == FNiagaraTypeDefinition::GetVec3Def())
		{
			Effect->SetVariableVec3(RuntimeParameterName, Value);
			return;
		}

		if (ExistingParameter->GetType() == FNiagaraTypeDefinition::GetPositionDef())
		{
			Effect->SetVariablePosition(RuntimeParameterName, Value);
		}
	}

	void SetEnvironmentVisualComponentNiagaraPositionCompatible(UNiagaraComponent* Effect, FName ParameterName, const FVector& Value)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return;
		}

		TArray<FName, TInlineAllocator<3>> RuntimeParameterNames;
		RuntimeParameterNames.AddUnique(ParameterName);

		const FName NormalizedParameterName = NormalizeEnvironmentVisualComponentNiagaraParameterName(ParameterName);
		RuntimeParameterNames.AddUnique(NormalizedParameterName);

		const FString NormalizedParameterString = NormalizedParameterName.ToString();
		if (!NormalizedParameterString.StartsWith(TEXT("User.")))
		{
			RuntimeParameterNames.AddUnique(FName(*FString::Printf(TEXT("User.%s"), *NormalizedParameterString)));
		}

		for (const FName RuntimeParameterName : RuntimeParameterNames)
		{
			Effect->SetVariablePosition(RuntimeParameterName, Value);
			Effect->SetVariableVec3(RuntimeParameterName, Value);
		}
	}

	void SetEnvironmentVisualComponentNiagaraFloat(UNiagaraComponent* Effect, FName ParameterName, float Value)
	{
		if (Effect != nullptr && !ParameterName.IsNone()
			&& CanSetEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName, FNiagaraTypeDefinition::GetFloatDef()))
		{
			Effect->SetVariableFloat(NormalizeEnvironmentVisualComponentNiagaraParameterName(ParameterName), Value);
		}
	}

	void SetEnvironmentVisualComponentNiagaraFloatCompatible(UNiagaraComponent* Effect, FName ParameterName, float Value)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return;
		}

		TArray<FName, TInlineAllocator<3>> RuntimeParameterNames;
		RuntimeParameterNames.AddUnique(ParameterName);

		const FName NormalizedParameterName = NormalizeEnvironmentVisualComponentNiagaraParameterName(ParameterName);
		RuntimeParameterNames.AddUnique(NormalizedParameterName);

		const FString NormalizedParameterString = NormalizedParameterName.ToString();
		if (!NormalizedParameterString.StartsWith(TEXT("User.")))
		{
			RuntimeParameterNames.AddUnique(FName(*FString::Printf(TEXT("User.%s"), *NormalizedParameterString)));
		}

		for (const FName RuntimeParameterName : RuntimeParameterNames)
		{
			Effect->SetVariableFloat(RuntimeParameterName, Value);
		}
	}

	void SetEnvironmentVisualComponentNiagaraFallSpeed(UNiagaraComponent* Effect, FName ParameterName, float WorldFallSpeed)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return;
		}

		const FName RuntimeParameterName = NormalizeEnvironmentVisualComponentNiagaraParameterName(ParameterName);
		// Rain fall speed is authored in world units per second. Keep it independent from
		// RainArea/Niagara component scale so taller rain volumes do not change fall speed.
		const float EffectFallSpeed = WorldFallSpeed;
		const FNiagaraVariableWithOffset* ExistingParameter = FindEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName);

		if (ExistingParameter == nullptr || ExistingParameter->GetType() == FNiagaraTypeDefinition::GetFloatDef())
		{
			Effect->SetVariableFloat(RuntimeParameterName, EffectFallSpeed);
			return;
		}

		if (ExistingParameter->GetType() == FNiagaraTypeDefinition::GetVec3Def())
		{
			Effect->SetVariableVec3(RuntimeParameterName, FVector(0.0f, 0.0f, EffectFallSpeed));
			return;
		}

		if (ExistingParameter->GetType() == FNiagaraTypeDefinition::GetPositionDef())
		{
			Effect->SetVariablePosition(RuntimeParameterName, FVector(0.0f, 0.0f, EffectFallSpeed));
		}
	}

	void SetEnvironmentVisualComponentNiagaraBool(UNiagaraComponent* Effect, FName ParameterName, bool bValue)
	{
		if (Effect != nullptr && !ParameterName.IsNone()
			&& CanSetEnvironmentVisualComponentNiagaraParameter(Effect, ParameterName, FNiagaraTypeDefinition::GetBoolDef()))
		{
			Effect->SetVariableBool(NormalizeEnvironmentVisualComponentNiagaraParameterName(ParameterName), bValue);
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

void UUOUEnvironmentVisualComponent::SetEffectComponentList(const TArray<UNiagaraComponent*>& NewEffectComponents)
{
	EffectComponents.Reset();
	for (UNiagaraComponent* EffectComponent : NewEffectComponents)
	{
		if (EffectComponent != nullptr)
		{
			EffectComponents.AddUnique(EffectComponent);
		}
	}

	ApplyVisualEffectSettings();
	ApplyVisualEffectTransforms();
	ApplyNiagaraParameters();
	RefreshNiagaraActivation();
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

	// 강도 값은 추가 연출용 수치일 뿐 비 차단 박스의 활성 여부를 결정하지 않습니다.
	// 우산이 비를 막고 있고 박스 크기가 유효하면 나이아가라 차단 볼륨은 켜져야 합니다.
	bCachedRainBlockerActive = bIsBlocking && !SafeHalfExtent.IsNearlyZero();
	CachedRainBlockerLocalPosition = bCachedRainBlockerActive ? BlockerLocalPosition : FVector::ZeroVector;
	CachedRainBlockerHalfExtent = bCachedRainBlockerActive ? SafeHalfExtent : FVector::ZeroVector;
	CachedRainBlockerIntensity = bCachedRainBlockerActive ? FMath::Clamp(BlockerIntensity, 0.0f, 1.0f) : 0.0f;

	static double LastSetDataLogTime = -1000.0;
	if (ShouldLogEnvironmentVisualRainBlocker(this, LastSetDataLogTime))
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("[RainBlocker][SetRainBlockerData] Owner=%s InputBlocking=%s CachedActive=%s LocalPos=(%.1f %.1f %.1f) Half=(%.1f %.1f %.1f) Intensity=%.2f"),
			GetOwner() != nullptr ? *GetOwner()->GetName() : TEXT("None"),
			bIsBlocking ? TEXT("true") : TEXT("false"),
			bCachedRainBlockerActive ? TEXT("true") : TEXT("false"),
			CachedRainBlockerLocalPosition.X,
			CachedRainBlockerLocalPosition.Y,
			CachedRainBlockerLocalPosition.Z,
			CachedRainBlockerHalfExtent.X,
			CachedRainBlockerHalfExtent.Y,
			CachedRainBlockerHalfExtent.Z,
			CachedRainBlockerIntensity);
	}

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

	static double LastSpawnRateLogTime = -1000.0;
	if (ShouldLogEnvironmentVisualRainBlocker(this, LastSpawnRateLogTime))
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("[RainSpawnRate][RainVisual] Owner=%s CachedRate=%.1f"),
			GetOwner() != nullptr ? *GetOwner()->GetName() : TEXT("None"),
			CachedRainSpawnRate);
	}

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

void UUOUEnvironmentVisualComponent::GatherRegisteredEffectComponents(TArray<UNiagaraComponent*>& OutEffects) const
{
	OutEffects.Reset();
	auto AddEffect = [&OutEffects](UNiagaraComponent* Effect)
		{
			if (Effect != nullptr && Effect->IsRegistered())
			{
				OutEffects.AddUnique(Effect);
			}
		};

	AddEffect(GetPrimaryEffectComponent());
	AddEffect(GetSecondaryEffectComponent());
	for (const TObjectPtr<UNiagaraComponent>& EffectComponent : EffectComponents)
	{
		AddEffect(EffectComponent.Get());
	}
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
	TArray<UNiagaraComponent*> ActiveEffects;
	GatherRegisteredEffectComponents(ActiveEffects);

	for (UNiagaraComponent* ActiveEffect : ActiveEffects)
	{
		const bool bIsSecondaryEffect = ActiveEffect == GetSecondaryEffectComponent();
		const float EffectIntensity = bIsSecondaryEffect ? CachedSecondaryIntensity : CachedPrimaryIntensity;
		const bool bShouldShowEffect = bShouldAllowVisuals
			&& ActiveEffect->GetAsset() != nullptr
			&& EffectIntensity > UE_KINDA_SMALL_NUMBER;

		const bool bWasVisible = ActiveEffect->IsVisible();
		ActiveEffect->SetVisibility(bShouldShowEffect, true);

		if (bShouldShowEffect && (!ActiveEffect->IsActive() || !bWasVisible))
		{
			ActiveEffect->Activate(true);
			ApplyNiagaraParameters();
		}
		else if (!bShouldShowEffect && ActiveEffect->IsActive())
		{
			ActiveEffect->Deactivate();
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
	TArray<UNiagaraComponent*> ActiveEffects;
	GatherRegisteredEffectComponents(ActiveEffects);

	static double LastApplyLogTime = -1000.0;
	const bool bShouldLogApplyRainBlocker = bCachedRainBlockerActive && ShouldLogEnvironmentVisualRainBlocker(this, LastApplyLogTime);
	if (bShouldLogApplyRainBlocker)
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("[RainBlocker][ApplyNiagaraParameters] Owner=%s ActiveEffects=%d Primary=%s Secondary=%s CachedLocal=(%.1f %.1f %.1f) CachedHalf=(%.1f %.1f %.1f)"),
			GetOwner() != nullptr ? *GetOwner()->GetName() : TEXT("None"),
			ActiveEffects.Num(),
			ActivePrimaryEffect != nullptr ? *ActivePrimaryEffect->GetName() : TEXT("None"),
			ActiveSecondaryEffect != nullptr ? *ActiveSecondaryEffect->GetName() : TEXT("None"),
			CachedRainBlockerLocalPosition.X,
			CachedRainBlockerLocalPosition.Y,
			CachedRainBlockerLocalPosition.Z,
			CachedRainBlockerHalfExtent.X,
			CachedRainBlockerHalfExtent.Y,
			CachedRainBlockerHalfExtent.Z);
	}

	if (ActivePrimaryEffect != nullptr && !PrimaryAreaSizeParameterName.IsNone())
	{
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(ActivePrimaryEffect);
		const FVector EffectLocalAreaSize(
			CachedAreaSize.X / EffectScale.X,
			CachedAreaSize.Y / EffectScale.Y,
			CachedRainKillVolumeSize.Z / EffectScale.Z);
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
	SetEnvironmentVisualComponentNiagaraFloatCompatible(ActivePrimaryEffect, RainSpawnRateParameterName, CachedRainSpawnRate * CachedPrimaryIntensity);
	SetEnvironmentVisualComponentNiagaraFallSpeed(ActivePrimaryEffect, RainFallSpeedParameterName, CachedRainFallSpeed);

	if (ActiveSecondaryEffect != nullptr && !SecondaryAreaSizeParameterName.IsNone())
	{
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(ActiveSecondaryEffect);
		const FVector EffectLocalAreaSize(
			CachedAreaSize.X / EffectScale.X,
			CachedAreaSize.Y / EffectScale.Y,
			CachedRainKillVolumeSize.Z / EffectScale.Z);
		SetEnvironmentVisualComponentNiagaraAreaSize(ActiveSecondaryEffect, SecondaryAreaSizeParameterName, EffectLocalAreaSize);
	}

	SetEnvironmentVisualComponentNiagaraFloat(ActiveSecondaryEffect, SecondaryIntensityParameterName, CachedSecondaryIntensity);

	for (UNiagaraComponent* ActiveEffect : ActiveEffects)
	{
		const bool bIsSecondaryEffect = ActiveEffect == ActiveSecondaryEffect;
		const float EffectIntensity = bIsSecondaryEffect ? CachedSecondaryIntensity : CachedPrimaryIntensity;
		const FName AreaParameterName = bIsSecondaryEffect ? SecondaryAreaSizeParameterName : PrimaryAreaSizeParameterName;
		const FName IntensityParameterName = bIsSecondaryEffect ? SecondaryIntensityParameterName : PrimaryIntensityParameterName;
		const FTransform EffectTransform = ActiveEffect->GetComponentTransform();
		const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(ActiveEffect);
		const FVector RainSpawnWorldPosition = GetComponentTransform().TransformPosition(CachedPrimaryLocalPosition);
		const FVector GroundSplashWorldPosition = GetComponentTransform().TransformPosition(CachedSecondaryLocalPosition);
		const FVector KillVolumeWorldCenter = GetComponentTransform().TransformPosition(CachedRainKillVolumeLocalCenter);
		const FVector RainSpawnEffectLocalPosition = EffectTransform.InverseTransformPosition(RainSpawnWorldPosition);
		const FVector GroundSplashEffectLocalPosition = EffectTransform.InverseTransformPosition(GroundSplashWorldPosition);
		const FVector KillVolumeEffectLocalCenter = EffectTransform.InverseTransformPosition(KillVolumeWorldCenter);
		const FVector FullEffectLocalAreaSize(
			CachedAreaSize.X / EffectScale.X,
			CachedAreaSize.Y / EffectScale.Y,
			CachedRainKillVolumeSize.Z / EffectScale.Z);
		const FVector PlaneEffectLocalAreaSize(
			CachedAreaSize.X / EffectScale.X,
			CachedAreaSize.Y / EffectScale.Y,
			0.0f);
		const FVector EffectLocalKillVolumeSize(
			CachedRainKillVolumeSize.X / EffectScale.X,
			CachedRainKillVolumeSize.Y / EffectScale.Y,
			CachedRainKillVolumeSize.Z / EffectScale.Z);

		if (!AreaParameterName.IsNone())
		{
			SetEnvironmentVisualComponentNiagaraAreaSize(ActiveEffect, AreaParameterName, FullEffectLocalAreaSize);
		}

		// 한 Niagara System 안의 여러 emitter가 서로 다른 위치를 쓸 수 있도록
		// 비 생성 면과 바닥 튐 면을 별도 User Parameter로 같이 넘깁니다.
		SetEnvironmentVisualComponentNiagaraAreaSize(ActiveEffect, RainSpawnAreaSizeParameterName, PlaneEffectLocalAreaSize);
		SetEnvironmentVisualComponentNiagaraVec3(ActiveEffect, RainSpawnLocalPositionParameterName, RainSpawnEffectLocalPosition);
		SetEnvironmentVisualComponentNiagaraAreaSize(ActiveEffect, SecondaryAreaSizeParameterName, PlaneEffectLocalAreaSize);
		SetEnvironmentVisualComponentNiagaraVec3(ActiveEffect, GroundSplashLocalPositionParameterName, GroundSplashEffectLocalPosition);
		SetEnvironmentVisualComponentNiagaraVec3(ActiveEffect, RainKillVolumeCenterParameterName, KillVolumeEffectLocalCenter);
		SetEnvironmentVisualComponentNiagaraVec3(ActiveEffect, RainKillVolumeSizeParameterName, EffectLocalKillVolumeSize);

		SetEnvironmentVisualComponentNiagaraFloat(ActiveEffect, IntensityParameterName, EffectIntensity);
		SetEnvironmentVisualComponentNiagaraFloatCompatible(ActiveEffect, RainSpawnRateParameterName, CachedRainSpawnRate * EffectIntensity);
		SetEnvironmentVisualComponentNiagaraFallSpeed(ActiveEffect, RainFallSpeedParameterName, CachedRainFallSpeed);
	}

	auto ApplyRainBlockerParameters = [this](UNiagaraComponent* Effect)
		{
			if (Effect == nullptr)
			{
				return;
			}

			const FTransform EffectTransform = Effect->GetComponentTransform();
			const FVector BlockerWorldPosition = GetComponentTransform().TransformPosition(CachedRainBlockerLocalPosition);
			const FVector EffectLocalBlockerPosition = bCachedRainBlockerActive
				? EffectTransform.InverseTransformPosition(BlockerWorldPosition)
				: FVector::ZeroVector;
			const FVector EffectScale = GetEnvironmentVisualComponentSafeEffectScale(Effect);
			const FVector EffectLocalBlockerBoxSize = bCachedRainBlockerActive
				? FVector(
					CachedRainBlockerHalfExtent.X * 2.0f / EffectScale.X,
					CachedRainBlockerHalfExtent.Y * 2.0f / EffectScale.Y,
					CachedRainBlockerHalfExtent.Z * 2.0f / EffectScale.Z)
				: FVector::ZeroVector;

			SetEnvironmentVisualComponentNiagaraBool(Effect, RainBlockerActiveParameterName, bCachedRainBlockerActive);
			SetEnvironmentVisualComponentNiagaraPositionCompatible(Effect, RainBlockerLocalPositionParameterName, EffectLocalBlockerPosition);
			SetEnvironmentVisualComponentNiagaraVec3(Effect, RainBlockerHalfExtentParameterName, EffectLocalBlockerBoxSize);
			SetEnvironmentVisualComponentNiagaraFloat(Effect, RainBlockerIntensityParameterName, CachedRainBlockerIntensity);

			static double LastSetVariableLogTime = -1000.0;
			if (bCachedRainBlockerActive && ShouldLogEnvironmentVisualRainBlocker(this, LastSetVariableLogTime))
			{
				const UNiagaraSystem* EffectAsset = Effect->GetAsset();
				UE_LOG(
					LogTemp,
					Verbose,
					TEXT("[RainBlocker][SetNiagaraVariables] Effect=%s Asset=%s Active=%s SentLocal=(%.1f %.1f %.1f) SentBoxSize=(%.1f %.1f %.1f) Binding={%s}"),
					*Effect->GetName(),
					EffectAsset != nullptr ? *EffectAsset->GetName() : TEXT("None"),
					bCachedRainBlockerActive ? TEXT("true") : TEXT("false"),
					EffectLocalBlockerPosition.X,
					EffectLocalBlockerPosition.Y,
					EffectLocalBlockerPosition.Z,
					EffectLocalBlockerBoxSize.X,
					EffectLocalBlockerBoxSize.Y,
					EffectLocalBlockerBoxSize.Z,
					*DescribeEnvironmentVisualComponentNiagaraParameterBinding(Effect, RainBlockerLocalPositionParameterName).Replace(TEXT("\n"), TEXT(" ")));
			}

			DrawRainBlockerNiagaraDebug(Effect, BlockerWorldPosition, CachedRainBlockerHalfExtent);
		};

	for (UNiagaraComponent* ActiveEffect : ActiveEffects)
	{
		ApplyRainBlockerParameters(ActiveEffect);
	}
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
		ActivePrimaryEffect->SetWorldScale3D(FVector::OneVector);
	}

	if (ActiveSecondaryEffect != nullptr)
	{
		ActiveSecondaryEffect->SetRelativeLocation(CachedSecondaryLocalPosition);
		ActiveSecondaryEffect->SetRelativeRotation(CachedEffectLocalRotation);
		ActiveSecondaryEffect->SetWorldScale3D(FVector::OneVector);
	}

	TArray<UNiagaraComponent*> ActiveEffects;
	GatherRegisteredEffectComponents(ActiveEffects);
	for (UNiagaraComponent* ActiveEffect : ActiveEffects)
	{
		if (ActiveEffect == nullptr || ActiveEffect == ActivePrimaryEffect || ActiveEffect == ActiveSecondaryEffect)
		{
			continue;
		}

		// RainVisual 아래에 직접 붙인 범용 Niagara는 RainVolume 중심을 기준으로 둡니다.
		// 실제 비 영역 크기는 User.RainAreaSize 같은 Niagara 파라미터가 맞추기 때문에,
		// 자식 컴포넌트의 이전 상대 위치가 남아 있으면 박스와 비가 서로 어긋날 수 있습니다.
		ActiveEffect->SetWorldLocation(GetComponentLocation());
		ActiveEffect->SetWorldRotation(GetComponentRotation() + CachedEffectLocalRotation);
		ActiveEffect->SetWorldScale3D(FVector::OneVector);
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

	const FTransform EffectTransform = Effect->GetComponentTransform();
	const FVector EffectLocalBlockerPosition = EffectTransform.InverseTransformPosition(BlockerWorldCenter);
	const FString BindingDebugText = DescribeEnvironmentVisualComponentNiagaraParameterBinding(Effect, RainBlockerLocalPositionParameterName);
	DrawDebugString(
		GetWorld(),
		BlockerWorldCenter + FVector(0.0f, 0.0f, BlockerHalfExtent.Z + 36.0f),
		FString::Printf(
			TEXT("RB Local %.1f %.1f %.1f"),
			EffectLocalBlockerPosition.X,
			EffectLocalBlockerPosition.Y,
			EffectLocalBlockerPosition.Z),
		nullptr,
		VFXDebugColor,
		0.0f,
		false,
		1.0f);

	if (GEngine != nullptr)
	{
		const uint64 EffectHash = static_cast<uint64>(PointerHash(Effect));
		const uint64 DebugKey = 0x554F55000000ull + EffectHash;
		GEngine->AddOnScreenDebugMessage(
			DebugKey,
			0.0f,
			VFXDebugColor,
			FString::Printf(
				TEXT("%s\nRainBlockerLocal %.1f %.1f %.1f\n%s"),
				*Effect->GetName(),
				EffectLocalBlockerPosition.X,
				EffectLocalBlockerPosition.Y,
				EffectLocalBlockerPosition.Z,
				*BindingDebugText));
	}
}
