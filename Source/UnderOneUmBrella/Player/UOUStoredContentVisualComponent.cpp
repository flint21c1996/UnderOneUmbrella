// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUStoredContentVisualComponent.h"

#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Player/UOUWaterContainerComponent.h"

UUOUStoredContentVisualComponent::UUOUStoredContentVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UUOUStoredContentVisualComponent::OnRegister()
{
	Super::OnRegister();

	ResolveReferences();
}

void UUOUStoredContentVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveReferences();
	BindWaterContainerEvents();
	BindUmbrellaEvents();
	ApplyStoredVisualContentProfile();
	DisplayedFillVisualRatio = GetTargetFillVisualRatio();
	UpdateStoredVisual(0.0f, true);
	SetComponentTickEnabled(bUpdateStoredVisual);
}

void UUOUStoredContentVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindWaterContainerEvents();
	UnbindUmbrellaEvents();

	Super::EndPlay(EndPlayReason);
}

void UUOUStoredContentVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateStoredVisual(DeltaTime);
}

void UUOUStoredContentVisualComponent::RefreshStoredContentVisual(bool bSnapToTarget)
{
	ResolveReferences();
	BindWaterContainerEvents();
	BindUmbrellaEvents();
	ApplyStoredVisualContentProfile();
	UpdateStoredVisual(0.0f, bSnapToTarget);
}

void UUOUStoredContentVisualComponent::ResolveReferences()
{
	ResolveWaterContainerComponent();
	ResolveUmbrellaComponent();
	ResolveSocketSourceComponent();
	ResolveStoredVisualComponent();
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
	if (Owner == nullptr || SocketSourceComponentName.IsNone())
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

void UUOUStoredContentVisualComponent::ResolveUmbrellaComponent()
{
	if (IsValid(UmbrellaComponent))
	{
		bResolvedUmbrellaComponent = true;
		ResolvedUmbrellaComponentName = UmbrellaComponent->GetName();
		return;
	}

	UmbrellaComponent = nullptr;
	bResolvedUmbrellaComponent = false;
	ResolvedUmbrellaComponentName = TEXT("None");

	UmbrellaComponent = FindUmbrellaComponent();
	bResolvedUmbrellaComponent = UmbrellaComponent != nullptr;
	ResolvedUmbrellaComponentName = UmbrellaComponent != nullptr ? UmbrellaComponent->GetName() : TEXT("None");
}

UUOUUmbrellaComponent* UUOUStoredContentVisualComponent::FindUmbrellaComponent() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	TInlineComponentArray<UUOUUmbrellaComponent*> UmbrellaComponents(Owner);
	if (UmbrellaComponents.IsEmpty())
	{
		return nullptr;
	}

	return UmbrellaComponents[0];
}

bool UUOUStoredContentVisualComponent::IsUmbrellaVisualStateAllowed() const
{
	return UmbrellaComponent != nullptr
		&& (UmbrellaComponent->IsUpsideDown() || UmbrellaComponent->IsPouring());
}

void UUOUStoredContentVisualComponent::ResolveSocketSourceComponent()
{
	if (IsValid(SocketSourceComponent))
	{
		bResolvedSocketSourceComponent = true;
		ResolvedSocketSourceComponentName = SocketSourceComponent->GetName();
		return;
	}

	SocketSourceComponent = nullptr;
	bResolvedSocketSourceComponent = false;
	ResolvedSocketSourceComponentName = TEXT("None");

	SocketSourceComponent = FindSocketSourceComponent();
	bResolvedSocketSourceComponent = SocketSourceComponent != nullptr;
	ResolvedSocketSourceComponentName = SocketSourceComponent != nullptr ? SocketSourceComponent->GetName() : TEXT("None");
}

