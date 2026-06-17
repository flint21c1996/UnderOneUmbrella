// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUInteractionConditionActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UUOUInteractionConditionComponent;

// 플레이어 상호작용 입력을 퍼즐 조건 만족으로 바꾸는 배치용 액터입니다.
UCLASS(meta=(DisplayName="UOU Interaction Condition Actor"))
class UNDERONEUMBRELLA_API AUOUInteractionConditionActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUInteractionConditionActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UBoxComponent> InteractionVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUInteractionConditionComponent> InteractionConditionComponent = nullptr;
};
