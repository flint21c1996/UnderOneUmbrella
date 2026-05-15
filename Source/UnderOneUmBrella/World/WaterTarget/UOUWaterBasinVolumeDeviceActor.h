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
	UseComponentDefault UMETA(DisplayName = "Use Component Default", ToolTip = "기존 VolumeDeviceComponent 설정과 기존 액션 기본 동작을 그대로 사용합니다."),
	RunOperation UMETA(DisplayName = "Run Operation", ToolTip = "이 설정에 담긴 물 동작을 실행합니다. Continuous가 켜져 있으면 연속 동작을 시작하고, 꺼져 있으면 1회 실행합니다."),
	StopDevice UMETA(DisplayName = "Stop Device", ToolTip = "현재 진행 중인 연속 물 동작을 멈춥니다."),
	ToggleDevice UMETA(DisplayName = "Toggle Device", ToolTip = "이 설정을 VolumeDeviceComponent에 적용한 뒤 실행 또는 정지를 전환합니다."),
	Ignore UMETA(DisplayName = "Ignore", ToolTip = "해당 퍼즐 결과 액션을 무시합니다.")
};

// 퍼즐 결과 액션 하나가 실행할 물 장치 동작 설정입니다.
// 이 구조체는 ActivateAction, DeactivateAction처럼 액션별 필드에 직접 들어갑니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUWaterBasinVolumeDeviceActionSetting
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle Result", meta = (ToolTip = "퍼즐 결과 액션을 받았을 때 실행할 장치 명령입니다."))
	EUOUWaterBasinVolumeDevicePuzzleCommand Command = EUOUWaterBasinVolumeDevicePuzzleCommand::RunOperation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ToolTip = "물을 Target 하나만 바꿀지, 연결 그룹 전체에 나눠 적용할지 정합니다."))
	EUOUWaterBasinDeviceControlScope ControlScope = EUOUWaterBasinDeviceControlScope::ConnectedGroup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ToolTip = "실제로 실행할 물 제어 동작입니다."))
	EUOUWaterBasinVolumeDeviceOperation Operation = EUOUWaterBasinVolumeDeviceOperation::AddAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ClampMin = "0.0", ToolTip = "Add Amount 또는 Remove Amount에서 한 번에 추가/제거할 물 부피입니다."))
	float Amount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ClampMin = "0.0", ToolTip = "Set Water Depth에서 사용할 목표 물 깊이입니다."))
	float TargetWaterDepth = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ToolTip = "Set Surface World Z에서 사용할 목표 수면 월드 Z입니다."))
	float TargetSurfaceWorldZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ToolTip = "켜져 있으면 Run Operation이 1회 실행이 아니라 연속 동작 시작으로 처리됩니다."))
	bool bUseContinuousChange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ClampMin = "0.0", ToolTip = "연속 Add/Remove/Fill/Drain에서 초당 변화시킬 물 부피입니다."))
	float ContinuousVolumePerSecond = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ClampMin = "0.0", ToolTip = "연속 Set Water Depth 또는 Set Surface World Z에서 초당 이동할 높이입니다."))
	float ContinuousHeightPerSecond = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ToolTip = "목표 수위, 가득 참, 비어 있음에 도달하면 연속 동작을 자동으로 멈춥니다."))
	bool bDeactivateWhenReachedGoal = true;
};

// 퍼즐 결과를 받아 WaterBasinVolumeDeviceComponent를 실행하는 물 장치 Actor입니다.
// 실제 물 계산은 VolumeDeviceComponent가 담당하고, 이 Actor는 액션별 설정을 선택합니다.
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

	// 외부 레버/조건에서 가장 일반적으로 호출할 진입점입니다.
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
