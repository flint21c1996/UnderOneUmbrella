// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UOUPuzzleDebugInfoProvider.generated.h"

// Interface for puzzle actors or components that expose compact runtime values to the condition-group debug board.
UINTERFACE(BlueprintType)
class UUOUPuzzleDebugInfoProvider : public UInterface
{
	GENERATED_BODY()
};

class IUOUPuzzleDebugInfoProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug|Puzzle")
	TArray<FString> GetPuzzleDebugInfo() const;
};
