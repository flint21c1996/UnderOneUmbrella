// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinVolumeDeviceComponent.h"

#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

namespace
{
	// 연속 동작은 Tick마다 조금씩 목표에 접근하므로 값이 정확히 같은 순간을 기다리면 정지하지 못할 수 있습니다.
	// 0.001은 물 부피/타일 높이 단위에서 눈에 보이지 않는 작은 오차만 도달로 허용하기 위한 값입니다.
	constexpr float OperationGoalVolumeTolerance = 0.001f;
	constexpr float OperationGoalHeightToleranceInTiles = 0.001f;
}

UUOUWaterBasinVolumeDeviceComponent::UUOUWaterBasinVolumeDeviceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUWaterBasinVolumeDeviceComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UUOUWaterBasinVolumeDeviceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bDeviceActive || !bUseContinuousChange)
	{
		return;
	}

	// 연속 동작은 매 Tick 작은 변화량을 적용한 뒤, 목표 상태에 도달하면 선택적으로 자동 중지합니다.
	ExecuteContinuousOperation(DeltaTime);

	if (bDeactivateWhenReachedGoal && IsAtOperationGoal())
	{
		DeactivateDevice();
	}
}

void UUOUWaterBasinVolumeDeviceComponent::TriggerDevice()
{
	// 장치의 외부 진입점입니다. 레버/버튼은 이 함수만 호출하면 되고,
	// 이 함수가 설정에 따라 "한 번 실행" 또는 "연속 활성화"로 분기합니다.
	if (bUseContinuousChange)
	{
		ActivateDevice();
		return;
	}

	ExecuteOperationOnce();
}

void UUOUWaterBasinVolumeDeviceComponent::ActivateDevice()
{
	if (!HasValidTarget())
	{
		OnDeviceFailed.Broadcast(this);
		return;
	}

	if (!bUseContinuousChange)
	{
		// 연속 모드가 아니면 활성 상태를 유지할 필요가 없으므로 즉시 한 번만 실행합니다.
		ExecuteOperationOnce();
		return;
	}

	if (bDeviceActive)
	{
		return;
	}

	bDeviceActive = true;
	OnDeviceActivated.Broadcast(this);
}

void UUOUWaterBasinVolumeDeviceComponent::DeactivateDevice()
{
	if (!bDeviceActive)
	{
		return;
	}

	bDeviceActive = false;
	OnDeviceDeactivated.Broadcast(this);
}

void UUOUWaterBasinVolumeDeviceComponent::ToggleDevice()
{
	if (bDeviceActive)
	{
		DeactivateDevice();
	}
	else
	{
		TriggerDevice();
	}
}

void UUOUWaterBasinVolumeDeviceComponent::ExecuteOperationOnce()
{
	if (!HasValidTarget())
	{
		OnDeviceFailed.Broadcast(this);
		return;
	}

	// Operation enum을 실제 WaterBasinTargetComponent 호출로 변환하는 중앙 분기입니다.
	switch (Operation)
	{
	case EUOUWaterBasinVolumeDeviceOperation::AddAmount:
		AddWaterOnce();
		break;

	case EUOUWaterBasinVolumeDeviceOperation::RemoveAmount:
		RemoveWaterOnce();
		break;

	case EUOUWaterBasinVolumeDeviceOperation::FillAll:
		FillWater();
		break;

	case EUOUWaterBasinVolumeDeviceOperation::DrainAll:
		DrainWater();
		break;

	case EUOUWaterBasinVolumeDeviceOperation::SetWaterDepth:
		SetWaterDepthOnce();
		break;

	case EUOUWaterBasinVolumeDeviceOperation::SetSurfaceWorldZ:
		SetSurfaceWorldZOnce();
		break;

	default:
		OnDeviceFailed.Broadcast(this);
		return;
	}

	OnDeviceExecuted.Broadcast(this);
}

void UUOUWaterBasinVolumeDeviceComponent::AddWaterOnce()
{
	AddWaterCustom(Amount);
}

void UUOUWaterBasinVolumeDeviceComponent::RemoveWaterOnce()
{
	RemoveWaterCustom(Amount);
}

void UUOUWaterBasinVolumeDeviceComponent::FillWater()
{
	// Fill/Drain/Set 계열은 Custom 함수보다 의미가 분명한 래퍼입니다.
	// 실제 대상은 Target Actor 안의 WaterBasinTargetComponent입니다.
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		OnDeviceFailed.Broadcast(this);
		return;
	}

	TargetComponent->FillWater(ShouldApplyToConnectedGroup());
}

void UUOUWaterBasinVolumeDeviceComponent::DrainWater()
{
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		OnDeviceFailed.Broadcast(this);
		return;
	}

	TargetComponent->DrainWater(ShouldApplyToConnectedGroup());
}

