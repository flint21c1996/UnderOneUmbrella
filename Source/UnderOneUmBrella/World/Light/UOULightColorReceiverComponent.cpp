// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightColorReceiverComponent.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UUOULightColorReceiverComponent::UUOULightColorReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	StateMaterials.SetNum(7);

	// 색상 반응만 필요한 액터가 기존 온도 시스템까지 함께 갱신되지 않도록 합니다.
	// 두 기능을 함께 쓰고 싶은 블루프린트에서는 이 값을 다시 설정할 수 있습니다.
	TemperatureRisePerIntensity = 0.0f;
	bRecoverToAmbientWhenNotExposed = false;
}

void UUOULightColorReceiverComponent::PostLoad()
{
	Super::PostLoad();

	// 이전 버전은 0번에 기본 머티리얼, 1~7번에 RGB 상태를 저장했습니다.
	// 저장된 BP/레벨 컴포넌트가 8칸이면 색 상태 머티리얼만 한 칸씩 당겨 이관합니다.
	if (StateMaterials.Num() == 8)
	{
		for (int32 NewIndex = 0; NewIndex < 7; ++NewIndex)
		{
			StateMaterials[NewIndex] = StateMaterials[NewIndex + 1];
		}
	}
	StateMaterials.SetNum(7);
}

void UUOULightColorReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshTargetMaterials();
}

void UUOULightColorReceiverComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveColorExposures.Reset();
	MaterialStateTargets.Reset();
	Super::EndPlay(EndPlayReason);
}

void UUOULightColorReceiverComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateMaterialTransitions(DeltaTime);

	const int32 PreviousExposureCount = ActiveColorExposures.Num();
	RemoveExpiredColorExposures();
	if (ActiveColorExposures.Num() != PreviousExposureCount)
	{
		RecalculateMixedLightColor();
	}
}

void UUOULightColorReceiverComponent::ReceiveLightExposure_Implementation(
	const FUOULightExposureData& ExposureData)
{
	Super::ReceiveLightExposure_Implementation(ExposureData);

	if (ExposureData.Source == nullptr ||
		ExposureData.Intensity < FMath::Max(0.0f, MinimumColorExposureIntensity))
	{
		return;
	}

	FActiveColorExposure& ActiveExposure = ActiveColorExposures.FindOrAdd(ExposureData.Source);
	ActiveExposure.Color = ExposureData.LightColor.GetClamped(0.0f, 1.0f);
	ActiveExposure.Intensity = FMath::Max(0.0f, ExposureData.Intensity);
	ActiveExposure.LastReceivedWorldTime = GetWorld() != nullptr
		? GetWorld()->GetTimeSeconds()
		: 0.0f;

	RecalculateMixedLightColor();
}

void UUOULightColorReceiverComponent::ClearColorExposures()
{
	ActiveColorExposures.Reset();
	RecalculateMixedLightColor(true);
}

int32 UUOULightColorReceiverComponent::GetCurrentStateMaterialIndex() const
{
	return ResolveStateMaterialIndex(CurrentColorState);
}

void UUOULightColorReceiverComponent::RefreshTargetMaterials()
{
	MaterialStateTargets.Reset();

	if (!bApplyStateMaterials)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	for (const FComponentReference& MeshReference : TargetMeshReferences)
	{
		AddMeshComponentTarget(Cast<UMeshComponent>(MeshReference.GetComponent(Owner)));
	}

	if (MaterialStateTargets.IsEmpty() && bAutoFindMeshComponents)
	{
		TInlineComponentArray<UMeshComponent*> MeshComponents(Owner);
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			AddMeshComponentTarget(MeshComponent);
		}
	}

	ApplyStateMaterial(CurrentColorState);
}

void UUOULightColorReceiverComponent::RemoveExpiredColorExposures()
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float CurrentWorldTime = World->GetTimeSeconds();
	const float SafeGraceTime = FMath::Max(0.0f, ColorExposureEndGraceTime);
	for (auto It = ActiveColorExposures.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() ||
			CurrentWorldTime - It.Value().LastReceivedWorldTime > SafeGraceTime)
		{
			It.RemoveCurrent();
		}
	}
}

