// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UOUStageSelectTypes.generated.h"

USTRUCT(BlueprintType)
struct FUOUStageDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> Level;

	// Each entry identifies one Reward Actor in the stage. The array length is the maximum star count.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select|Rewards")
	TArray<FName> RewardIds;

	// Initial catalog state only. The player's actual unlock state belongs in progress save data.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select|Progress")
	bool bInitiallyUnlocked = false;
};
