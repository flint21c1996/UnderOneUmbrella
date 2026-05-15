// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "Puzzle/Core/UOUPuzzleWeightSource.h"
#include "UOUWeightedButtonComponent.generated.h"

class USceneComponent;
class UUOUWeightSensorComponent;

// 센서가 읽은 무게를 바탕으로 버튼 눌림 상태를 계산하는 버튼 핵심 컴포넌트입니다.
// 조건 소스와 무게 소스 역할을 동시에 맡아 퍼즐 그룹과 저울에 함께 연결할 수 있습니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUWeightedButtonComponent : public UUOUPuzzleConditionSourceComponent, public IUOUPuzzleWeightSource
{
	GENERATED_BODY()

public:
	UUOUWeightedButtonComponent();

	// 시작 시 센서와 비주얼 참조를 정리하고 초기 상태를 맞춥니다.
	virtual void BeginPlay() override;

	// 매 틱마다 버튼 비주얼을 눌림 위치로 보간 이동합니다.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 현재 버튼이 퍼즐 무게 소스로 제공할 값을 반환합니다.
	virtual float GetPuzzleWeight() const override;

	// 같은 액터 안에서 센서 컴포넌트를 자동으로 찾을지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	bool bAutoFindSensor = true;

	// 수동으로 연결할 센서 컴포넌트 참조입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	FComponentReference SensorReference;

	// 실제로 연결된 센서 컴포넌트입니다.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	TObjectPtr<UUOUWeightSensorComponent> Sensor = nullptr;

	// 버튼이 눌리기 시작하는 최소 무게입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float PressWeight = 5.0f;

	// 버튼이 다시 올라오는 기준 무게입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float ReleaseWeight = 4.5f;

	// 수동으로 연결할 버튼 비주얼 참조입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FComponentReference ButtonVisualReference;

	// 실제로 움직일 버튼 비주얼 컴포넌트입니다.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	TObjectPtr<USceneComponent> ButtonVisual = nullptr;

	// 버튼이 원래 위치에 있을 때의 기준점 참조입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FComponentReference ReleasedPointReference;

	// 실제로 연결된 원래 위치 기준점입니다.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	TObjectPtr<USceneComponent> ReleasedPoint = nullptr;

	// 버튼이 눌린 위치 기준점 참조입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FComponentReference PressedPointReference;

	// 실제로 연결된 눌림 위치 기준점입니다.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	TObjectPtr<USceneComponent> PressedPoint = nullptr;

	// 같은 액터 안에서 버튼 비주얼과 포인트를 자동으로 찾을지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	bool bAutoFindMotionReferences = true;

	// 자동 탐색 시 버튼 비주얼로 우선 찾을 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FName PreferredButtonVisualName = TEXT("ButtonVisual");

	// 자동 탐색 시 원래 위치 기준점으로 우선 찾을 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FName PreferredReleasedPointName = TEXT("ReleasedPoint");

	// 자동 탐색 시 눌림 위치 기준점으로 우선 찾을 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FName PreferredPressedPointName = TEXT("PressedPoint");

	// 버튼 비주얼을 이동시킬 때 사용할 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion", meta = (ClampMin = "0.0"))
	float MoveSpeed = 120.0f;

	// 화면 디버그를 켜서 현재 무게와 상태를 확인할지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Debug")
	bool bShowScreenDebug = true;

	// 현재 센서가 읽은 버튼 무게 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float CurrentWeight = 0.0f;

	// 현재 버튼이 눌린 상태인지 바로 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle|Weight")
	bool IsPressed() const;

protected:
	// 화면 왼쪽 위에 버튼 상태 디버그를 출력합니다.
	void DrawScreenDebug() const;

	// 센서와 비주얼 관련 참조를 자동 탐색하거나 수동 참조로 해석합니다.
	void ResolveReferences();

	// 현재 무게를 기준으로 눌림 상태를 다시 계산합니다.
	void RefreshPressedState();

	// 버튼 비주얼을 현재 상태에 맞는 기준점 쪽으로 이동합니다.
	void MoveButtonVisual(float DeltaTime);

	// 버튼 비주얼을 현재 상태 위치에 즉시 맞춥니다.
	void SnapVisualToCurrentState();
};
