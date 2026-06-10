// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UObject/Interface.h"
#include "UOUPuzzleResultCompletionState.generated.h"

// Optional interface for result receivers that can report whether a result action has finished.
UINTERFACE(BlueprintType)
class UUOUPuzzleResultCompletionState : public UInterface
{
	GENERATED_BODY()
};

class IUOUPuzzleResultCompletionState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Puzzle|Result")
	bool IsPuzzleResultCompleted(EOUUPuzzleResultAction Action) const;
};
