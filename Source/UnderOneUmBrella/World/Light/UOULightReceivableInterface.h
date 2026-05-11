// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "World/Light/UOULightExposureTypes.h"
#include "UOULightReceivableInterface.generated.h"

UINTERFACE(BlueprintType)
class UUOULightReceivableInterface : public UInterface
{
	GENERATED_BODY()
};

class IUOULightReceivableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Light")
	FVector GetLightReceiverPosition() const;
	virtual FVector GetLightReceiverPosition_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Light")
	void ReceiveLightExposure(const FUOULightExposureData& ExposureData);
	virtual void ReceiveLightExposure_Implementation(const FUOULightExposureData& ExposureData);
};
