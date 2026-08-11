// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightBeamVisualInterface.h"
#include "UOULightBeamMeshVisualActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * 계산된 직접광 및 반사광 구간을 원기둥 또는 원뿔 메시로 표시합니다.
 * UOULightBeamVisualComponent의 VFX Actor Class에 지정해서 사용합니다.
 */
UCLASS(
	Blueprintable,
	meta = (
		DisplayName = "UOU Light Beam Mesh Visual",
		ToolTip = "계산된 빛 경로를 반투명 원기둥 또는 원뿔 메시로 표시합니다."))
class UNDERONEUMBRELLA_API AUOULightBeamMeshVisualActor
	: public AActor
	, public IUOULightBeamVisualInterface
{
	GENERATED_BODY()

public:
	AUOULightBeamMeshVisualActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void ApplyLightBeamSegment_Implementation(
		const FUOULightBeamVisualSegmentData& SegmentData) override;

	virtual void SetLightBeamVisualActive_Implementation(bool bActive) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light Beam|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light Beam|Components")
	TObjectPtr<UStaticMeshComponent> BeamMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light Beam|Components")
	TObjectPtr<UStaticMeshComponent> CoreBeamMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Mesh", meta = (ToolTip = "반지름이 일정한 구간에 사용할 원기둥 메시입니다. 로컬 Z축이 길이 방향이어야 합니다."))
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Mesh", meta = (ToolTip = "폭이 커지는 구간에 사용할 원뿔 메시입니다. 로컬 Z축이 길이 방향이어야 합니다."))
	TObjectPtr<UStaticMesh> ConeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Mesh", meta = (ToolTip = "시작과 끝 반지름이 다를 때 원뿔 메시를 사용합니다."))
	bool bUseConeForExpandingSegments = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Mesh", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "시작과 끝 반지름 차이가 이 값 이하면 원기둥으로 처리합니다."))
	float CylinderRadiusTolerance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Mesh", meta = (ToolTip = "Engine 기본 Cone처럼 꼭짓점이 +Z에 있는 메시를 시작점이 꼭짓점이 되도록 뒤집습니다."))
	bool bReverseConeAxis = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Material", meta = (ToolTip = "빛줄기에 사용할 반투명 Emissive 머티리얼입니다."))
	TObjectPtr<UMaterialInterface> BeamMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Material", meta = (ClampMin = "0.0", ToolTip = "게임플레이 빛 세기를 머티리얼 Emissive 세기로 변환하는 배율입니다."))
	float EmissiveIntensityScale = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Material", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "빛줄기의 기본 불투명도입니다."))
	float Opacity = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Style", meta = (ClampMin = "0.05", ClampMax = "1.0", ToolTip = "외곽 빔 안에 겹치는 밝은 코어의 반지름 비율입니다."))
	float CoreRadiusScale = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Style", meta = (ClampMin = "0.0", ToolTip = "내부 코어의 Emissive 밝기 배율입니다."))
	float CoreIntensityMultiplier = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Style", meta = (ClampMin = "0.0", ToolTip = "내부 코어의 불투명도 배율입니다."))
	float CoreOpacityMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Variation", meta = (ToolTip = "Unity Dynamic Ray처럼 빛줄기 밝기가 천천히 변화하게 합니다."))
	bool bEnableVariation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Variation", meta = (ClampMin = "0.0", ToolTip = "밝기 변화 속도입니다. Unity Dynamic Ray 1의 기본값은 1입니다."))
	float VariationSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Variation", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "밝기 변화량입니다."))
	float VariationAmount = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Beam|Rendering", meta = (ToolTip = "반사 구간을 직접광보다 조금 앞에 렌더링하기 위한 Translucency Sort Priority입니다."))
	int32 BaseTranslucencySortPriority = 10;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicBeamMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicCoreMaterial;

	TWeakObjectPtr<UMaterialInterface> DynamicMaterialSource;
	FLinearColor CurrentBeamColor = FLinearColor::White;
	float CurrentEmissiveIntensity = 0.0f;

	void ConfigureMeshComponent(UStaticMeshComponent* MeshComponent) const;
	void EnsureDynamicMaterials();
	void ApplyMaterialParameters(const FUOULightBeamVisualSegmentData& SegmentData);
	void UpdateAnimatedMaterialParameters(float TimeSeconds);
	void ApplyMeshTransform(const FUOULightBeamVisualSegmentData& SegmentData, bool bUseCone);
};
