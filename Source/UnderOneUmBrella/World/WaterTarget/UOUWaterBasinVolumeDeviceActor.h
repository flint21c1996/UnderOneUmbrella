// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUWaterBasinVolumeDeviceActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UUOUWaterBasinVolumeDeviceComponent;

// 퍼즐 결과를 받아 WaterBasinVolumeDeviceComponent를 실행하는 물 장치 Actor입니다.
// 블루프린트로 구성했던 WaterDevice(Self) -> DefaultSceneRoot -> Cube + UOUWaterBasinVolumeDevice 구조를
// C++ 기본 구조로 제공하여 조건 컴포넌트/퍼즐 결과에서 바로 제어할 수 있게 합니다.
UCLASS(meta=(DisplayName="UOU Water Basin Volume Device Actor"))
class UNDERONEUMBRELLA_API AUOUWaterBasinVolumeDeviceActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUWaterBasinVolumeDeviceActor();

	// 장치 Actor의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Device")
	TObjectPtr<USceneComponent> DefaultSceneRoot = nullptr;

	// 에디터에서 장치의 위치와 크기를 눈으로 확인하기 위한 기본 표시 Mesh입니다.
	// 실제 게임용 메시나 머티리얼은 블루프린트 파생 클래스에서 교체해도 됩니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Device")
	TObjectPtr<UStaticMeshComponent> BaseMesh = nullptr;

	// 물 추가, 제거, 채우기, 비우기, 목표 수위 설정을 실제로 수행하는 장치 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Device")
	TObjectPtr<UUOUWaterBasinVolumeDeviceComponent> VolumeDevice = nullptr;

	// 외부 레버/조건에서 가장 일반적으로 호출할 진입점입니다.
	// 컴포넌트 설정에 따라 1회 실행 또는 연속 동작 시작으로 분기됩니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void TriggerDevice();

	// 연속 동작을 시작합니다. 연속 모드가 꺼져 있으면 컴포넌트 규칙에 따라 1회 실행됩니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void ActivateDevice();

	// 연속 동작을 멈춥니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void DeactivateDevice();

	// 현재 장치 활성 상태에 따라 실행 또는 정지를 전환합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void ToggleDevice();

	// 퍼즐 조건이 전달하는 결과 액션을 물 장치 동작으로 변환합니다.
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
};
