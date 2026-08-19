// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugProvider.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "World/Wind/UOUWindTypes.h"
#include "UOUWindEmitterActor.generated.h"

class UArrowComponent;
class UBoxComponent;
class UNiagaraComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
struct FCollisionObjectQueryParams;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUOUWindPathChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUOUWindPhaseChangedSignature, bool, bIsBlowing);

// 선풍기처럼 현재 방향으로 바람을 방출하고 반사 표면에 따라 실시간 경로를 계산합니다.
UCLASS(meta=(DisplayName="UOU Wind Emitter"))
class UNDERONEUMBRELLA_API AUOUWindEmitterActor
	: public AActor
	, public IUOUPuzzleResultReceiver
	, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	AUOUWindEmitterActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostRegisterAllComponents() override;
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<UStaticMeshComponent> EmitterVisual = nullptr;

	// 이 화살표의 위치와 Forward가 바람의 시작점과 최초 방향이 됩니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<UArrowComponent> WindOrigin = nullptr;

	// 에디터에서 직선 바람의 최대 범위와 반경을 보여 주는 와이어 박스입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind|Preview")
	TObjectPtr<UBoxComponent> WindRangePreview = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Preview")
	bool bShowWindRangePreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay")
	bool bWindEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxWindDistance = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "1.0", Units = "cm"))
	float WindRadius = 120.0f;

	// Emitter가 바람 방향으로 직접 더하는 실제 가속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float WindAcceleration = 700.0f;

	// 수신체에 합산되는 바람 가속도 벡터의 최대 크기입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float MaximumWindAcceleration = 1200.0f;

	// 바람 방향으로 누적되는 캐릭터 속도의 최대값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MaximumWindSpeed = 1400.0f;

	// 바람에 처음 진입할 때 즉시 보장할 최소 바람 방향 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Entry", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MinimumWindEntrySpeed = 400.0f;

	// 진입 직전 낙하 속도 중 바람 방향 속도로 전환할 비율입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Entry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FallingMomentumConversion = 0.5f;

	// DeltaTime 기반 가속과 별개로 진입 순간 한 번만 더하는 고정 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Entry", meta = (ClampMin = "0.0", Units = "cm/s"))
	float InitialWindVelocityBoost = 150.0f;

	// 낙하 관성을 전환하더라도 진입 순간 이 속도를 넘지 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Entry", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MaximumWindEntrySpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay")
	bool bUseRadialFalloff = true;

	// 켜면 Wind On Duration과 Wind Off Duration을 번갈아 반복합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Pulse")
	bool bUsePulseCycle = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Pulse", meta = (EditCondition = "bUsePulseCycle", ClampMin = "0.05", Units = "s"))
	float WindOnDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Pulse", meta = (EditCondition = "bUsePulseCycle", ClampMin = "0.05", Units = "s"))
	float WindOffDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Pulse", meta = (EditCondition = "bUsePulseCycle"))
	bool bStartCycleWithWind = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Reflection", meta = (ClampMin = "0", ClampMax = "8"))
	int32 MaxReflections = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Reflection", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float MinimumReflectedStrength = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Collision")
	TEnumAsByte<ECollisionChannel> WindTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Collision")
	TArray<TEnumAsByte<EObjectTypeQuery>> ReceiverObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Debug")
	bool bDrawWindDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Debug", meta = (ClampMin = "0.0"))
	float DebugDrawTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	TArray<FUOUWindPathSegment> WindPathSegments;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	int32 LastAffectedReceiverCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	FUOUWindPulseRuntimeState PulseRuntimeState;

	// 이 액터에 추가된 Niagara 효과를 실제 바람 길이와 반경에 자동으로 맞춥니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX")
	bool bAutoFitWindVFX = true;

	// 켜면 Niagara 중심을 바람 범위 중앙에 맞추기 위해 X 위치를 MaxWindDistance의 절반으로 고정합니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Wind|VFX",
		meta = (
			DisplayName = "바람 거리 절반만큼 X축 이동",
			EditCondition = "bAutoFitWindVFX",
			ToolTip = "켜면 Niagara의 상대 X 위치를 MaxWindDistance의 절반으로 자동 설정합니다. 끄면 Niagara 컴포넌트의 X 위치를 직접 수정할 수 있습니다."))
	bool bOffsetWindVFXByHalfDistance = true;

	// 경로가 장애물에 막히면 MaxWindDistance 대신 실제 첫 번째 직선 구간 길이를 VFX에 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bAutoFitWindVFX"))
	bool bUseActualWindPathLengthForVFX = true;

	// 반사로 경로가 꺾이면 첫 번째 Niagara를 템플릿으로 사용해 나머지 직선 구간의 효과를 생성합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bAutoFitWindVFX && bUseActualWindPathLengthForVFX"))
	bool bCreateWindVFXForReflectedSegments = true;

	// 인접한 직선 VFX가 꺾이는 지점에서 끊겨 보이지 않도록 서로 겹치는 거리입니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Wind|VFX",
		meta = (EditCondition = "bAutoFitWindVFX && bCreateWindVFXForReflectedSegments", ClampMin = "0.0", Units = "cm"))
	float WindVFXJointOverlap = 75.0f;

	// 1보다 작게 설정하면 VFX가 판정 범위 양끝에서 조금 안쪽으로 들어옵니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Wind|VFX",
		meta = (EditCondition = "bAutoFitWindVFX", ClampMin = "0.1", ClampMax = "1.5", UIMin = "0.8", UIMax = "1.1"))
	float WindVFXLengthCoverage = 0.95f;

	// VisualWind 팩의 Length는 중심에서 한쪽 끝까지의 반길이이므로 전체 바람 거리에 곱할 비율입니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Wind|VFX",
		meta = (EditCondition = "bAutoFitWindVFX", ClampMin = "0.01", UIMin = "0.1", UIMax = "1.0"))
	float WindVFXLengthParameterRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX")
	bool bOverrideWindVFXColor = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Wind|VFX",
		meta = (EditCondition = "bOverrideWindVFXColor", DisplayName = "바람 VFX 색상"))
	FLinearColor WindVFXColor = FLinearColor(0.12f, 0.8f, 0.3f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bOverrideWindVFXColor"))
	FName WindVFXColorParameterName = TEXT("User.Color");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bAutoFitWindVFX"))
	FName WindVFXLengthParameterName = TEXT("User.Length");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bAutoFitWindVFX"))
	FName WindVFXWidthParameterName = TEXT("User.Width");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bAutoFitWindVFX"))
	FName WindVFXHeightParameterName = TEXT("User.Height");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bAutoFitWindVFX"))
	FName WindVFXMinRadiusParameterName = TEXT("User.MinRadius");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bAutoFitWindVFX"))
	FName WindVFXMaxRadiusParameterName = TEXT("User.MaxRadius");

	// 켜면 치수 User Parameter가 없는 VFX의 Scale을 Fixed Bounds 기준으로 자동 설정합니다.
	// 일반 레벨에서 Niagara 컴포넌트 Scale을 직접 조절할 수 있도록 기본값은 끕니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Wind|VFX",
		meta = (
			EditCondition = "bAutoFitWindVFX",
			DisplayName = "치수 없는 VFX 스케일 자동 보정",
			ToolTip = "켜면 Length/Width/Height 파라미터가 없는 축의 Scale을 자동으로 덮어씁니다. 직접 Scale을 조절하려면 끄세요."))
	bool bFitMissingWindVFXDimensionsFromBounds = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bAutoFitWindVFX", ClampMin = "1.0"))
	float MinimumWindVFXBoundsSize = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|VFX", meta = (EditCondition = "bAutoFitWindVFX", ClampMin = "1.0"))
	float MaximumWindVFXAutoScale = 100.0f;

	UPROPERTY(BlueprintAssignable, Category = "Wind|Events")
	FOnUOUWindPathChangedSignature OnWindPathChanged;

	UPROPERTY(BlueprintAssignable, Category = "Wind|Events")
	FOnUOUWindPhaseChangedSignature OnWindPhaseChanged;

	UFUNCTION(BlueprintCallable, Category = "Wind")
	void SetWindEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure, Category = "Wind")
	bool IsWindEnabled() const { return bWindEnabled; }

	UFUNCTION(BlueprintPure, Category = "Wind|Pulse")
	bool IsWindBlowing() const;

	UFUNCTION(BlueprintCallable, Category = "Wind|Pulse")
	void SetPulseCycleEnabled(bool bNewPulseCycleEnabled);

	UFUNCTION(BlueprintCallable, Category = "Wind|Pulse")
	void ResetPulseCycle();

	UFUNCTION(BlueprintCallable, Category = "Wind")
	void RebuildWindPath();

	// 에디터에서 버튼을 누른 순간에만 현재 직선/반사 경로와 실제 바람 범위를 다시 표시합니다.
	UFUNCTION(
		BlueprintCallable,
		CallInEditor,
		Category = "Wind|Preview",
		meta = (
			DisplayName = "Rebuild Wind Path Preview",
			ToolTip = "현재 발생기와 반사 표면 배치로 바람 경로를 한 번 계산하고 에디터 프리뷰를 다시 만듭니다. 실시간 자동 계산은 하지 않습니다."))
	void RebuildWindPathPreview();

	UFUNCTION(BlueprintPure, Category = "Wind")
	TArray<FUOUWindPathSegment> GetWindPathSegments() const { return WindPathSegments; }

