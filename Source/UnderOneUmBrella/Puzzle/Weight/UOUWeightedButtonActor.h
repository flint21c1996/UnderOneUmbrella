// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUWeightedButtonActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOUWeightedButtonComponent;
class UUOUWeightSensorComponent;

// 무게 버튼 퍼즐을 바로 월드에 배치할 수 있게 센서와 버튼 비주얼을 묶어 둔 액터입니다.
// 버튼 조건 판정은 제자리에 두고, 결과 연출이 필요하면 버튼 비주얼만 따로 아래로 내릴 수 있습니다.
UCLASS(meta=(DisplayName="UOU Weighted Button Actor"))
class AUOUWeightedButtonActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUWeightedButtonActor();

	// 퍼즐 조건 그룹에서 전달한 결과 액션을 받아 버튼 비주얼 침하 연출을 켜고 끕니다.
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

protected:
	// 버튼 비주얼이 침하 위치로 이동하는 연출을 매 프레임 갱신합니다.
	virtual void Tick(float DeltaSeconds) override;

	// 버튼 액터 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 플레이어나 무게 오브젝트가 올라왔는지 감지하는 판정 볼륨입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UBoxComponent> WeightSensorVolume = nullptr;

	// 실제 화면에 보이는 버튼 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UStaticMeshComponent> ButtonVisual = nullptr;

	// 버튼이 눌리지 않았을 때 ButtonVisual이 돌아갈 기준 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> ReleasedPoint = nullptr;

	// 버튼이 눌렸을 때 ButtonVisual이 내려갈 기준 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> PressedPoint = nullptr;

	// 퍼즐이 완전히 해결된 뒤 ButtonVisual이 추가로 내려갈 기준 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Result Sink")
	TObjectPtr<USceneComponent> ResultSinkPoint = nullptr;

	// WeightSensorVolume의 겹침 액터를 보고 현재 무게를 계산합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUWeightSensorComponent> WeightSensorComponent = nullptr;

	// WeightSensorComponent의 값을 버튼 눌림 조건으로 바꾸고 ButtonVisual 눌림 이동을 담당합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUWeightedButtonComponent> WeightedButtonComponent = nullptr;

	// 켜져 있으면 Activate 결과를 받았을 때 센서는 그대로 두고 ButtonVisual만 ResultSinkPoint로 내려갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Result Sink")
	bool bEnableResultSinkMotion = true;

	// 침하 연출 중 일반 버튼 눌림 애니메이션이 ButtonVisual을 다시 끌어올리지 못하게 막습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Result Sink")
	bool bPausePressMotionWhileSinking = true;

	// ButtonVisual이 ResultSinkPoint로 이동하는 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Result Sink", meta = (ClampMin = "0.0"))
	float ResultSinkMoveSpeed = 120.0f;

	// 현재 퍼즐 결과 침하 연출이 켜져 있는지 확인하는 런타임 상태입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bResultSinkActive = false;

private:
	// 침하 상태에 따라 ButtonVisual을 ResultSinkPoint 쪽으로 보간 이동합니다.
	void MoveResultSinkVisual(float DeltaSeconds);

	// 침하 상태에 맞춰 기존 버튼 눌림 이동 틱을 켜거나 끕니다.
	void RefreshButtonMotionTickState() const;
};
