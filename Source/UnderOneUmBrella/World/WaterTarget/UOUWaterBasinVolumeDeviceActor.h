// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "World/WaterTarget/UOUWaterBasinVolumeDeviceComponent.h"
#include "UOUWaterBasinVolumeDeviceActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

// 퍼즐 결과 액션을 받았을 때 WaterDeviceActor가 어떤 방식으로 반응할지 정합니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinVolumeDevicePuzzleCommand : uint8
{
	// 기존 VolumeDeviceComponent에 직접 설정된 Operation/Amount/Continuous 값을 그대로 사용합니다.
	UseComponentDefault UMETA(DisplayName = "Use Component Default", ToolTip = "기존 VolumeDeviceComponent 설정과 기본 액션 동작을 그대로 사용합니다."),

	// 이 액션 설정의 Operation 값을 VolumeDeviceComponent에 복사한 뒤 실행합니다.
	RunOperation UMETA(DisplayName = "Run Operation", ToolTip = "이 액션 설정에 담긴 물 동작을 실행합니다. Continuous가 켜져 있으면 연속 동작을 시작하고, 꺼져 있으면 1회 실행합니다."),

	// 연속 동작만 멈춥니다. 1회 동작에는 추가 실행을 하지 않습니다.
	StopDevice UMETA(DisplayName = "Stop Device", ToolTip = "현재 진행 중인 연속 물 동작을 멈춥니다."),

	// 이 액션 설정을 적용한 뒤 VolumeDeviceComponent의 현재 활성 상태를 토글합니다.
	ToggleDevice UMETA(DisplayName = "Toggle Device", ToolTip = "이 액션 설정을 적용한 뒤 장치 실행 또는 정지를 전환합니다."),

	// 이 퍼즐 결과 액션을 의도적으로 무시합니다.
	Ignore UMETA(DisplayName = "Ignore", ToolTip = "해당 퍼즐 결과 액션을 무시합니다.")
};

// 퍼즐 결과 액션 하나가 실행할 물 장치 동작 설정입니다.
// 예: ActivateAction은 FillAll, DeactivateAction은 DrainAll처럼 액션별로 다른 Operation을 지정할 수 있습니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUWaterBasinVolumeDeviceActionSetting
{
	GENERATED_BODY()

	// 이 액션이 들어왔을 때 어떤 방식으로 처리할지 정합니다.
	// 대부분의 퍼즐 장치는 RunOperation을 사용하고, 일시정지/정지는 StopDevice를 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle Result", meta = (ToolTip = "이 액션이 들어왔을 때 어떤 방식으로 처리할지 정합니다. 일반적인 물 조작은 RunOperation을 사용합니다."))
	EUOUWaterBasinVolumeDevicePuzzleCommand Command = EUOUWaterBasinVolumeDevicePuzzleCommand::RunOperation;

	// 물을 Target 하나에만 적용할지, 연결 그룹 전체에 나눠 적용할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ToolTip = "물을 Target 하나에만 적용할지, 연결 그룹 전체에 나눠 적용할지 정합니다."))
	EUOUWaterBasinDeviceControlScope ControlScope = EUOUWaterBasinDeviceControlScope::ConnectedGroup;

	// 실제로 실행할 물 조작입니다.
	// FillAll, DrainAll, AddAmount, RemoveAmount, SetWaterDepth, SetSurfaceWorldZ 중에서 선택합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ToolTip = "실제로 실행할 물 조작입니다. Fill/Drain/Add/Remove/Set 계열을 액션별로 다르게 지정할 수 있습니다."))
	EUOUWaterBasinVolumeDeviceOperation Operation = EUOUWaterBasinVolumeDeviceOperation::AddAmount;

	// AddAmount/RemoveAmount에서 사용할 1회 물 부피입니다.
	// 기본 1은 1x1x1 타일 한 칸 부피를 기준으로 테스트하기 쉬운 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ClampMin = "0.0", ToolTip = "AddAmount 또는 RemoveAmount에서 한 번에 추가/제거할 물 부피입니다. 기본 1은 1x1x1 타일 한 칸 부피 기준입니다."))
	float Amount = 1.0f;

	// SetWaterDepth에서 사용할 목표 깊이입니다.
	// 단위는 타일 높이 기준이며, 실제 월드 높이는 WaterTile의 WorldUnitsPerTile을 곱해 계산됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ClampMin = "0.0", ToolTip = "SetWaterDepth에서 사용할 목표 물 깊이입니다. 타일 높이 단위이며 월드 높이는 WorldUnitsPerTile을 곱해 계산됩니다."))
	float TargetWaterDepth = 1.0f;

	// SetSurfaceWorldZ에서 사용할 목표 수면 월드 Z입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ToolTip = "SetSurfaceWorldZ에서 사용할 목표 수면 월드 Z입니다."))
	float TargetSurfaceWorldZ = 0.0f;

	// 켜져 있으면 RunOperation이 1회 실행이 아니라 연속 동작 시작으로 처리됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ToolTip = "켜져 있으면 RunOperation이 1회 실행이 아니라 연속 동작 시작으로 처리됩니다."))
	bool bUseContinuousChange = false;

	// 연속 Add/Remove/Fill/Drain에서 초당 변화시킬 물 부피입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ClampMin = "0.0", ToolTip = "연속 Add/Remove/Fill/Drain에서 초당 변화시킬 물 부피입니다."))
	float ContinuousVolumePerSecond = 1.0f;

	// 연속 SetWaterDepth/SetSurfaceWorldZ에서 초당 이동할 높이입니다.
	// SetWaterDepth에서는 타일 높이 단위, SetSurfaceWorldZ에서는 내부에서 월드 높이로 변환되어 사용됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ClampMin = "0.0", ToolTip = "연속 SetWaterDepth 또는 SetSurfaceWorldZ에서 초당 이동할 높이입니다."))
	float ContinuousHeightPerSecond = 1.0f;

	// 목표 수위, 가득 참, 비어 있음에 도달하면 연속 동작을 자동으로 멈춥니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ToolTip = "목표 수위, 가득 참, 비어 있음에 도달하면 연속 동작을 자동으로 멈춥니다."))
	bool bDeactivateWhenReachedGoal = true;
};

