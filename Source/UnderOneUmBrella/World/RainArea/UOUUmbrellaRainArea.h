// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUUmbrellaRainArea.generated.h"

class UBoxComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

// 플레이어가 들어가면 비 노출과 물 받기 판정을 적용하는 테스트용 비 구역 액터입니다.
// 에디터와 게임 안에서 범위를 쉽게 볼 수 있도록 프리뷰 메쉬도 함께 관리합니다.
UCLASS(meta=(DisplayName="UOU Umbrella Rain Area"))
class AUOUUmbrellaRainArea : public AActor
{
	GENERATED_BODY()

public:
	AUOUUmbrellaRainArea();

protected:
	// 시작 시 프리뷰와 판정 상태를 초기화합니다.
	virtual void BeginPlay() override;

	// 게임 중 프리뷰 표시 상태를 유지할 때 필요하면 갱신합니다.
	virtual void Tick(float DeltaSeconds) override;

	// 에디터에서 비 구역 크기나 프리뷰 설정이 바뀔 때 외형을 다시 맞춥니다.
	virtual void OnConstruction(const FTransform& Transform) override;

	// 비 구역 액터 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 실제 비 판정에 사용할 박스 볼륨입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	TObjectPtr<UBoxComponent> RainVolume = nullptr;

	// 에디터와 테스트 중 범위를 보여 주는 프리뷰 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	TObjectPtr<UStaticMeshComponent> PreviewVolumeMesh = nullptr;

	// 에디터에서 프리뷰 메쉬를 보일지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	bool bShowEditorPreview = true;

	// 게임 중에도 프리뷰 메쉬를 보일지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	bool bShowPreviewInGame = true;

	// 프리뷰 메쉬 스케일에 추가로 곱할 보정값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	FVector PreviewScaleMultiplier = FVector(1.0f, 1.0f, 1.0f);

	// 프리뷰 메쉬 크기를 RainVolume에 자동으로 맞출지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	bool bAutoFitPreviewScaleToRainVolume = true;

	// 자동 맞춤을 끈 경우 사용할 수동 프리뷰 스케일입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview", meta = (EditCondition = "!bAutoFitPreviewScaleToRainVolume"))
	FVector ManualPreviewRelativeScale = FVector::OneVector;

	// 프리뷰 메쉬에 적용할 머티리얼입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain|Preview")
	TObjectPtr<UMaterialInterface> PreviewMaterial = nullptr;

	// 비 구역 안에서 물이 차는 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float RainFillRate = 1.0f;

	// 현재 프리뷰 메쉬의 표시와 스케일을 설정값에 맞게 갱신합니다.
	void ApplyPreviewSettings();
};
