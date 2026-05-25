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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "코드와 블루프린트에서 호출할 오디오 이벤트 ID입니다. 예: Umbrella.Open, BGM.Title, Ambience.Campfire"))
	FName EventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "이 이벤트가 재생할 실제 사운드 에셋입니다. Sound Wave, Sound Cue, MetaSound Source 등을 지정할 수 있습니다."))
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "볼륨 설정에 사용할 분류입니다. SFX는 효과음, UI는 메뉴음, BGM은 배경음, Ambience는 모닥불/비/바람 같은 환경음에 사용합니다."))
	EUOUAudioCategory Category = EUOUAudioCategory::SFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "재생 방식입니다. BGM은 기존 배경음을 페이드로 교체하고, 2D 재생은 위치와 거리감 없이 들리며, 위치 재생은 월드 좌표에서 거리감 있게 재생합니다."))
	EUOUAudioPlaybackMode PlaybackMode = EUOUAudioPlaybackMode::AtLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (EditCondition = "PlaybackMode != EUOUAudioPlaybackMode::BGM", ToolTip = "켜져 있으면 AudioComponent를 보관해 중복 재생을 막고 나중에 정지할 수 있습니다. 실제 반복은 Sound Cue, MetaSound, Sound Wave의 루프 설정이 담당합니다."))
	bool bManagedLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ToolTip = "이 이벤트 자체의 볼륨 배율입니다. 최종 볼륨은 저장된 카테고리 볼륨과 이 값을 곱해서 계산합니다."))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ToolTip = "재생 속도와 음높이 배율입니다. 1.0은 원본 그대로, 0.8은 낮고 느리게, 1.2는 높고 빠르게 재생합니다."))
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", EditCondition = "PlaybackMode == EUOUAudioPlaybackMode::BGM || bManagedLoop", EditConditionHides, ToolTip = "BGM이나 관리형 루프 사운드가 시작될 때 서서히 들어오는 시간입니다."))
	float FadeTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", EditCondition = "bManagedLoop", EditConditionHides, ToolTip = "관리형 루프 사운드를 정지할 때 서서히 줄어드는 시간입니다."))
	float FadeOutTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ToolTip = "사운드를 처음부터 재생하지 않고 이 초 위치부터 시작합니다. 대부분의 효과음은 0으로 둡니다."))
	float StartTime = 0.0f;
};

UCLASS(BlueprintType)
class UNDERONEUMBRELLA_API UUOUAudioDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Audio", meta = (DisplayName = "오디오 이벤트 찾기"))
	bool TryGetAudioEvent(FName EventId, FUOUAudioEventDefinition& OutEventDefinition) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "프로젝트에서 사용할 오디오 이벤트 목록입니다. 같은 Event ID가 여러 개 있으면 가장 먼저 등록된 항목이 사용됩니다."))
	TArray<FUOUAudioEventDefinition> AudioEvents;
};
