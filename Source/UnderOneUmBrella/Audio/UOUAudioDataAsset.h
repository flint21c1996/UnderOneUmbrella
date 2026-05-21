// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Audio/UOUAudioTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "UOUAudioDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FUOUAudioEventDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "코드와 블루프린트에서 호출할 오디오 이벤트 ID입니다. 예: Umbrella.Open"))
	FName EventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "이 이벤트가 재생할 사운드입니다. Sound Wave, Sound Cue, MetaSound Source 등을 지정할 수 있습니다."))
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	EUOUAudioCategory Category = EUOUAudioCategory::SFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	EUOUAudioPlaybackMode PlaybackMode = EUOUAudioPlaybackMode::AtLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", EditCondition = "PlaybackMode == EUOUAudioPlaybackMode::BGM", EditConditionHides))
	float FadeTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float StartTime = 0.0f;
};

UCLASS(BlueprintType)
class UNDERONEUMBRELLA_API UUOUAudioDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Audio", meta = (DisplayName = "오디오 이벤트 찾기"))
	bool TryGetAudioEvent(FName EventId, FUOUAudioEventDefinition& OutEventDefinition) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TArray<FUOUAudioEventDefinition> AudioEvents;
};
