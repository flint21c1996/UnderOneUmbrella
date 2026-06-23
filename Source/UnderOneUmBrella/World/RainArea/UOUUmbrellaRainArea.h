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

UENUM(BlueprintType)
enum class EUOURainAreaFlowDirection : uint8
{
	Downward UMETA(DisplayName = "Downward"),
	Upward UMETA(DisplayName = "Upward")
};

// 이 클래스는 맵에 배치되는 비 영역 액터입니다.
// RainVolume 안의 플레이어 우산 상태를 읽어 비 노출, 우산 차단, 물받이 입력, Niagara 비주얼을 함께 갱신합니다.
// 실제 파티클 제어는 UOUEnvironmentVisualComponent에 위임하고, 이 클래스는 영역 판정과 게임플레이 연결을 담당합니다.
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

	// 액터 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 플레이어, 우산, 물받이 대상이 비 영역 안에 있는지 판단하는 박스 범위입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	TObjectPtr<UBoxComponent> RainVolume = nullptr;

	// 에디터와 게임에서 비가 내리는 영역을 눈으로 확인하기 위한 프리뷰 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	TObjectPtr<UStaticMeshComponent> PreviewVolumeMesh = nullptr;

	// Niagara 파라미터와 비 차단 데이터를 실제 이펙트 컴포넌트에 전달하는 내부 비주얼 컴포넌트입니다.
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Rain|Internal")
	TObjectPtr<UUOUEnvironmentVisualComponent> RainVisual = nullptr;

	// 주 비 파티클입니다. 일반적으로 빗줄기 Niagara를 연결합니다.
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Rain|Internal")
	TObjectPtr<UNiagaraComponent> PrimaryRainEffect = nullptr;

	// 보조 비 파티클입니다. 일반적으로 바닥 물튀김 Niagara를 연결합니다.
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Rain|Internal")
	TObjectPtr<UNiagaraComponent> SecondaryRainEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	// 에디터 안에서 프리뷰 메쉬를 보여줄지 정한 값입니다.
	bool bShowEditorPreview = true;

	// 플레이 중에도 프리뷰 메쉬를 보여줄지 정한 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	bool bShowPreviewInGame = true;

	// RainVolume 크기에 맞춘 뒤 추가로 곱해줄 프리뷰 스케일 보정값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	FVector PreviewScaleMultiplier = FVector(1.0f, 1.0f, 1.0f);

	// 켜져 있으면 프리뷰 메쉬가 RainVolume 크기를 자동으로 따라갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	bool bAutoFitPreviewScaleToRainVolume = true;

	// 자동 스케일을 끈 경우 직접 사용할 프리뷰 메쉬 상대 스케일입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview", meta = (EditCondition = "!bAutoFitPreviewScaleToRainVolume"))
	FVector ManualPreviewRelativeScale = FVector::OneVector;

	// 프리뷰 메쉬에 입힐 머티리얼입니다. 비 영역을 반투명 박스로 보여줄 때 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	TObjectPtr<UMaterialInterface> PreviewMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Gameplay", meta = (ClampMin = "0.0", ToolTip = "플레이어가 RainArea 안에 있을 때 물이 차오르는 게임플레이 속도입니다. 비주얼 양과는 별도로 사용됩니다."))
	// 비 영역 안에서 물이 차는 속도를 정한 값입니다.
	float RainFillRate = 1.0f;

	// 켜져 있으면 RainVolume 안의 WaterBasinTarget에 비 입력을 지속적으로 전달합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Water Basin", meta = (ToolTip = "RainVolume 안의 WaterBasinTarget에 비 입력을 전달할지 여부입니다. 런타임에는 SetWaterBasinRainFillEnabled로 변경할 수 있습니다."))
	bool bEnableWaterBasinRainFill = false;

	// 비가 아래로 떨어질지, 아래에서 위로 올라갈지 정하는 방향값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Flow Direction", DisplayPriority = "0", ToolTip = "Downward는 기존처럼 위에서 아래로 떨어지고, Upward는 같은 영역/Blocker 흐름을 사용하면서 아래에서 위로 뿜어집니다."))
	EUOURainAreaFlowDirection FlowDirection = EUOURainAreaFlowDirection::Downward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Rain System", DisplayPriority = "1", ToolTip = "비 내림 표현에 사용할 Niagara System입니다."))
	TObjectPtr<UNiagaraSystem> RainEffectSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Ground Splash System", DisplayPriority = "2", ToolTip = "바닥 물 튐 표현에 사용할 Niagara System입니다."))
	TObjectPtr<UNiagaraSystem> GroundSplashEffectSystem = nullptr;

	// 에디터에서 RainEffectSystem을 직접 골랐는지 기록해서, Construction 중 자동 덮어쓰기를 막습니다.
	UPROPERTY()
	bool bHasExplicitRainEffectSystemSelection = false;

	// 에디터에서 GroundSplashEffectSystem을 직접 골랐는지 기록해서, Construction 중 자동 덮어쓰기를 막습니다.
	UPROPERTY()
	bool bHasExplicitGroundSplashEffectSystemSelection = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Enable", DisplayPriority = "10", ToolTip = "이 RainArea의 비 내림 Niagara 표시 여부입니다. 게임플레이 RainFillRate와는 별도입니다."))
	// RainVisual 컴포넌트에도 비주얼 세팅을 함께 밀어넣을지 정한 값입니다.
	bool bEnableRainVisuals = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Intensity", ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableRainVisuals", DisplayPriority = "11", ToolTip = "비 내림 표현의 전체 강도입니다. 0이면 보이지 않고, 1이면 RainSpawnRate가 그대로 적용됩니다."))
	float RainVisualIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Rate", ClampMin = "0.0", UIMin = "0.0", UIMax = "6000.0", EditCondition = "bEnableRainVisuals", DisplayPriority = "12", ToolTip = "비 Niagara가 초당 생성할 기본 파티클 수입니다. 최종 Spawn Rate는 RainSpawnRate와 RainVisualIntensity를 곱한 값입니다."))
	float RainSpawnRate = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Speed", ClampMin = "-3000.0", ClampMax = "3000.0", UIMin = "-3000.0", UIMax = "3000.0", EditCondition = "bEnableRainVisuals", DisplayPriority = "13", ToolTip = "Niagara에 전달할 수직 속도입니다. Downward는 음수로, Upward는 양수로 자동 보정됩니다."))
	float RainFallSpeed = -900.0f;

	// 바닥 물튀김 파티클 강도를 주 비 강도에 비례해서 줄이기 위한 내부 보정값입니다.
	UPROPERTY()
	float GroundSplashIntensityMultiplier = 0.45f;

	// 바닥 물튀김 위치가 RainVolume 바닥면과 겹치지 않도록 살짝 띄우는 내부 보정값입니다.
	UPROPERTY()
	float GroundSplashHeightOffset = 5.0f;

	// VFX 디버그가 켜졌을 때 RainVolume, 스폰면, 소멸 범위를 월드에 그릴지 정합니다.
	UPROPERTY()
	bool bDrawRainVisualDebug = true;

	// VFX 디버그 선 두께입니다.
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
