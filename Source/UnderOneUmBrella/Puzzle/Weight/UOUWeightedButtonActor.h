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

// 무게 버튼 퍼즐의 로컬 구조를 한 번에 배치할 수 있게 묶어 둔 베이스 액터다.
UCLASS(meta=(DisplayName="UOU Weighted Button Actor"))
class AUOUWeightedButtonActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUWeightedButtonActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UBoxComponent> WeightSensorVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UStaticMeshComponent> ButtonVisual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> ReleasedPoint = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> PressedPoint = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUWeightSensorComponent> WeightSensorComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUWeightedButtonComponent> WeightedButtonComponent = nullptr;
};
