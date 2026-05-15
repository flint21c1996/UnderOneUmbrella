// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinVolumeDeviceActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WaterTarget/UOUWaterBasinVolumeDeviceComponent.h"

AUOUWaterBasinVolumeDeviceActor::AUOUWaterBasinVolumeDeviceActor()
{
	// 물 장치 Actor 자체는 Tick이 필요하지 않습니다.
	// 연속 수위 변경은 VolumeDevice 컴포넌트의 Tick에서 처리합니다.
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
	// 퍼즐 조건에서는 결과 액션 enum만 전달하고,
	// 실제 물 제어 방식은 기존 VolumeDevice 컴포넌트 설정에 맡깁니다.
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
