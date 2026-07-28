// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOURewardPresentationTypes.generated.h"

class UTexture2D;

// Reward 수집 움직임의 특정 시간에 월드 연출과 UI로 전달되는 신호입니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOURewardPresentationCue
{
	GENERATED_BODY()

	// Blueprint에서 어떤 연출을 실행할지 구분하는 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation|Cue")
	FName CueId = NAME_None;

	// 수집 움직임이 시작된 뒤 Cue가 발생할 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation|Cue", meta = (ClampMin = "0.0"))
	float TriggerTime = 0.0f;

	// 텍스트 Cue처럼 추가 문자열이 필요할 때 사용하는 선택 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation|Cue", meta = (MultiLine = "true"))
	FText Text;
};

// 보상 수집 시 HUD가 결과 화면과 애니메이션을 구성하는 데 사용하는 표시 데이터입니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOURewardPresentationData
{
	GENERATED_BODY()

	// RewardActor가 수집 시작 시 자신의 고유 ID로 채우는 런타임 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Presentation")
	FName RewardId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation", meta = (MultiLine = "true"))
	FText ResultMessage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 이번 수집으로 획득한 양입니다. 누적 개수는 추후 진행도 시스템이 별도로 제공할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation", meta = (ClampMin = "1"))
	int32 AcquiredAmount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation")
	bool bShowAcquiredAmount = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation")
	bool bShowResultUI = true;

	// HUD Blueprint가 Compact, FullScreen 같은 연출 형태를 선택할 때 사용하는 식별자입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation")
	FName PresentationStyleId = TEXT("Default");

	// HUD 애니메이션의 권장 표시 시간입니다. 실제 종료 처리는 UMG가 선택할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Presentation", meta = (ClampMin = "0.0"))
	float DisplayDuration = 1.5f;
};
