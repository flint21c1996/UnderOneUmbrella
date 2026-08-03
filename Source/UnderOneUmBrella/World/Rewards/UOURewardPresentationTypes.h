// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UOURewardPresentationTypes.generated.h"

class UTexture2D;

// Motion Cue를 어느 시스템에서 처리할지 구분합니다.
UENUM(BlueprintType)
enum class EUOURewardMotionCueChannel : uint8
{
	Presentation,
	Feedback
};

// Feedback Cue가 실행할 플레이어 연출 동작을 목록으로 제한합니다.
UENUM(BlueprintType)
enum class EUOURewardFeedbackCueAction : uint8
{
	PlayPlayerAnimation UMETA(DisplayName = "플레이어 애니메이션 재생"),
	SpawnNiagara UMETA(DisplayName = "나이아가라 생성"),
	StartCameraFocus UMETA(DisplayName = "카메라 포커스 시작")
};

// Reward 수집 움직임의 특정 시간에 담당 시스템으로 전달되는 신호입니다.
USTRUCT(BlueprintType, meta = (DisplayName = "UOU Reward Motion Cue"))
struct UNDERONEUMBRELLA_API FUOURewardPresentationCue
{
	GENERATED_BODY()

	// Motion 타임라인이 이 요청의 실행 시점을 연결할 때 사용하는 내부 식별자입니다.
	UPROPERTY()
	FGuid RequestId;

	// 이 Cue를 처리할 시스템입니다. Feedback Cue는 UI Host로 전달되지 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Cue")
	EUOURewardMotionCueChannel Channel =
		EUOURewardMotionCueChannel::Presentation;

	// Presentation 채널에서 실행할 Layout DataTable 행입니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Motion|Cue",
		meta = (
			EditCondition = "Channel == EUOURewardMotionCueChannel::Presentation",
			EditConditionHides,
			RowType = "/Script/UnderOneUmBrella.UOURewardPresentationLayoutRow"))
	FDataTableRowHandle PresentationRow;

	// Feedback 채널에서 실행할 개별 동작입니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Reward|Motion|Cue",
		meta = (
			EditCondition = "Channel == EUOURewardMotionCueChannel::Feedback",
			EditConditionHides))
	EUOURewardFeedbackCueAction FeedbackAction =
		EUOURewardFeedbackCueAction::PlayPlayerAnimation;

	// HUD가 미리 생성한 Presentation Widget을 찾을 Row Name을 반환합니다.
	FName GetPresentationKey() const
	{
		return PresentationRow.RowName;
	}
};

// MotionComponent가 CueRequest와 실행 시간을 연결해 보존하는 스케줄 항목입니다.
USTRUCT(BlueprintType, meta = (DisplayName = "UOU Reward Motion Cue Timing"))
struct UNDERONEUMBRELLA_API FUOURewardMotionCueTiming
{
	GENERATED_BODY()

	// FeedbackComponent CueRequest의 내부 식별자입니다. 타임라인 UI가 자동으로 관리합니다.
	UPROPERTY()
	FGuid RequestId;

	// 수집 움직임이 시작된 뒤 CueRequest를 실행할 시간입니다.
	UPROPERTY()
	float TriggerTime = 0.0f;
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
