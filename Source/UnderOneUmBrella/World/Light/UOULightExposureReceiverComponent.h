// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightExposureTypes.h"
#include "World/Light/UOULightReceivableInterface.h"
#include "UOULightExposureReceiverComponent.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUOULightExposureReceivedSignature, const FUOULightExposureData&, ExposureData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUOULightExposureStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUOULightExposureEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUOULightTemperatureChangedSignature, float, NewTemperature, float, PreviousTemperature);

UCLASS(ClassGroup=(Light), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Exposure Receiver"))
class UUOULightExposureReceiverComponent : public UActorComponent, public IUOULightReceivableInterface
{
	GENERATED_BODY()

public:
	UUOULightExposureReceiverComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent", DisplayName = "Receiver Transform"))
	FComponentReference ReceiverTransformReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver")
	bool bAutoFindReceiverTransform = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature")
	bool bStartAtAmbientTemperature = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature")
	float AmbientTemperature = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature")
	float CurrentTemperature = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature")
	float MinTemperature = -50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature")
	float MaxTemperature = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ClampMin = "0.0"))
	float TemperatureRisePerIntensity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ClampMin = "0.0"))
	float TemperatureRecoveryRate = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature")
	bool bRecoverToAmbientWhenNotExposed = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.0"))
	float ExposureEndGraceTime = 0.15f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	bool bIsReceivingLight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	float LastExposureIntensity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	FString LastExposureSourceName = TEXT("None");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug")
	bool bDrawTemperatureDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug")
	FVector TemperatureDebugOffset = FVector(0.0f, 0.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ClampMin = "0.0"))
	float TemperatureDebugDrawTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ClampMin = "0.1"))
	float TemperatureDebugTextScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug")
	FColor TemperatureDebugColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug")
	FColor ExposedTemperatureDebugColor = FColor::Orange;

	UFUNCTION(BlueprintCallable, Category = "Light|Temperature")
	void SetTemperature(float NewTemperature);

	UFUNCTION(BlueprintCallable, Category = "Light|Temperature")
	void ApplyTemperatureDelta(float DeltaTemperature);

	UFUNCTION(BlueprintPure, Category = "Light|Exposure")
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
