// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUDebugTypes.generated.h"

// Shared debug categories used by the integrated UOU debug system.
UENUM(BlueprintType)
enum class EUOUDebugCategory : uint8
{
	Player UMETA(DisplayName = "Player"),
	NPC UMETA(DisplayName = "NPC"),
	Puzzle UMETA(DisplayName = "Puzzle"),
	Interaction UMETA(DisplayName = "Interaction"),
	VFX UMETA(DisplayName = "VFX"),
	Performance UMETA(DisplayName = "Performance"),
	System UMETA(DisplayName = "System")
};

UENUM(BlueprintType)
enum class EUOUDebugConnectionType : uint8
{
	Generic UMETA(DisplayName = "Generic"),
	PuzzleInput UMETA(DisplayName = "Puzzle Input"),
	PuzzleCondition UMETA(DisplayName = "Puzzle Condition"),
	PuzzleResult UMETA(DisplayName = "Puzzle Result")
};

USTRUCT(BlueprintType)
struct FUOUDebugConnection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	TObjectPtr<UObject> SourceObject = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	TObjectPtr<UObject> TargetObject = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	EUOUDebugConnectionType ConnectionType = EUOUDebugConnectionType::Generic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FColor Color = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ClampMin = "0.0"))
	float Thickness = 2.0f;
};
