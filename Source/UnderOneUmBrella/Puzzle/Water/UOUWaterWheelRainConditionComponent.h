// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "World/Pour/UOUPourReceiverInterface.h"
#include "UOUWaterWheelRainConditionComponent.generated.h"

class USceneComponent;
class AActor;

UENUM(BlueprintType)
enum class EUOUWaterWheelRotationSpace : uint8
{
	Relative UMETA(DisplayName = "상대 회전"),
	World UMETA(DisplayName = "월드 회전")
};

USTRUCT(BlueprintType)
struct FUOUWaterWheelRainCatchPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain", meta = (ToolTip = "비를 맞는 위치로 사용할 SceneComponent입니다. 비워 두면 물레방아 회전 대상 또는 Owner 기준 Local Offset을 사용합니다."))
	FComponentReference CatchComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain", meta = (ToolTip = "Catch Component 기준 로컬 오프셋입니다. Catch Component가 없으면 회전 대상 또는 Owner 기준 오프셋으로 사용됩니다."))
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain", meta = (ClampMin = "0.0", ToolTip = "이 위치에 닿는 비 입력의 가중치입니다."))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain", meta = (ClampMin = "0.0", ToolTip = "비가 이 위치에 얼마나 넓게 걸쳤는지 계산할 때 사용하는 반경입니다."))
	float CoverageRadius = 75.0f;
};

USTRUCT(BlueprintType)
struct FUOUWaterWheelRainCatchSample
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Water Wheel|Rain")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Water Wheel|Rain")
	float Weight = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Wheel|Rain")
	float CoverageRadius = 75.0f;
};

USTRUCT(BlueprintType)
struct FUOUWaterWheelRainInputContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain", meta = (ClampMin = "0.0"))
	float Strength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain")
	FVector WorldDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain")
	bool bHasValidWorldLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain")
	TObjectPtr<AActor> InstigatorActor = nullptr;
};

UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Water Wheel Rain Condition"))
class UNDERONEUMBRELLA_API UUOUWaterWheelRainConditionComponent : public UUOUPuzzleConditionSourceComponent, public IUOUPourReceiver
{
	GENERATED_BODY()

public:
	UUOUWaterWheelRainConditionComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain", meta = (ToolTip = "켜져 있으면 RainArea에서 전달한 빗물 입력으로 물레방아가 회전합니다."))
	bool bRainInputEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain", meta = (ToolTip = "비를 맞는 위치들입니다. 위치에 따라 회전 방향이 달라집니다."))
	TArray<FUOUWaterWheelRainCatchPoint> RainCatchPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rain", meta = (ToolTip = "Catch Point가 없을 때 Owner 위치를 비 입력 위치로 사용할지 결정합니다."))
	bool bUseOwnerLocationWhenNoCatchPoints = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Target", meta = (ToolTip = "실제로 회전시킬 물레방아 SceneComponent입니다. 비워 두면 이름/태그로 찾고, 그래도 없으면 Owner Root를 사용합니다."))
	TObjectPtr<USceneComponent> RotationTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Target", meta = (ToolTip = "Rotation Target을 자동으로 찾을 때 사용할 Component 이름 또는 태그입니다."))
	FName RotationTargetComponentName = TEXT("WaterWheel");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Target", meta = (ToolTip = "회전 중심으로 사용할 SceneComponent입니다. 비워 두면 Rotation Target의 위치를 사용합니다."))
	TObjectPtr<USceneComponent> RotationCenterComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Target", meta = (ToolTip = "Rotation Center를 자동으로 찾을 때 사용할 Component 이름 또는 태그입니다."))
	FName RotationCenterComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Target", meta = (ToolTip = "명시적인 회전 대상이 없을 때 Owner Root를 회전 대상으로 사용할지 결정합니다."))
	bool bUseOwnerRootWhenTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rotation", meta = (ToolTip = "회전축입니다. Relative에서는 회전 대상의 로컬축, World에서는 월드축으로 해석합니다."))
	FVector RotationAxis = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rotation", meta = (ToolTip = "회전 적용 공간입니다."))
	EUOUWaterWheelRotationSpace RotationSpace = EUOUWaterWheelRotationSpace::Relative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rotation", meta = (ClampMin = "0.0", ToolTip = "빗물이 충분히 한 방향으로 들어올 때의 최대 회전 속도입니다. 초당 각도 단위입니다."))
	float MaxRotationSpeedDegreesPerSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rotation", meta = (ClampMin = "0.0001", ToolTip = "이 값만큼의 빗물 입력 강도가 최대 회전 속도에 대응됩니다. RainArea의 RainFillRate가 1이면 기본값 1과 맞습니다."))
	float RainInputStrengthForMaxSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Pour", meta = (ToolTip = "켜져 있으면 우산에서 붓는 물 입력으로 물레방아를 더 빠르게 회전시킵니다."))
	bool bPouredWaterInputEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Pour", meta = (ClampMin = "0.0001", ToolTip = "이 값만큼의 물 붓기 입력 강도가 기본 최대 회전 속도에 대응됩니다."))
	float PouredWaterInputStrengthForMaxSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Pour", meta = (ClampMin = "0.0", ToolTip = "물 붓기 입력이 비 입력보다 얼마나 더 빠른 목표 속도를 만드는지 정합니다."))
	float PouredWaterSpeedMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Pour", meta = (ClampMin = "0.0", ToolTip = "비와 물 붓기 입력을 합산했을 때 허용할 최대 회전 속도입니다."))
	float MaxBoostedRotationSpeedDegreesPerSecond = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rotation", meta = (ClampMin = "0.0", ToolTip = "비를 맞기 시작했을 때 목표 속도까지 올라가는 가속도입니다."))
	float AccelerationDegreesPerSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rotation", meta = (ClampMin = "0.0", ToolTip = "비가 막혔거나 반대 방향 입력이 들어왔을 때 속도가 줄어드는 감속도입니다."))
	float DecelerationDegreesPerSecond = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rotation", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "중심부에 가까운 입력이나 회전력이 거의 없는 입력을 무시하기 위한 토크 데드존입니다."))
	float TorqueDeadZone = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rotation", meta = (ToolTip = "회전 방향을 반대로 뒤집습니다. 메쉬 축이 반대로 잡혔을 때 사용합니다."))
	bool bInvertRotationDirection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Rotation", meta = (ToolTip = "켜면 여러 Catch Point 입력을 평균 내고, 끄면 입력 강도를 합산한 뒤 최대 속도로 클램프합니다."))
	bool bNormalizeInputByTotalWeight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Stop Condition", meta = (ClampMin = "0.0", ToolTip = "이 시간 안에 새 비 입력이 들어오지 않으면 비가 막힌 것으로 봅니다."))
	float RainInputGraceTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Stop Condition", meta = (ClampMin = "0.0", ToolTip = "현재 속도가 이 값 이하이면 정지 상태 후보로 봅니다."))
	float StoppedSpeedThreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Stop Condition", meta = (ClampMin = "0.0", ToolTip = "정지 상태가 이 시간만큼 유지되어야 조건을 만족합니다."))
	float StoppedConfirmTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Stop Condition", meta = (ToolTip = "켜면 한 번이라도 실제로 돈 뒤 멈춰야 조건을 만족합니다. 게임 시작 직후 문이 열리는 것을 막는 기본 안전장치입니다."))
	bool bRequireEverSpunBeforeSatisfied = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Stop Condition", meta = (ToolTip = "켜면 비 입력이 완전히 끊긴 상태에서 멈춰야 조건을 만족합니다."))
	bool bRequireNoRainInputForStoppedCondition = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	float CurrentRotationSpeedDegreesPerSecond = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	float TargetRotationSpeedDegreesPerSecond = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	float CurrentRotationAngleDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	float LastSignedRainInput = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	float LastRainInputStrength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	float LastSignedPouredWaterInput = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	float LastPouredWaterInputStrength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	bool bHasRainInputRecently = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	bool bHasPouredWaterInputRecently = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	bool bHasWaterInputRecently = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	bool bHasEverSpun = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	float StoppedConditionElapsedTime = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Water Wheel|Rain")
	void ReceiveRainInput(const FUOUWaterWheelRainInputContext& InputContext);

