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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain|Visual")
	TObjectPtr<UNiagaraComponent> RainEffect = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain|Visual")
	TObjectPtr<UNiagaraComponent> GroundSplashEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual")
	TObjectPtr<AUOUEnvironmentVisualActor> EnvironmentVisual = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float RainFillRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual")
	bool bEnableRainVisuals = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual")
	TObjectPtr<UNiagaraSystem> RainSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual")
	TObjectPtr<UNiagaraSystem> GroundSplashSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RainVisualIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float GroundSplashIntensityMultiplier = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual", meta = (ClampMin = "0.0"))
	float RainEmitterTopPadding = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual", meta = (ClampMin = "0.0"))
	float GroundSplashHeightOffset = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual|Parameters")
	FName RainIntensityParameterName = TEXT("User.RainIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual|Parameters")
	FName RainAreaSizeParameterName = TEXT("User.RainAreaSize");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual|Parameters")
	FName GroundSplashIntensityParameterName = TEXT("User.GroundSplashIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Visual|Parameters")
	FName GroundSplashAreaSizeParameterName = TEXT("User.GroundSplashAreaSize");

	void ApplyPreviewSettings();
	void ApplyVisualEffectSettings();
	void ApplyVisualEffectTransforms();
	void ApplyNiagaraParameters();
	void RefreshNiagaraActivation();
	void ApplyEnvironmentVisualGeometry();
	void ApplyEnvironmentVisualState();
};
