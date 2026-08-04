// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	bool bUnlocked = true;
};
