// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUWeightedButtonActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOUWeightedButtonComponent;
class UUOUWeightSensorComponent;

// 무게 버튼 퍼즐을 바로 배치할 수 있게 센서와 비주얼을 묶어 둔 액터입니다.
// 버튼 테스트용 프리셋 부모 클래스로 사용하기 좋게 구성되어 있습니다.
UCLASS(meta=(DisplayName="UOU Weighted Button Actor"))
class AUOUWeightedButtonActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUWeightedButtonActor();

protected:
	// 버튼 액터 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 무게를 감지하는 오버랩 볼륨입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UBoxComponent> WeightSensorVolume = nullptr;

	// 눌렸다 올라오는 버튼 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UStaticMeshComponent> ButtonVisual = nullptr;

	// 버튼이 원래 위치에 있을 때의 기준점입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> ReleasedPoint = nullptr;

	// 버튼이 눌린 상태일 때의 기준점입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> PressedPoint = nullptr;

	// 겹친 액터 무게를 계산하는 센서 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUWeightSensorComponent> WeightSensorComponent = nullptr;

	// 센서 무게를 바탕으로 버튼 상태를 계산하는 핵심 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUWeightedButtonComponent> WeightedButtonComponent = nullptr;
};
