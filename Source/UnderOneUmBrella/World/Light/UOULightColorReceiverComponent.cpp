// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightColorReceiverComponent.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UUOULightColorReceiverComponent::UUOULightColorReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 색상 반응만 필요한 액터가 기존 온도 시스템까지 함께 갱신되지 않도록 합니다.
	// 두 기능을 함께 쓰고 싶은 블루프린트에서는 이 값을 다시 설정할 수 있습니다.
	TemperatureRisePerIntensity = 0.0f;
	bRecoverToAmbientWhenNotExposed = false;
}

void UUOULightColorReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshTargetMaterials();
}

void UUOULightColorReceiverComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveColorExposures.Reset();
	PaintMaterialTargets.Reset();
	Super::EndPlay(EndPlayReason);
}

void UUOULightColorReceiverComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePaintTransition(DeltaTime);

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

void UUOULightColorReceiverComponent::ResetPaintTint(bool bImmediate)
{
	SetPaintTarget(FLinearColor::White, bImmediate);
}

int32 UUOULightColorReceiverComponent::GetCurrentStateMaterialIndex() const
{
	return ResolveStateMaterialIndex(CurrentColorState);
}

FLinearColor UUOULightColorReceiverComponent::GetPaintTintForColorState(
	EUOULightColorState ColorState) const
{
	switch (ColorState)
	{
	case EUOULightColorState::Red:
		return CalculatePaintTint(FLinearColor::Red);
	case EUOULightColorState::Green:
		return CalculatePaintTint(FLinearColor::Green);
	case EUOULightColorState::Blue:
		return CalculatePaintTint(FLinearColor::Blue);
	case EUOULightColorState::RedGreen:
		return CalculatePaintTint(FLinearColor::Yellow);
	case EUOULightColorState::RedBlue:
		return CalculatePaintTint(FLinearColor(1.0f, 0.0f, 1.0f, 1.0f));
	case EUOULightColorState::GreenBlue:
		return CalculatePaintTint(FLinearColor(0.0f, 1.0f, 1.0f, 1.0f));
	case EUOULightColorState::RedGreenBlue:
		return FLinearColor::White;
	case EUOULightColorState::None:
	default:
		return FLinearColor::White;
	}
}

void UUOULightColorReceiverComponent::RefreshTargetMaterials()
{
	PaintMaterialTargets.Reset();

	if (!bApplyPaintTint)
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

	if (PaintMaterialTargets.IsEmpty() && bAutoFindMeshComponents)
	{
		TInlineComponentArray<UMeshComponent*> MeshComponents(Owner);
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			AddMeshComponentTarget(MeshComponent);
		}
	}

	ApplyCurrentPaintTint();
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

