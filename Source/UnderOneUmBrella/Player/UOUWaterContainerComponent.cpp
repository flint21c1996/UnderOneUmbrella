// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUWaterContainerComponent.h"

#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"

UUOUWaterContainerComponent::UUOUWaterContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUWaterContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxAmount = FMath::Max(0.0f, MaxAmount);
	WeightMultiplier = FMath::Max(0.0f, WeightMultiplier);
	ResolveFillVisualComponent();
	ApplyFillVisualContentProfile();
	SetAmount(InitialAmount);
	DisplayedFillVisualRatio = TargetFillVisualRatio;
	UpdateFillVisual(0.0f, true);
	SetComponentTickEnabled(bUpdateFillVisual);
}

void UUOUWaterContainerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFillVisual(DeltaTime);
}

float UUOUWaterContainerComponent::AddAmount(float AmountToAdd)
{
	if (AmountToAdd <= 0.0f)
	{
		return CurrentAmount;
	}

	SetAmount(CurrentAmount + AmountToAdd);
	return CurrentAmount;
}

float UUOUWaterContainerComponent::AddAmountWithContent(float AmountToAdd, UUOUPourContentProfile* NewContentProfile, bool bReplaceCurrentContent)
{
	if (AmountToAdd <= 0.0f)
	{
		return CurrentAmount;
	}

	if (NewContentProfile != nullptr
		&& (bReplaceCurrentContent || CurrentAmount <= KINDA_SMALL_NUMBER || PourContentProfile == nullptr))
	{
		SetPourContentProfile(NewContentProfile);
	}

	return AddAmount(AmountToAdd);
}

float UUOUWaterContainerComponent::RemoveAmount(float AmountToRemove)
{
	if (AmountToRemove <= 0.0f)
	{
		return CurrentAmount;
	}

	SetAmount(CurrentAmount - AmountToRemove);
	return CurrentAmount;
}

void UUOUWaterContainerComponent::SetAmount(float NewAmount)
{
	const float ClampedAmount = FMath::Clamp(NewAmount, 0.0f, MaxAmount);
	if (FMath::IsNearlyEqual(CurrentAmount, ClampedAmount))
	{
		CurrentAmount = ClampedAmount;
		RefreshFillVisualTarget();
		return;
	}

	CurrentAmount = ClampedAmount;
	RefreshFillVisualTarget();
	BroadcastAmountChanged();
}

void UUOUWaterContainerComponent::SetPourContentProfile(UUOUPourContentProfile* NewContentProfile)
{
	if (PourContentProfile == NewContentProfile)
	{
		return;
	}

	PourContentProfile = NewContentProfile;
	ApplyFillVisualContentProfile();
	BroadcastPourContentProfileChanged();
}

UUOUPourContentProfile* UUOUWaterContainerComponent::GetPourContentProfile() const
{
	return PourContentProfile.Get();
}

float UUOUWaterContainerComponent::GetFillRatio() const
{
	return MaxAmount > 0.0f ? CurrentAmount / MaxAmount : 0.0f;
}

float UUOUWaterContainerComponent::GetWeightContribution() const
{
	return CurrentAmount * WeightMultiplier;
}

void UUOUWaterContainerComponent::BroadcastAmountChanged()
{
	OnWaterAmountChanged.Broadcast(CurrentAmount, MaxAmount);
}

void UUOUWaterContainerComponent::BroadcastPourContentProfileChanged()
{
	OnPourContentProfileChanged.Broadcast(PourContentProfile.Get());
}

void UUOUWaterContainerComponent::ResolveFillVisualComponent()
{
	if (IsValid(FillVisualComponent))
	{
		bResolvedFillVisualComponent = true;
		ResolvedFillVisualComponentName = FillVisualComponent->GetName();
		CaptureFillVisualTransformIfNeeded();
		return;
	}

	FillVisualComponent = nullptr;
	bCapturedFillVisualTransform = false;
	bResolvedFillVisualComponent = false;
	ResolvedFillVisualComponentName = TEXT("None");

	if (!bAutoFindFillVisualComponent)
	{
		return;
	}

	FillVisualComponent = FindFillVisualComponent();
	bResolvedFillVisualComponent = FillVisualComponent != nullptr;
	ResolvedFillVisualComponentName = FillVisualComponent != nullptr ? FillVisualComponent->GetName() : TEXT("None");
	CaptureFillVisualTransformIfNeeded();
	ApplyFillVisualContentProfile();
}

