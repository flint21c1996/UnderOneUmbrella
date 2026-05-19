// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUWaterBasinVolumeDeviceComponent.generated.h"

class UUOUWaterBasinTargetComponent;
class UUOUWaterBasinVolumeDeviceComponent;
class AActor;

// Device가 물을 제어할 범위입니다.
// TargetOnly는 지정 Actor의 TargetComp 하나만, ConnectedGroup은 그 TargetComp가 속한 연결 그룹 전체를 제어합니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinDeviceControlScope : uint8
{
	ConnectedGroup UMETA(DisplayName = "Connected Group", ToolTip = "지정한 Target이 포함된 연결 그룹 전체의 물을 제어합니다."),
	TargetOnly UMETA(DisplayName = "Target Only", ToolTip = "연결된 그룹을 무시하고 지정한 Target 하나의 물만 제어합니다.")
};

// 장치가 실행될 때 수행할 물 제어 방식입니다.
// 같은 Operation이라도 bUseContinuousChange가 켜져 있으면 Tick마다 목표에 가까워지도록 천천히 처리됩니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinVolumeDeviceOperation : uint8
{
	AddAmount UMETA(DisplayName = "Add Amount", ToolTip = "작동할 때 Amount만큼 물 부피를 추가합니다."),
	RemoveAmount UMETA(DisplayName = "Remove Amount", ToolTip = "작동할 때 Amount만큼 물 부피를 제거합니다."),
	FillAll UMETA(DisplayName = "Fill All", ToolTip = "작동할 때 대상 범위를 최대 수위까지 채웁니다."),
	DrainAll UMETA(DisplayName = "Drain All", ToolTip = "작동할 때 대상 범위의 물을 전부 제거합니다."),
	SetWaterDepth UMETA(DisplayName = "Set Water Depth", ToolTip = "작동할 때 지정한 Target의 바닥 기준 깊이가 Target Water Depth가 되도록 수면을 맞춥니다."),
	SetSurfaceWorldZ UMETA(DisplayName = "Set Surface World Z", ToolTip = "작동할 때 수면의 월드 Z가 Target Surface World Z가 되도록 맞춥니다.")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUWaterBasinVolumeDeviceEvent, UUOUWaterBasinVolumeDeviceComponent*, Device);

UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUWaterBasinVolumeDeviceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUWaterBasinVolumeDeviceComponent();

	// Tick은 연속 동작이 켜져 있고 장치가 활성화된 동안에만 물을 서서히 변경합니다.
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ToolTip = "이 장치가 제어할 WaterBasinTargetComponent를 가진 Actor입니다. 레버, 버튼, 펌프 Actor에 이 컴포넌트를 붙이고 이 값을 지정합니다."))
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ToolTip = "물을 Target 하나만 바꿀지, Target이 속한 연결 그룹 전체를 바꿀지 정합니다."))
	EUOUWaterBasinDeviceControlScope ControlScope = EUOUWaterBasinDeviceControlScope::ConnectedGroup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ToolTip = "장치가 작동할 때 수행할 물 제어 방식입니다."))
	EUOUWaterBasinVolumeDeviceOperation Operation = EUOUWaterBasinVolumeDeviceOperation::AddAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ClampMin = "0.0", EditCondition = "Operation == EUOUWaterBasinVolumeDeviceOperation::AddAmount || Operation == EUOUWaterBasinVolumeDeviceOperation::RemoveAmount", EditConditionHides, ToolTip = "Add Amount 또는 Remove Amount에서 한 번에 추가/제거할 물 부피입니다."))
	float Amount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (ClampMin = "0.0", EditCondition = "Operation == EUOUWaterBasinVolumeDeviceOperation::SetWaterDepth", EditConditionHides, ToolTip = "Set Water Depth에서 사용할 목표 물 깊이입니다. 단위는 퍼즐 타일 높이입니다."))
	float TargetWaterDepth = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device", meta = (EditCondition = "Operation == EUOUWaterBasinVolumeDeviceOperation::SetSurfaceWorldZ", EditConditionHides, ToolTip = "Set Surface World Z에서 사용할 목표 수면 월드 Z입니다."))
	float TargetSurfaceWorldZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ToolTip = "켜져 있으면 TriggerDevice는 한 번 실행 대신 장치를 활성화하고, 활성화된 동안 Tick마다 서서히 물을 바꿉니다."))
	bool bUseContinuousChange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ClampMin = "0.0", EditCondition = "bUseContinuousChange && (Operation == EUOUWaterBasinVolumeDeviceOperation::AddAmount || Operation == EUOUWaterBasinVolumeDeviceOperation::RemoveAmount || Operation == EUOUWaterBasinVolumeDeviceOperation::FillAll || Operation == EUOUWaterBasinVolumeDeviceOperation::DrainAll)", EditConditionHides, ToolTip = "연속 동작 중 초당 추가/제거할 물 부피입니다. Fill All과 Drain All도 이 속도로 천천히 진행됩니다."))
	float ContinuousVolumePerSecond = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (ClampMin = "0.0", EditCondition = "bUseContinuousChange && (Operation == EUOUWaterBasinVolumeDeviceOperation::SetWaterDepth || Operation == EUOUWaterBasinVolumeDeviceOperation::SetSurfaceWorldZ)", EditConditionHides, ToolTip = "Set Water Depth 또는 Set Surface World Z를 연속 동작으로 사용할 때 초당 이동할 수면 높이입니다. 단위는 퍼즐 타일 높이입니다."))
	float ContinuousHeightPerSecond = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Device|Continuous", meta = (EditCondition = "bUseContinuousChange", EditConditionHides, ToolTip = "연속 동작 중 목표 수위, 가득 참, 비어 있음에 도달하면 자동으로 비활성화합니다."))
	bool bDeactivateWhenReachedGoal = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Device|Runtime")
	bool bDeviceActive = false;

	// 연속 동작이 활성화될 때 호출됩니다. 일회성 실행에서는 OnDeviceExecuted만 호출됩니다.
	UPROPERTY(BlueprintAssignable, Category = "Water Device")
	FUOUWaterBasinVolumeDeviceEvent OnDeviceActivated;

	// 연속 동작이 중지될 때 호출됩니다.
	UPROPERTY(BlueprintAssignable, Category = "Water Device")
	FUOUWaterBasinVolumeDeviceEvent OnDeviceDeactivated;

	// 일회성 장치 동작이 성공적으로 실행된 뒤 호출됩니다.
	UPROPERTY(BlueprintAssignable, Category = "Water Device")
	FUOUWaterBasinVolumeDeviceEvent OnDeviceExecuted;

	// Target Actor가 없거나 Target Actor에 WaterBasinTargetComponent가 없을 때 호출됩니다.
	UPROPERTY(BlueprintAssignable, Category = "Water Device")
	FUOUWaterBasinVolumeDeviceEvent OnDeviceFailed;

	// 외부 레버/버튼/디버그 컨트롤러가 호출하는 기본 진입점입니다. 연속 모드면 Activate, 아니면 ExecuteOperationOnce로 분기합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void TriggerDevice();

	// 연속 동작을 시작합니다. bUseContinuousChange가 꺼져 있으면 즉시 한 번 실행합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void ActivateDevice();

	// 연속 동작을 중지합니다. 이미 비활성 상태면 아무 일도 하지 않습니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void DeactivateDevice();

	// 현재 활성 상태에 따라 TriggerDevice 또는 DeactivateDevice를 호출합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void ToggleDevice();

	// Operation enum에 따라 Add/Remove/Fill/Drain/Set 계열 함수를 한 번 실행합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void ExecuteOperationOnce();

	// Amount만큼 물을 한 번 추가합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void AddWaterOnce();

	// Amount만큼 물을 한 번 제거합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void RemoveWaterOnce();

	// 대상 범위를 최대 용량까지 채웁니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void FillWater();

	// 대상 범위의 물을 모두 제거합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void DrainWater();

	// TargetWaterDepth를 목표 깊이로 한 번 설정합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void SetWaterDepthOnce();

	// TargetSurfaceWorldZ를 목표 수면 월드 Z로 한 번 설정합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void SetSurfaceWorldZOnce();

	// 외부에서 임의 부피를 넘겨 물을 추가할 때 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void AddWaterCustom(float Volume);

	// 외부에서 임의 부피를 넘겨 물을 제거할 때 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void RemoveWaterCustom(float Volume);

	// 외부에서 임의 깊이를 넘겨 물 깊이를 설정할 때 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void SetWaterDepthCustom(float Depth);

	// 외부에서 임의 수면 월드 Z를 넘겨 수면 높이를 설정할 때 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Device")
	void SetSurfaceWorldZCustom(float SurfaceWorldZ);

	// Target Actor가 지정되어 있고, 그 Actor 안에 WaterBasinTargetComponent가 있는지 확인합니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Device")
	bool HasValidTarget() const;

	// 연속 동작에서 목표 상태에 도달했는지 확인합니다. bDeactivateWhenReachedGoal이 켜져 있으면 이 값으로 자동 중지됩니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Device")
	bool IsAtOperationGoal() const;

private:
	// Target Actor에서 실제 물 계산을 담당하는 WaterBasinTargetComponent를 찾습니다.
	UUOUWaterBasinTargetComponent* GetTargetComponent() const;

	// 현재 ControlScope가 연결 그룹 전체를 대상으로 하는지 반환합니다.
	bool ShouldApplyToConnectedGroup() const;

	// bUseContinuousChange 상태에서 Tick마다 Operation에 맞는 소량의 변화량을 적용합니다.
	void ExecuteContinuousOperation(float DeltaTime);

	// 현재 깊이/그룹 수면을 DesiredDepth에 가까워지도록 일정 속도로 이동시킵니다.
	void MoveWaterDepthToward(float DesiredDepth, float DeltaTime);

	// 현재 SurfaceWorldZ를 DesiredSurfaceWorldZ에 가까워지도록 일정 속도로 이동시킵니다.
	void MoveSurfaceWorldZToward(float DesiredSurfaceWorldZ, float DeltaTime);
};
