// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOULightTemperatureConditionComponent.generated.h"

class UUOULightExposureReceiverComponent;

// 빛 수신 컴포넌트의 온도가 기준값을 넘으면 만족되는 퍼즐 조건 소스입니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Temperature Condition", ToolTip = "빛 수신 컴포넌트의 온도를 퍼즐 조건으로 변환합니다."))
class UUOULightTemperatureConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOULightTemperatureConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light", meta = (ToolTip = "명시적인 참조가 없으면 소유 액터의 UOU Light Exposure Receiver를 자동으로 찾습니다."))
	bool bAutoFindLightReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light", meta = (UseComponentPicker, AllowedClasses = "/Script/UnderOneUmBrella.UOULightExposureReceiverComponent", DisplayName = "Light Receiver", ToolTip = "이 조건을 판단할 때 사용할 빛 수신 컴포넌트입니다. Current Temperature 값을 읽습니다."))
	FComponentReference LightReceiverReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light", meta = (ToolTip = "조건을 만족하기 위해 필요한 온도입니다."))
	float ActivateTemperature = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light", meta = (ToolTip = "온도가 이 값 이하로 내려가면 조건을 다시 불만족 상태로 바꿉니다."))
	float DeactivateTemperature = 25.0f;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (ToolTip = "런타임에 실제로 연결된 빛 수신 컴포넌트입니다."))
	TObjectPtr<UUOULightExposureReceiverComponent> LightReceiver = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (ToolTip = "연결된 빛 수신 컴포넌트에서 마지막으로 읽은 온도입니다."))
	float CurrentTemperature = 0.0f;

protected:
	UFUNCTION()
	void HandleTemperatureChanged(float NewTemperature, float PreviousTemperature);

	void ResolveLightReceiver();
	void SubscribeLightReceiver();
	void UnsubscribeLightReceiver();
	void RefreshSatisfiedState();
	bool HasLightReceiverReference() const;
};
