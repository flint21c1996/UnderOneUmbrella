// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUStoredContentVisualComponent.h"

#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "Player/UOUWaterContainerComponent.h"

UUOUStoredContentVisualComponent::UUOUStoredContentVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUStoredContentVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveWaterContainerComponent();
	ResolveStoredVisualComponent();
	BindWaterContainerEvents();
	ApplyStoredVisualContentProfile();
	DisplayedFillVisualRatio = GetTargetFillVisualRatio();
	UpdateStoredVisual(0.0f, true);
	SetComponentTickEnabled(bUpdateStoredVisual);
}

void UUOUStoredContentVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindWaterContainerEvents();

	Super::EndPlay(EndPlayReason);
}

void UUOUStoredContentVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateStoredVisual(DeltaTime);
}

void UUOUStoredContentVisualComponent::RefreshStoredContentVisual(bool bSnapToTarget)
{
	ResolveWaterContainerComponent();
	ResolveStoredVisualComponent();
	BindWaterContainerEvents();
	ApplyStoredVisualContentProfile();
	UpdateStoredVisual(0.0f, bSnapToTarget);
}

void UUOUStoredContentVisualComponent::ResolveWaterContainerComponent()
{
	if (IsValid(WaterContainerComponent))
	{
		bResolvedWaterContainerComponent = true;
		ResolvedWaterContainerComponentName = WaterContainerComponent->GetName();
		return;
	}

	WaterContainerComponent = nullptr;
	bResolvedWaterContainerComponent = false;
	ResolvedWaterContainerComponentName = TEXT("None");

	if (!bAutoFindWaterContainerComponent)
	{
		return;
	}

	WaterContainerComponent = FindWaterContainerComponent();
	bResolvedWaterContainerComponent = WaterContainerComponent != nullptr;
	ResolvedWaterContainerComponentName = WaterContainerComponent != nullptr ? WaterContainerComponent->GetName() : TEXT("None");
}

UUOUWaterContainerComponent* UUOUStoredContentVisualComponent::FindWaterContainerComponent() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	TInlineComponentArray<UUOUWaterContainerComponent*> WaterContainers(Owner);
	if (WaterContainers.IsEmpty())
	{
		return nullptr;
	}

	if (WaterContainerComponentName.IsNone())
	{
		return WaterContainers[0];
	}

	const FString TargetName = WaterContainerComponentName.ToString();
	for (UUOUWaterContainerComponent* Candidate : WaterContainers)
	{
		if (Candidate == nullptr)
		{
			continue;
		}

		if (Candidate->GetFName() == WaterContainerComponentName
			|| Candidate->ComponentTags.Contains(WaterContainerComponentName)
			|| Candidate->GetName().Equals(TargetName, ESearchCase::IgnoreCase)
			|| Candidate->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
		{
			return Candidate;
		}
	}

	return nullptr;
}

void UUOUStoredContentVisualComponent::ResolveStoredVisualComponent()
{
	if (IsValid(StoredVisualComponent))
	{
		bResolvedStoredVisualComponent = true;
		ResolvedStoredVisualComponentName = StoredVisualComponent->GetName();
		CaptureStoredVisualTransformIfNeeded();
		return;
	}

	StoredVisualComponent = nullptr;
	bCapturedStoredVisualTransform = false;
	bResolvedStoredVisualComponent = false;
	ResolvedStoredVisualComponentName = TEXT("None");

	if (!bAutoFindStoredVisualComponent)
	{
		return;
	}

	StoredVisualComponent = FindStoredVisualComponent();
	bResolvedStoredVisualComponent = StoredVisualComponent != nullptr;
	ResolvedStoredVisualComponentName = StoredVisualComponent != nullptr ? StoredVisualComponent->GetName() : TEXT("None");
	CaptureStoredVisualTransformIfNeeded();
}

