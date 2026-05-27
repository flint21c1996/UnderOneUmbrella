// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Audio/UOUAudioCueProfile.h"
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UOUAudioCueComponent.generated.h"

class UUOUAudioSubsystem;

UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUAudioCueComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUOUAudioCueComponent();

	UFUNCTION(BlueprintCallable, Category = "Audio|Cue", meta = (DisplayName = "오디오 Cue 재생"))
	bool PlayCue(FName CueId);

	UFUNCTION(BlueprintCallable, Category = "Audio|Cue", meta = (DisplayName = "위치에서 오디오 Cue 재생"))
	bool PlayCueAtLocation(FName CueId, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Audio|Cue", meta = (DisplayName = "인스턴스 오디오 Cue 재생"))
	bool PlayCueWithInstance(FName CueId, FName InstanceId, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Audio|Cue", meta = (DisplayName = "오디오 Cue 정지", ClampMin = "-1.0"))
	bool StopCue(FName CueId, float OverrideFadeOutTime = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Cue", meta = (DisplayName = "인스턴스 오디오 Cue 정지", ClampMin = "-1.0"))
	bool StopCueWithInstance(FName CueId, FName InstanceId, float OverrideFadeOutTime = -1.0f);

	UFUNCTION(BlueprintPure, Category = "Audio|Cue", meta = (DisplayName = "오디오 Cue 존재 여부"))
	bool HasCue(FName CueId) const;

	UFUNCTION(BlueprintPure, Category = "Audio|Cue", meta = (DisplayName = "Audio Event ID 찾기"))
	bool ResolveAudioEventId(FName CueId, FName& OutAudioEventId) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool TryGetCueDefinition(FName CueId, FUOUAudioCueDefinition& OutCueDefinition) const;
	bool PlayCueDefinition(const FUOUAudioCueDefinition& CueDefinition, FName OverrideInstanceId, FVector Location);
	FName ResolveInstanceId(const FUOUAudioCueDefinition& CueDefinition, FName OverrideInstanceId) const;
	FName BuildDefaultInstanceId() const;
	FName BuildPlayedCueKey(FName CueId, FName InstanceId) const;
	UUOUAudioSubsystem* GetAudioSubsystem() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue", meta = (AllowPrivateAccess = "true", ToolTip = "이 컴포넌트가 사용할 기본 Audio Cue Profile입니다. Cue ID를 실제 Audio Event ID로 매핑합니다."))
	TObjectPtr<UUOUAudioCueProfile> AudioProfile = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue", meta = (AllowPrivateAccess = "true", ToolTip = "특정 액터 인스턴스에서만 덮어쓸 Cue 매핑입니다. Profile보다 우선합니다."))
	TArray<FUOUAudioCueDefinition> CueOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue", meta = (AllowPrivateAccess = "true", ToolTip = "Cue 인스턴스 ID가 비어 있을 때 사용할 기본 인스턴스 ID입니다."))
	FName DefaultInstanceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue", meta = (AllowPrivateAccess = "true", ToolTip = "기본 인스턴스 ID가 비어 있을 때 Owner 이름을 인스턴스 ID로 사용합니다. 같은 Audio Event를 여러 액터가 루프로 재생할 때 충돌을 줄입니다."))
	bool bUseOwnerNameAsDefaultInstanceId = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue|AutoPlay", meta = (AllowPrivateAccess = "true", ToolTip = "BeginPlay 때 자동 재생할 Cue ID입니다. 비워두면 자동 재생하지 않습니다."))
	FName AutoPlayCueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue|AutoPlay", meta = (AllowPrivateAccess = "true", ToolTip = "EndPlay 때 이 컴포넌트가 재생 요청한 Cue를 정지합니다. BGM처럼 컴포넌트 수명과 별개로 유지할 사운드에는 끄세요."))
	bool bStopPlayedCuesOnEndPlay = true;

	TMap<FName, FName> PlayedCueEventIds;
};