USceneComponent* UUOUWaterContainerComponent::FindFillVisualComponent() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || FillVisualComponentName.IsNone())
	{
		return nullptr;
	}

	const FString TargetName = FillVisualComponentName.ToString();
	TInlineComponentArray<USceneComponent*> SceneComponents(Owner);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent == nullptr)
		{
			continue;
		}

		if (SceneComponent->GetFName() == FillVisualComponentName
			|| SceneComponent->ComponentTags.Contains(FillVisualComponentName)
			|| SceneComponent->GetName().Equals(TargetName, ESearchCase::IgnoreCase)
			|| SceneComponent->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void UUOUWaterContainerComponent::CaptureFillVisualTransformIfNeeded()
{
	if (!FillVisualComponent || bCapturedFillVisualTransform)
	{
		return;
	}

	InitialFillVisualRelativeLocation = FillVisualComponent->GetRelativeLocation();
	InitialFillVisualRelativeScale = FillVisualComponent->GetRelativeScale3D();
	bCapturedFillVisualTransform = true;
}

void UUOUWaterContainerComponent::RefreshFillVisualTarget()
{
	TargetFillVisualRatio = FMath::Clamp(GetFillRatio(), 0.0f, 1.0f);
}

void UUOUWaterContainerComponent::UpdateFillVisual(float DeltaTime, bool bSnapToTarget)
{
	if (!bUpdateFillVisual)
	{
		return;
	}

	ResolveFillVisualComponent();
	if (FillVisualComponent == nullptr)
	{
		return;
	}

	RefreshFillVisualTarget();
	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	const FUOUPourStoredVisualSettings* StoredVisualSettings = GetActiveStoredVisualSettings();
	const float ResolvedInterpSpeed = StoredVisualSettings != nullptr
		? StoredVisualSettings->FillVisualInterpSpeed
		: FillVisualInterpSpeed;
	const float SafeInterpSpeed = FMath::Max(0.0f, ResolvedInterpSpeed);
	if (bSnapToTarget || SafeInterpSpeed <= KINDA_SMALL_NUMBER)
	{
		DisplayedFillVisualRatio = TargetFillVisualRatio;
	}
	else
	{
		DisplayedFillVisualRatio = FMath::FInterpTo(
			DisplayedFillVisualRatio,
			TargetFillVisualRatio,
			SafeDeltaTime,
			SafeInterpSpeed);
	}

	DisplayedFillVisualRatio = FMath::Clamp(DisplayedFillVisualRatio, 0.0f, 1.0f);
	CaptureFillVisualTransformIfNeeded();

	FVector EffectiveEmptyScaleMultiplier = StoredVisualSettings != nullptr
		? StoredVisualSettings->EmptyScaleMultiplier
		: FillVisualEmptyScaleMultiplier;
	const FVector EffectiveFullScaleMultiplier = StoredVisualSettings != nullptr
		? StoredVisualSettings->FullScaleMultiplier
		: FillVisualFullScaleMultiplier;
	if (EffectiveEmptyScaleMultiplier.Equals(FVector::OneVector)
		&& EffectiveFullScaleMultiplier.Equals(FVector::OneVector))
	{
		EffectiveEmptyScaleMultiplier.Z = 0.0f;
	}

	const FVector ScaleMultiplier = FMath::Lerp(
		EffectiveEmptyScaleMultiplier,
		EffectiveFullScaleMultiplier,
		DisplayedFillVisualRatio);
	const FVector NewScale(
		InitialFillVisualRelativeScale.X * ScaleMultiplier.X,
		InitialFillVisualRelativeScale.Y * ScaleMultiplier.Y,
		InitialFillVisualRelativeScale.Z * ScaleMultiplier.Z);
	const FVector ResolvedFullLocationOffset = StoredVisualSettings != nullptr
		? StoredVisualSettings->FullLocationOffset
		: FillVisualFullLocationOffset;
	const FVector NewLocation = InitialFillVisualRelativeLocation
		+ ResolvedFullLocationOffset * DisplayedFillVisualRatio;

	FillVisualComponent->SetRelativeLocation(NewLocation);
	FillVisualComponent->SetRelativeScale3D(NewScale);

	const FName ResolvedMeshFillRatioParameterName = StoredVisualSettings != nullptr
		? StoredVisualSettings->MeshFillRatioParameterName
		: MeshFillRatioParameterName;
	if (!ResolvedMeshFillRatioParameterName.IsNone())
	{
		if (UMeshComponent* FillVisualMesh = Cast<UMeshComponent>(FillVisualComponent.Get()))
		{
			FillVisualMesh->SetScalarParameterValueOnMaterials(ResolvedMeshFillRatioParameterName, DisplayedFillVisualRatio);
		}
	}

	const FName ResolvedNiagaraFillRatioParameterName = StoredVisualSettings != nullptr
		? StoredVisualSettings->NiagaraFillRatioParameterName
		: NiagaraFillRatioParameterName;
	if (!ResolvedNiagaraFillRatioParameterName.IsNone())
	{
		if (UNiagaraComponent* FillVisualNiagara = Cast<UNiagaraComponent>(FillVisualComponent.Get()))
		{
			FillVisualNiagara->SetVariableFloat(ResolvedNiagaraFillRatioParameterName, DisplayedFillVisualRatio);
		}
	}

	const bool bShouldShow = ShouldShowFillVisual();
	if (UNiagaraComponent* FillVisualNiagara = Cast<UNiagaraComponent>(FillVisualComponent.Get()))
	{
		if (bShouldShow && !FillVisualNiagara->IsActive())
		{
			FillVisualNiagara->Activate(true);
		}
		else if (!bShouldShow && FillVisualNiagara->IsActive())
		{
			FillVisualNiagara->Deactivate();
		}
	}

	FillVisualComponent->SetHiddenInGame(!bShouldShow, true);
	FillVisualComponent->SetVisibility(bShouldShow, true);
}

bool UUOUWaterContainerComponent::ShouldShowFillVisual() const
{
	if (!bUpdateFillVisual || FillVisualComponent == nullptr)
	{
		return false;
	}

	const bool bResolvedHideWhenEmpty = GetActiveStoredVisualSettings() != nullptr
		? GetActiveStoredVisualSettings()->bHideWhenEmpty
		: bHideFillVisualWhenEmpty;
	if (bResolvedHideWhenEmpty
		&& TargetFillVisualRatio <= KINDA_SMALL_NUMBER
		&& DisplayedFillVisualRatio <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	return true;
}

const FUOUPourStoredVisualSettings* UUOUWaterContainerComponent::GetActiveStoredVisualSettings() const
{
	if (PourContentProfile != nullptr && PourContentProfile->StoredVisual.bOverrideContainerFillVisual)
	{
		return &PourContentProfile->StoredVisual;
	}

	return nullptr;
}

void UUOUWaterContainerComponent::ApplyFillVisualContentProfile()
{
	if (FillVisualComponent == nullptr)
	{
		return;
	}

	const FUOUPourStoredVisualSettings* StoredVisualSettings = GetActiveStoredVisualSettings();
	if (StoredVisualSettings == nullptr)
	{
		return;
	}

	if (UStaticMeshComponent* FillVisualStaticMesh = Cast<UStaticMeshComponent>(FillVisualComponent.Get()))
	{
		if (StoredVisualSettings->Mesh != nullptr)
		{
			FillVisualStaticMesh->SetStaticMesh(StoredVisualSettings->Mesh);
		}

		for (int32 MaterialIndex = 0; MaterialIndex < StoredVisualSettings->Materials.Num(); ++MaterialIndex)
		{
			if (StoredVisualSettings->Materials[MaterialIndex] != nullptr)
			{
				FillVisualStaticMesh->SetMaterial(MaterialIndex, StoredVisualSettings->Materials[MaterialIndex]);
			}
		}
	}

	if (UNiagaraComponent* FillVisualNiagara = Cast<UNiagaraComponent>(FillVisualComponent.Get()))
	{
		if (StoredVisualSettings->NiagaraSystem != nullptr)
		{
			FillVisualNiagara->SetAsset(StoredVisualSettings->NiagaraSystem);
		}
	}
}
