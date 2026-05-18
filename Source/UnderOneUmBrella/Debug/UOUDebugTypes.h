// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUDebugTypes.generated.h"

// 디버그 시스템에서 공통으로 다루는 표시 영역입니다.
// 새 도메인이 생기면 우선 이 카테고리에 추가하고, 세부 옵션은 별도 Controller Component에서 관리합니다.
UENUM(BlueprintType)
enum class EUOUDebugCategory : uint8
{
	Player UMETA(DisplayName = "플레이어"),
	NPC UMETA(DisplayName = "NPC"),
	Puzzle UMETA(DisplayName = "퍼즐"),
	Interaction UMETA(DisplayName = "상호작용"),
	VFX UMETA(DisplayName = "VFX"),
	Performance UMETA(DisplayName = "성능"),
	System UMETA(DisplayName = "시스템")
};
