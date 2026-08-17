// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/Light/UOULumenStaticRayVisualActor.h"
#include "UOULumenZClippedStaticRayVisualActor.generated.h"

/** ReferenceLength로 메시를 유지하고 머티리얼 로컬 Z로 실제 가시 길이를 제한합니다. */
UCLASS(Blueprintable, meta = (DisplayName = "UOU Lumen Z-Clipped Static Ray Visual"))
class UNDERONEUMBRELLA_API AUOULumenZClippedStaticRayVisualActor
	: public AUOULumenStaticRayVisualActor
{
	GENERATED_BODY()

public:
	AUOULumenZClippedStaticRayVisualActor();

protected:
	virtual float ResolveVisualLength(
		const FUOULightBeamVisualSegmentData& SegmentData) const override;
	virtual float ResolveLayerCenterOffset(
		const FUOULightBeamVisualSegmentData& SegmentData,
		float FullLayerLength) const override;
	virtual void ApplySegmentClipParameters(
		UMaterialInstanceDynamic* Material,
		const FUOULightBeamVisualSegmentData& SegmentData,
		const FBoxSphereBounds& MeshBounds,
		const FVector& AppliedLayerScale,
		float FullLayerLength) const override;
};
