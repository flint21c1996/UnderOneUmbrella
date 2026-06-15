// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUUmbrellaRainArea.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class UStaticMeshComponent;
class UUOUEnvironmentVisualComponent;

// 이 클래스는 우산 플레이어가 들어가면 시간당 비 노출과 물 받기를 적용하는 테스트용 비 영역을 담당한다.
UCLASS(meta=(DisplayName="UOU Umbrella Rain Area"))
class AUOUUmbrellaRainArea : public AActor
{
	GENERATED_BODY()

public:
	AUOUUmbrellaRainArea();

	UFUNCTION(BlueprintCallable, Category = "Rain|Water Basin")
	void SetWaterBasinRainFillEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Rain|Water Basin")
	bool IsWaterBasinRainFillEnabled() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	TObjectPtr<UBoxComponent> RainVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	TObjectPtr<UStaticMeshComponent> PreviewVolumeMesh = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Rain|Internal")
	TObjectPtr<UUOUEnvironmentVisualComponent> RainVisual = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Rain|Internal")
	TObjectPtr<UNiagaraComponent> PrimaryRainEffect = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Rain|Internal")
	TObjectPtr<UNiagaraComponent> SecondaryRainEffect = nullptr;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Water Basin", meta = (ToolTip = "RainVolume 안의 WaterBasinTarget에 비 입력을 전달할지 여부입니다. 런타임에는 SetWaterBasinRainFillEnabled로 변경할 수 있습니다."))
	bool bEnableWaterBasinRainFill = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Rain System", DisplayPriority = "1", ToolTip = "비 내림 표현에 사용할 Niagara System입니다."))
	TObjectPtr<UNiagaraSystem> RainEffectSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Ground Splash System", DisplayPriority = "2", ToolTip = "바닥 물 튐 표현에 사용할 Niagara System입니다."))
	TObjectPtr<UNiagaraSystem> GroundSplashEffectSystem = nullptr;

	UPROPERTY()
	bool bHasExplicitRainEffectSystemSelection = false;

	UPROPERTY()
	bool bHasExplicitGroundSplashEffectSystemSelection = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Enable", DisplayPriority = "10", ToolTip = "이 RainArea의 비 내림 Niagara 표시 여부입니다. 게임플레이 RainFillRate와는 별도입니다."))
	// RainVisual 컴포넌트에도 비주얼 세팅을 함께 밀어넣을지 정한 값입니다.
	bool bEnableRainVisuals = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Intensity", ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableRainVisuals", DisplayPriority = "11", ToolTip = "비 내림 표현의 전체 강도입니다. 0이면 보이지 않고, 1이면 RainSpawnRate가 그대로 적용됩니다."))
	float RainVisualIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Rate", ClampMin = "0.0", UIMin = "0.0", UIMax = "6000.0", EditCondition = "bEnableRainVisuals", DisplayPriority = "12", ToolTip = "비 Niagara가 초당 생성할 기본 파티클 수입니다. 최종 Spawn Rate는 RainSpawnRate와 RainVisualIntensity를 곱한 값입니다."))
	float RainSpawnRate = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Speed", ClampMin = "-3000.0", ClampMax = "0.0", UIMin = "-3000.0", UIMax = "0.0", EditCondition = "bEnableRainVisuals", DisplayPriority = "13", ToolTip = "비 파티클의 Z 낙하 속도입니다. 아래로 떨어지는 값을 Niagara에 직접 전달하므로 보통 음수로 설정합니다."))
	float RainFallSpeed = -900.0f;

	UPROPERTY()
	float GroundSplashIntensityMultiplier = 0.45f;

	UPROPERTY()
	float GroundSplashHeightOffset = 5.0f;

	UPROPERTY()
	bool bDrawRainVisualDebug = true;

	UPROPERTY()
	float RainVisualDebugThickness = 2.0f;

	// 현재 프리뷰 메쉬의 표시 상태와 스케일을 설정값에 맞게 다시 맞춥니다.
	void ApplyPreviewSettings();
	// RainArea에서 선택한 Niagara System을 내부 RainVisual 컴포넌트에 전달합니다.
	void ApplyEnvironmentVisualEffectSystems();
	// RainVisual 컴포넌트에 강도와 표시 옵션 같은 공통 설정을 넘깁니다.
	void ApplyEnvironmentVisualSettings();
	// 비 영역 크기에 맞춰 RainVisual 컴포넌트의 배치 범위를 갱신합니다.
	void ApplyEnvironmentVisualGeometry();
	// 현재 비 영역이 켜져 있는지에 따라 RainVisual 컴포넌트의 활성 상태를 맞춥니다.
	void ApplyEnvironmentVisualState();
	// 우산이 비를 막을 때 RainVisual 컴포넌트에도 차단 위치와 강도를 전달합니다.
	void ApplyEnvironmentVisualRainBlocker(bool bIsBlocking, const FVector& BlockerWorldCenter, const FVector& BlockerHalfExtent, float BlockerIntensity);
	// RainVolume 안에 있는 WaterBasinTarget에 기존 물 입력 규칙으로 비를 전달합니다.
	void ApplyRainToWaterBasinTargets(float DeltaSeconds, bool bHasRainBlocker, const FVector& RainBlockerWorldCenter, const FRotator& RainBlockerWorldRotation, const FVector& RainBlockerHalfExtent) const;
	// 대상 Actor의 bounds가 RainVolume과 겹치는지 확인합니다.
	bool DoesActorBoundsOverlapRainVolume(const AActor* Actor) const;
	// 대상 Actor의 bounds가 우산 차단 영역 아래에 걸치는지 확인합니다.
	bool IsActorBlockedByRainBlocker(const AActor* Actor, const FVector& BlockerWorldCenter, const FRotator& BlockerWorldRotation, const FVector& BlockerHalfExtent) const;
	// 비주얼 디버그 박스를 그려서 환경 연동 범위를 확인합니다.
	void DrawRainVisualDebug() const;
};
