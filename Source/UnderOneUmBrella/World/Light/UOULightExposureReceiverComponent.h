// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightExposureTypes.h"
#include "World/Light/UOULightReceivableInterface.h"
#include "UOULightExposureReceiverComponent.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUOULightExposureReceivedSignature, const FUOULightExposureData&, ExposureData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUOULightExposureStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUOULightExposureEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUOULightTemperatureChangedSignature, float, NewTemperature, float, PreviousTemperature);

// 게임플레이용 빛 노출을 받아 온도 값으로 변환하는 컴포넌트입니다.
UCLASS(ClassGroup=(Light), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Exposure Receiver", ToolTip = "게임플레이용 빛 노출을 받아 온도를 갱신합니다."))
class UUOULightExposureReceiverComponent : public UActorComponent, public IUOULightReceivableInterface, public IUOUPuzzleDebugInfoProvider
{
	GENERATED_BODY()

public:
	UUOULightExposureReceiverComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;

	virtual FVector GetLightReceiverPosition_Implementation() const override;
	virtual void ReceiveLightExposure_Implementation(const FUOULightExposureData& ExposureData) override;

	UPROPERTY(BlueprintAssignable, Category = "Light|Exposure")
	FOnUOULightExposureReceivedSignature OnLightExposureReceived;

	UPROPERTY(BlueprintAssignable, Category = "Light|Exposure")
	FOnUOULightExposureStartedSignature OnLightExposureStarted;

	UPROPERTY(BlueprintAssignable, Category = "Light|Exposure")
	FOnUOULightExposureEndedSignature OnLightExposureEnded;

	UPROPERTY(BlueprintAssignable, Category = "Light|Temperature")
	FOnUOULightTemperatureChangedSignature OnTemperatureChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent", DisplayName = "Receiver Transform", ToolTip = "빛을 받는 기준 위치로 사용할 Scene Component입니다."))
	FComponentReference ReceiverTransformReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver", meta = (ToolTip = "Receiver Transform이 비어 있으면 Primitive Component 또는 Root Component를 자동으로 사용합니다."))
	bool bAutoFindReceiverTransform = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "BeginPlay 시 Current Temperature를 Ambient Temperature 값으로 초기화합니다."))
	bool bStartAtAmbientTemperature = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "빛을 받지 않을 때 회복될 기준 온도입니다."))
	float AmbientTemperature = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "현재 게임플레이 온도입니다. 빛 노출과 회복 처리로 갱신됩니다."))
	float CurrentTemperature = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "온도가 내려갈 수 있는 최소값입니다."))
	float MinTemperature = -50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "온도가 올라갈 수 있는 최대값입니다."))
	float MaxTemperature = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ClampMin = "0.0", ToolTip = "빛 세기가 1.0일 때 초당 증가하는 온도입니다."))
	float TemperatureRisePerIntensity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ClampMin = "0.0", ToolTip = "빛을 받지 않을 때 기준 온도로 회복되는 속도입니다."))
	float TemperatureRecoveryRate = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "빛 노출이 끝난 뒤 기준 온도로 회복할지 여부입니다."))
	bool bRecoverToAmbientWhenNotExposed = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.0", ToolTip = "마지막 빛 샘플 이후 노출 종료로 판단하기까지의 유예 시간입니다."))
	float ExposureEndGraceTime = 0.15f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "현재 빛을 받고 있다고 판단되면 true입니다."))
	bool bIsReceivingLight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막으로 적용된 빛 노출 강도입니다."))
	float LastExposureIntensity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막으로 이 수신체에 빛을 준 광원 이름입니다."))
	FString LastExposureSourceName = TEXT("None");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ToolTip = "수신체 위에 온도와 빛 노출 상태를 디버그 텍스트로 표시합니다."))
	bool bDrawTemperatureDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ToolTip = "디버그 텍스트 위치에 더할 월드 오프셋입니다."))
	FVector TemperatureDebugOffset = FVector(0.0f, 0.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ClampMin = "0.0", ToolTip = "온도 디버그 텍스트의 유지 시간입니다. 0이면 한 프레임만 표시합니다."))
	float TemperatureDebugDrawTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ClampMin = "0.1", ToolTip = "온도 디버그 텍스트의 크기입니다."))
	float TemperatureDebugTextScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ToolTip = "빛을 받지 않을 때 사용할 디버그 텍스트 색상입니다."))
	FColor TemperatureDebugColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ToolTip = "빛을 받고 있을 때 사용할 디버그 텍스트 색상입니다."))
	FColor ExposedTemperatureDebugColor = FColor::Orange;

	UFUNCTION(BlueprintCallable, Category = "Light|Temperature", meta = (ToolTip = "현재 온도를 설정합니다. Min Temperature와 Max Temperature 사이로 제한됩니다."))
	void SetTemperature(float NewTemperature);

	UFUNCTION(BlueprintCallable, Category = "Light|Temperature", meta = (ToolTip = "현재 온도에 변화량을 더하고 결과를 제한합니다."))
	void ApplyTemperatureDelta(float DeltaTemperature);

	UFUNCTION(BlueprintPure, Category = "Light|Exposure", meta = (ToolTip = "현재 게임플레이용 빛에 노출되어 있으면 true를 반환합니다."))
	bool IsReceivingLight() const;

protected:
	float LastExposureWorldTime = -BIG_NUMBER;

	void ValidateTemperatureSettings();
	USceneComponent* GetReferencedReceiverTransform() const;
	USceneComponent* FindAutoReceiverTransform() const;
	void SetReceivingLight(bool bNewReceivingLight);
	void RecoverTemperature(float DeltaTime);
	void DrawTemperatureDebug() const;
};
