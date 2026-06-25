// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUPourDropTypes.generated.h"

UENUM(BlueprintType)
enum class EUOUPourDropReceiverType : uint8
{
	None,
	PurePourReceiver,
	UmbrellaWaterTarget,
	WaterBasinTarget,
	WaterContainer
};
