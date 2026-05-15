// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinVolumeDeviceActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AUOUWaterBasinVolumeDeviceActor::AUOUWaterBasinVolumeDeviceActor()
{
	// Actor 자체는 Tick이 필요하지 않습니다.
	// 연속 수위 변경은 VolumeDeviceComponent의 Tick에서 처리합니다.
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	SetRootComponent(DefaultSceneRoot);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube"));
	BaseMesh->SetupAttachment(DefaultSceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		BaseMesh->SetStaticMesh(CubeMeshAsset.Object);
	}

	VolumeDevice = CreateDefaultSubobject<UUOUWaterBasinVolumeDeviceComponent>(TEXT("UOUWaterBasinVolumeDevice"));

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
	VolumeDevice->Amount = FMath::Max(0.0f, Setting.Amount);
	VolumeDevice->TargetWaterDepth = FMath::Max(0.0f, Setting.TargetWaterDepth);
	VolumeDevice->TargetSurfaceWorldZ = Setting.TargetSurfaceWorldZ;
	VolumeDevice->bUseContinuousChange = Setting.bUseContinuousChange;
	VolumeDevice->ContinuousVolumePerSecond = FMath::Max(0.0f, Setting.ContinuousVolumePerSecond);
	VolumeDevice->ContinuousHeightPerSecond = FMath::Max(0.0f, Setting.ContinuousHeightPerSecond);
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
