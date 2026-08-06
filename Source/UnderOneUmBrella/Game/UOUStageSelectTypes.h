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

	/** DataTable 행에서 복사된 Reward ID 목록입니다. 각 ID는 수집 가능한 Reward 하나를 의미합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select|Rewards")
	TArray<FName> RewardIds;

	/** 이 스테이지에서 획득 가능한 전체 Reward 수입니다. RewardIds의 원소 수로 계산합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select|Rewards")
	int32 TotalRewardCount = 0;

	/** 플레이어 진행 SaveGame에 영구 기록된 획득 Reward 수입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select|Rewards")
	int32 CollectedRewardCount = 0;

	/** 전체 Reward 중 플레이어가 아직 획득하지 못한 Reward 수입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select|Rewards")
	int32 MissingRewardCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	bool bUnlocked = true;
};

/** 스테이지 정의 DataTable에 저장하는 편집용 행입니다. DataTable 행 이름을 StageId로 사용합니다. */
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

	/** 스테이지에 배치된 Reward Actor마다 ID 하나를 작성합니다. ID는 스테이지 안에서 고유해야 합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select|Rewards")
	TArray<FName> RewardIds;

	/** 편집 단계에서 지정하는 초기 잠금 해제 값입니다. 이후 플레이어 진행도에 따라 런타임에 덮어쓸 수 있습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Select")
	bool bUnlocked = true;
};
