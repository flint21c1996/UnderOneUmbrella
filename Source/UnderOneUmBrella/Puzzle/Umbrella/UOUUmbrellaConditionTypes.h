// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUUmbrellaConditionTypes.generated.h"

// 퍼즐 조건에서 우산 방향을 검사할지 결정하는 필터입니다.
UENUM(BlueprintType)
enum class EUOUUmbrellaDirectionRequirement : uint8
{
	Any,
	Normal,
	Reversed
};
