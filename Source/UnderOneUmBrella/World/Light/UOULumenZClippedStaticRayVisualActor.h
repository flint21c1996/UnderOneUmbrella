// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/Light/UOULumenStaticRayVisualActor.h"
#include "UOULumenZClippedStaticRayVisualActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/** ReferenceLength로 메시를 유지하고 머티리얼 로컬 Z로 실제 가시 길이를 제한합니다. */
UCLASS(Blueprintable, meta = (DisplayName = "UOU Lumen Z-Clipped Static Ray Visual"))
class UNDERONEUMBRELLA_API AUOULumenZClippedStaticRayVisualActor
	: public AUOULumenStaticRayVisualActor
{
	GENERATED_BODY()

public:
	AUOULumenZClippedStaticRayVisualActor();
	virtual void ApplyLightBeamSegment_Implementation(
		const FUOULightBeamVisualSegmentData& SegmentData) override;
	virtual void SetLightBeamVisualActive_Implementation(bool bActive) override;
	void SetSparkleSystemOverride(bool bOverrideNiagaraSystem, UNiagaraSystem* NiagaraSystemOverride);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Sparkle", meta = (DisplayName = "Sparkle Effect", ToolTip = "빛줄 메시 안에서 재생할 반짝임 Niagara입니다. BP_ZClipRay에서 Niagara System을 지정합니다."))
	TObjectPtr<UNiagaraComponent> SparkleEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Sparkle", meta = (DisplayName = "반짝임 사용", ToolTip = "메시와 함께 Sparkle Effect를 표시합니다."))
	bool bEnableSparkleEffect = true;

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

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> DefaultSparkleSystem = nullptr;

	bool bHasCachedDefaultSparkleSystem = false;

	void ApplySparkleParameters(const FUOULightBeamVisualSegmentData& SegmentData);
};
