// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUWaterBasinTargetComponent.generated.h"

class AActor;
class UUOUWaterBasinTargetComponent;

// 물 바닥 높이(BottomWorldZ)를 어떤 기준으로 잡을지 정합니다.
// 같은 수위 SurfaceWorldZ라도 바닥 기준이 달라지면 실제 물 깊이와 부피가 달라집니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinBottomHeightMode : uint8
{
	ActorBoundsMinZ UMETA(DisplayName = "Actor Bounds Min Z", ToolTip = "소유 Actor의 렌더링 경계 최하단 Z를 물 바닥 높이로 사용합니다. 일반적인 블록 메쉬에 가장 안전합니다."),
	ActorLocationZ UMETA(DisplayName = "Actor Location Z", ToolTip = "소유 Actor의 월드 위치 Z를 물 바닥 높이로 사용합니다. 피벗이 바닥에 있는 블록에 적합합니다."),
	ManualWorldZ UMETA(DisplayName = "Manual World Z", ToolTip = "Manual Bottom World Z 값을 물 바닥 높이로 직접 사용합니다.")
};

// 수면 면적과 최대 물 높이를 계산하는 기준입니다.
// ActorBounds는 배치된 액터의 실제 월드 bounds를 사용하고, Manual은 수동 기준값에 Actor Scale을 곱합니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinVolumeSizeMode : uint8
{
	ActorBounds UMETA(DisplayName = "Actor Bounds", ToolTip = "소유 Actor의 경계를 World Units Per Tile로 나누어 가로/세로/높이를 계산합니다. UE 기본 큐브 100cm는 기본 설정에서 1x1x1 부피가 됩니다."),
	Manual UMETA(DisplayName = "Manual", ToolTip = "Manual Surface Area와 Manual Max Water Height를 Scale 1 기준값으로 사용하고, 최종 용량에는 Actor Scale을 반영합니다.")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinInitialWaterFillMode : uint8
{
	Volume UMETA(DisplayName = "Volume", ToolTip = "초기 물량을 직접 부피 값으로 설정합니다. 기존 배치와 같은 방식입니다."),
	FillRatio UMETA(DisplayName = "Fill Ratio", ToolTip = "초기 물량을 전체 용량에 대한 비율로 설정합니다.")
};

// 플레이어가 붓는 물을 이 Basin이 어떤 기준으로 해석할지 정합니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinPouredWaterFillMode : uint8
{
	Volume UMETA(DisplayName = "Volume", ToolTip = "전달된 물 양을 그대로 Basin 부피로 더합니다. 기존 동작을 유지합니다."),
	FillRatio UMETA(DisplayName = "Fill Ratio", ToolTip = "플레이어가 물을 붓는 동안 초당 지정한 용량 비율만큼 채웁니다."),
	WaterDepth UMETA(DisplayName = "Water Depth", ToolTip = "플레이어가 물을 붓는 동안 초당 지정한 타일 깊이만큼 수면을 올립니다."),
	SurfaceWorldZ UMETA(DisplayName = "Surface World Z", ToolTip = "플레이어가 물을 붓는 동안 초당 지정한 월드 Z 높이만큼 수면을 올립니다.")
};

// Basin이 매 Tick 자체적으로 물을 배출할 때 배출 속도를 어떤 기준으로 해석할지 정합니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinPassiveDrainMode : uint8
{
	Volume UMETA(DisplayName = "Volume", ToolTip = "초당 지정한 부피만큼 물을 배출합니다."),
	FillRatio UMETA(DisplayName = "Fill Ratio", ToolTip = "초당 지정한 용량 비율만큼 물을 배출합니다."),
	WaterDepth UMETA(DisplayName = "Water Depth", ToolTip = "초당 지정한 타일 깊이만큼 수면을 낮춥니다."),
	SurfaceWorldZ UMETA(DisplayName = "Surface World Z", ToolTip = "초당 지정한 월드 Z 높이만큼 수면을 낮춥니다.")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinInputSource : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	PlayerPour UMETA(DisplayName = "Player Pour"),
	Rain UMETA(DisplayName = "Rain"),
	Script UMETA(DisplayName = "Script")
};

USTRUCT(BlueprintType)
struct FUOUWaterBasinInputContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Input", meta = (ClampMin = "0.0"))
	float Volume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Input", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Input")
	EUOUWaterBasinInputSource Source = EUOUWaterBasinInputSource::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Input")
	FVector WorldDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Input")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Input", meta = (ToolTip = "World Location이 해당 WaterBasinTarget 소유 Actor의 영역 안에 있는 실제 물 입력 지점인지 나타냅니다. 좌우 판정 같은 위치 기반 반응은 이 값이 켜져 있을 때만 World Location을 사용합니다."))
	bool bHasValidWorldLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Input")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Input")
	bool bApplyToConnectedGroup = true;

};

