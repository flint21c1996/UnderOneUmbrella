// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUPushPullBlockActor.generated.h"

class UStaticMeshComponent;
class UUOUPuzzleWeightComponent;
class UUOUPushPullObjectComponent;
class UUOUWaterContainerComponent;

// 밀고 당길 수 있는 테스트용 블럭 액터를 제공한다.
UCLASS()
class AUOUPushPullBlockActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUPushPullBlockActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull")
	TObjectPtr<UStaticMeshComponent> BlockVisual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull")
	TObjectPtr<UUOUPushPullObjectComponent> PushPullObjectComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull")
	TObjectPtr<UUOUPuzzleWeightComponent> PuzzleWeightComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UUOUWaterContainerComponent> WaterContainerComponent = nullptr;
};
