// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightBeamVisualInterface.h"
#include "UOULumenDynamicRayVisualActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable, meta = (DisplayName = "UOU Lumen Dynamic Ray Visual"))
class UNDERONEUMBRELLA_API AUOULumenDynamicRayVisualActor
	: public AActor
	, public IUOULightBeamVisualInterface
{
	GENERATED_BODY()

public:
	AUOULumenDynamicRayVisualActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void ApplyLightBeamSegment_Implementation(
		const FUOULightBeamVisualSegmentData& SegmentData) override;
	virtual void SetLightBeamVisualActive_Implementation(bool bActive) override;
	void CopyVisualWidthFrom(const AUOULumenDynamicRayVisualActor* SourceVisual);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen Dynamic Ray|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Dynamic Ray|Style", meta = (ClampMin = "1", ClampMax = "8", ToolTip = "원본 Unity Lumen Dynamic Ray 프리셋 번호입니다."))
	int32 Preset = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Dynamic Ray|Style", meta = (ClampMin = "0.0", ToolTip = "원본 프리셋 밝기에 추가로 적용되는 배율입니다."))
	float EmissiveIntensityScale = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Dynamic Ray|Style", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "원본 프리셋 투명도에 추가로 적용되는 값입니다."))
	float OpacityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Dynamic Ray|Material")
	TObjectPtr<UMaterialInterface> RayMaterial;

private:
	static constexpr int32 MaxLayerCount = 5;

	UPROPERTY(VisibleAnywhere, Category = "Lumen Dynamic Ray|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> LayerComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> ShapeMeshes;

	bool bHasAppliedVisualWidth = false;

	void ConfigureComponents();
	void EnsureDynamicMaterials();
	void ApplyPreset(const FUOULightBeamVisualSegmentData& SegmentData);
};
