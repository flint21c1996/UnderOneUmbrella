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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen Dynamic Ray|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Lumen Dynamic Ray|Renderer Defaults", meta = (ClampMin = "1", ClampMax = "8", DisplayName = "기본 Dynamic Ray 프리셋", ToolTip = "Beam Visual 컴포넌트의 프리셋 Override가 0일 때 사용할 렌더러 기본값입니다."))
	int32 Preset = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Lumen Dynamic Ray|Renderer Defaults", meta = (ClampMin = "0.0", DisplayName = "렌더러 기본 밝기 배율", ToolTip = "원본 프리셋 밝기와 Beam Visual의 개별 밝기 배율에 추가로 곱하는 렌더러 기본값입니다."))
	float EmissiveIntensityScale = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Lumen Dynamic Ray|Renderer Defaults", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "렌더러 기본 투명도 배율", ToolTip = "Beam Visual의 개별 투명도 배율에 추가로 곱하는 렌더러 기본값입니다."))
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

	void ConfigureComponents();
	void EnsureDynamicMaterials();
	void ApplyPreset(const FUOULightBeamVisualSegmentData& SegmentData);
};
