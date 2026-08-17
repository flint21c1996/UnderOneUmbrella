// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULumenZClippedStaticRayVisualActor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const FName ZClippedRayBeamClipLocalZParameter(TEXT("BeamClipLocalZ"));
}

AUOULumenZClippedStaticRayVisualActor::AUOULumenZClippedStaticRayVisualActor()
{
	if (UMaterialInterface* ZClippedMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/UOU/Effects/StylizedLightFX/Materials/M_SLF_StaticRay_Master_V4.M_SLF_StaticRay_Master_V4")))
	{
		RayMaterial = ZClippedMaterial;
	}
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
