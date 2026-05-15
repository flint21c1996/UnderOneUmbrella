// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "World/Light/UOULightExposureTypes.h"
#include "UOULightReceivableInterface.generated.h"

UINTERFACE(BlueprintType, meta = (ToolTip = "게임플레이용 빛 노출을 받을 수 있는 오브젝트가 구현하는 인터페이스입니다."))
class UUOULightReceivableInterface : public UInterface
{
	GENERATED_BODY()
};

class IUOULightReceivableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Light", meta = (ToolTip = "이 수신체로 게임플레이 빛을 트레이스할 때 사용할 월드 위치를 반환합니다."))
	FVector GetLightReceiverPosition() const;
	virtual FVector GetLightReceiverPosition_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Light", meta = (ToolTip = "게임플레이 빛이 이 수신체에 도달했을 때 호출됩니다."))
	void ReceiveLightExposure(const FUOULightExposureData& ExposureData);
	virtual void ReceiveLightExposure_Implementation(const FUOULightExposureData& ExposureData);
};
