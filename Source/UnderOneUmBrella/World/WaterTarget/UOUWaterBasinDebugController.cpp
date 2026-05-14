// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinDebugController.h"

#include "World/WaterTarget/UOUWaterBasinVolumeDeviceComponent.h"

AUOUWaterBasinDebugController::AUOUWaterBasinDebugController()
{
	// Details 패널이나 블루프린트에서 디버그 옵션을 바꾸는 즉시 반영하기 위해 Tick을 사용합니다.
	PrimaryActorTick.bCanEverTick = true;
}

void AUOUWaterBasinDebugController::BeginPlay()
{
	Super::BeginPlay();

	// PIE 시작 시 에디터에 배치된 DebugTargetActor / Scope / 선 표시 설정을 최초 적용합니다.
	ApplyDebugSettings();
}

void AUOUWaterBasinDebugController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 디버깅 중에는 런타임 Details 패널에서 값을 직접 바꾸는 경우가 많습니다.
	// 매 프레임 재적용하면 별도 버튼 없이 선택 Target, Scope, 연결선 표시가 바로 갱신됩니다.
	ApplyDebugSettings();
}

void AUOUWaterBasinDebugController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// WaterBasinTargetComponent의 디버그 설정은 static 값이므로,
	// 플레이가 끝날 때 꺼두지 않으면 다음 실행에 이전 Target/Scope가 남을 수 있습니다.
	UUOUWaterBasinTargetComponent::SetRuntimeDebugOverlay(false, false, DebugOverlayScope, nullptr);
	Super::EndPlay(EndPlayReason);
}

void AUOUWaterBasinDebugController::ApplyDebugSettings()
{
	// 컨트롤러는 Actor를 외부 설정값으로 받고,
	// 실제 디버그 출력은 해당 Actor 안의 WaterBasinTargetComponent를 기준으로 처리합니다.
	UUOUWaterBasinTargetComponent* ResolvedDebugTarget = DebugTargetActor
		? DebugTargetActor->FindComponentByClass<UUOUWaterBasinTargetComponent>()
		: nullptr;

	// TargetComponent 쪽 static 설정으로 전달하면,
	// 각 Target의 Tick/디버그 드로잉 로직이 같은 기준을 공유하게 됩니다.
	UUOUWaterBasinTargetComponent::SetRuntimeDebugOverlay(
		bEnableWaterBasinDebug,
		bShowConnectionLines,
		DebugOverlayScope,
		ResolvedDebugTarget);
}

void AUOUWaterBasinDebugController::SetDebugEnabled(bool bEnabled)
{
	// 외부에서 디버그 표시 여부를 바꾸면 다음 Tick을 기다리지 않고 바로 적용합니다.
	bEnableWaterBasinDebug = bEnabled;
	ApplyDebugSettings();
}

void AUOUWaterBasinDebugController::SetDebugTargetActor(AActor* NewTargetActor)
{
	// Actor만 교체하면 ApplyDebugSettings에서 Component를 다시 찾아 디버그 대상이 갱신됩니다.
	DebugTargetActor = NewTargetActor;
	ApplyDebugSettings();
}

void AUOUWaterBasinDebugController::SetDebugScope(EUOUWaterBasinDebugOverlayScope NewScope)
{
	// Scope 변경은 텍스트 의미를 바꿉니다.
	// SpecificTarget은 단일 Target, SpecificConnectedGroup은 연결 그룹의 합산 데이터를 표시합니다.
	DebugOverlayScope = NewScope;
	ApplyDebugSettings();
}

void AUOUWaterBasinDebugController::TriggerDebugDevice()
{
	// TriggerDevice는 Device 설정에 맞춰 1회 실행 또는 지속 동작 토글을 수행하는 진입점입니다.
	if (UUOUWaterBasinVolumeDeviceComponent* DeviceComponent = GetDebugDeviceComponent())
	{
		DeviceComponent->TriggerDevice();
	}
}

void AUOUWaterBasinDebugController::ActivateDebugDevice()
{
	// 지속 동작 테스트용입니다. Device의 Tick에서 수위가 서서히 목표로 이동합니다.
	if (UUOUWaterBasinVolumeDeviceComponent* DeviceComponent = GetDebugDeviceComponent())
	{
		DeviceComponent->ActivateDevice();
	}
}

void AUOUWaterBasinDebugController::DeactivateDebugDevice()
{
	// 지속 동작을 멈춥니다. 현재 물의 양은 유지되고 Device의 활성 상태만 꺼집니다.
	if (UUOUWaterBasinVolumeDeviceComponent* DeviceComponent = GetDebugDeviceComponent())
	{
		DeviceComponent->DeactivateDevice();
	}
}

void AUOUWaterBasinDebugController::ToggleDebugDevice()
{
	// 반복 테스트 중 버튼 하나로 활성/비활성을 전환할 때 사용합니다.
	if (UUOUWaterBasinVolumeDeviceComponent* DeviceComponent = GetDebugDeviceComponent())
	{
		DeviceComponent->ToggleDevice();
	}
}

void AUOUWaterBasinDebugController::ExecuteDebugDeviceOnce()
{
	// 지속 동작 속도와 관계없이 현재 Operation이 계산한 결과를 즉시 확인할 때 사용합니다.
	if (UUOUWaterBasinVolumeDeviceComponent* DeviceComponent = GetDebugDeviceComponent())
	{
		DeviceComponent->ExecuteOperationOnce();
	}
}

void AUOUWaterBasinDebugController::SetDebugDeviceActor(AActor* NewDeviceActor)
{
	// Device도 Target과 마찬가지로 Actor 단위로 지정합니다.
	// 실제 Component 확인은 호출 시점에 GetDebugDeviceComponent에서 수행합니다.
	DebugDeviceActor = NewDeviceActor;
}

bool AUOUWaterBasinDebugController::HasValidDebugDevice() const
{
	// 지정된 Actor에 WaterBasinVolumeDeviceComponent가 실제로 붙어 있는지 확인합니다.
	return IsValid(GetDebugDeviceComponent());
}

UUOUWaterBasinVolumeDeviceComponent* AUOUWaterBasinDebugController::GetDebugDeviceComponent() const
{
	// 블루프린트에서 Actor만 지정하면 되도록 Component 참조를 직접 노출하지 않습니다.
	// Actor가 유효하지 않거나 DeviceComponent가 없으면 nullptr을 반환해 호출부에서 안전하게 무시합니다.
	return IsValid(DebugDeviceActor)
		? DebugDeviceActor->FindComponentByClass<UUOUWaterBasinVolumeDeviceComponent>()
		: nullptr;
}
