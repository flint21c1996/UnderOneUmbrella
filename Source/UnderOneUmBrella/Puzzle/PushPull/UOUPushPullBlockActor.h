// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUPushPullBlockActor.generated.h"

class UStaticMeshComponent;
class UUOUPuzzleWeightComponent;
class UUOUPushPullObjectComponent;
class UUOUWaterContainerComponent;

// 밀고 당기기와 물 저장, 퍼즐 무게를 한 번에 테스트할 수 있는 블럭 액터입니다.
// 실제 퍼즐용 상자 프리셋처럼 조합해 두고 레벨에 바로 배치할 수 있습니다.
UCLASS()
class AUOUPushPullBlockActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUPushPullBlockActor();

	// 상자의 실제 외형과 물리 충돌을 담당하는 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull")
	TObjectPtr<UStaticMeshComponent> BlockVisual = nullptr;

	// 상자의 밀기와 당기기 동작을 담당하는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull")
	TObjectPtr<UUOUPushPullObjectComponent> PushPullObjectComponent = nullptr;

	// 상자의 퍼즐용 무게 계산을 담당하는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull")
	TObjectPtr<UUOUPuzzleWeightComponent> PuzzleWeightComponent = nullptr;

	// 상자 안에 저장된 물 양을 관리하는 물 컨테이너 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UUOUWaterContainerComponent> WaterContainerComponent = nullptr;
};
