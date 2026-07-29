// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UOURewardPresentationLayoutRow.generated.h"

class UUOURewardPresentationWidget;

/**
 * Reward Presentation Key와 실제 연출 Widget Blueprint를 연결하는 DataTable 행입니다.
 * DataTable의 RowName을 Motion Cue의 CueId와 동일하게 사용합니다.
 */
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOURewardPresentationLayoutRow : public FTableRowBase
{
	GENERATED_BODY()

	// 해당 Presentation Key가 실행할 공통 Reward Presentation Widget 파생 클래스입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward Presentation")
	TSubclassOf<UUOURewardPresentationWidget> WidgetClass;
};
