// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineMeshComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "GameFramework/Actor.h"
#include "Interaction/UOUInteractable.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUHeatWireActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class USplineComponent;
class UStaticMesh;
class UUOUHeatWireComponent;
class UUOULightExposureReceiverComponent;

// 레벨에 바로 배치할 수 있는 기본 열선 퍼즐 액터입니다.
UCLASS(meta=(DisplayName="UOU Heat Wire Actor"))
class UNDERONEUMBRELLA_API AUOUHeatWireActor
	: public AActor
	, public IUOUPuzzleResultReceiver
	, public IUOUInteractable
	, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	AUOUHeatWireActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual bool IsDebugProviderEnabled_Implementation() const override;
	virtual FText GetDebugDisplayName_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual FVector GetDebugWorldLocation_Implementation() const override;
	virtual void GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const override;

#if UOU_WITH_DEVELOPMENT_TOOLS
	virtual void GatherDevelopmentDebugDraw(IUOUDevelopmentDebugDrawContext& Context) const override;
#endif

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Heat Wire", meta = (ToolTip = "Heat Wire Path를 따라 열선 Spline Mesh 조각을 다시 생성합니다."))
	void RebuildHeatWireVisualSegments();

	USplineComponent* GetHeatWirePathComponent() const { return HeatWirePath; }
	UUOUHeatWireComponent* GetHeatWireComponent() const { return HeatWireComponent; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> HeatWirePath = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUOULightExposureReceiverComponent> LightReceiverComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUOUHeatWireComponent> HeatWireComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (ToolTip = "Heat Wire Path를 따라 반복 배치할 짧은 열선 메시입니다. 최종 에셋은 X축 또는 Z축 중 Spline Mesh Forward Axis에 맞춰 길게 제작하면 됩니다."))
	TObjectPtr<UStaticMesh> HeatWireSegmentMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (ToolTip = "Heat Wire Segment Mesh가 비어 있을 때 엔진 기본 Cylinder 메시를 임시 열선으로 사용합니다."))
	bool bUseDefaultCylinderSegmentMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (ClampMin = "1.0", ToolTip = "열선을 몇 cm 단위 조각으로 나눠 Spline Mesh를 만들지 정합니다. 값이 작을수록 곡선이 부드럽지만 컴포넌트 수가 늘어납니다."))
	float VisualSegmentLength = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (ClampMin = "1", ToolTip = "비정상적으로 긴 Spline에서 생성할 수 있는 최대 열선 조각 수입니다."))
	int32 MaxVisualSegmentCount = 128;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (ToolTip = "열선 메시의 길이 방향 축입니다. 엔진 기본 Cylinder는 Z축, 별도 제작한 막대형 열선은 보통 X축을 사용합니다."))
	TEnumAsByte<ESplineMeshAxis::Type> SplineMeshForwardAxis = ESplineMeshAxis::Z;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (ClampMin = "0.001", ToolTip = "Spline Mesh 시작/끝에 적용할 단면 스케일입니다. 엔진 기본 Cylinder 기준 0.08 정도가 얇은 열선에 가깝습니다."))
	FVector2D VisualSegmentScale = FVector2D(0.08f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (ToolTip = "열선 메시를 Spline 축 기준으로 회전시킬 각도입니다."))
	float VisualSegmentRollDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (ToolTip = "켜져 있으면 생성된 열선 조각이 Visibility trace를 막습니다."))
	bool bEnableVisualCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (ToolTip = "생성된 열선 조각이 그림자를 만들지 정합니다."))
	bool bCastVisualShadow = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (DisplayName = "Cold Wire Material", ToolTip = "아직 가열되지 않은 열선에 사용할 재질입니다. 비워두면 메시 기본 재질을 사용합니다."))
	TObjectPtr<UMaterialInterface> DryHeatWireMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (DisplayName = "Wet Wire Material", ToolTip = "젖어서 전류/열 진행을 막는 열선 조각에 사용할 재질입니다. 비워두면 기존 재질에 Wetness 파라미터만 전달합니다."))
	TObjectPtr<UMaterialInterface> WetHeatWireMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (DisplayName = "Heated Wire Material", ToolTip = "이미 열이 지나간 열선 조각에 사용할 재질입니다. 비워두면 기존 재질에 BurnRatio 파라미터만 전달합니다."))
	TObjectPtr<UMaterialInterface> BurnedHeatWireMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (DisplayName = "Active Heat Material", ToolTip = "현재 열이 이동 중인 조각에 사용할 재질입니다. 나이아가라 없이 가열 지점을 보여줄 때 사용합니다."))
	TObjectPtr<UMaterialInterface> ActiveHeatMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (DisplayName = "Heat Progress Parameter Name", AdvancedDisplay, ToolTip = "전체 열 진행률을 전달할 Scalar 파라미터 이름입니다."))
	FName BurnProgressParameterName = TEXT("BurnProgress");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (DisplayName = "Segment Heat Ratio Parameter Name", AdvancedDisplay, ToolTip = "각 조각 내부에서 열이 얼마나 지나갔는지 전달할 Scalar 파라미터 이름입니다."))
	FName SegmentBurnRatioParameterName = TEXT("BurnRatio");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (AdvancedDisplay, ToolTip = "각 조각의 젖음 정도를 전달할 Scalar 파라미터 이름입니다."))
	FName WetnessParameterName = TEXT("Wetness");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (AdvancedDisplay, ToolTip = "각 조각이 현재 열 진행을 막는 젖은 구간이면 1, 아니면 0을 전달할 Scalar 파라미터 이름입니다."))
	FName BlockedParameterName = TEXT("Blocked");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (AdvancedDisplay, ToolTip = "각 조각이 현재 열 진행 지점이면 1, 아니면 0을 전달할 Scalar 파라미터 이름입니다."))
	FName ActiveHeatParameterName = TEXT("ActiveHeat");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (AdvancedDisplay, ToolTip = "각 조각의 시작 진행률을 전달할 Scalar 파라미터 이름입니다."))
	FName SegmentStartProgressParameterName = TEXT("SegmentStartProgress");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat Wire", meta = (AdvancedDisplay, ToolTip = "각 조각의 끝 진행률을 전달할 Scalar 파라미터 이름입니다."))
	FName SegmentEndProgressParameterName = TEXT("SegmentEndProgress");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Wire Debug", meta = (ToolTip = "켜져 있으면 Puzzle 디버그 모드에서 열선 상태와 남은 시간을 표시합니다."))
	bool bRegisterDebugProvider = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Wire Debug", meta = (ToolTip = "열선 디버그 월드 라벨을 액터 위치에서 얼마나 띄울지 정합니다."))
	FVector DebugWorldLocationOffset = FVector(0.0f, 0.0f, 120.0f);

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> HeatWireVisualSegments;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> HeatWireVisualSegmentMaterialInstances;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> HeatWireVisualSegmentMaterialSources;

	UPROPERTY(Transient)
	TArray<float> HeatWireVisualSegmentStartProgresses;

	UPROPERTY(Transient)
	TArray<float> HeatWireVisualSegmentEndProgresses;

	UPROPERTY(Transient)
	bool bHeatWireVisualEventsBound = false;

	void ValidateVisualSettings();
	void ClearHeatWireVisualSegments();
	UStaticMesh* ResolveHeatWireSegmentMesh();
	void RefreshHeatWireVisualState();
	float GetSegmentWetnessAlpha(float SegmentStartProgress, float SegmentEndProgress, bool& bOutBlocked) const;
	UMaterialInterface* ResolveSegmentMaterial(float SegmentBurnRatio, float SegmentWetnessAlpha, bool bActiveHeat) const;
	void ApplySegmentMaterialState(int32 SegmentIndex, float SegmentBurnRatio, float SegmentWetnessAlpha, bool bBlocked, bool bActiveHeat);
	void BindHeatWireVisualEvents();
	void UnbindHeatWireVisualEvents();

	UFUNCTION()
	void HandleHeatWireProgressChanged(float NewProgress, float RemainingTime);

	UFUNCTION()
	void HandleWetSectionChanged(int32 SectionIndex, float NewWetness);

	UFUNCTION()
	void HandleBlockedSectionChanged(int32 BlockedSectionIndex);

	UFUNCTION()
	void HandleHeatWireSimpleVisualChanged();

	UFUNCTION()
	void HandleHeatWireIgnited(AActor* Igniter);
};

