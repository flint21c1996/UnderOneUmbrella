// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/UOUUITypes.h"
#include "UOUDialogueSequenceData.generated.h"

// Data asset that stores NPC or situation dialogue lines.
// C++ controls flow, while Blueprint/DataAsset content fills the actual text.
UCLASS(BlueprintType)
class UNDERONEUMBRELLA_API UUOUDialogueSequenceData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FUOUDialogueLine> Lines;

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool HasValidLines() const;
};