void UUOULightColorReceiverComponent::UpdateMaterialTransitions(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	for (FMaterialStateTarget& Target : MaterialStateTargets)
	{
		UMaterialInstanceDynamic* BlendMaterial = Target.BlendMaterial.Get();
		UMaterialInstance* DestinationMaterial = Target.DestinationMaterial.Get();
		UMeshComponent* MeshComponent = Target.Mesh.Get();
		if (BlendMaterial == nullptr || DestinationMaterial == nullptr || MeshComponent == nullptr ||
			Target.TransitionTimeRemaining <= 0.0f)
		{
			continue;
		}

		const float StepAlpha = FMath::Clamp(
			DeltaTime / Target.TransitionTimeRemaining,
			0.0f,
			1.0f);
		BlendMaterial->K2_InterpolateMaterialInstanceParams(
			BlendMaterial,
			DestinationMaterial,
			StepAlpha);
		Target.TransitionTimeRemaining = FMath::Max(
			0.0f,
			Target.TransitionTimeRemaining - DeltaTime);

		if (Target.TransitionTimeRemaining <= KINDA_SMALL_NUMBER)
		{
			MeshComponent->SetMaterial(TargetMaterialSlotIndex, DestinationMaterial);
			Target.BlendMaterial.Reset();
			Target.DestinationMaterial.Reset();
		}
	}
}

void UUOULightColorReceiverComponent::RecalculateMixedLightColor(bool bForceApply)
{
	FLinearColor NewMixedColor = FLinearColor::Black;
	for (const TPair<TWeakObjectPtr<UObject>, FActiveColorExposure>& Pair : ActiveColorExposures)
	{
		if (!Pair.Key.IsValid())
		{
			continue;
		}

		const float Weight = bWeightColorByExposureIntensity
			? Pair.Value.Intensity
			: 1.0f;
		NewMixedColor += Pair.Value.Color * Weight;
	}

	NewMixedColor.R = FMath::Clamp(NewMixedColor.R, 0.0f, 1.0f);
	NewMixedColor.G = FMath::Clamp(NewMixedColor.G, 0.0f, 1.0f);
	NewMixedColor.B = FMath::Clamp(NewMixedColor.B, 0.0f, 1.0f);
	NewMixedColor.A = 1.0f;

	const bool bNewHasAnyColorLight = !ActiveColorExposures.IsEmpty();
	const float SafeChannelThreshold = FMath::Clamp(ActiveChannelThreshold, 0.0f, 1.0f);
	const bool bNewHasRedLight = bNewHasAnyColorLight && NewMixedColor.R >= SafeChannelThreshold;
	const bool bNewHasGreenLight = bNewHasAnyColorLight && NewMixedColor.G >= SafeChannelThreshold;
	const bool bNewHasBlueLight = bNewHasAnyColorLight && NewMixedColor.B >= SafeChannelThreshold;
	const EUOULightColorState NewColorState = ResolveColorState(
		bNewHasRedLight,
		bNewHasGreenLight,
		bNewHasBlueLight);
	const bool bColorStateChanged = CurrentColorState != NewColorState;
	const bool bMixedColorChanged = !MixedLightColor.Equals(NewMixedColor, KINDA_SMALL_NUMBER);
	const bool bAnyLightChanged = bHasAnyColorLight != bNewHasAnyColorLight;

	MixedLightColor = NewMixedColor;
	CurrentColorState = NewColorState;
	bHasAnyColorLight = bNewHasAnyColorLight;
	bHasRedLight = bNewHasRedLight;
	bHasGreenLight = bNewHasGreenLight;
	bHasBlueLight = bNewHasBlueLight;

	if (bForceApply || bColorStateChanged)
	{
		ApplyStateMaterial(CurrentColorState);
	}

	if (bColorStateChanged)
	{
		OnLightColorStateChanged.Broadcast(
			CurrentColorState,
			ResolveStateMaterialIndex(CurrentColorState));
	}

	if (bForceApply || bColorStateChanged || bMixedColorChanged || bAnyLightChanged)
	{
		OnMixedLightColorChanged.Broadcast(MixedLightColor, bHasAnyColorLight);
	}
}

void UUOULightColorReceiverComponent::ApplyStateMaterial(EUOULightColorState NewState)
{
	if (!bApplyStateMaterials)
	{
		return;
	}

	const int32 StateIndex = ResolveStateMaterialIndex(NewState);
	UMaterialInterface* StateMaterial = StateMaterials.IsValidIndex(StateIndex)
		? StateMaterials[StateIndex].Get()
		: nullptr;

	for (FMaterialStateTarget& Target : MaterialStateTargets)
	{
		UMaterialInterface* MaterialToApply = NewState == EUOULightColorState::None
			? Target.OriginalMaterial.Get()
			: StateMaterial;
		if (MaterialToApply != nullptr)
		{
			ApplyOrTransitionMaterial(Target, MaterialToApply);
		}
	}
}

