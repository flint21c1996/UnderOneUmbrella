// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UOURewardPresentationLayoutRow.generated.h"

class UUOURewardPresentationWidget;

/**
 * Reward Presentation Key와 실제 연출 Widget Blueprint를 연결하는 DataTable 행입니다.
 * Cue의 Presentation Row가 이 행을 직접 선택합니다.
 */
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOURewardPresentationLayoutRow : public FTableRowBase
{
	GENERATED_BODY()

	// 해당 Presentation Key가 실행할 공통 Reward Presentation Widget 파생 클래스입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward Presentation")
	TSubclassOf<UUOURewardPresentationWidget> WidgetClass;
};
