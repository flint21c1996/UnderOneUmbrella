// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULumenZClippedStaticRayVisualActor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

namespace
{
	const FName ZClippedRayBeamClipLocalZParameter(TEXT("BeamClipLocalZ"));
	const FName SparkleBeamLengthParameter(TEXT("User.BeamLength"));
	const FName SparkleBeamStartRadiusParameter(TEXT("User.BeamStartRadius"));
	const FName SparkleBeamEndRadiusParameter(TEXT("User.BeamEndRadius"));
	const FName SparkleBeamColorParameter(TEXT("User.BeamColor"));
	const FName SparkleBeamIntensityParameter(TEXT("User.BeamIntensity"));
	const FName SparkleBeamOpacityParameter(TEXT("User.BeamOpacity"));
	const FName SparkleCylinderHeightParameter(TEXT("User.CylinderHeight"));
	const FName SparkleCylinderRadiusParameter(TEXT("User.CylinderRadius"));
	const FName SparkleMoteColorParameter(TEXT("User.MoteColor"));
}

AUOULumenZClippedStaticRayVisualActor::AUOULumenZClippedStaticRayVisualActor()
{
	SparkleEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SparkleEffect"));
	SparkleEffect->SetupAttachment(RootScene);
	SparkleEffect->SetAutoActivate(false);
	SparkleEffect->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (UMaterialInterface* ZClippedMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/UOU/Effects/StylizedLightFX/Materials/M_SLF_StaticRay_Master_V4.M_SLF_StaticRay_Master_V4")))
	{
		RayMaterial = ZClippedMaterial;
	}
}

void AUOULumenZClippedStaticRayVisualActor::ApplyLightBeamSegment_Implementation(
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	ApplySparkleParameters(SegmentData);
	Super::ApplyLightBeamSegment_Implementation(SegmentData);
}

void AUOULumenZClippedStaticRayVisualActor::SetLightBeamVisualActive_Implementation(
	const bool bActive)
{
	Super::SetLightBeamVisualActive_Implementation(bActive);
	if (SparkleEffect == nullptr)
	{
		return;
	}

	const bool bShouldBeActive = bActive && bEnableSparkleEffect && SparkleEffect->GetAsset() != nullptr;
	SparkleEffect->SetVisibility(bShouldBeActive, true);
	if (bShouldBeActive)
	{
		if (!SparkleEffect->IsActive())
		{
			SparkleEffect->Activate(true);
		}
	}
	else
	{
		SparkleEffect->DeactivateImmediate();
	}
}

void AUOULumenZClippedStaticRayVisualActor::SetSparkleSystemOverride(
	const bool bOverrideNiagaraSystem,
	UNiagaraSystem* NiagaraSystemOverride)
{
	if (SparkleEffect == nullptr)
	{
		return;
	}
	if (!bHasCachedDefaultSparkleSystem)
	{
		DefaultSparkleSystem = SparkleEffect->GetAsset();
		bHasCachedDefaultSparkleSystem = true;
	}

	UNiagaraSystem* DesiredSystem = bOverrideNiagaraSystem
		? NiagaraSystemOverride
		: DefaultSparkleSystem.Get();
	if (SparkleEffect->GetAsset() == DesiredSystem)
	{
		return;
	}

	SparkleEffect->DeactivateImmediate();
	SparkleEffect->SetAsset(DesiredSystem);
	SparkleEffect->SetVisibility(DesiredSystem != nullptr, true);
}

float AUOULumenZClippedStaticRayVisualActor::ResolveVisualLength(
	const FUOULightBeamVisualSegmentData& SegmentData) const
{
	return FMath::Max(SegmentData.Length, SegmentData.ReferenceLength);
}

float AUOULumenZClippedStaticRayVisualActor::ResolveLayerCenterOffset(
	const FUOULightBeamVisualSegmentData&,
	const float FullLayerLength) const
{
	return FullLayerLength * 0.5f;
}

void AUOULumenZClippedStaticRayVisualActor::ApplySegmentClipParameters(
	UMaterialInstanceDynamic* Material,
	const FUOULightBeamVisualSegmentData& SegmentData,
	const FBoxSphereBounds& MeshBounds,
	const FVector& AppliedLayerScale,
	const float FullLayerLength) const
{
	if (Material == nullptr)
	{
		return;
	}

	const float LocalMeshMinZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
	const float VisibleLayerLength = FMath::Min(
		FMath::Max(0.0f, SegmentData.Length),
		FMath::Max(0.0f, FullLayerLength));
	const float BeamClipLocalZ = LocalMeshMinZ +
		VisibleLayerLength /
		FMath::Max(KINDA_SMALL_NUMBER, FMath::Abs(AppliedLayerScale.Z));
	Material->SetScalarParameterValue(
		ZClippedRayBeamClipLocalZParameter,
		BeamClipLocalZ);
}

void AUOULumenZClippedStaticRayVisualActor::ApplySparkleParameters(
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	if (SparkleEffect == nullptr)
	{
		return;
	}

	const float BeamLength = FMath::Max(0.0f, SegmentData.Length);
	const float BeamRadius = FMath::Max(
		FMath::Max(0.0f, SegmentData.StartRadius),
		FMath::Max(0.0f, SegmentData.EndRadius));
	FLinearColor MoteColor = SegmentData.Color;
	MoteColor.A = 1.0f;

	SparkleEffect->SetRelativeLocation(FVector(0.0f, 0.0f, BeamLength * 0.5f));
	SparkleEffect->SetRelativeRotation(FRotator::ZeroRotator);
	SparkleEffect->SetRelativeScale3D(FVector::OneVector);
	SparkleEffect->SetVariableFloat(
		SparkleBeamLengthParameter,
		BeamLength);
	SparkleEffect->SetVariableFloat(
		SparkleBeamStartRadiusParameter,
		FMath::Max(0.0f, SegmentData.StartRadius));
	SparkleEffect->SetVariableFloat(
		SparkleBeamEndRadiusParameter,
		FMath::Max(0.0f, SegmentData.EndRadius));
	SparkleEffect->SetVariableLinearColor(
		SparkleBeamColorParameter,
		SegmentData.Color);
	SparkleEffect->SetVariableFloat(
		SparkleBeamIntensityParameter,
		FMath::Max(0.0f, SegmentData.Intensity * SegmentData.VisualBrightnessMultiplier));
	SparkleEffect->SetVariableFloat(
		SparkleBeamOpacityParameter,
		1.0f);
	SparkleEffect->SetVariableFloat(
		SparkleCylinderHeightParameter,
		BeamLength);
	SparkleEffect->SetVariableFloat(
		SparkleCylinderRadiusParameter,
		BeamRadius);
	SparkleEffect->SetVariableLinearColor(
		SparkleMoteColorParameter,
		MoteColor);
}
