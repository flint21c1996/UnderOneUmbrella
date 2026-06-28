// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUWaterWheelRainConditionActor.generated.h"

class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOUWaterWheelRainConditionComponent;
class UUOUWaterWheelSpeedConditionComponent;

// 임시 실린더 비주얼과 Rain Catch Point를 포함한 물레방아 배치용 Actor입니다.
// Blueprint는 이 클래스를 상속한 뒤 메쉬/크기/속도만 조정해서 사용할 수 있습니다.
UCLASS(meta=(DisplayName="UOU Water Wheel Rain Condition Actor"))
class UNDERONEUMBRELLA_API AUOUWaterWheelRainConditionActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUWaterWheelRainConditionActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor, Category = "Water Wheel|Rain Catch", meta = (DisplayName = "기본 Catch Point로 재설정", ToolTip = "CatchLeft/CatchRight를 기본 좌우 위치로 되돌리고 RainCatchPoints를 다시 동기화합니다."))
	void ResetDefaultRainCatchPoints();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Wheel|Rain Catch", meta = (DisplayName = "Catch Arrow 위치 자동 동기화", ToolTip = "켜면 CatchLeft/CatchRight Arrow의 현재 위치를 RainCatchPoints로 동기화합니다. RainCatchPoints 배열을 직접 편집하려면 끄세요."))
	bool bAutoCreateDefaultRainCatchPoints = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Wheel")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Wheel")
	TObjectPtr<USceneComponent> RotationPivot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Wheel")
	TObjectPtr<UStaticMeshComponent> WheelVisual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Wheel|Rain Catch")
	TObjectPtr<UArrowComponent> CatchLeft = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Wheel|Rain Catch")
	TObjectPtr<UArrowComponent> CatchRight = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Wheel")
	TObjectPtr<UUOUWaterWheelRainConditionComponent> WaterWheelCondition = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Wheel")
	TObjectPtr<UUOUWaterWheelSpeedConditionComponent> FastSpeedCondition = nullptr;

	void EnsureDefaultRainCatchPoints();
	void ApplyDefaultRainCatchPointsFromArrows();
};
