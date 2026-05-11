// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOULightTemperatureConditionComponent.generated.h"

class UUOULightExposureReceiverComponent;

UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Temperature Condition"))
class UUOULightTemperatureConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOULightTemperatureConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light")
	bool bAutoFindLightReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light", meta = (UseComponentPicker, AllowedClasses = "/Script/UnderOneUmBrella.UOULightExposureReceiverComponent", DisplayName = "Light Receiver"))
	FComponentReference LightReceiverReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light")
	float ActivateTemperature = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light")
	float DeactivateTemperature = 25.0f;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	TObjectPtr<UUOULightExposureReceiverComponent> LightReceiver = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
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