	UFUNCTION(BlueprintCallable, Category = "Water Wheel|Pour")
	void ReceivePouredWaterInput(const FUOUWaterWheelRainInputContext& InputContext);

	UFUNCTION(BlueprintCallable, Category = "Water Wheel|Rain")
	void GetRainCatchSamples(TArray<FUOUWaterWheelRainCatchSample>& OutSamples) const;

	UFUNCTION(BlueprintPure, Category = "Water Wheel|Rain")
	bool CanReceiveRainInput() const;

	UFUNCTION(BlueprintPure, Category = "Water Wheel|Pour")
	bool CanReceivePouredWaterInput() const;

	UFUNCTION(BlueprintCallable, Category = "Water Wheel|Runtime")
	void ResetWaterWheelState(bool bClearEverSpun = true, bool bApplyBaseRotation = true);

	virtual bool CanAcceptPour_Implementation(const FUOUPourInputContext& Context) const override;
	virtual FUOUPourReceiveResult TryReceivePour_Implementation(const FUOUPourInputContext& Context) override;
	virtual int32 GetPourReceivePriority_Implementation() const override;
	virtual bool CanAcceptPourAtLocation_Implementation(const FUOUPourInputContext& Context) const override;

protected:
	void SanitizeSettings();
	void CacheBaseRotationIfNeeded();
	void ConsumePendingRainInput();
	void UpdateRotationSpeed(float DeltaTime);
	void ApplyRotationAngle(float AngleDegrees);
	void RefreshStoppedCondition(float DeltaTime);

	USceneComponent* ResolveRotationTargetComponent() const;
	USceneComponent* ResolveRotationCenterComponent() const;
	USceneComponent* FindSceneComponentByNameOrTag(FName ComponentName) const;
	FVector ResolveWheelCenterWorldLocation() const;
	FVector ResolveWorldRotationAxis(const USceneComponent* TargetComponent) const;
	FVector ResolveFallbackCatchWorldLocation() const;
	float CalculateSignedRainContribution(const FUOUWaterWheelRainInputContext& InputContext) const;
	bool HasRecentRainInput() const;
	bool HasRecentPouredWaterInput() const;

private:
	bool bHasCachedBaseRotation = false;
	FQuat BaseRelativeRotation = FQuat::Identity;
	FQuat BaseWorldRotation = FQuat::Identity;
	float LastRainInputTimestamp = -BIG_NUMBER;
	float LastPouredWaterInputTimestamp = -BIG_NUMBER;
	bool bHasPendingRainInput = false;
	float PendingSignedRainInput = 0.0f;
	float PendingRainInputWeight = 0.0f;
	bool bHasPendingPouredWaterInput = false;
	float PendingSignedPouredWaterInput = 0.0f;
	float PendingPouredWaterInputWeight = 0.0f;
};