USceneComponent* UUOUStoredContentVisualComponent::FindSocketSourceComponent() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	const FString TargetName = SocketSourceComponentName.ToString();
	TInlineComponentArray<USceneComponent*> SceneComponents(Owner);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent == nullptr || SceneComponent == this)
		{
			continue;
		}

		if (SceneComponent->GetFName() == SocketSourceComponentName
			|| SceneComponent->ComponentTags.Contains(SocketSourceComponentName)
			|| SceneComponent->GetName().Equals(TargetName, ESearchCase::IgnoreCase)
			|| SceneComponent->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void UUOUStoredContentVisualComponent::UpdateSocketFollowLocation()
{
	ResolveSocketSourceComponent();
	if (SocketSourceComponent == nullptr)
	{
		return;
	}

	FTransform SocketWorldTransform = SocketSourceComponent->GetComponentTransform();
	if (!StoredContentSocketName.IsNone() && SocketSourceComponent->DoesSocketExist(StoredContentSocketName))
	{
		SocketWorldTransform = SocketSourceComponent->GetSocketTransform(StoredContentSocketName, RTS_World);
	}

	FTransform TargetWorldTransform = GetComponentTransform();
	TargetWorldTransform.SetLocation(SocketWorldTransform.GetLocation());
	SetWorldTransform(TargetWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void UUOUStoredContentVisualComponent::ResolveStoredVisualComponent()
{
	if (IsValid(StoredVisualComponent))
	{
		bResolvedStoredVisualComponent = true;
		ResolvedStoredVisualComponentName = StoredVisualComponent->GetName();
		ApplyStoredVisualCollisionSettings();
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
	ApplyStoredVisualCollisionSettings();
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

void UUOUStoredContentVisualComponent::BindUmbrellaEvents()
{
	if (BoundUmbrellaComponent == UmbrellaComponent)
	{
		return;
	}

	UnbindUmbrellaEvents();
	BoundUmbrellaComponent = UmbrellaComponent;
	if (!IsValid(BoundUmbrellaComponent))
	{
		return;
	}

	BoundUmbrellaComponent->OnUmbrellaStateChanged.RemoveDynamic(this, &UUOUStoredContentVisualComponent::HandleUmbrellaStateChanged);
	BoundUmbrellaComponent->OnUmbrellaStateChanged.AddDynamic(this, &UUOUStoredContentVisualComponent::HandleUmbrellaStateChanged);
}

void UUOUStoredContentVisualComponent::UnbindUmbrellaEvents()
{
	if (!IsValid(BoundUmbrellaComponent))
	{
		BoundUmbrellaComponent = nullptr;
		return;
	}

	BoundUmbrellaComponent->OnUmbrellaStateChanged.RemoveDynamic(this, &UUOUStoredContentVisualComponent::HandleUmbrellaStateChanged);
	BoundUmbrellaComponent = nullptr;
}

void UUOUStoredContentVisualComponent::CaptureStoredVisualTransformIfNeeded()
{
	if (!StoredVisualComponent || bCapturedStoredVisualTransform)
	{
		return;
	}

	InitialStoredVisualRelativeLocation = FVector::ZeroVector;
	InitialStoredVisualRelativeScale = StoredVisualComponent->GetRelativeScale3D();
	bCapturedStoredVisualTransform = true;
}

void UUOUStoredContentVisualComponent::ApplyStoredVisualCollisionSettings() const
{
	UPrimitiveComponent* StoredVisualPrimitive = Cast<UPrimitiveComponent>(StoredVisualComponent.Get());
	if (StoredVisualPrimitive == nullptr)
	{
		return;
	}

	// 저장 내용물 비주얼은 표시 전용이므로 플레이어 주변에 추가 충돌체를 만들지 않습니다.
	StoredVisualPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StoredVisualPrimitive->SetCollisionResponseToAllChannels(ECR_Ignore);
	StoredVisualPrimitive->SetGenerateOverlapEvents(false);
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

	ApplyStoredVisualCollisionSettings();
}

void UUOUStoredContentVisualComponent::UpdateStoredVisual(float DeltaTime, bool bSnapToTarget)
{
	ResolveReferences();
	BindWaterContainerEvents();
	BindUmbrellaEvents();

	if (!bUpdateStoredVisual)
	{
		return;
	}

	UpdateSocketFollowLocation();
	if (WaterContainerComponent == nullptr || StoredVisualComponent == nullptr)
	{
		return;
	}

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

	if (bKeepStaticMeshScaleForFill && Cast<UStaticMeshComponent>(StoredVisualComponent.Get()) != nullptr)
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

	if (!IsUmbrellaVisualStateAllowed())
	{
		return false;
	}

	if (GetTargetFillVisualRatio() <= KINDA_SMALL_NUMBER
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

void UUOUStoredContentVisualComponent::HandleUmbrellaStateChanged(EUOUUmbrellaState, bool)
{
	RefreshStoredContentVisual(true);
}