USceneComponent* UUOUStoredContentVisualComponent::FindStoredVisualComponent() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	TInlineComponentArray<USceneComponent*> SceneComponents(Owner);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent == nullptr || SceneComponent == this)
		{
			continue;
		}

		if (SceneComponent->GetAttachParent() == this)
		{
			return SceneComponent;
		}
	}

	if (StoredVisualComponentName.IsNone())
	{
		return nullptr;
	}

	const FString TargetName = StoredVisualComponentName.ToString();
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent == nullptr || SceneComponent == this)
		{
			continue;
		}

		if (SceneComponent->GetFName() == StoredVisualComponentName
			|| SceneComponent->ComponentTags.Contains(StoredVisualComponentName)
			|| SceneComponent->GetName().Equals(TargetName, ESearchCase::IgnoreCase)
			|| SceneComponent->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void UUOUStoredContentVisualComponent::BindWaterContainerEvents()
{
	if (BoundWaterContainerComponent == WaterContainerComponent)
	{
		return;
	}

	UnbindWaterContainerEvents();
	BoundWaterContainerComponent = WaterContainerComponent;
	if (!IsValid(BoundWaterContainerComponent))
	{
		return;
	}

	BoundWaterContainerComponent->OnWaterAmountChanged.RemoveDynamic(this, &UUOUStoredContentVisualComponent::HandleWaterAmountChanged);
	BoundWaterContainerComponent->OnWaterAmountChanged.AddDynamic(this, &UUOUStoredContentVisualComponent::HandleWaterAmountChanged);
	BoundWaterContainerComponent->OnPourContentProfileChanged.RemoveDynamic(this, &UUOUStoredContentVisualComponent::HandlePourContentProfileChanged);
	BoundWaterContainerComponent->OnPourContentProfileChanged.AddDynamic(this, &UUOUStoredContentVisualComponent::HandlePourContentProfileChanged);
}

void UUOUStoredContentVisualComponent::UnbindWaterContainerEvents()
{
	if (!IsValid(BoundWaterContainerComponent))
	{
		BoundWaterContainerComponent = nullptr;
		return;
	}

	BoundWaterContainerComponent->OnWaterAmountChanged.RemoveDynamic(this, &UUOUStoredContentVisualComponent::HandleWaterAmountChanged);
	BoundWaterContainerComponent->OnPourContentProfileChanged.RemoveDynamic(this, &UUOUStoredContentVisualComponent::HandlePourContentProfileChanged);
	BoundWaterContainerComponent = nullptr;
}

void UUOUStoredContentVisualComponent::CaptureStoredVisualTransformIfNeeded()
{
	if (!StoredVisualComponent || bCapturedStoredVisualTransform)
	{
		return;
	}

	InitialStoredVisualRelativeLocation = StoredVisualComponent->GetRelativeLocation();
	InitialStoredVisualRelativeScale = StoredVisualComponent->GetRelativeScale3D();
	bCapturedStoredVisualTransform = true;
}

void UUOUStoredContentVisualComponent::ApplyStoredVisualContentProfile()
{
	if (StoredVisualComponent == nullptr)
	{
		return;
	}

	const FUOUPourStoredVisualSettings* StoredVisualSettings = GetProfileStoredVisualSettings();
	if (StoredVisualSettings == nullptr)
	{
		return;
	}

	if (UStaticMeshComponent* StoredVisualStaticMesh = Cast<UStaticMeshComponent>(StoredVisualComponent.Get()))
	{
		if (StoredVisualSettings->Mesh != nullptr)
		{
			StoredVisualStaticMesh->SetStaticMesh(StoredVisualSettings->Mesh);
		}

		for (int32 MaterialIndex = 0; MaterialIndex < StoredVisualSettings->Materials.Num(); ++MaterialIndex)
		{
			if (StoredVisualSettings->Materials[MaterialIndex] != nullptr)
			{
				StoredVisualStaticMesh->SetMaterial(MaterialIndex, StoredVisualSettings->Materials[MaterialIndex]);
			}
		}
	}

	if (UNiagaraComponent* StoredVisualNiagara = Cast<UNiagaraComponent>(StoredVisualComponent.Get()))
	{
		if (StoredVisualSettings->NiagaraSystem != nullptr)
		{
			StoredVisualNiagara->SetAsset(StoredVisualSettings->NiagaraSystem);
		}
	}
}

void UUOUStoredContentVisualComponent::UpdateStoredVisual(float DeltaTime, bool bSnapToTarget)
{
	if (!bUpdateStoredVisual)
	{
		return;
	}

	ResolveWaterContainerComponent();
	ResolveStoredVisualComponent();
	if (WaterContainerComponent == nullptr || StoredVisualComponent == nullptr)
	{
		return;
	}

	BindWaterContainerEvents();
	const float TargetFillRatio = GetTargetFillVisualRatio();
	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	const FUOUPourStoredVisualSettings* MotionSettings = GetActiveMotionSettings();
	const float ResolvedInterpSpeed = MotionSettings != nullptr
		? MotionSettings->FillVisualInterpSpeed
		: FillVisualInterpSpeed;
	const float SafeInterpSpeed = FMath::Max(0.0f, ResolvedInterpSpeed);
	if (bSnapToTarget || SafeInterpSpeed <= KINDA_SMALL_NUMBER)
	{
		DisplayedFillVisualRatio = TargetFillRatio;
	}
	else
	{
		DisplayedFillVisualRatio = FMath::FInterpTo(
			DisplayedFillVisualRatio,
			TargetFillRatio,
			SafeDeltaTime,
			SafeInterpSpeed);
	}

	DisplayedFillVisualRatio = FMath::Clamp(DisplayedFillVisualRatio, 0.0f, 1.0f);
	CaptureStoredVisualTransformIfNeeded();

	ApplyStoredVisualTransform(DisplayedFillVisualRatio);
	ApplyStoredVisualParameters(DisplayedFillVisualRatio);

	const bool bShouldShow = ShouldShowStoredVisual();
	if (bAutoActivateNiagara)
	{
		if (UNiagaraComponent* StoredVisualNiagara = Cast<UNiagaraComponent>(StoredVisualComponent.Get()))
		{
			if (bShouldShow && !StoredVisualNiagara->IsActive())
			{
				StoredVisualNiagara->Activate(true);
			}
			else if (!bShouldShow && StoredVisualNiagara->IsActive())
			{
				StoredVisualNiagara->Deactivate();
			}
		}
	}

	StoredVisualComponent->SetHiddenInGame(!bShouldShow, true);
	StoredVisualComponent->SetVisibility(bShouldShow, true);
}

void UUOUStoredContentVisualComponent::ApplyStoredVisualTransform(float FillRatio)
{
	if (StoredVisualComponent == nullptr)
	{
		return;
	}

	const FUOUPourStoredVisualSettings* MotionSettings = GetActiveMotionSettings();
	const FVector ResolvedFullLocationOffset = MotionSettings != nullptr
		? MotionSettings->FullLocationOffset
		: FullLocationOffset;
	const FVector NewLocation = InitialStoredVisualRelativeLocation
		+ ResolvedFullLocationOffset * FillRatio;
	StoredVisualComponent->SetRelativeLocation(NewLocation);

	if (bKeepNiagaraScaleForFill && Cast<UNiagaraComponent>(StoredVisualComponent.Get()) != nullptr)
	{
		StoredVisualComponent->SetRelativeScale3D(InitialStoredVisualRelativeScale);
		return;
	}

	FVector EffectiveEmptyScaleMultiplier = MotionSettings != nullptr
		? MotionSettings->EmptyScaleMultiplier
		: EmptyScaleMultiplier;
	const FVector EffectiveFullScaleMultiplier = MotionSettings != nullptr
		? MotionSettings->FullScaleMultiplier
		: FullScaleMultiplier;
	if (MotionSettings == nullptr
		&& EffectiveEmptyScaleMultiplier.Equals(FVector::OneVector)
		&& EffectiveFullScaleMultiplier.Equals(FVector::OneVector))
	{
		EffectiveEmptyScaleMultiplier.Z = 0.0f;
	}

	const FVector ScaleMultiplier = FMath::Lerp(
		EffectiveEmptyScaleMultiplier,
		EffectiveFullScaleMultiplier,
		FillRatio);
	const FVector NewScale(
		InitialStoredVisualRelativeScale.X * ScaleMultiplier.X,
		InitialStoredVisualRelativeScale.Y * ScaleMultiplier.Y,
		InitialStoredVisualRelativeScale.Z * ScaleMultiplier.Z);
	StoredVisualComponent->SetRelativeScale3D(NewScale);
}

void UUOUStoredContentVisualComponent::ApplyStoredVisualParameters(float FillRatio)
{
	if (StoredVisualComponent == nullptr)
	{
		return;
	}

	const FUOUPourStoredVisualSettings* MotionSettings = GetActiveMotionSettings();
	if (UMeshComponent* StoredVisualMesh = Cast<UMeshComponent>(StoredVisualComponent.Get()))
	{
		const FName ResolvedMeshFillRatioParameterName = MotionSettings != nullptr
			? MotionSettings->MeshFillRatioParameterName
			: MeshFillRatioParameterName;
		if (!ResolvedMeshFillRatioParameterName.IsNone())
		{
			StoredVisualMesh->SetScalarParameterValueOnMaterials(ResolvedMeshFillRatioParameterName, FillRatio);
		}
	}

	if (UNiagaraComponent* StoredVisualNiagara = Cast<UNiagaraComponent>(StoredVisualComponent.Get()))
	{
		const FName ResolvedNiagaraFillRatioParameterName = MotionSettings != nullptr
			? MotionSettings->NiagaraFillRatioParameterName
			: NiagaraFillRatioParameterName;
		if (!ResolvedNiagaraFillRatioParameterName.IsNone())
		{
			StoredVisualNiagara->SetVariableFloat(ResolvedNiagaraFillRatioParameterName, FillRatio);
		}
	}
}

bool UUOUStoredContentVisualComponent::ShouldShowStoredVisual() const
{
	if (!bUpdateStoredVisual || StoredVisualComponent == nullptr || WaterContainerComponent == nullptr)
	{
		return false;
	}

	const FUOUPourStoredVisualSettings* MotionSettings = GetActiveMotionSettings();
	const bool bResolvedHideWhenEmpty = MotionSettings != nullptr
		? MotionSettings->bHideWhenEmpty
		: bHideWhenEmpty;
	if (bResolvedHideWhenEmpty
		&& GetTargetFillVisualRatio() <= KINDA_SMALL_NUMBER
		&& DisplayedFillVisualRatio <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	return true;
}

float UUOUStoredContentVisualComponent::GetTargetFillVisualRatio() const
{
	return WaterContainerComponent != nullptr
		? FMath::Clamp(WaterContainerComponent->GetFillRatio(), 0.0f, 1.0f)
		: 0.0f;
}

const FUOUPourStoredVisualSettings* UUOUStoredContentVisualComponent::GetProfileStoredVisualSettings() const
{
	UUOUPourContentProfile* ContentProfile = WaterContainerComponent != nullptr
		? WaterContainerComponent->GetPourContentProfile()
		: nullptr;
	return ContentProfile != nullptr ? &ContentProfile->StoredVisual : nullptr;
}

const FUOUPourStoredVisualSettings* UUOUStoredContentVisualComponent::GetActiveMotionSettings() const
{
	UUOUPourContentProfile* ContentProfile = WaterContainerComponent != nullptr
		? WaterContainerComponent->GetPourContentProfile()
		: nullptr;
	if (ContentProfile != nullptr && ContentProfile->StoredVisual.bOverrideContainerFillVisual)
	{
		return &ContentProfile->StoredVisual;
	}

	return nullptr;
}

void UUOUStoredContentVisualComponent::HandleWaterAmountChanged(float, float)
{
	UpdateStoredVisual(0.0f, false);
}

void UUOUStoredContentVisualComponent::HandlePourContentProfileChanged(UUOUPourContentProfile*)
{
	ApplyStoredVisualContentProfile();
	UpdateStoredVisual(0.0f, false);
}
