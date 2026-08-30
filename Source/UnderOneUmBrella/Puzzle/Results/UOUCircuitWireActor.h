// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUCircuitWireActor.generated.h"

class UMaterialInterface;
class USceneComponent;
class USplineComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EUOUCircuitWireTransitionMode : uint8
{
	Instant UMETA(DisplayName = "즉시 전환", ToolTip = "전원 상태가 바뀌는 순간 전선 전체 재질을 교체합니다."),
	Progressive UMETA(DisplayName = "진행형 전환", ToolTip = "스플라인 시작점부터 끝점까지 전원이 이동하듯 재질을 순서대로 교체합니다.")
};

// 퍼즐 Result를 받아 스플라인 전선을 즉시 또는 진행형으로 점등하는 액터입니다.
UCLASS(meta=(DisplayName="UOU Circuit Wire Actor"))
class UNDERONEUMBRELLA_API AUOUCircuitWireActor
	: public AActor
	, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUCircuitWireActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Circuit Wire", meta = (DisplayName = "전선 비주얼 다시 생성"))
	void RebuildWireVisualSegments();

	UFUNCTION(BlueprintCallable, Category = "Circuit Wire")
	void RefreshCircuitState();

	UFUNCTION(BlueprintPure, Category = "Circuit Wire")
	bool IsCircuitPowered() const { return bIsPowered; }

	USplineComponent* GetCircuitPathComponent() const { return CircuitPath; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> CircuitPath = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "전선 조각 메시", ToolTip = "스플라인을 따라 반복 배치할 짧은 전선 메시입니다. 비어 있으면 엔진 기본 Cylinder를 사용합니다."))
	TObjectPtr<UStaticMesh> WireSegmentMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "비활성 재질", ToolTip = "하나 이상의 필수 접점이 꺼져 있을 때 전선 전체에 적용할 검은색 계열 재질입니다."))
	TObjectPtr<UMaterialInterface> UnpoweredMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "활성 재질", ToolTip = "모든 필수 접점이 켜졌을 때 전선 전체에 즉시 적용할 노란색 계열 재질입니다."))
	TObjectPtr<UMaterialInterface> PoweredMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "조각 길이", ClampMin = "1.0", ToolTip = "전선을 몇 cm 단위로 나눠 Spline Mesh를 생성할지 정합니다."))
	float VisualSegmentLength = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "최대 조각 수", ClampMin = "1"))
	int32 MaxVisualSegmentCount = 256;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "메시 길이 축", ToolTip = "엔진 기본 Cylinder는 Z축, 별도 제작한 전선 메시는 보통 X축을 사용합니다."))
	TEnumAsByte<ESplineMeshAxis::Type> SplineMeshForwardAxis = ESplineMeshAxis::Z;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "전선 굵기", ClampMin = "0.001"))
	FVector2D VisualSegmentScale = FVector2D(0.06f, 0.06f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "전선 롤 각도"))
	float VisualSegmentRollDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "비주얼 콜리전 사용"))
	bool bEnableVisualCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Visual", meta = (DisplayName = "그림자 생성"))
	bool bCastVisualShadow = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Transition", meta = (DisplayName = "전환 방식", ToolTip = "전선 전체가 즉시 바뀔지, 시작점부터 순서대로 바뀔지 정합니다."))
	EUOUCircuitWireTransitionMode TransitionMode = EUOUCircuitWireTransitionMode::Instant;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Transition", meta = (DisplayName = "전원 이동 시간", ClampMin = "0.01", EditCondition = "TransitionMode == EUOUCircuitWireTransitionMode::Progressive", EditConditionHides, ToolTip = "진행형 전환에서 전원이 스플라인 전체를 통과하는 데 걸리는 시간입니다."))
	float PowerTravelDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit Wire|Logic", meta = (DisplayName = "시작 시 활성화", ToolTip = "켜져 있으면 Result를 받기 전부터 전선을 활성 상태로 표시합니다."))
	bool bStartPowered = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Circuit Wire|Runtime", meta = (DisplayName = "현재 전원 상태"))
	bool bIsPowered = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Circuit Wire|Runtime", meta = (DisplayName = "시각 전원 진행률"))
	float VisualPowerProgress = 0.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> WireVisualSegments;

private:
	void ClearWireVisualSegments();
	void ApplyWireMaterial();
	UStaticMesh* ResolveWireSegmentMesh() const;
	void ApplyTargetPowerState(bool bNewPowered, bool bAllowTransition);
};