void UUOUWaterBasinVolumeDeviceComponent::SetWaterDepthOnce()
{
	SetWaterDepthCustom(TargetWaterDepth);
}

void UUOUWaterBasinVolumeDeviceComponent::SetSurfaceWorldZOnce()
{
	SetSurfaceWorldZCustom(TargetSurfaceWorldZ);
}

void UUOUWaterBasinVolumeDeviceComponent::AddWaterCustom(float Volume)
{
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		OnDeviceFailed.Broadcast(this);
		return;
	}

	TargetComponent->AddWater(FMath::Max(Volume, 0.0f), ShouldApplyToConnectedGroup());
}

void UUOUWaterBasinVolumeDeviceComponent::RemoveWaterCustom(float Volume)
{
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		OnDeviceFailed.Broadcast(this);
		return;
	}

	TargetComponent->RemoveWater(FMath::Max(Volume, 0.0f), ShouldApplyToConnectedGroup());
}

void UUOUWaterBasinVolumeDeviceComponent::SetWaterDepthCustom(float Depth)
{
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		OnDeviceFailed.Broadcast(this);
		return;
	}

	TargetComponent->SetWaterDepth(FMath::Max(Depth, 0.0f), ShouldApplyToConnectedGroup());
}

void UUOUWaterBasinVolumeDeviceComponent::SetSurfaceWorldZCustom(float SurfaceWorldZ)
{
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		OnDeviceFailed.Broadcast(this);
		return;
	}

	TargetComponent->SetWaterSurfaceWorldZ(SurfaceWorldZ, ShouldApplyToConnectedGroup());
}

bool UUOUWaterBasinVolumeDeviceComponent::HasValidTarget() const
{
	return IsValid(GetTargetComponent());
}

bool UUOUWaterBasinVolumeDeviceComponent::IsAtOperationGoal() const
{
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		return false;
	}

	const float HeightToleranceWorld = OperationGoalHeightToleranceInTiles * TargetComponent->WorldUnitsPerTile;

	if (ShouldApplyToConnectedGroup())
	{
		// 그룹 제어에서는 Target 하나의 값이 아니라 그룹 합산 부피와 공통 SurfaceWorldZ를 기준으로 목표 도달을 판단합니다.
		const FUOUWaterBasinGroupDebugData GroupData = TargetComponent->GetConnectedGroupDebugData();

		switch (Operation)
		{
		case EUOUWaterBasinVolumeDeviceOperation::FillAll:
			return GroupData.TotalVolume >= GroupData.TotalCapacity - OperationGoalVolumeTolerance;

		case EUOUWaterBasinVolumeDeviceOperation::DrainAll:
			return GroupData.TotalVolume <= OperationGoalVolumeTolerance;

		case EUOUWaterBasinVolumeDeviceOperation::SetWaterDepth:
		{
			const float TargetDepthSurfaceWorldZ = TargetComponent->GetBottomWorldZ() + (TargetWaterDepth * TargetComponent->WorldUnitsPerTile);
			return FMath::IsNearlyEqual(GroupData.SurfaceWorldZ, TargetDepthSurfaceWorldZ, HeightToleranceWorld);
		}

		case EUOUWaterBasinVolumeDeviceOperation::SetSurfaceWorldZ:
			return FMath::IsNearlyEqual(GroupData.SurfaceWorldZ, TargetSurfaceWorldZ, HeightToleranceWorld);

		default:
			return false;
		}
	}

	switch (Operation)
	{
	// TargetOnly에서는 해당 TargetComponent 하나의 런타임 값을 직접 비교합니다.
	case EUOUWaterBasinVolumeDeviceOperation::FillAll:
		return TargetComponent->CurrentWaterVolume >= TargetComponent->GetCapacity() - OperationGoalVolumeTolerance;

	case EUOUWaterBasinVolumeDeviceOperation::DrainAll:
		return TargetComponent->CurrentWaterVolume <= OperationGoalVolumeTolerance;

	case EUOUWaterBasinVolumeDeviceOperation::SetWaterDepth:
		return FMath::IsNearlyEqual(TargetComponent->CurrentWaterDepth, TargetWaterDepth, OperationGoalHeightToleranceInTiles);

	case EUOUWaterBasinVolumeDeviceOperation::SetSurfaceWorldZ:
		return FMath::IsNearlyEqual(TargetComponent->WaterSurfaceWorldZ, TargetSurfaceWorldZ, HeightToleranceWorld);

	default:
		return false;
	}
}

bool UUOUWaterBasinVolumeDeviceComponent::ShouldApplyToConnectedGroup() const
{
	return ControlScope == EUOUWaterBasinDeviceControlScope::ConnectedGroup;
}

