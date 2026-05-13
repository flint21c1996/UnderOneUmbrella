// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"
#include "UOUWaterBasinDebugController.generated.h"

class UUOUWaterBasinVolumeDeviceComponent;

// WaterBasin 관련 런타임 디버깅을 한 곳에서 켜고 끄기 위한 컨트롤러입니다.
// 각 WaterBasinTargetComponent가 직접 디버그 표시 범위를 판단하지 않도록,
// 선택한 Target Actor와 표시 범위, 연결선 표시 여부를 이 Actor가 전역 설정으로 전달합니다.
// 또한 테스트용 Device Actor를 지정해 에디터/블루프린트에서 장치 동작을 바로 실행할 수 있습니다.
UCLASS()
class UNDERONEUMBRELLA_API AUOUWaterBasinDebugController : public AActor
{
	GENERATED_BODY()

public:
	AUOUWaterBasinDebugController();

	// 시작 시 현재 에디터/블루프린트에 입력된 디버그 설정을 WaterBasinTargetComponent에 반영합니다.
	virtual void BeginPlay() override;

	// 런타임 중 Details 패널이나 블루프린트에서 값이 바뀌어도 즉시 반영되도록 매 프레임 설정을 재적용합니다.
	virtual void Tick(float DeltaSeconds) override;

	// 플레이 종료 시 static 디버그 설정을 꺼서 다음 PIE 실행에 이전 설정이 남지 않도록 정리합니다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 전체 WaterBasin 디버그 오버레이 출력 여부입니다.
	// false이면 Target/Group 정보와 연결선 표시가 모두 꺼집니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Debug", meta = (ToolTip = "런타임 WaterBasin 디버그 문자열 표시를 켜고 끕니다."))
	bool bEnableWaterBasinDebug = true;

	// 선택된 Target 또는 선택된 Target이 속한 그룹의 연결 관계를 선으로 표시할지 결정합니다.
	// 실제 연결 정보는 WaterBasinTargetComponent의 ConnectedTargets를 기준으로 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Debug", meta = (ToolTip = "디버그 대상의 연결선을 런타임에 함께 표시합니다."))
	bool bShowConnectionLines = true;

	// 디버그 텍스트의 대상 범위입니다.
	// SpecificTarget은 지정한 Target 하나의 수위/부피를 보여주고,
	// SpecificConnectedGroup은 지정한 Target이 포함된 연결 그룹의 합산 정보를 보여줍니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Debug", meta = (ToolTip = "특정 Target 하나만 볼지, 그 Target이 포함된 연결 그룹의 합산 정보를 볼지 정합니다."))
	EUOUWaterBasinDebugOverlayScope DebugOverlayScope = EUOUWaterBasinDebugOverlayScope::SpecificTarget;

	// 디버깅 기준이 되는 Actor입니다.
	// 이 Actor 안에서 WaterBasinTargetComponent를 찾아 실제 디버그 대상으로 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Debug", meta = (ToolTip = "디버깅할 WaterBasinTarget입니다. Specific Connected Group일 때는 이 Target이 포함된 그룹 전체가 표시됩니다."))
	TObjectPtr<AActor> DebugTargetActor = nullptr;

	// 테스트할 WaterBasinVolumeDeviceComponent를 가진 Actor입니다.
	// 이 값을 지정하면 아래 CallInEditor 함수들로 장치 동작을 즉시 검증할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Debug|Device", meta = (ToolTip = "디버그 컨트롤러에서 테스트할 WaterBasinVolumeDeviceComponent를 가진 Actor입니다."))
	TObjectPtr<AActor> DebugDeviceActor = nullptr;

	// 현재 컨트롤러에 입력된 값을 WaterBasinTargetComponent의 런타임 디버그 static 설정에 적용합니다.
	// 디버그 표시의 중심 함수이며 BeginPlay, Tick, setter 함수에서 호출됩니다.
	UFUNCTION(BlueprintCallable, Category = "Water Debug")
	void ApplyDebugSettings();

	// 디버그 오버레이 표시 여부를 바꾸고 즉시 적용합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Debug")
	void SetDebugEnabled(bool bEnabled);

	// 디버그 기준 Actor를 변경하고 즉시 적용합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Debug")
	void SetDebugTargetActor(AActor* NewTargetActor);

	// Target 하나만 볼지, 연결 그룹 합산 정보를 볼지 변경하고 즉시 적용합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Debug")
	void SetDebugScope(EUOUWaterBasinDebugOverlayScope NewScope);

	// 지정된 Device의 TriggerDevice를 호출합니다.
	// Device 설정에 따라 즉시 1회 실행 또는 지속 동작 토글로 처리됩니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Water Debug|Device")
	void TriggerDebugDevice();

	// 지정된 Device를 활성화하여 지속 동작을 시작합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Water Debug|Device")
	void ActivateDebugDevice();

	// 지정된 Device를 비활성화하여 지속 동작을 멈춥니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Water Debug|Device")
	void DeactivateDebugDevice();

	// 지정된 Device의 활성 상태를 반대로 전환합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Water Debug|Device")
	void ToggleDebugDevice();

	// 지정된 Device의 설정된 동작을 강제로 1회 실행합니다.
	// 지속 동작 설정과 관계없이 장치 계산이 정상인지 빠르게 확인할 때 사용합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Water Debug|Device")
	void ExecuteDebugDeviceOnce();

	// 디버그 컨트롤러가 제어할 Device Actor를 변경합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Debug|Device")
	void SetDebugDeviceActor(AActor* NewDeviceActor);

	// DebugDeviceActor에서 WaterBasinVolumeDeviceComponent를 찾을 수 있는지 확인합니다.
	// 블루프린트 UI나 테스트 로직에서 버튼 활성화 조건으로 사용할 수 있습니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Debug|Device")
	bool HasValidDebugDevice() const;

private:
	// DebugDeviceActor에 붙어 있는 WaterBasinVolumeDeviceComponent를 가져옵니다.
	// DebugController는 Actor 참조만 노출하고, 실제 호출 직전에 Component를 해석합니다.
	UUOUWaterBasinVolumeDeviceComponent* GetDebugDeviceComponent() const;
};
