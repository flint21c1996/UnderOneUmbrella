// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UOUPlayerProgressSaveGame.generated.h"

/** 스테이지 하나의 영구 진행 기록입니다. 이 레코드를 소유하는 SaveGame이 StageId로 구분해 보관합니다. */
USTRUCT(BlueprintType)
struct FUOUStageProgressRecord
{
	GENERATED_BODY()

	/** 스테이지 클리어 시 획득이 확정된 고유 Reward ID 목록입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Progress|Stage")
	TArray<FName> CollectedRewardIds;

	/** 플레이어가 이 스테이지를 한 번 이상 클리어했는지를 나타냅니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Progress|Stage")
	bool bCompleted = false;
};

/** 설정 데이터와 별도로 저장하는 플레이어 개인 게임 진행도입니다. */
UCLASS()
class UNDERONEUMBRELLA_API UUOUPlayerProgressSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** StageId로 사용하는 고정된 DataTable 행 이름을 키로 삼아 스테이지별 진행 기록을 보관합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Progress|Stage")
	TMap<FName, FUOUStageProgressRecord> StageProgress;
};
