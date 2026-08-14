// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUWaterWheelSpeedConditionComponent.generated.h"

class UUOUWaterWheelRainConditionComponent;
class AActor;

UENUM(BlueprintType)
enum class EUOUWaterWheelSpeedConditionMode : uint8
{
	AbsoluteSpeedAtLeast UMETA(DisplayName = "방향 무관 속도 이상"),
	PositiveSpeedAtLeast UMETA(DisplayName = "양수 방향 속도 이상"),
	NegativeSpeedAtLeast UMETA(DisplayName = "음수 방향 속도 이상"),
	AbsoluteSpeedAtMost UMETA(DisplayName = "방향 무관 속도 이하"),
	PositiveSpeedAtMost UMETA(DisplayName = "양수 방향 속도 이하"),
	NegativeSpeedAtMost UMETA(DisplayName = "음수 방향 속도 이하"),
	AbsoluteSpeedInRange UMETA(DisplayName = "방향 무관 속도 범위"),
	PositiveSpeedInRange UMETA(DisplayName = "양수 방향 속도 범위"),
	NegativeSpeedInRange UMETA(DisplayName = "음수 방향 속도 범위")
};

// 물레방아가 지정 속도 이상으로 돌고 있는지를 별도 퍼즐 조건으로 노출합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Water Wheel Speed Condition"))
class UNDERONEUMBRELLA_API UUOUWaterWheelSpeedConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUWaterWheelSpeedConditionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Water Wheel|Source", meta = (ToolTip = "속도를 관찰할 물레방아 조건 컴포넌트입니다."))
	TObjectPtr<UUOUWaterWheelRainConditionComponent> ObservedWaterWheelCondition = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Water Wheel|Source", meta = (ToolTip = "직접 컴포넌트 참조로 물레방아 조건을 지정할 때 사용합니다."))
	FComponentReference WaterWheelConditionReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Source", meta = (ToolTip = "명시 참조가 없으면 같은 액터의 WaterWheelRainConditionComponent를 자동으로 찾습니다."))
	bool bAutoFindWaterWheelCondition = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Speed", meta = (ToolTip = "속도 조건을 어떤 방식으로 판정할지 정합니다."))
	EUOUWaterWheelSpeedConditionMode ConditionMode = EUOUWaterWheelSpeedConditionMode::AbsoluteSpeedAtLeast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Speed", meta = (ClampMin = "0.0", ToolTip = "조건을 만족하기 위해 필요한 초당 회전 각도입니다."))
	float RequiredSpeedDegreesPerSecond = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Speed", meta = (ClampMin = "0.0", ToolTip = "속도 범위 조건에서 사용하는 최대 속도입니다. 범위 모드가 아니면 무시됩니다."))
	float MaximumSpeedDegreesPerSecond = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Wheel|Speed", meta = (ToolTip = "켜져 있으면 매 프레임 물레방아 속도를 다시 읽어 조건 상태를 갱신합니다."))
	bool bRefreshEveryTick = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Wheel|Runtime")
	TObjectPtr<UUOUWaterWheelRainConditionComponent> ResolvedWaterWheelCondition = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Water Wheel|Speed")
	void RefreshConditionState();

protected:
	UUOUWaterWheelRainConditionComponent* ResolveWaterWheelCondition() const;
	bool EvaluateSpeedCondition(float CurrentSpeedDegreesPerSecond) const;
};
