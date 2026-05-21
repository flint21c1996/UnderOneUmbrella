// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUAudioTypes.generated.h"

UENUM(BlueprintType)
enum class EUOUAudioCategory : uint8
{
	Master UMETA(DisplayName = "마스터"),
	BGM UMETA(DisplayName = "배경음"),
	SFX UMETA(DisplayName = "효과음"),
	UI UMETA(DisplayName = "UI"),
	Ambience UMETA(DisplayName = "환경음")
};

UENUM(BlueprintType)
enum class EUOUAudioPlaybackMode : uint8
{
	BGM UMETA(DisplayName = "배경음"),
	TwoDimensional UMETA(DisplayName = "2D 재생"),
	AtLocation UMETA(DisplayName = "위치 재생")
};
