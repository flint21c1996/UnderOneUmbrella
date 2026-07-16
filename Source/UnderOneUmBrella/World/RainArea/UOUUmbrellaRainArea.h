// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUUmbrellaRainArea.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UNiagaraComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOUHeatWireComponent;
class UUOUAudioSubsystem;
class UUOUEnvironmentVisualComponent;
class UUOUWaterWheelRainConditionComponent;
struct FUOUWaterWheelRainCatchSample;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Rain|Internal", meta = (DisplayName = "Rain VFX Controller"))
	TObjectPtr<UUOUEnvironmentVisualComponent> RainVisual = nullptr;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Water Wheel", meta = (ToolTip = "RainVolume 안의 WaterWheelRainConditionComponent에 위치 기반 비 입력을 전달할지 여부입니다."))
	bool bEnableWaterWheelRainInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Heat Wire", meta = (ToolTip = "RainVolume 안의 UOU Heat Wire Wet Sections에 비 입력을 전달할지 여부입니다. 우산으로 막힌 구간은 젖지 않습니다."))
	bool bEnableHeatWireRainInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Heat Wire", meta = (ClampMin = "1", ToolTip = "Heat Wire Wet Section 하나를 비 판정할 때 스플라인을 따라 샘플링할 위치 수입니다. 값이 높을수록 얇은 비 영역을 놓칠 가능성이 줄어듭니다."))
	int32 HeatWireWetSectionPathSampleCount = 7;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Heat Wire|Runtime")
	bool bLastHeatWireRainInputTickRan = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Heat Wire|Runtime")
	int32 LastHeatWireActorScanCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Heat Wire|Runtime")
	int32 LastHeatWireComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Heat Wire|Runtime")
	int32 LastHeatWireValidComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Heat Wire|Runtime")
	int32 LastHeatWireWetSectionCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Heat Wire|Runtime")
	int32 LastHeatWireAcceptedSectionCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Heat Wire|Runtime")
	float LastHeatWireDeliveredWetness = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Heat Wire|Runtime")
	FString LastHeatWireRainDebugReason = TEXT("Not Run");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Water Wheel", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "물레방아 입력 계산에서 RainVolume 가장자리 샘플이 가지는 최소 강도입니다."))
	float WaterWheelRainEdgeStrength = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Water Wheel", meta = (ClampMin = "0.1", ToolTip = "물레방아 입력 계산에서 RainVolume 중심부 쪽으로 강해지는 곡선입니다."))
	float WaterWheelRainCenterFalloffExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Water Wheel", meta = (ToolTip = "켜면 Catch Point 중심점이 RainVolume 안에 있을 때만 물레방아 비 입력을 허용합니다. Coverage Radius는 중심점이 들어온 뒤 강도 계산에만 사용됩니다."))
	bool bRequireWaterWheelCatchPointCenterInsideRainVolume = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Water Wheel|Debug", meta = (ToolTip = "켜져 있으면 물레방아 비 입력 샘플 위치와 판정 결과를 월드에 표시합니다."))
	bool bDrawWaterWheelRainInputDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Water Wheel|Debug", meta = (ClampMin = "0.0", ToolTip = "물레방아 비 입력 디버그 표시 유지 시간입니다. 0이면 매 프레임 갱신됩니다."))
	float WaterWheelRainInputDebugLifeTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Water Wheel|Runtime")
	bool bLastWaterWheelRainInputTickRan = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Water Wheel|Runtime")
	int32 LastWaterWheelActorScanCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Water Wheel|Runtime")
	int32 LastWaterWheelComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Water Wheel|Runtime")
	int32 LastWaterWheelValidComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Water Wheel|Runtime")
	int32 LastWaterWheelCatchSampleCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Water Wheel|Runtime")
	int32 LastWaterWheelAcceptedSampleCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Water Wheel|Runtime")
	float LastWaterWheelDeliveredStrength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Water Wheel|Runtime")
	FVector LastWaterWheelSampleLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Rain|Water Wheel|Runtime")
	FString LastWaterWheelRainDebugReason = TEXT("Not Run");

	// 비가 아래로 떨어질지, 아래에서 위로 올라갈지 정하는 방향값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Flow Direction", DisplayPriority = "0", ToolTip = "Downward는 기존처럼 위에서 아래로 떨어지고, Upward는 같은 영역/Blocker 흐름을 사용하면서 아래에서 위로 뿜어집니다."))
	EUOURainAreaFlowDirection FlowDirection = EUOURainAreaFlowDirection::Downward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Enable", DisplayPriority = "10", ToolTip = "이 RainArea의 비 내림 Niagara 표시 여부입니다. 게임플레이 RainFillRate와는 별도입니다."))
	// RainVisual 컴포넌트에도 비주얼 세팅을 함께 밀어넣을지 정한 값입니다.
	bool bEnableRainVisuals = true;

	// 켜져 있으면 RainVisual 아래에 추가한 Niagara 컴포넌트를 모두 비 효과로 자동 등록합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Auto Collect Niagara Children", DisplayPriority = "10"))
	bool bAutoCollectNiagaraChildren = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Intensity", ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableRainVisuals", DisplayPriority = "11", ToolTip = "비 내림 표현의 전체 강도입니다. 0이면 보이지 않고, 1이면 RainSpawnRate가 그대로 적용됩니다."))
	float RainVisualIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Rate", ClampMin = "0.0", UIMin = "0.0", UIMax = "6000.0", EditCondition = "bEnableRainVisuals", DisplayPriority = "12", ToolTip = "비 Niagara가 초당 생성할 기본 파티클 수입니다. 최종 Spawn Rate는 RainSpawnRate와 RainVisualIntensity를 곱한 값입니다."))
	float RainSpawnRate = 2400.0f;

	// RainVolume의 XY 면적이 커질수록 SpawnRate도 같은 비율로 보정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Scale Rate By Area", DisplayPriority = "12", EditCondition = "bEnableRainVisuals"))
	bool bScaleRainSpawnRateByArea = true;

	// SpawnRate가 1배로 적용되는 기준 면적입니다. 기본값은 500 x 500 RainVolume 기준입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Falling", meta = (DisplayName = "Rate Reference Area", ClampMin = "1.0", UIMin = "1.0", DisplayPriority = "12", EditCondition = "bEnableRainVisuals && bScaleRainSpawnRateByArea"))
	float RainSpawnRateReferenceArea = 250000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Audio", meta = (DisplayName = "Enable Rain Audio", ToolTip = "RainArea가 활성 비 상태일 때 주변 빗소리 이벤트를 관리형 인스턴스로 유지합니다."))
	bool bEnableRainAudio = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Audio", meta = (DisplayName = "Rain Audio Event Id", EditCondition = "bEnableRainAudio", ToolTip = "RainArea 주변에서 들릴 빗소리 AudioData 이벤트 ID입니다. 별도 ambience 이벤트를 지정하는 것을 권장합니다."))
	FName RainAudioEventId = TEXT("Rain.Ambience");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Audio", meta = (DisplayName = "Rain Audio Refresh Interval", ClampMin = "0.02", EditCondition = "bEnableRainAudio", ToolTip = "관리형 오디오 인스턴스 위치와 재생 상태를 갱신하는 간격입니다."))
	float RainAudioRefreshInterval = 0.1f;

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
	// 매 프레임 월드 전체를 탐색하지 않도록 고정 배치된 비 반응 대상을 미리 수집합니다.
	void RefreshRainTargetCache();
	// RainVisual 아래에 배치한 Niagara 컴포넌트들을 수집해 n개짜리 비 효과로 등록합니다.
	void RefreshRainVisualEffectComponents();
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
	// RainVolume 안의 물레방아 Catch Point에 위치 기반 비 입력을 전달합니다.
	void ApplyRainToWaterWheelTargets(float DeltaSeconds, bool bHasRainBlocker, const FVector& RainBlockerWorldCenter, const FRotator& RainBlockerWorldRotation, const FVector& RainBlockerHalfExtent);
	void ApplyRainToHeatWireTargets(float DeltaSeconds, bool bHasRainBlocker, const FVector& RainBlockerWorldCenter, const FRotator& RainBlockerWorldRotation, const FVector& RainBlockerHalfExtent);
	float CalculateHeatWireWetSectionRainScale(const UUOUHeatWireComponent* HeatWire, int32 SectionIndex, bool bHasRainBlocker, const FVector& RainBlockerWorldCenter, const FRotator& RainBlockerWorldRotation, const FVector& RainBlockerHalfExtent) const;
	float CalculateWaterWheelCatchRainScale(const FUOUWaterWheelRainCatchSample& CatchSample, bool bHasRainBlocker, const FVector& RainBlockerWorldCenter, const FRotator& RainBlockerWorldRotation, const FVector& RainBlockerHalfExtent) const;
	float CalculateRainVolumeCenterStrength(const FVector& WorldLocation) const;
	// 대상 Actor의 bounds가 RainVolume과 겹치는지 확인합니다.
	bool DoesActorBoundsOverlapRainVolume(const AActor* Actor) const;
	// 월드 위치가 RainVolume 안에 있는지 확인합니다.
	bool IsWorldLocationInsideRainVolume(const FVector& WorldLocation) const;
	// 대상 Actor의 bounds가 우산 차단 영역 아래에 걸치는지 확인합니다.
	bool IsActorBlockedByRainBlocker(const AActor* Actor, const FVector& BlockerWorldCenter, const FRotator& BlockerWorldRotation, const FVector& BlockerHalfExtent) const;
	// 월드 위치가 우산 비 차단 영역에 가려지는지 확인합니다.
	bool IsWorldLocationBlockedByRainBlocker(const FVector& WorldLocation, const FVector& BlockerWorldCenter, const FRotator& BlockerWorldRotation, const FVector& BlockerHalfExtent) const;
	// 비주얼 디버그 박스를 그려서 환경 연동 범위를 확인합니다.
	void DrawRainVisualDebug() const;
	float GetAreaScaledRainSpawnRate() const;
	bool ShouldRainAudioBePlaying() const;
	FVector GetRainAudioLocation() const;
	FName BuildRainAudioInstanceId() const;
	UUOUAudioSubsystem* GetAudioSubsystem() const;
	void UpdateRainAudio();
	void StopRainAudio(float OverrideFadeOutTime = -1.0f);

	UPROPERTY(Transient)
	bool bRainAudioPlaying = false;

	UPROPERTY(Transient)
	float LastRainAudioRefreshTime = -1000.0f;

	UPROPERTY(Transient)
	FName ActiveRainAudioEventId = NAME_None;

	// 대상은 다른 액터가 소유하므로 수명에 영향을 주지 않는 약한 참조로 보관합니다.
	TArray<TWeakObjectPtr<UUOUWaterWheelRainConditionComponent>> CachedWaterWheelTargets;
	TArray<TWeakObjectPtr<UUOUHeatWireComponent>> CachedHeatWireTargets;
	int32 CachedRainTargetActorScanCount = 0;
};
