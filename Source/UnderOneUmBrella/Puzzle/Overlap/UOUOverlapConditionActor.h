// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUOverlapConditionActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UUOUOverlapConditionComponent;

// 특정 Actor가 감지 영역에 진입했는지를 퍼즐 조건으로 제공하는 배치용 Actor입니다.
UCLASS(meta=(DisplayName="UOU Overlap Condition Actor"))
class UNDERONEUMBRELLA_API AUOUOverlapConditionActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUOverlapConditionActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UBoxComponent> OverlapVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUOverlapConditionComponent> OverlapConditionComponent = nullptr;
};
