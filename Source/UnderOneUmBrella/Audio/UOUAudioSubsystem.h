// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Audio/UOUAudioDataAsset.h"
#include "Audio/UOUAudioTypes.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UOUAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class USoundClass;
class USoundMix;
class UUOUAudioDataAsset;
class UUOUAudioSettingsSaveGame;

DECLARE_LOG_CATEGORY_EXTERN(LogUOUAudio, Log, All);

UCLASS(Config=Game)
class UNDERONEUMBRELLA_API UUOUAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM", meta = (DisplayName = "배경음 재생", ClampMin = "0.0"))
	void PlayBGM(USoundBase* Sound, float FadeTime = 1.0f, float StartTime = 0.0f, float VolumeMultiplier = 1.0f, float PitchMultiplier = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM", meta = (DisplayName = "배경음 정지", ClampMin = "0.0"))
	void StopBGM(float FadeTime = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Audio|BGM", meta = (DisplayName = "배경음 재생 중"))
	bool IsBGMPlaying() const;

	UFUNCTION(BlueprintPure, Category = "Audio|BGM", meta = (DisplayName = "현재 배경음 이벤트 ID"))
	FName GetCurrentBGMEventId() const { return CurrentBGMEventId; }

	UFUNCTION(BlueprintCallable, Category = "Audio|Playback", meta = (DisplayName = "UI 사운드 재생"))
	void PlayUISound(USoundBase* Sound, float VolumeMultiplier = 1.0f, float PitchMultiplier = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Playback", meta = (DisplayName = "효과음 재생"))
	void PlaySFXAtLocation(USoundBase* Sound, FVector Location, float VolumeMultiplier = 1.0f, float PitchMultiplier = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Playback", meta = (DisplayName = "환경음 재생"))
	void PlayAmbienceAtLocation(USoundBase* Sound, FVector Location, float VolumeMultiplier = 1.0f, float PitchMultiplier = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Events", meta = (DisplayName = "오디오 데이터 설정"))
	void SetAudioData(UUOUAudioDataAsset* InAudioData);

	UFUNCTION(BlueprintCallable, Category = "Audio|Events", meta = (DisplayName = "오디오 이벤트 재생"))
	bool PlayAudioEvent(FName EventId, FVector Location = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Audio|Events", meta = (DisplayName = "오디오 이벤트 인스턴스 재생"))
	bool PlayAudioEventInstance(FName EventId, FName InstanceId, FVector Location = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Audio|Events", meta = (DisplayName = "관리형 오디오 이벤트 인스턴스 재생"))
	bool PlayManagedAudioEventInstance(FName EventId, FName InstanceId, FVector Location = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Audio|Events", meta = (DisplayName = "위치 오디오 이벤트 재생"))
	bool PlayAudioEventAtLocation(FName EventId, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Audio|Events", meta = (DisplayName = "2D 오디오 이벤트 재생"))
	bool PlayAudioEvent2D(FName EventId);

	UFUNCTION(BlueprintCallable, Category = "Audio|Events", meta = (DisplayName = "오디오 이벤트 정지"))
	bool StopAudioEvent(FName EventId, FName InstanceId = NAME_None, float OverrideFadeOutTime = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Events", meta = (DisplayName = "관리 중인 오디오 이벤트 모두 정지"))
	void StopAllManagedAudioEvents(float FadeOutTime = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Audio|Events", meta = (DisplayName = "오디오 이벤트 존재 여부"))
	bool HasAudioEvent(FName EventId);

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings", meta = (DisplayName = "볼륨 설정", ClampMin = "0.0", ClampMax = "1.0"))
	void SetCategoryVolume(EUOUAudioCategory Category, float Volume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Audio|Settings", meta = (DisplayName = "볼륨 가져오기"))
	float GetCategoryVolume(EUOUAudioCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Audio|Settings", meta = (DisplayName = "최종 볼륨 가져오기"))
	float GetEffectiveCategoryVolume(EUOUAudioCategory Category) const;

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings", meta = (DisplayName = "오디오 설정 저장"))
	void SaveAudioSettings();

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings", meta = (DisplayName = "오디오 설정 다시 불러오기"))
	void ReloadAudioSettings();

private:
	UUOUAudioSettingsSaveGame* CreateDefaultAudioSettings() const;
	void LoadAudioSettings();
	void PlayBGMInternal(FName EventId, USoundBase* Sound, float FadeTime, float StartTime, float VolumeMultiplier, float PitchMultiplier);
	void ApplyAllCategoryVolumes(float FadeTime);
	void ApplyCategoryVolume(EUOUAudioCategory Category, float FadeTime);
	void UpdateCurrentBGMVolume();
	void UpdateManagedAudioVolumes();
	bool PlayAudioEventDefinition(const FUOUAudioEventDefinition& AudioEvent, FVector Location, bool bForceTwoDimensional, FName InstanceId);
	bool PlayManagedAudioEventDefinition(const FUOUAudioEventDefinition& AudioEvent, USoundBase* Sound, FVector Location, bool bForceTwoDimensional, FName InstanceId);
	void FinishManagedAudioStop(FName ManagedAudioKey, UAudioComponent* AudioComponent);
	float GetManagedPlaybackVolume(EUOUAudioCategory Category, float VolumeMultiplier) const;
	FName BuildManagedAudioKey(FName EventId, FName InstanceId) const;
	UUOUAudioDataAsset* GetAudioData();
	USoundMix* GetDefaultSoundMix();
	USoundClass* GetSoundClassForCategory(EUOUAudioCategory Category);
	const UUOUAudioSettingsSaveGame* GetAudioSettings() const;
	UUOUAudioSettingsSaveGame* GetMutableAudioSettings();

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Settings")
	FString SaveSlotName = TEXT("UOUAudioSettings");

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Settings", meta = (ClampMin = "0"))
	int32 SaveUserIndex = 0;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Playback", meta = (ToolTip = "서브시스템이 직접 재생한 사운드에 저장된 볼륨 값을 곱합니다. SoundClass만으로 볼륨을 관리할 때는 끄세요."))
	bool bApplyManagedPlaybackVolume = true;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Events", meta = (ToolTip = "기본 오디오 이벤트 DataAsset입니다. 예: /Game/UOU/Audio/DA_UOUAudioEvents.DA_UOUAudioEvents"))
	TSoftObjectPtr<UUOUAudioDataAsset> DefaultAudioData;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Mix", meta = (ToolTip = "볼륨 조절에 사용할 기본 SoundMix입니다. 비워두면 저장 값만 사용합니다."))
	TSoftObjectPtr<USoundMix> DefaultSoundMix;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Mix")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Mix")
	TSoftObjectPtr<USoundClass> BGMSoundClass;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Mix")
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Mix")
	TSoftObjectPtr<USoundClass> UISoundClass;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Audio|Mix")
	TSoftObjectPtr<USoundClass> AmbienceSoundClass;

	UPROPERTY(Transient)
	TObjectPtr<UUOUAudioSettingsSaveGame> AudioSettings;

	UPROPERTY(Transient)
	TObjectPtr<UUOUAudioDataAsset> RuntimeAudioData;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentBGMComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentBGMSound;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UAudioComponent>> ManagedAudioComponents;

	UPROPERTY(Transient)
	TMap<FName, EUOUAudioCategory> ManagedAudioCategories;

	UPROPERTY(Transient)
	TMap<FName, float> ManagedAudioVolumeMultipliers;

	UPROPERTY(Transient)
	TMap<FName, float> ManagedAudioFadeOutTimes;

	FName CurrentBGMEventId = NAME_None;

	float CurrentBGMVolumeMultiplier = 1.0f;
};