void UUOULightColorReceiverComponent::UpdatePaintTransition(float DeltaTime)
{
	if (!bPaintTransitionActive || DeltaTime <= 0.0f)
	{
		return;
	}

	const float SafeDuration = FMath::Max(0.0f, MaterialTransitionDuration);
	if (SafeDuration <= KINDA_SMALL_NUMBER)
	{
		CurrentPaintTint = TargetPaintTint;
		bPaintTransitionActive = false;
		ApplyCurrentPaintTint();
		return;
	}

	PaintTransitionElapsed = FMath::Min(
		PaintTransitionElapsed + DeltaTime,
		SafeDuration);
	const float Alpha = PaintTransitionElapsed / SafeDuration;
	const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
	CurrentPaintTint = FMath::Lerp(PaintTransitionStart, TargetPaintTint, SmoothAlpha);
	CurrentPaintTint.A = 1.0f;
	ApplyCurrentPaintTint();

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		CurrentPaintTint = TargetPaintTint;
		bPaintTransitionActive = false;
		ApplyCurrentPaintTint();
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

	if (bNewHasAnyColorLight && (bForceApply || bColorStateChanged || bMixedColorChanged))
	{
		SetPaintTargetFromLight(MixedLightColor);
	}
	else if (!bNewHasAnyColorLight && (bForceApply || bAnyLightChanged))
	{
		// 빛이 사라지면 원래 색으로 복구하지 않고, 그 순간까지 묻은 색을 유지합니다.
		HoldCurrentPaintTint();
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

void UUOULightColorReceiverComponent::SetPaintTargetFromLight(
	const FLinearColor& LightColor)
{
	SetPaintTarget(CalculatePaintTint(LightColor));
}

void UUOULightColorReceiverComponent::SetPaintTarget(
	const FLinearColor& NewTarget,
	bool bImmediate)
{
	FLinearColor ClampedTarget = NewTarget.GetClamped(0.0f, 1.0f);
	ClampedTarget.A = 1.0f;
	if (!bImmediate && TargetPaintTint.Equals(ClampedTarget, KINDA_SMALL_NUMBER))
	{
		return;
	}

	TargetPaintTint = ClampedTarget;
	if (bImmediate || MaterialTransitionDuration <= KINDA_SMALL_NUMBER)
	{
		CurrentPaintTint = TargetPaintTint;
		PaintTransitionStart = CurrentPaintTint;
		PaintTransitionElapsed = 0.0f;
		bPaintTransitionActive = false;
		ApplyCurrentPaintTint();
		return;
	}

	PaintTransitionStart = CurrentPaintTint;
	PaintTransitionElapsed = 0.0f;
	bPaintTransitionActive = true;
}

void UUOULightColorReceiverComponent::HoldCurrentPaintTint()
{
	TargetPaintTint = CurrentPaintTint;
	PaintTransitionStart = CurrentPaintTint;
	PaintTransitionElapsed = 0.0f;
	bPaintTransitionActive = false;
}

void UUOULightColorReceiverComponent::ApplyCurrentPaintTint()
{
	OnPaintTintChanged.Broadcast(CurrentPaintTint);

	if (!bApplyPaintTint || PaintTintParameterName.IsNone())
	{
		return;
	}

	for (FPaintMaterialTarget& Target : PaintMaterialTargets)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = Target.DynamicMaterial.Get())
		{
			DynamicMaterial->SetVectorParameterValue(
				PaintTintParameterName,
				CurrentPaintTint);
		}
	}
}

void UUOULightColorReceiverComponent::AddMeshComponentTarget(UMeshComponent* MeshComponent)
{
	if (MeshComponent == nullptr ||
		TargetMaterialSlotIndex < 0 ||
		TargetMaterialSlotIndex >= MeshComponent->GetNumMaterials())
	{
		return;
	}

	const bool bAlreadyAdded = PaintMaterialTargets.ContainsByPredicate(
		[MeshComponent](const FPaintMaterialTarget& ExistingTarget)
		{
			return ExistingTarget.Mesh.Get() == MeshComponent;
		});
	if (bAlreadyAdded)
	{
		return;
	}

	UMaterialInterface* SourceMaterial = MeshComponent->GetMaterial(TargetMaterialSlotIndex);
	UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(
		TargetMaterialSlotIndex,
		SourceMaterial);
	if (DynamicMaterial == nullptr)
	{
		return;
	}

	FPaintMaterialTarget& NewTarget = PaintMaterialTargets.AddDefaulted_GetRef();
	NewTarget.Mesh = MeshComponent;
	NewTarget.DynamicMaterial = DynamicMaterial;
	DynamicMaterial->SetVectorParameterValue(PaintTintParameterName, CurrentPaintTint);
}

FLinearColor UUOULightColorReceiverComponent::CalculatePaintTint(
	const FLinearColor& LightColor) const
{
	const float SafeThreshold = FMath::Clamp(ActiveChannelThreshold, 0.0f, 1.0f);
	FLinearColor ActiveColor(
		LightColor.R >= SafeThreshold ? LightColor.R : 0.0f,
		LightColor.G >= SafeThreshold ? LightColor.G : 0.0f,
		LightColor.B >= SafeThreshold ? LightColor.B : 0.0f,
		1.0f);
	const float MaxChannel = FMath::Max3(ActiveColor.R, ActiveColor.G, ActiveColor.B);
	if (MaxChannel <= KINDA_SMALL_NUMBER)
	{
		return CurrentPaintTint;
	}

	ActiveColor.R /= MaxChannel;
	ActiveColor.G /= MaxChannel;
	ActiveColor.B /= MaxChannel;
	const float SafeMinimumChannel = FMath::Clamp(MinimumPaintChannel, 0.0f, 1.0f);
	return FLinearColor(
		FMath::Lerp(SafeMinimumChannel, 1.0f, ActiveColor.R),
		FMath::Lerp(SafeMinimumChannel, 1.0f, ActiveColor.G),
		FMath::Lerp(SafeMinimumChannel, 1.0f, ActiveColor.B),
		1.0f);
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
