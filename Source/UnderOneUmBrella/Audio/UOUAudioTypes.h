// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUAudioTypes.generated.h"

UENUM(BlueprintType)
enum class EUOUAudioCategory : uint8
{
	Master UMETA(DisplayName = "Master"),
	BGM UMETA(DisplayName = "BGM"),
	SFX UMETA(DisplayName = "SFX"),
	UI UMETA(DisplayName = "UI"),
	Ambience UMETA(DisplayName = "Ambience")
};

UENUM(BlueprintType)
enum class EUOUAudioPlaybackMode : uint8
{
	BGM UMETA(DisplayName = "BGM"),
	TwoDimensional UMETA(DisplayName = "2D"),
	AtLocation UMETA(DisplayName = "AtLocation")
};
