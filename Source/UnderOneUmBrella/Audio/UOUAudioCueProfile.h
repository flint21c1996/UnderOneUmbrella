// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UOUAudioCueProfile.generated.h"

USTRUCT(BlueprintType)
struct FUOUAudioCueDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue", meta = (ToolTip = "게임플레이 코드에서 호출할 의미적 Cue ID입니다. 예: Open, Close, Unlock, Note.C"))
	FName CueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue", meta = (ToolTip = "오디오 이벤트 DataAsset에 등록된 실제 Audio Event ID입니다. 예: Door.Wood.Open"))
	FName AudioEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue", meta = (ToolTip = "관리형 루프 사운드를 구분할 인스턴스 ID입니다. 비워두면 컴포넌트의 기본 인스턴스 ID를 사용합니다."))
	FName InstanceId = NAME_None;
};

UCLASS(BlueprintType)
class UNDERONEUMBRELLA_API UUOUAudioCueProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Audio|Cue", meta = (DisplayName = "오디오 Cue 찾기"))
	bool TryGetCue(FName CueId, FUOUAudioCueDefinition& OutCueDefinition) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue", meta = (ToolTip = "오브젝트 종류별 Cue와 실제 Audio Event ID의 매핑입니다. 같은 Cue ID가 여러 개 있으면 가장 먼저 등록된 항목이 사용됩니다."))
	TArray<FUOUAudioCueDefinition> Cues;
};