UUOUWaterBasinTargetComponent* UUOUWaterBasinVolumeDeviceComponent::GetTargetComponent() const
{
	// DeviceComp의 Target은 에디터에서 지정하기 쉬운 Actor 참조입니다.
	// 실제 물 계산은 그 Actor에 붙은 WaterBasinTargetComponent가 담당합니다.
	return IsValid(Target) ? Target->FindComponentByClass<UUOUWaterBasinTargetComponent>() : nullptr;
}

void UUOUWaterBasinVolumeDeviceComponent::ExecuteContinuousOperation(float DeltaTime)
{
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		OnDeviceFailed.Broadcast(this);
		DeactivateDevice();
		return;
	}

	const float VolumeStep = FMath::Max(ContinuousVolumePerSecond, 0.0f) * DeltaTime;

	// Add/Remove/Fill/Drain은 초당 부피 변화량을 그대로 누적합니다.
	// Set 계열은 높이 보간 함수로 위임해 수면이 일정 속도로 움직이게 합니다.
	switch (Operation)
	{
	case EUOUWaterBasinVolumeDeviceOperation::AddAmount:
	case EUOUWaterBasinVolumeDeviceOperation::FillAll:
		TargetComponent->AddWater(VolumeStep, ShouldApplyToConnectedGroup());
		break;

	case EUOUWaterBasinVolumeDeviceOperation::RemoveAmount:
	case EUOUWaterBasinVolumeDeviceOperation::DrainAll:
		TargetComponent->RemoveWater(VolumeStep, ShouldApplyToConnectedGroup());
		break;

	case EUOUWaterBasinVolumeDeviceOperation::SetWaterDepth:
		MoveWaterDepthToward(TargetWaterDepth, DeltaTime);
		break;

	case EUOUWaterBasinVolumeDeviceOperation::SetSurfaceWorldZ:
		MoveSurfaceWorldZToward(TargetSurfaceWorldZ, DeltaTime);
		break;

	default:
		OnDeviceFailed.Broadcast(this);
		DeactivateDevice();
		break;
	}
}

void UUOUWaterBasinVolumeDeviceComponent::MoveWaterDepthToward(float DesiredDepth, float DeltaTime)
{
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		return;
	}

	const float HeightSpeed = FMath::Max(ContinuousHeightPerSecond, 0.0f);
	if (ShouldApplyToConnectedGroup())
	{
		// 그룹 제어에서 "깊이"는 Target의 바닥을 기준으로 목표 SurfaceWorldZ를 만든 뒤,
		// 그룹 공통 SurfaceWorldZ를 그 목표까지 일정 속도로 이동시키는 방식으로 처리합니다.
		const FUOUWaterBasinGroupDebugData GroupData = TargetComponent->GetConnectedGroupDebugData();
		const float DesiredSurface = TargetComponent->GetBottomWorldZ() + (FMath::Max(DesiredDepth, 0.0f) * TargetComponent->WorldUnitsPerTile);
		const float NewSurface = FMath::FInterpConstantTo(GroupData.SurfaceWorldZ, DesiredSurface, DeltaTime, HeightSpeed * TargetComponent->WorldUnitsPerTile);
		TargetComponent->SetWaterSurfaceWorldZ(NewSurface, true);
		return;
	}

	// TargetOnly는 해당 Target의 CurrentWaterDepth를 직접 목표 깊이에 가까워지게 합니다.
	const float NewDepth = FMath::FInterpConstantTo(TargetComponent->CurrentWaterDepth, FMath::Max(DesiredDepth, 0.0f), DeltaTime, HeightSpeed);
	TargetComponent->SetWaterDepth(NewDepth, false);
}

void UUOUWaterBasinVolumeDeviceComponent::MoveSurfaceWorldZToward(float DesiredSurfaceWorldZ, float DeltaTime)
{
	UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		return;
	}

	const float HeightSpeedWorld = FMath::Max(ContinuousHeightPerSecond, 0.0f) * TargetComponent->WorldUnitsPerTile;
	const float CurrentSurfaceWorldZ = ShouldApplyToConnectedGroup()
		? TargetComponent->GetConnectedGroupDebugData().SurfaceWorldZ
		: TargetComponent->WaterSurfaceWorldZ;

	// SurfaceWorldZ 직접 제어는 월드 좌표 기준이므로 ContinuousHeightPerSecond를 월드 단위 속도로 변환해 사용합니다.
	const float NewSurfaceWorldZ = FMath::FInterpConstantTo(CurrentSurfaceWorldZ, DesiredSurfaceWorldZ, DeltaTime, HeightSpeedWorld);
	TargetComponent->SetWaterSurfaceWorldZ(NewSurfaceWorldZ, ShouldApplyToConnectedGroup());
}