// 런타임 디버그 표시 범위입니다.
// 수치 디버그가 겹치지 않도록 특정 Target 또는 해당 Target이 포함된 그룹만 표시합니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinDebugOverlayScope : uint8
{
	SpecificTarget UMETA(DisplayName = "Specific Target", ToolTip = "지정한 WaterBasinTarget 하나의 런타임 정보와 직접 연결선을 표시합니다."),
	SpecificConnectedGroup UMETA(DisplayName = "Specific Connected Group", ToolTip = "지정한 WaterBasinTarget이 포함된 연결 그룹의 합산 정보와 그룹 내부 연결선을 표시합니다.")
};

// 연결 그룹 전체를 하나의 물 저장소처럼 볼 때 필요한 합산 디버그 데이터입니다.
// DeviceComp와 DebugController가 그룹 수위/용량을 검증할 때 사용합니다.
USTRUCT(BlueprintType)
struct FUOUWaterBasinGroupDebugData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin")
	int32 TargetCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin")
	float TotalVolume = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin")
	float TotalCapacity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin")
	float FillRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin")
	float SurfaceWorldZ = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin")
	float LowestBottomWorldZ = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin")
	float HighestTopWorldZ = 0.0f;
};

//수면의 정보가 바뀔때 발생할 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUWaterBasinTargetChangedSignature, UUOUWaterBasinTargetComponent*, Target);
// 실제 수위 변화 여부와 무관하게 물 입력이 들어왔을 때 발생하는 이벤트입니다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUOUWaterBasinWaterInputSignature, UUOUWaterBasinTargetComponent*, Target, const FUOUWaterBasinInputContext&, InputContext);

UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUWaterBasinTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUWaterBasinTargetComponent();

	// 초기 물 부피를 적용하고, 첫 Tick에서 연결 그룹 전체의 수면 높이를 한 번 맞춥니다.
	virtual void BeginPlay() override;

	// 초기 그룹 재분배와 런타임 디버그 표시를 처리합니다. 실제 물 계산은 이벤트/장치 호출 시점에 수행됩니다.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 이 Target과 물을 공유하는 수동 연결 목록입니다. 한쪽에만 등록해도 그룹 탐색에서는 연결로 취급합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Connection", meta = (ToolTip = "물을 공유할 다른 WaterBasinTarget Actor 목록입니다. 목록에 넣은 Actor에 WaterBasinTargetComponent가 붙어 있어야 연결로 사용됩니다."))
	TArray<TObjectPtr<AActor>> ConnectedTargets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Water Basin|Volume", meta = (ToolTip = "물 바닥 월드 Z를 어떤 기준으로 계산할지 정합니다. 기본값은 Actor Bounds Min Z이며, 일반적인 블록 배치에서는 이 값을 바꿀 필요가 없습니다."))
	EUOUWaterBasinBottomHeightMode BottomHeightMode = EUOUWaterBasinBottomHeightMode::ActorBoundsMinZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Water Basin|Volume", meta = (ToolTip = "수면 면적과 최대 물 높이를 어떤 기준으로 계산할지 정합니다. 기본값은 Actor Bounds이며, Actor Scale/Bounds를 기준으로 부피가 자동 계산됩니다."))
	EUOUWaterBasinVolumeSizeMode VolumeSizeMode = EUOUWaterBasinVolumeSizeMode::ActorBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Water Basin|Volume", meta = (ClampMin = "1.0", ToolTip = "언리얼 월드 단위(cm) 몇 개를 퍼즐의 1칸으로 볼지 정합니다. 기본값 100이면 UE 기본 큐브 한 변이 1 부피 단위입니다. 프로젝트 단위 기준이 바뀔 때만 수정합니다."))
	float WorldUnitsPerTile = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Water Basin|Volume", meta = (EditCondition = "BottomHeightMode == EUOUWaterBasinBottomHeightMode::ManualWorldZ", EditConditionHides, ToolTip = "Bottom Height Mode가 Manual World Z일 때 사용하는 물 바닥 월드 Z입니다."))
	float ManualBottomWorldZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Water Basin|Volume", meta = (ClampMin = "0.0001", EditCondition = "VolumeSizeMode == EUOUWaterBasinVolumeSizeMode::Manual", EditConditionHides, ToolTip = "Volume Size Mode가 Manual일 때 사용하는 Scale 1 기준 수면 면적입니다. 최종 면적은 이 값에 Actor Scale X/Y를 곱합니다."))
	float ManualSurfaceArea = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Water Basin|Volume", meta = (ClampMin = "0.0001", EditCondition = "VolumeSizeMode == EUOUWaterBasinVolumeSizeMode::Manual", EditConditionHides, ToolTip = "Volume Size Mode가 Manual일 때 사용하는 Scale 1 기준 최대 물 높이입니다. 최종 높이는 이 값에 Actor Scale Z를 곱합니다."))
	float ManualMaxWaterHeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Volume", meta = (ToolTip = "게임 시작 시 초기 물량을 어떤 기준으로 해석할지 정합니다. Volume은 기존 부피 기반 동작을 유지합니다."))
	EUOUWaterBasinInitialWaterFillMode InitialWaterFillMode = EUOUWaterBasinInitialWaterFillMode::Volume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Volume", meta = (ClampMin = "0.0", EditCondition = "InitialWaterFillMode == EUOUWaterBasinInitialWaterFillMode::Volume", EditConditionHides, ToolTip = "게임 시작 시 이 Target이 가진 초기 물 부피입니다. 연결 그룹이면 시작 직후 그룹 전체 부피로 다시 분배됩니다."))
	float InitialWaterVolume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Volume", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "InitialWaterFillMode == EUOUWaterBasinInitialWaterFillMode::FillRatio", EditConditionHides, ToolTip = "게임 시작 시 이 Target을 채울 초기 비율입니다. 0은 비어 있음, 1은 가득 참입니다."))
	float InitialWaterFillRatio = 0.0f;

	// 플레이어의 물 붓기 행위를 이 Basin의 물 상태로 변환하는 기준입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Pour", meta = (ToolTip = "플레이어가 붓는 물을 이 Target이 해석하는 방식입니다. Volume은 기존 부피 기반 동작을 유지합니다."))
	EUOUWaterBasinPouredWaterFillMode PouredWaterFillMode = EUOUWaterBasinPouredWaterFillMode::Volume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Pour", meta = (ClampMin = "0.0", EditCondition = "PouredWaterFillMode == EUOUWaterBasinPouredWaterFillMode::FillRatio", EditConditionHides, ToolTip = "Poured Water Fill Mode가 Fill Ratio일 때 초당 더할 용량 비율입니다. 0.1이면 Target 또는 그룹이 약 10초에 가득 찹니다."))
	float PouredWaterFillRatioPerSecond = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Pour", meta = (ClampMin = "0.0", EditCondition = "PouredWaterFillMode == EUOUWaterBasinPouredWaterFillMode::WaterDepth", EditConditionHides, ToolTip = "Poured Water Fill Mode가 Water Depth일 때 초당 더할 타일 깊이입니다."))
	float PouredWaterDepthPerSecond = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Pour", meta = (ClampMin = "0.0", EditCondition = "PouredWaterFillMode == EUOUWaterBasinPouredWaterFillMode::SurfaceWorldZ", EditConditionHides, ToolTip = "Poured Water Fill Mode가 Surface World Z일 때 초당 더할 월드 Z 높이입니다."))
	float PouredWaterSurfaceWorldZPerSecond = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Rain", meta = (ToolTip = "RainArea가 비 입력을 전달해도 이 Target이 실제로 비를 받을지 정합니다. 기본값은 꺼짐이며, 런타임에는 SetRainFillReceivingEnabled로 변경할 수 있습니다."))
	bool bReceiveRainFill = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Passive Drain", meta = (ToolTip = "이 Target이 매 Tick 자체적으로 물을 배출할지 정합니다. 하수구나 누수처럼 입력과 동시에 빠지는 물을 표현할 때 사용합니다."))
	bool bEnablePassiveDrain = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Passive Drain", meta = (EditCondition = "bEnablePassiveDrain", EditConditionHides, ToolTip = "Passive Drain의 배출 속도를 어떤 기준으로 해석할지 정합니다."))
	EUOUWaterBasinPassiveDrainMode PassiveDrainMode = EUOUWaterBasinPassiveDrainMode::WaterDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Passive Drain", meta = (ClampMin = "0.0", EditCondition = "bEnablePassiveDrain && PassiveDrainMode == EUOUWaterBasinPassiveDrainMode::Volume", EditConditionHides, ToolTip = "Passive Drain Mode가 Volume일 때 초당 배출할 부피입니다."))
	float PassiveDrainVolumePerSecond = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Passive Drain", meta = (ClampMin = "0.0", EditCondition = "bEnablePassiveDrain && PassiveDrainMode == EUOUWaterBasinPassiveDrainMode::FillRatio", EditConditionHides, ToolTip = "Passive Drain Mode가 Fill Ratio일 때 초당 배출할 용량 비율입니다."))
	float PassiveDrainFillRatioPerSecond = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Passive Drain", meta = (ClampMin = "0.0", EditCondition = "bEnablePassiveDrain && PassiveDrainMode == EUOUWaterBasinPassiveDrainMode::WaterDepth", EditConditionHides, ToolTip = "Passive Drain Mode가 Water Depth일 때 초당 낮출 타일 깊이입니다."))
	float PassiveDrainWaterDepthPerSecond = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Passive Drain", meta = (ClampMin = "0.0", EditCondition = "bEnablePassiveDrain && PassiveDrainMode == EUOUWaterBasinPassiveDrainMode::SurfaceWorldZ", EditConditionHides, ToolTip = "Passive Drain Mode가 Surface World Z일 때 초당 낮출 월드 Z 높이입니다."))
	float PassiveDrainSurfaceWorldZPerSecond = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Passive Drain", meta = (ClampMin = "0.0", EditCondition = "bEnablePassiveDrain", EditConditionHides, ToolTip = "이 Target 기준으로 유지할 최소 물 깊이입니다. Passive Drain은 이 수위 아래로 물을 배출하지 않습니다."))
	float PassiveDrainTargetWaterDepth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin|Passive Drain", meta = (EditCondition = "bEnablePassiveDrain", EditConditionHides, ToolTip = "켜져 있으면 이 Target이 속한 연결 그룹 전체에서 물을 배출합니다. 꺼져 있으면 이 Target 하나에서만 배출합니다."))
	bool bPassiveDrainApplyToConnectedGroup = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "Water Basin|Runtime Test", meta = (ClampMin = "0.0", ToolTip = "테스트용 현재 물 부피입니다. 에디터에서 바꾸면 깊이, 비율, 수면 Z가 즉시 갱신됩니다."))
	float CurrentWaterVolume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "Water Basin|Runtime Test", meta = (ClampMin = "0.0", ToolTip = "테스트용 현재 물 깊이입니다. 에디터에서 바꾸면 Current Water Volume으로 환산됩니다."))
	float CurrentWaterDepth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "Water Basin|Runtime Test", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "테스트용 현재 물 채움 비율입니다. 에디터에서 바꾸면 Current Water Volume으로 환산됩니다."))
	float CurrentFillRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "Water Basin|Runtime Test", meta = (ToolTip = "테스트용 현재 수면 월드 Z입니다. 에디터에서 바꾸면 Current Water Volume으로 환산됩니다."))
	float WaterSurfaceWorldZ = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin|Runtime")
	float LastGroupTotalVolume = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin|Runtime")
	float LastGroupTotalCapacity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin|Runtime")
	float LastGroupSurfaceWorldZ = 0.0f;

	// 현재 Target의 물 상태가 바뀔 때 호출됩니다. 그룹 작업에서는 그룹에 포함된 각 Target에서 각각 Broadcast됩니다.
	UPROPERTY(BlueprintAssignable, Category = "Water Basin")
	FUOUWaterBasinTargetChangedSignature OnWaterStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Water Basin")
	FUOUWaterBasinWaterInputSignature OnWaterInputReceived;

	// 물 부피를 추가합니다. 그룹 적용 시 현재 연결 그룹의 총 부피에 Volume을 더한 뒤 공통 수면 높이로 재분배합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void AddWater(float Volume, bool bApplyToConnectedGroup = true);

	// 플레이어의 물 붓기 행위는 유지하고, Target별 설정에 따라 물 상태로 해석합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void ReceivePouredWater(float Volume, float PourDuration, bool bApplyToConnectedGroup = true);

	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void ReceiveWaterInput(const FUOUWaterBasinInputContext& InputContext);

	UFUNCTION(BlueprintCallable, Category = "Water Basin|Rain")
	void SetRainFillReceivingEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Water Basin|Rain")
	bool CanReceiveRainFill() const;

	UFUNCTION(BlueprintCallable, Category = "Water Basin|Passive Drain")
	void SetPassiveDrainEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Water Basin|Passive Drain")
	bool IsPassiveDrainEnabled() const;

	// 물 부피를 제거합니다. 그룹 적용 시 현재 연결 그룹의 총 부피에서 Volume을 뺀 뒤 공통 수면 높이로 재분배합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void RemoveWater(float Volume, bool bApplyToConnectedGroup = true);

	// 바닥 기준 물 깊이를 설정합니다. Depth는 퍼즐 타일 단위이며 월드 높이는 Depth * WorldUnitsPerTile입니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void SetWaterDepth(float Depth, bool bApplyToConnectedGroup = true);

	// 수면의 월드 Z를 직접 설정합니다. 연결 그룹에 적용하면 모든 Target이 같은 SurfaceWorldZ를 기준으로 부피를 다시 계산합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void SetWaterSurfaceWorldZ(float SurfaceWorldZ, bool bApplyToConnectedGroup = true);

	// 대상 범위의 총 부피를 최대 용량까지 채웁니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void FillWater(bool bApplyToConnectedGroup = true);

	// 대상 범위의 총 부피를 0으로 만듭니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void DrainWater(bool bApplyToConnectedGroup = true);

	// 현재 연결 그룹이 가진 총 부피는 유지하고, 공통 수면 높이에 맞춰 각 Target의 부피를 다시 나눕니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void RedistributeConnectedWater();

	// 이 Target과 수동 연결된 모든 Target을 탐색해 하나의 연결 그룹으로 반환합니다. 역방향 연결도 함께 인정합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin")
	void GetConnectedGroup(TArray<UUOUWaterBasinTargetComponent*>& OutGroup) const;

	// 물이 시작되는 월드 Z이며 수면 높이 계산의 기준점입니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin")
	float GetBottomWorldZ() const;

	// 물이 가득 찼을 때의 월드 Z입니다. BottomWorldZ + MaxWaterHeight * WorldUnitsPerTile입니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin")
	float GetTopWorldZ() const;

	// 타일 단위의 수면 면적입니다. 부피와 깊이 변환식 Volume = Area * Depth에서 Area로 사용됩니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin")
	float GetSurfaceArea() const;

	// 타일 단위의 최대 물 깊이입니다. 월드 높이로 바꿀 때 WorldUnitsPerTile을 곱합니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin")
	float GetMaxWaterHeight() const;

	// 이 Target 하나가 가질 수 있는 최대 물 부피입니다. Capacity = SurfaceArea * MaxWaterHeight입니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin")
	float GetCapacity() const;

	// 지정한 월드 위치가 이 Target 소유 Actor의 X/Y 영역 안에 있는지 확인합니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin")
	bool IsWorldLocationInsideBasin(const FVector& WorldLocation) const;

	// 현재 물 깊이를 언리얼 월드 단위(cm)로 변환한 값입니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin")
	float GetWaterDepthWorld() const;

	// 연결 그룹 전체의 총 부피, 총 용량, 공통 수면 높이를 계산해 반환합니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin")
	FUOUWaterBasinGroupDebugData GetConnectedGroupDebugData() const;

	// DebugController가 런타임 오버레이 표시 상태를 전역으로 제어할 때 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin|Debug")
	static void SetRuntimeDebugOverlay(bool bEnabled, bool bShowConnectionLines, EUOUWaterBasinDebugOverlayScope Scope, UUOUWaterBasinTargetComponent* Target);

	// 런타임 디버그 오버레이가 켜져 있는지 반환합니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin|Debug")
	static bool IsRuntimeDebugOverlayEnabled();

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	// DebugController에서 공유하는 런타임 디버그 설정입니다. 표시 대상은 RuntimeDebugTarget 하나로 제한됩니다.
	static bool bRuntimeDebugOverlayEnabled;
	static bool bRuntimeDebugConnectionLinesEnabled;
	static EUOUWaterBasinDebugOverlayScope RuntimeDebugOverlayScope;
	static TWeakObjectPtr<UUOUWaterBasinTargetComponent> RuntimeDebugTarget;

	bool bPendingInitialRedistribution = false;

	// 연결 리스트에서 null, 자기 자신, 중복, WaterBasinTargetComponent가 없는 Actor를 제거합니다.
	void NormalizeConnections();

	// ConnectedTargets Actor 목록을 실제 UUOUWaterBasinTargetComponent 목록으로 변환합니다.
	void GetConnectedTargetComponents(TArray<UUOUWaterBasinTargetComponent*>& OutTargets) const;

	// 연결 그룹을 고려하지 않고 이 Target 하나의 부피만 갱신합니다.
	void ApplyWaterVolumeToSingleTarget(float NewVolume);

	// 연결 그룹의 총 부피를 지정하고, 같은 수면 높이가 되도록 각 Target에 부피를 분배합니다.
	void ApplyWaterVolumeToConnectedGroup(float NewTotalVolume);

	// 에디터에서 선택한 초기 물량 기준을 실제 부피로 변환합니다.
	float ResolveInitialWaterVolume() const;

	// 매 Tick 이 Target의 기본 배출 규칙을 적용합니다.
	void ApplyPassiveDrain(float DeltaTime);

	// 연결 그룹 전체 배수는 그룹 안의 대표 Target 하나만 실행하게 합니다.
	bool ShouldApplyPassiveDrainForConnectedGroup(const TArray<UUOUWaterBasinTargetComponent*>& Group) const;

	// Passive Drain이 물을 남겨둘 목표 수면 높이를 월드 Z로 반환합니다.
	float GetPassiveDrainTargetSurfaceWorldZ() const;

	// 주어진 수면 높이에서 대상 목록이 가져야 하는 총 부피를 계산합니다.
	float GetTotalVolumeAtSurfaceWorldZ(const TArray<UUOUWaterBasinTargetComponent*>& Targets, float SurfaceWorldZ) const;

	// 공통 SurfaceWorldZ를 기준으로 각 Target의 CurrentWaterVolume을 다시 계산합니다.
	void ApplyGroupSurfaceToTargets(const TArray<UUOUWaterBasinTargetComponent*>& Group, float SurfaceWorldZ);

	// CurrentWaterVolume에서 Depth, FillRatio, SurfaceWorldZ를 다시 계산합니다.
	void UpdateCachedWaterState();

	// 그룹 합산 정보를 그룹에 속한 각 Target의 LastGroup... 런타임 값에 복사합니다.
	void UpdateGroupRuntimeCache(const FUOUWaterBasinGroupDebugData& GroupData);

	// 그룹에 속한 모든 Target의 OnWaterStateChanged를 호출합니다.
	void BroadcastGroupChanged(const TArray<UUOUWaterBasinTargetComponent*>& Group);

	// 실제 수위 변화 여부와 무관하게 물 입력이 들어왔음을 대상 범위에 알립니다.
	void NotifyWaterInputReceived(const FUOUWaterBasinInputContext& InputContext);

	// 현재 RuntimeDebugTarget에 해당하는 경우 디버그 문자열과 연결선을 그립니다.
	void DrawRuntimeDebug();

	// 현재 Target이 최대로 물을 채울 수 있는 영역을 DebugBox로 표시합니다.
	void DrawMaxWaterCapacityDebugBox() const;

	// 최대 물 영역 DebugBox에 사용할 중심, 크기, 회전을 계산합니다.
	bool BuildMaxWaterCapacityDebugBox(FVector& OutCenter, FVector& OutExtent, FQuat& OutRotation) const;

	// 특정 Target 또는 연결 그룹의 수치 디버그 문자열을 그립니다.
	void DrawTargetDebugString() const;

	// 현재 Target에 직접 연결된 Target들만 선으로 표시합니다.
	void DrawSpecificTargetConnections() const;

	// 현재 Target이 속한 연결 그룹 내부의 직접 연결선을 표시합니다.
	void DrawConnectedGroupConnections() const;

	// 디버그 선/문자열의 기준 위치로 사용할 Basin 중심을 반환합니다.
	FVector GetDebugCenterWorld() const;

	// 디버그 문자열이 겹치지 않도록 Target 또는 그룹 bounds 위쪽 위치를 반환합니다.
	FVector GetDebugLabelWorld() const;

	// 이 인스턴스가 현재 런타임 디버그 표시 대상인지 확인합니다.
	bool ShouldDrawTargetDebug() const;

	// ConnectedTargets 목록에 Other의 소유 Actor가 들어 있는지 확인합니다.
	bool IsDirectlyConnectedTo(const UUOUWaterBasinTargetComponent* Other) const;

	// 소유 Actor의 유효한 Primitive bounds를 합산해 실제 Basin 영역을 구합니다.
	bool TryGetBasinBounds(FBox& OutBounds) const;

	// 특정 SurfaceWorldZ까지 물이 찼을 때 이 Target이 가져야 하는 부피를 계산합니다.
	float GetVolumeAtSurfaceWorldZ(float SurfaceWorldZ) const;

	// 연결 그룹의 총 목표 부피에 대응하는 공통 SurfaceWorldZ를 이분 탐색으로 찾습니다.
	float SolveSurfaceWorldZForVolume(const TArray<UUOUWaterBasinTargetComponent*>& Group, float TargetVolume) const;

	// 그룹의 현재 총 부피/용량/수면 높이 디버그 데이터를 구성합니다.
	FUOUWaterBasinGroupDebugData BuildGroupDebugData(const TArray<UUOUWaterBasinTargetComponent*>& Group) const;
};