void UUOULightColorReceiverComponent::ApplyOrTransitionMaterial(
	FMaterialStateTarget& Target,
	UMaterialInterface* DesiredMaterial)
{
	UMeshComponent* MeshComponent = Target.Mesh.Get();
	if (MeshComponent == nullptr || DesiredMaterial == nullptr)
	{
		return;
	}

	UMaterialInterface* CurrentMaterial = MeshComponent->GetMaterial(TargetMaterialSlotIndex);
	if (CurrentMaterial == DesiredMaterial)
	{
		Target.BlendMaterial.Reset();
		Target.DestinationMaterial.Reset();
		Target.TransitionTimeRemaining = 0.0f;
		return;
	}

	UMaterialInstance* DestinationInstance = Cast<UMaterialInstance>(DesiredMaterial);
	const bool bCanInterpolate =
		bSmoothMaterialTransitions &&
		MaterialTransitionDuration > KINDA_SMALL_NUMBER &&
		CurrentMaterial != nullptr &&
		DestinationInstance != nullptr &&
		CurrentMaterial->GetBaseMaterial() == DesiredMaterial->GetBaseMaterial();
	if (!bCanInterpolate)
	{
		MeshComponent->SetMaterial(TargetMaterialSlotIndex, DesiredMaterial);
		Target.BlendMaterial.Reset();
		Target.DestinationMaterial.Reset();
		Target.TransitionTimeRemaining = 0.0f;
		return;
	}

	UMaterialInstanceDynamic* BlendMaterial = UMaterialInstanceDynamic::Create(
		DesiredMaterial,
		this);
	if (BlendMaterial == nullptr)
	{
		MeshComponent->SetMaterial(TargetMaterialSlotIndex, DesiredMaterial);
		return;
	}

	// 현재 화면에 보이는 파라미터를 복사한 뒤 목표 Material Instance까지 보간합니다.
	// 도중에 다른 색 상태가 들어와도 현재 중간값에서 다시 이어집니다.
	BlendMaterial->K2_CopyMaterialInstanceParameters(CurrentMaterial, true);
	MeshComponent->SetMaterial(TargetMaterialSlotIndex, BlendMaterial);
	Target.BlendMaterial = BlendMaterial;
	Target.DestinationMaterial = DestinationInstance;
	Target.TransitionTimeRemaining = MaterialTransitionDuration;
}

void UUOULightColorReceiverComponent::AddMeshComponentTarget(UMeshComponent* MeshComponent)
{
	if (MeshComponent == nullptr ||
		TargetMaterialSlotIndex < 0 ||
		TargetMaterialSlotIndex >= MeshComponent->GetNumMaterials())
	{
		return;
	}

	const bool bAlreadyAdded = MaterialStateTargets.ContainsByPredicate(
		[MeshComponent](const FMaterialStateTarget& ExistingTarget)
		{
			return ExistingTarget.Mesh.Get() == MeshComponent;
		});
	if (bAlreadyAdded)
	{
		return;
	}

	FMaterialStateTarget& NewTarget = MaterialStateTargets.AddDefaulted_GetRef();
	NewTarget.Mesh = MeshComponent;
	NewTarget.OriginalMaterial = MeshComponent->GetMaterial(TargetMaterialSlotIndex);
}

EUOULightColorState UUOULightColorReceiverComponent::ResolveColorState(
	bool bRed,
	bool bGreen,
	bool bBlue)
{
	if (bRed && bGreen && bBlue)
	{
		return EUOULightColorState::RedGreenBlue;
	}
	if (bRed && bGreen)
	{
		return EUOULightColorState::RedGreen;
	}
	if (bRed && bBlue)
	{
		return EUOULightColorState::RedBlue;
	}
	if (bGreen && bBlue)
	{
		return EUOULightColorState::GreenBlue;
	}
	if (bRed)
	{
		return EUOULightColorState::Red;
	}
	if (bGreen)
	{
		return EUOULightColorState::Green;
	}
	if (bBlue)
	{
		return EUOULightColorState::Blue;
	}
	return EUOULightColorState::None;
}

int32 UUOULightColorReceiverComponent::ResolveStateMaterialIndex(
	EUOULightColorState State)
{
	switch (State)
	{
	case EUOULightColorState::Red:
		return 0;
	case EUOULightColorState::Green:
		return 1;
	case EUOULightColorState::Blue:
		return 2;
	case EUOULightColorState::RedGreen:
		return 3;
	case EUOULightColorState::RedBlue:
		return 4;
	case EUOULightColorState::GreenBlue:
		return 5;
	case EUOULightColorState::RedGreenBlue:
		return 6;
	case EUOULightColorState::None:
	default:
		return INDEX_NONE;
	}
}
