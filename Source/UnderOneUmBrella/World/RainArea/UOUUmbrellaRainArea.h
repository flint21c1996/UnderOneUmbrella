// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUUmbrellaRainArea.generated.h"

class UBoxComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
class AUOUEnvironmentVisualActor;

// 이 클래스는 우산 플레이어가 들어가면 시간당 비 노출과 물 받기를 적용하는 테스트용 비 영역을 담당한다.
UCLASS(meta=(DisplayName="UOU Umbrella Rain Area"))
class AUOUUmbrellaRainArea : public AActor
{
	GENERATED_BODY()

public:
	AUOUUmbrellaRainArea();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	TObjectPtr<UBoxComponent> RainVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	TObjectPtr<UStaticMeshComponent> PreviewVolumeMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|References", meta = (ToolTip = "이 RainArea와 연동되는 EnvironmentVisualActor입니다. 비 Niagara 재생과 파라미터 전달을 담당합니다."))
	// 비 영역과 연동되는 환경 비주얼 액터를 연결하는 참조값입니다.
	TObjectPtr<AUOUEnvironmentVisualActor> EnvironmentVisual = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	// 에디터 안에서 프리뷰 메쉬를 보여줄지 정한 값입니다.
	bool bShowEditorPreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	bool bShowPreviewInGame = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	FVector PreviewScaleMultiplier = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	bool bAutoFitPreviewScaleToRainVolume = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview", meta = (EditCondition = "!bAutoFitPreviewScaleToRainVolume"))
	FVector ManualPreviewRelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	TObjectPtr<UMaterialInterface> PreviewMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Gameplay", meta = (ClampMin = "0.0", ToolTip = "플레이어가 RainArea 안에 있을 때 물이 차오르는 게임플레이 속도입니다. 비주얼 양과는 별도로 사용됩니다."))
	// 비 영역 안에서 물이 차는 속도를 정한 값입니다.
	float RainFillRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayPriority = "1", ToolTip = "이 RainArea의 비 내림 Niagara 표시 여부입니다. 게임플레이 RainFillRate와는 별도입니다."))
	// 환경 비주얼 액터에도 비주얼 세팅을 함께 밀어넣을지 정한 값입니다.
	bool bEnableRainVisuals = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayPriority = "2", ToolTip = "비 내림 표현의 전체 강도입니다. 0이면 보이지 않고, 1이면 RainSpawnRate가 그대로 적용됩니다."))
	float RainVisualIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "6000.0", DisplayPriority = "3", ToolTip = "비 Niagara가 초당 생성할 기본 파티클 수입니다. 최종 Spawn Rate는 RainSpawnRate와 RainVisualIntensity를 곱한 값입니다."))
	float RainSpawnRate = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Ground Splash", meta = (ClampMin = "0.0", ClampMax = "2.0", ToolTip = "바닥 물 튐 표현의 강도 배율입니다. RainVisualIntensity와 곱해져 GroundSplashIntensity로 전달됩니다."))
	float GroundSplashIntensityMultiplier = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Ground Splash", meta = (ClampMin = "0.0", ToolTip = "RainVolume 바닥에서 바닥 물 튐 Niagara를 얼마나 위로 띄울지 정합니다."))
	float GroundSplashHeightOffset = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Umbrella Block", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "300.0", ToolTip = "빠른 비 파티클이 우산 차단 영역을 지나치지 않도록 Niagara kill 반지름에 더하는 여유 값입니다."))
	float RainBlockerKillRadiusPadding = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Debug", meta = (ToolTip = "RainArea가 비주얼에 전달하는 영역 계산값을 월드에 표시합니다."))
	bool bDrawRainVisualDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Debug", meta = (ClampMin = "0.0", ToolTip = "RainArea 비주얼 디버그 박스와 선의 두께입니다."))
	float RainVisualDebugThickness = 2.0f;

	// 현재 프리뷰 메쉬의 표시 상태와 스케일을 설정값에 맞게 다시 맞춥니다.
	void ApplyPreviewSettings();
	// 연결된 환경 비주얼 액터를 자동으로 찾거나 다시 연결합니다.
	void ResolveEnvironmentVisual();
	// 환경 비주얼 액터에 강도와 표시 옵션 같은 공통 설정을 넘깁니다.
	void ApplyEnvironmentVisualSettings();
	// 비 영역 크기에 맞춰 환경 비주얼의 배치 범위를 갱신합니다.
	void ApplyEnvironmentVisualGeometry();
	// 현재 비 영역이 켜져 있는지에 따라 환경 비주얼의 활성 상태를 맞춥니다.
	void ApplyEnvironmentVisualState();
	// 우산이 비를 막을 때 환경 비주얼에도 차단 위치와 강도를 전달합니다.
	void ApplyEnvironmentVisualRainBlocker(bool bIsBlocking, const FVector& BlockerWorldLocation, float BlockerRadius, float BlockerIntensity);
	// 비주얼 디버그 박스를 그려서 환경 연동 범위를 확인합니다.
	void DrawRainVisualDebug() const;
};
