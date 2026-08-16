// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightBeamVisualInterface.h"
#include "UOULumenStaticRayVisualActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Unity Lumen Static Ray 프리셋 1~19를 원본 레이어 구성으로 표시합니다. */
UCLASS(Blueprintable, meta = (DisplayName = "UOU Lumen Static Ray Visual"))
class UNDERONEUMBRELLA_API AUOULumenStaticRayVisualActor
	: public AActor
	, public IUOULightBeamVisualInterface
{
	GENERATED_BODY()

public:
	AUOULumenStaticRayVisualActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ApplyLightBeamSegment_Implementation(const FUOULightBeamVisualSegmentData& SegmentData) override;
	virtual void SetLightBeamVisualActive_Implementation(bool bActive) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Preview", meta = (DisplayName = "에디터 프리뷰", ToolTip = "레벨에 직접 배치했을 때 지정한 길이와 굵기로 Static Ray를 미리 표시합니다."))
	bool bPreviewInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Preview", meta = (ClampMin = "1.0", Units = "cm", EditCondition = "bPreviewInEditor", DisplayName = "프리뷰 길이"))
	float PreviewLength = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Preview", meta = (ClampMin = "1.0", Units = "cm", EditCondition = "bPreviewInEditor", DisplayName = "프리뷰 반지름"))
	float PreviewRadius = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Components")
	TObjectPtr<USceneComponent> CameraFacingRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Preset", meta = (ClampMin = "1", ClampMax = "19", DisplayName = "Static Ray 프리셋"))
	int32 Preset = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Appearance", meta = (ClampMin = "0.0", DisplayName = "밝기 배율"))
	float EmissiveIntensityScale = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "투명도 배율"))
	float OpacityScale = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Appearance", meta = (ClampMin = "0.01", DisplayName = "광선 폭 배율", ToolTip = "계산된 원뿔 반지름에 적용되는 시각적 폭 배율입니다."))
	float BeamWidthScale = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Appearance", meta = (ClampMin = "0.0", Units = "cm", DisplayName = "반사 연결부 보강 길이", ToolTip = "반사면 직전의 메시 끝 페이드를 메우는 짧은 보강 레이어 길이입니다. 0이면 사용하지 않습니다."))
	float ReflectionJunctionFillLength = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "반사 연결부 보강 투명도", ToolTip = "반사면 직전 보강 레이어의 투명도 배율입니다."))
	float ReflectionJunctionFillOpacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Variation", meta = (DisplayName = "노이즈 변화 사용"))
	bool bUseVariation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Camera", meta = (DisplayName = "카메라 방향 보정", ToolTip = "광선 진행축은 유지하면서 메시 레이어가 카메라를 향하도록 회전합니다."))
	bool bFaceCameraAroundBeamAxis = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Fade", meta = (DisplayName = "카메라 거리 페이드 사용"))
	bool bUseCameraDistanceFade = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Fade", meta = (ClampMin = "0.0", Units = "cm", DisplayName = "카메라 페이드 시작 거리"))
	float CameraDistanceFadeStart = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Fade", meta = (ClampMin = "0.0", Units = "cm", DisplayName = "카메라 페이드 종료 거리"))
	float CameraDistanceFadeEnd = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Fade", meta = (DisplayName = "시선 각도 페이드 사용"))
	bool bUseAngleFade = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Fade", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "시선 각도 페이드 시작"))
	float AngleFadeStart = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lumen Static Ray|Material")
	TObjectPtr<UMaterialInterface> RayMaterial;

private:
	static constexpr int32 MaxLayerCount = 5;

	UPROPERTY(VisibleAnywhere, Category = "Lumen Static Ray|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> LayerComponents;

	UPROPERTY(VisibleAnywhere, Category = "Lumen Static Ray|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> JunctionFillLayerComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> JunctionFillDynamicMaterials;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> ShapeMeshes;

	TArray<float> LayerBaseOpacities;
	TArray<float> JunctionFillBaseOpacities;

	FLinearColor CurrentColor = FLinearColor::White;
	float CurrentIntensity = 1.0f;
	float CurrentOpacity = 1.0f;

	void ConfigureComponents();
	void EnsureDynamicMaterials();
	void ApplyPreset(const FUOULightBeamVisualSegmentData& SegmentData);
	void UpdateCameraFacing();
	void UpdateMaterialParameters(float TimeSeconds);
};
