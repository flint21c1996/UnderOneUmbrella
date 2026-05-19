// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinVolumeDeviceActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 장치 동작의 방향은 Operation(Add/Remove/Fill/Drain)이 결정합니다.
	// Amount와 Continuous 값에 음수를 허용하면 Operation 의미와 충돌하므로 0 이상으로 고정합니다.
	constexpr float MinActionValue = 0.0f;
}

AUOUWaterBasinVolumeDeviceActor::AUOUWaterBasinVolumeDeviceActor()
{
	// Actor 자체는 Tick이 필요하지 않습니다.
	// 연속 수위 변경은 VolumeDeviceComponent의 Tick에서 처리합니다.
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	SetRootComponent(DefaultSceneRoot);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube"));
	BaseMesh->SetupAttachment(DefaultSceneRoot);

	// 기본 Mesh는 에디터에서 장치 위치를 빠르게 확인하기 위한 표시용입니다.
	// 실제 장치 외형은 Blueprint에서 교체하거나 별도 Mesh를 추가해서 구성합니다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		BaseMesh->SetStaticMesh(CubeMeshAsset.Object);
	}

	VolumeDevice = CreateDefaultSubobject<UUOUWaterBasinVolumeDeviceComponent>(TEXT("UOUWaterBasinVolumeDevice"));

	// 퍼즐 조건과 바로 연결해 테스트하기 쉽도록 기본 동작은 Activate=채우기, Deactivate=비우기로 둡니다.
	// 실제 퍼즐에서는 각 Action 설정을 에디터에서 Fill/Drain/Add/Remove/Set 계열로 덮어씁니다.
	ActivateAction.Command = EUOUWaterBasinVolumeDevicePuzzleCommand::RunOperation;
	ActivateAction.Operation = EUOUWaterBasinVolumeDeviceOperation::FillAll;

	DeactivateAction.Command = EUOUWaterBasinVolumeDevicePuzzleCommand::RunOperation;
	DeactivateAction.Operation = EUOUWaterBasinVolumeDeviceOperation::DrainAll;

	PauseAction.Command = EUOUWaterBasinVolumeDevicePuzzleCommand::StopDevice;
	ResumeAction.Command = EUOUWaterBasinVolumeDevicePuzzleCommand::UseComponentDefault;
	ToggleAction.Command = EUOUWaterBasinVolumeDevicePuzzleCommand::ToggleDevice;
}

void AUOUWaterBasinVolumeDeviceActor::TriggerDevice()
{
	if (VolumeDevice == nullptr)
	{
		return;
	}

	VolumeDevice->TriggerDevice();
}

void AUOUWaterBasinVolumeDeviceActor::ActivateDevice()
{
	if (VolumeDevice == nullptr)
	{
		return;
	}

	VolumeDevice->ActivateDevice();
}

void AUOUWaterBasinVolumeDeviceActor::DeactivateDevice()
{
	if (VolumeDevice == nullptr)
	{
		return;
	}

	VolumeDevice->DeactivateDevice();
}

void AUOUWaterBasinVolumeDeviceActor::ToggleDevice()
{
	if (VolumeDevice == nullptr)
	{
		return;
	}

	VolumeDevice->ToggleDevice();
}

void AUOUWaterBasinVolumeDeviceActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	if (const FUOUWaterBasinVolumeDeviceActionSetting* ActionSetting = GetActionSetting(Action))
	{
		ExecuteActionSetting(Action, *ActionSetting);
		return;
	}

	ExecuteDefaultPuzzleAction(Action);
}

const FUOUWaterBasinVolumeDeviceActionSetting* AUOUWaterBasinVolumeDeviceActor::GetActionSetting(EOUUPuzzleResultAction Action) const
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		return &ActivateAction;

	case EOUUPuzzleResultAction::Deactivate:
		return &DeactivateAction;

	case EOUUPuzzleResultAction::Pause:
		return &PauseAction;

	case EOUUPuzzleResultAction::Resume:
		return &ResumeAction;

	case EOUUPuzzleResultAction::Toggle:
		return &ToggleAction;

	case EOUUPuzzleResultAction::None:
	default:
		return nullptr;
	}
}

void AUOUWaterBasinVolumeDeviceActor::ExecuteActionSetting(EOUUPuzzleResultAction Action, const FUOUWaterBasinVolumeDeviceActionSetting& Setting)
{
	if (VolumeDevice == nullptr)
	{
		return;
	}

	switch (Setting.Command)
	{
	case EUOUWaterBasinVolumeDevicePuzzleCommand::UseComponentDefault:
		ExecuteDefaultPuzzleAction(Action);
		break;

	case EUOUWaterBasinVolumeDevicePuzzleCommand::RunOperation:
		ApplyActionSettingToComponent(Setting);
		VolumeDevice->TriggerDevice();
		break;

	case EUOUWaterBasinVolumeDevicePuzzleCommand::StopDevice:
		VolumeDevice->DeactivateDevice();
		break;

	case EUOUWaterBasinVolumeDevicePuzzleCommand::ToggleDevice:
		ApplyActionSettingToComponent(Setting);
		VolumeDevice->ToggleDevice();
		break;

	case EUOUWaterBasinVolumeDevicePuzzleCommand::Ignore:
	default:
		break;
	}
}

void AUOUWaterBasinVolumeDeviceActor::ApplyActionSettingToComponent(const FUOUWaterBasinVolumeDeviceActionSetting& Setting)
{
	if (VolumeDevice == nullptr)
	{
		return;
	}

	VolumeDevice->ControlScope = Setting.ControlScope;
	VolumeDevice->Operation = Setting.Operation;
	VolumeDevice->Amount = FMath::Max(MinActionValue, Setting.Amount);
	VolumeDevice->TargetWaterDepth = FMath::Max(MinActionValue, Setting.TargetWaterDepth);
	VolumeDevice->TargetSurfaceWorldZ = Setting.TargetSurfaceWorldZ;
	VolumeDevice->bUseContinuousChange = Setting.bUseContinuousChange;
	VolumeDevice->ContinuousVolumePerSecond = FMath::Max(MinActionValue, Setting.ContinuousVolumePerSecond);
	VolumeDevice->ContinuousHeightPerSecond = FMath::Max(MinActionValue, Setting.ContinuousHeightPerSecond);
	VolumeDevice->bDeactivateWhenReachedGoal = Setting.bDeactivateWhenReachedGoal;
}

void AUOUWaterBasinVolumeDeviceActor::ExecuteDefaultPuzzleAction(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		TriggerDevice();
		break;

	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		DeactivateDevice();
		break;

	case EOUUPuzzleResultAction::Resume:
		ActivateDevice();
		break;

	case EOUUPuzzleResultAction::Toggle:
		ToggleDevice();
		break;

	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}
