// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UOUStageDefinitionSettings.generated.h"

class UDataTable;

/** 프로젝트 전체에서 사용할 스테이지 정의 DataTable을 한 곳에 보관합니다. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "UOU Stage Definition"))
class UNDERONEUMBRELLA_API UUOUStageDefinitionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Reward Actor의 에디터 선택 목록을 제공할 스테이지 정의 DataTable입니다. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Stage Definition")
	TSoftObjectPtr<UDataTable> StageDefinitionTable;
};
