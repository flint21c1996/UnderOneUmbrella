// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UOUStageSelectTypes.generated.h"

USTRUCT(BlueprintType)
struct FUOUStageDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	FName StageId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> Level;

	/** Reward IDs authored for this stage. Each ID represents one collectible reward. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select|Rewards")
	TArray<FName> RewardIds;

	/** Total rewards available in this stage. Derived from RewardIds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select|Rewards")
	int32 TotalRewardCount = 0;

	/** Rewards permanently collected by the player. A future progress system will populate this value. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select|Rewards")
	int32 CollectedRewardCount = 0;

	/** Rewards the player still needs. A future progress system will populate this value. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select|Rewards")
	int32 MissingRewardCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	bool bUnlocked = true;
};

/** Authoring row stored in the stage-definition DataTable. The DataTable row name is the stage ID. */
USTRUCT(BlueprintType)
struct FUOUStageDefinitionRow : public FTableRowBase
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

	/** One entry per Reward Actor placed in the stage. IDs must be unique within the stage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select|Rewards")
	TArray<FName> RewardIds;

	/** Initial authoring value. A future save/progression system can override it at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	bool bUnlocked = true;
};