private:
	void RebuildWindPathInternal(bool bIgnoreRuntimeWindState);
	void ValidateSettings();
	void UpdateWindRangePreview();
	void InitializePulseCycleState();
	void UpdatePulseCycle(float DeltaSeconds);
	void HandleWindPhaseChanged(bool bWasBlowing);
	float GetWindVFXDisplayDistance() const;
	void ApplyWindVFXParameters(UNiagaraComponent* WindEffect, float DisplayDistance);
	void RefreshWindVFX();
	void RefreshReflectedWindVFX(UNiagaraComponent* TemplateEffect);
	void ClearGeneratedWindVFX();
	void SetWindVFXActive(bool bActive);
	void RefreshWindPathForCurrentState();
	void ClearWindPath();
	void ApplyWindToReceivers(float DeltaSeconds);
	FUOUWindExposureData MakeExposureData(
		const FUOUWindPathSegment& Segment,
		const FVector& ClosestPoint,
		float FinalStrength,
		float DeltaSeconds);
	void DrawWindDebug() const;
	FCollisionObjectQueryParams BuildReceiverObjectQueryParams() const;
	void AppendWindReceivers(
		AActor* TargetActor,
		UPrimitiveComponent* TargetComponent,
		TArray<UObject*>& OutReceivers) const;

#if WITH_EDITOR
	void RebuildEditorWindPathPreviewComponents();
	void ClearEditorWindPathPreviewComponents();
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> EditorWindPathPreviewComponents;
#endif

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> GeneratedWindVFXComponents;
};