// 퍼즐 결과를 받아 WaterBasinVolumeDeviceComponent를 실행하는 물 장치 Actor입니다.
// 실제 물 계산은 VolumeDeviceComponent가 담당하고, 이 Actor는 Activate/Deactivate 같은 액션별 설정을 선택합니다.
UCLASS(meta=(DisplayName="UOU Water Basin Volume Device Actor"))
class UNDERONEUMBRELLA_API AUOUWaterBasinVolumeDeviceActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUWaterBasinVolumeDeviceActor();

	// 장치 Actor의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Device")
	TObjectPtr<USceneComponent> DefaultSceneRoot = nullptr;

	// 에디터에서 장치 위치와 크기를 확인하기 위한 기본 표시 Mesh입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Device")
	TObjectPtr<UStaticMeshComponent> BaseMesh = nullptr;

	// 물 추가, 제거, 채우기, 비우기, 목표 수위 설정을 실제로 수행하는 장치 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Device")
	TObjectPtr<UUOUWaterBasinVolumeDeviceComponent> VolumeDevice = nullptr;

	// ApplyPuzzleResult(Activate)를 받았을 때 실행할 물 동작입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Puzzle Result|Activate")
	FUOUWaterBasinVolumeDeviceActionSetting ActivateAction;

	// ApplyPuzzleResult(Deactivate)를 받았을 때 실행할 물 동작입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Puzzle Result|Deactivate")
	FUOUWaterBasinVolumeDeviceActionSetting DeactivateAction;

	// ApplyPuzzleResult(Pause)를 받았을 때 실행할 물 동작입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Puzzle Result|Pause")
	FUOUWaterBasinVolumeDeviceActionSetting PauseAction;

	// ApplyPuzzleResult(Resume)를 받았을 때 실행할 물 동작입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Puzzle Result|Resume")
	FUOUWaterBasinVolumeDeviceActionSetting ResumeAction;

	// ApplyPuzzleResult(Toggle)를 받았을 때 실행할 물 동작입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Puzzle Result|Toggle")
	FUOUWaterBasinVolumeDeviceActionSetting ToggleAction;

	// 기존 VolumeDeviceComponent 설정을 그대로 실행합니다.
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

	// 퍼즐 조건이 전달하는 결과 액션을 액션별 물 장치 동작으로 변환합니다.
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

private:
	const FUOUWaterBasinVolumeDeviceActionSetting* GetActionSetting(EOUUPuzzleResultAction Action) const;
	void ExecuteActionSetting(EOUUPuzzleResultAction Action, const FUOUWaterBasinVolumeDeviceActionSetting& Setting);
	void ApplyActionSettingToComponent(const FUOUWaterBasinVolumeDeviceActionSetting& Setting);
	void ExecuteDefaultPuzzleAction(EOUUPuzzleResultAction Action);
};
