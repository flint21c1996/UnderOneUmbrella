// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "World/Wind/UOUWindTypes.h"
#include "UOUWindReceivableInterface.generated.h"

UINTERFACE(BlueprintType)
class UUOUWindReceivableInterface : public UInterface
{
	GENERATED_BODY()
};

// 플레이어, 물리 오브젝트, 퍼즐 장치가 같은 방식으로 바람을 받을 수 있게 합니다.
class IUOUWindReceivableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wind|Receiver")
	void ReceiveWind(const FUOUWindExposureData& WindData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wind|Receiver")
	FVector GetWindReceiverLocation() const;
};
