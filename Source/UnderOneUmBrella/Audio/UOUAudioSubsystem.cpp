// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/UOUAudioSubsystem.h"

#include "Audio/UOUAudioSettingsSaveGame.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogUOUAudio);

namespace
{
constexpr float DefaultVolumeApplyFadeTime = 0.1f;
constexpr TCHAR DefaultAudioSaveSlotName[] = TEXT("UOUAudioSettings");
}

void UUOUAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadAudioSettings();
	ApplyAllCategoryVolumes(0.0f);
}

void UUOUAudioSubsystem::Deinitialize()
{
	StopAllManagedAudioEvents(0.0f);
	StopBGM(0.0f);
	SaveAudioSettings();

	if (USoundMix* SoundMix = GetDefaultSoundMix())
	{
		UGameplayStatics::PopSoundMixModifier(this, SoundMix);
	}

	Super::Deinitialize();
}

void UUOUAudioSubsystem::PlayBGM(USoundBase* Sound, float FadeTime, float StartTime, float VolumeMultiplier, float PitchMultiplier)
{
	PlayBGMInternal(NAME_None, Sound, FadeTime, StartTime, VolumeMultiplier, PitchMultiplier);
}

void UUOUAudioSubsystem::PlayBGMInternal(FName EventId, USoundBase* Sound, float FadeTime, float StartTime, float VolumeMultiplier, float PitchMultiplier)
{
	if (Sound == nullptr)
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("PlayBGM failed because Sound is null."));
		return;
	}

	if (IsValid(CurrentBGMComponent)
		&& CurrentBGMComponent->IsPlaying()
		&& ((EventId != NAME_None && CurrentBGMEventId == EventId) || CurrentBGMSound == Sound))
	{
		CurrentBGMEventId = EventId;
		CurrentBGMVolumeMultiplier = VolumeMultiplier;
		CurrentBGMComponent->SetPitchMultiplier(PitchMultiplier);
		UpdateCurrentBGMVolume();
		return;
	}

	StopBGM(FadeTime);

	const float ClampedFadeTime = FMath::Max(0.0f, FadeTime);
	const float TargetVolume = GetManagedPlaybackVolume(EUOUAudioCategory::BGM, VolumeMultiplier);

	CurrentBGMComponent = UGameplayStatics::SpawnSound2D(this, Sound, ClampedFadeTime > 0.0f ? 0.0f : TargetVolume, PitchMultiplier, StartTime, nullptr, true, true);
	if (!IsValid(CurrentBGMComponent))
	{
		CurrentBGMSound = nullptr;
		UE_LOG(LogUOUAudio, Warning, TEXT("PlayBGM failed to spawn an audio component."));
		return;
	}

	CurrentBGMSound = Sound;
	CurrentBGMEventId = EventId;
	CurrentBGMVolumeMultiplier = VolumeMultiplier;

	if (ClampedFadeTime > 0.0f)
	{
		CurrentBGMComponent->FadeIn(ClampedFadeTime, TargetVolume, StartTime);
	}
}

void UUOUAudioSubsystem::StopBGM(float FadeTime)
{
	if (!IsValid(CurrentBGMComponent))
	{
		CurrentBGMComponent = nullptr;
		CurrentBGMSound = nullptr;
		CurrentBGMEventId = NAME_None;
		return;
	}

	const float ClampedFadeTime = FMath::Max(0.0f, FadeTime);
	if (ClampedFadeTime > 0.0f)
	{
		CurrentBGMComponent->FadeOut(ClampedFadeTime, 0.0f);
	}
	else
	{
		CurrentBGMComponent->Stop();
	}

	CurrentBGMComponent = nullptr;
	CurrentBGMSound = nullptr;
	CurrentBGMEventId = NAME_None;
}

bool UUOUAudioSubsystem::IsBGMPlaying() const
{
	return IsValid(CurrentBGMComponent) && CurrentBGMComponent->IsPlaying();
}

void UUOUAudioSubsystem::PlayUISound(USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier)
{
	if (Sound == nullptr)
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("PlayUISound failed because Sound is null."));
		return;
	}

	UGameplayStatics::PlaySound2D(this, Sound, GetManagedPlaybackVolume(EUOUAudioCategory::UI, VolumeMultiplier), PitchMultiplier);
}

void UUOUAudioSubsystem::PlaySFXAtLocation(USoundBase* Sound, FVector Location, float VolumeMultiplier, float PitchMultiplier)
{
	if (Sound == nullptr)
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("PlaySFXAtLocation failed because Sound is null."));
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, GetManagedPlaybackVolume(EUOUAudioCategory::SFX, VolumeMultiplier), PitchMultiplier);
}

void UUOUAudioSubsystem::PlayAmbienceAtLocation(USoundBase* Sound, FVector Location, float VolumeMultiplier, float PitchMultiplier)
{
	if (Sound == nullptr)
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("PlayAmbienceAtLocation failed because Sound is null."));
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, GetManagedPlaybackVolume(EUOUAudioCategory::Ambience, VolumeMultiplier), PitchMultiplier);
}

void UUOUAudioSubsystem::SetAudioData(UUOUAudioDataAsset* InAudioData)
{
	RuntimeAudioData = InAudioData;
}

bool UUOUAudioSubsystem::PlayAudioEvent(FName EventId, FVector Location)
{
	return PlayAudioEventInstance(EventId, NAME_None, Location);
}

bool UUOUAudioSubsystem::PlayAudioEventInstance(FName EventId, FName InstanceId, FVector Location)
{
	FUOUAudioEventDefinition AudioEvent;
	UUOUAudioDataAsset* AudioData = GetAudioData();
	if (AudioData == nullptr)
	{
		return false;
	}

	if (!AudioData->TryGetAudioEvent(EventId, AudioEvent))
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("Audio event '%s' was not found."), *EventId.ToString());
		return false;
	}

	return PlayAudioEventDefinition(AudioEvent, Location, false, InstanceId);
}

bool UUOUAudioSubsystem::PlayAudioEventAtLocation(FName EventId, FVector Location)
{
	return PlayAudioEvent(EventId, Location);
}

bool UUOUAudioSubsystem::PlayAudioEvent2D(FName EventId)
{
	FUOUAudioEventDefinition AudioEvent;
	UUOUAudioDataAsset* AudioData = GetAudioData();
	if (AudioData == nullptr)
	{
		return false;
	}

	if (!AudioData->TryGetAudioEvent(EventId, AudioEvent))
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("Audio event '%s' was not found."), *EventId.ToString());
		return false;
	}

	return PlayAudioEventDefinition(AudioEvent, FVector::ZeroVector, true, NAME_None);
}

bool UUOUAudioSubsystem::StopAudioEvent(FName EventId, FName InstanceId, float OverrideFadeOutTime)
{
	if (EventId.IsNone())
	{
		return false;
	}

	float ResolvedFadeOutTime = OverrideFadeOutTime;
	if (ResolvedFadeOutTime < 0.0f)
	{
		FUOUAudioEventDefinition AudioEvent;
		UUOUAudioDataAsset* AudioData = GetAudioData();
		if (AudioData != nullptr && AudioData->TryGetAudioEvent(EventId, AudioEvent))
		{
			ResolvedFadeOutTime = AudioEvent.PlaybackMode == EUOUAudioPlaybackMode::BGM
				? AudioEvent.FadeTime
				: AudioEvent.FadeOutTime;
		}
	}

	if (CurrentBGMEventId == EventId && IsBGMPlaying())
	{
		StopBGM(FMath::Max(0.0f, ResolvedFadeOutTime));
		return true;
	}

	const FName ManagedAudioKey = BuildManagedAudioKey(EventId, InstanceId);
	UAudioComponent* AudioComponent = ManagedAudioComponents.FindRef(ManagedAudioKey);
	if (!IsValid(AudioComponent))
	{
		ManagedAudioComponents.Remove(ManagedAudioKey);
		ManagedAudioCategories.Remove(ManagedAudioKey);
		ManagedAudioVolumeMultipliers.Remove(ManagedAudioKey);
		ManagedAudioFadeOutTimes.Remove(ManagedAudioKey);
		return false;
	}

	const float* DefaultFadeOutTime = ManagedAudioFadeOutTimes.Find(ManagedAudioKey);
	const float FadeOutTime = ResolvedFadeOutTime >= 0.0f
		? ResolvedFadeOutTime
		: (DefaultFadeOutTime != nullptr ? *DefaultFadeOutTime : 0.0f);

	if (FadeOutTime > 0.0f)
	{
		AudioComponent->FadeOut(FadeOutTime, 0.0f);

		if (UWorld* World = GetWorld())
		{
			FTimerDelegate CleanupDelegate = FTimerDelegate::CreateUObject(
				this,
				&UUOUAudioSubsystem::FinishManagedAudioStop,
				ManagedAudioKey,
				AudioComponent);

			FTimerHandle CleanupTimerHandle;
			World->GetTimerManager().SetTimer(CleanupTimerHandle, CleanupDelegate, FadeOutTime + 0.05f, false);
		}
		else
		{
			FinishManagedAudioStop(ManagedAudioKey, AudioComponent);
		}
	}
	else
	{
		AudioComponent->Stop();
		FinishManagedAudioStop(ManagedAudioKey, AudioComponent);
	}

	return true;
}

void UUOUAudioSubsystem::StopAllManagedAudioEvents(float FadeOutTime)
{
	TArray<FName> ManagedAudioKeys;
	ManagedAudioComponents.GetKeys(ManagedAudioKeys);

	for (const FName ManagedAudioKey : ManagedAudioKeys)
	{
		UAudioComponent* AudioComponent = ManagedAudioComponents.FindRef(ManagedAudioKey);
		if (!IsValid(AudioComponent))
		{
			FinishManagedAudioStop(ManagedAudioKey, AudioComponent);
			continue;
		}

		const float ClampedFadeOutTime = FMath::Max(0.0f, FadeOutTime);
		if (ClampedFadeOutTime > 0.0f)
		{
			AudioComponent->FadeOut(ClampedFadeOutTime, 0.0f);

			if (UWorld* World = GetWorld())
			{
				FTimerDelegate CleanupDelegate = FTimerDelegate::CreateUObject(
					this,
					&UUOUAudioSubsystem::FinishManagedAudioStop,
					ManagedAudioKey,
					AudioComponent);

				FTimerHandle CleanupTimerHandle;
				World->GetTimerManager().SetTimer(CleanupTimerHandle, CleanupDelegate, ClampedFadeOutTime + 0.05f, false);
			}
			else
			{
				FinishManagedAudioStop(ManagedAudioKey, AudioComponent);
			}
		}
		else
		{
			AudioComponent->Stop();
			FinishManagedAudioStop(ManagedAudioKey, AudioComponent);
		}
	}
}

bool UUOUAudioSubsystem::HasAudioEvent(FName EventId)
{
	FUOUAudioEventDefinition AudioEvent;
	UUOUAudioDataAsset* AudioData = GetAudioData();
	return AudioData != nullptr && AudioData->TryGetAudioEvent(EventId, AudioEvent);
}

void UUOUAudioSubsystem::SetCategoryVolume(EUOUAudioCategory Category, float Volume, bool bSaveImmediately)
{
	UUOUAudioSettingsSaveGame* MutableSettings = GetMutableAudioSettings();
	if (MutableSettings == nullptr)
	{
		return;
	}

	MutableSettings->SetCategoryVolume(Category, Volume);

	if (Category == EUOUAudioCategory::Master)
	{
		ApplyAllCategoryVolumes(DefaultVolumeApplyFadeTime);
	}
	else
	{
		ApplyCategoryVolume(Category, DefaultVolumeApplyFadeTime);
		UpdateCurrentBGMVolume();
	}

	UpdateManagedAudioVolumes();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

float UUOUAudioSubsystem::GetCategoryVolume(EUOUAudioCategory Category) const
{
	const UUOUAudioSettingsSaveGame* CurrentSettings = GetAudioSettings();
	return CurrentSettings != nullptr ? CurrentSettings->GetCategoryVolume(Category) : 1.0f;
}

float UUOUAudioSubsystem::GetEffectiveCategoryVolume(EUOUAudioCategory Category) const
{
	const UUOUAudioSettingsSaveGame* CurrentSettings = GetAudioSettings();
	if (CurrentSettings == nullptr)
	{
		return 1.0f;
	}

	const float MasterVolume = CurrentSettings->GetCategoryVolume(EUOUAudioCategory::Master);
	if (Category == EUOUAudioCategory::Master)
	{
		return MasterVolume;
	}

	return MasterVolume * CurrentSettings->GetCategoryVolume(Category);
}

void UUOUAudioSubsystem::SaveAudioSettings()
{
	UUOUAudioSettingsSaveGame* MutableSettings = GetMutableAudioSettings();
	if (MutableSettings == nullptr)
	{
		return;
	}

	if (SaveSlotName.IsEmpty())
	{
		SaveSlotName = DefaultAudioSaveSlotName;
	}

	if (!UGameplayStatics::SaveGameToSlot(MutableSettings, SaveSlotName, SaveUserIndex))
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("Failed to save audio settings to slot '%s'."), *SaveSlotName);
	}
}

void UUOUAudioSubsystem::ReloadAudioSettings()
{
	LoadAudioSettings();
	ApplyAllCategoryVolumes(DefaultVolumeApplyFadeTime);
}

UUOUAudioSettingsSaveGame* UUOUAudioSubsystem::CreateDefaultAudioSettings() const
{
	return Cast<UUOUAudioSettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(UUOUAudioSettingsSaveGame::StaticClass()));
}

void UUOUAudioSubsystem::LoadAudioSettings()
{
	if (SaveSlotName.IsEmpty())
	{
		SaveSlotName = DefaultAudioSaveSlotName;
	}

	AudioSettings = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		AudioSettings = Cast<UUOUAudioSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
		if (AudioSettings == nullptr)
		{
			UE_LOG(LogUOUAudio, Warning, TEXT("Audio settings save slot '%s' had an unexpected type. Defaults will be used."), *SaveSlotName);
		}
	}

	if (AudioSettings == nullptr)
	{
		AudioSettings = CreateDefaultAudioSettings();
	}
}

void UUOUAudioSubsystem::ApplyAllCategoryVolumes(float FadeTime)
{
	if (USoundMix* SoundMix = GetDefaultSoundMix())
	{
		UGameplayStatics::PushSoundMixModifier(this, SoundMix);
	}

	ApplyCategoryVolume(EUOUAudioCategory::Master, FadeTime);
	ApplyCategoryVolume(EUOUAudioCategory::BGM, FadeTime);
	ApplyCategoryVolume(EUOUAudioCategory::SFX, FadeTime);
	ApplyCategoryVolume(EUOUAudioCategory::UI, FadeTime);
	ApplyCategoryVolume(EUOUAudioCategory::Ambience, FadeTime);
	UpdateCurrentBGMVolume();
}

void UUOUAudioSubsystem::ApplyCategoryVolume(EUOUAudioCategory Category, float FadeTime)
{
	USoundMix* SoundMix = GetDefaultSoundMix();
	USoundClass* SoundClass = GetSoundClassForCategory(Category);
	if (SoundMix == nullptr || SoundClass == nullptr)
	{
		return;
	}

	UGameplayStatics::SetSoundMixClassOverride(
		this,
		SoundMix,
		SoundClass,
		GetCategoryVolume(Category),
		1.0f,
		FMath::Max(0.0f, FadeTime),
		true);
}

void UUOUAudioSubsystem::UpdateCurrentBGMVolume()
{
	if (IsValid(CurrentBGMComponent))
	{
		CurrentBGMComponent->SetVolumeMultiplier(GetManagedPlaybackVolume(EUOUAudioCategory::BGM, CurrentBGMVolumeMultiplier));
	}
}

void UUOUAudioSubsystem::UpdateManagedAudioVolumes()
{
	for (const TPair<FName, TObjectPtr<UAudioComponent>>& ManagedAudioPair : ManagedAudioComponents)
	{
		UAudioComponent* AudioComponent = ManagedAudioPair.Value;
		if (!IsValid(AudioComponent))
		{
			continue;
		}

		const EUOUAudioCategory* Category = ManagedAudioCategories.Find(ManagedAudioPair.Key);
		const float* VolumeMultiplier = ManagedAudioVolumeMultipliers.Find(ManagedAudioPair.Key);
		AudioComponent->SetVolumeMultiplier(GetManagedPlaybackVolume(
			Category != nullptr ? *Category : EUOUAudioCategory::Ambience,
			VolumeMultiplier != nullptr ? *VolumeMultiplier : 1.0f));
	}
}

bool UUOUAudioSubsystem::PlayAudioEventDefinition(const FUOUAudioEventDefinition& AudioEvent, FVector Location, bool bForceTwoDimensional, FName InstanceId)
{
	USoundBase* Sound = AudioEvent.Sound.IsNull() ? nullptr : AudioEvent.Sound.LoadSynchronous();
	if (Sound == nullptr)
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("Audio event '%s' has no valid sound."), *AudioEvent.EventId.ToString());
		return false;
	}

	if (!bForceTwoDimensional && AudioEvent.PlaybackMode == EUOUAudioPlaybackMode::BGM)
	{
		PlayBGMInternal(
			AudioEvent.EventId,
			Sound,
			AudioEvent.FadeTime,
			AudioEvent.StartTime,
			AudioEvent.VolumeMultiplier,
			AudioEvent.PitchMultiplier);
		return true;
	}

	if (AudioEvent.bManagedLoop)
	{
		return PlayManagedAudioEventDefinition(AudioEvent, Sound, Location, bForceTwoDimensional, InstanceId);
	}

	const float Volume = GetManagedPlaybackVolume(AudioEvent.Category, AudioEvent.VolumeMultiplier);
	if (bForceTwoDimensional || AudioEvent.PlaybackMode == EUOUAudioPlaybackMode::TwoDimensional)
	{
		UGameplayStatics::PlaySound2D(this, Sound, Volume, AudioEvent.PitchMultiplier, AudioEvent.StartTime);
		return true;
	}

	UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, Volume, AudioEvent.PitchMultiplier, AudioEvent.StartTime);
	return true;
}

bool UUOUAudioSubsystem::PlayManagedAudioEventDefinition(const FUOUAudioEventDefinition& AudioEvent, USoundBase* Sound, FVector Location, bool bForceTwoDimensional, FName InstanceId)
{
	const FName ManagedAudioKey = BuildManagedAudioKey(AudioEvent.EventId, InstanceId);
	if (ManagedAudioKey.IsNone())
	{
		return false;
	}

	const float TargetVolume = GetManagedPlaybackVolume(AudioEvent.Category, AudioEvent.VolumeMultiplier);
	UAudioComponent* ExistingAudioComponent = ManagedAudioComponents.FindRef(ManagedAudioKey);
	if (IsValid(ExistingAudioComponent))
	{
		if (AudioEvent.PlaybackMode == EUOUAudioPlaybackMode::AtLocation && !bForceTwoDimensional)
		{
			ExistingAudioComponent->SetWorldLocation(Location);
		}

		ExistingAudioComponent->SetVolumeMultiplier(TargetVolume);
		ExistingAudioComponent->SetPitchMultiplier(AudioEvent.PitchMultiplier);

		if (!ExistingAudioComponent->IsPlaying())
		{
			ExistingAudioComponent->Play(AudioEvent.StartTime);
		}

		ManagedAudioCategories.Add(ManagedAudioKey, AudioEvent.Category);
		ManagedAudioVolumeMultipliers.Add(ManagedAudioKey, AudioEvent.VolumeMultiplier);
		ManagedAudioFadeOutTimes.Add(ManagedAudioKey, FMath::Max(0.0f, AudioEvent.FadeOutTime));
		return true;
	}

	const float InitialVolume = AudioEvent.FadeTime > 0.0f ? 0.0f : TargetVolume;
	UAudioComponent* NewAudioComponent = nullptr;
	if (bForceTwoDimensional || AudioEvent.PlaybackMode == EUOUAudioPlaybackMode::TwoDimensional)
	{
		NewAudioComponent = UGameplayStatics::SpawnSound2D(
			this,
			Sound,
			InitialVolume,
			AudioEvent.PitchMultiplier,
			AudioEvent.StartTime,
			nullptr,
			true,
			false);
	}
	else
	{
		NewAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
			this,
			Sound,
			Location,
			FRotator::ZeroRotator,
			InitialVolume,
			AudioEvent.PitchMultiplier,
			AudioEvent.StartTime,
			nullptr,
			nullptr,
			false);
	}

	if (!IsValid(NewAudioComponent))
	{
		return false;
	}

	ManagedAudioComponents.Add(ManagedAudioKey, NewAudioComponent);
	ManagedAudioCategories.Add(ManagedAudioKey, AudioEvent.Category);
	ManagedAudioVolumeMultipliers.Add(ManagedAudioKey, AudioEvent.VolumeMultiplier);
	ManagedAudioFadeOutTimes.Add(ManagedAudioKey, FMath::Max(0.0f, AudioEvent.FadeOutTime));

	if (AudioEvent.FadeTime > 0.0f)
	{
		NewAudioComponent->FadeIn(AudioEvent.FadeTime, TargetVolume, AudioEvent.StartTime);
	}

	return true;
}

void UUOUAudioSubsystem::FinishManagedAudioStop(FName ManagedAudioKey, UAudioComponent* AudioComponent)
{
	UAudioComponent* CurrentAudioComponent = ManagedAudioComponents.FindRef(ManagedAudioKey);
	if (CurrentAudioComponent != AudioComponent)
	{
		return;
	}

	if (IsValid(AudioComponent))
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
	}

	ManagedAudioComponents.Remove(ManagedAudioKey);
	ManagedAudioCategories.Remove(ManagedAudioKey);
	ManagedAudioVolumeMultipliers.Remove(ManagedAudioKey);
	ManagedAudioFadeOutTimes.Remove(ManagedAudioKey);
}

float UUOUAudioSubsystem::GetManagedPlaybackVolume(EUOUAudioCategory Category, float VolumeMultiplier) const
{
	return bApplyManagedPlaybackVolume
		? VolumeMultiplier * GetEffectiveCategoryVolume(Category)
		: VolumeMultiplier;
}

FName UUOUAudioSubsystem::BuildManagedAudioKey(FName EventId, FName InstanceId) const
{
	if (EventId.IsNone())
	{
		return NAME_None;
	}

	if (InstanceId.IsNone())
	{
		return EventId;
	}

	return FName(FString::Printf(TEXT("%s::%s"), *EventId.ToString(), *InstanceId.ToString()));
}

UUOUAudioDataAsset* UUOUAudioSubsystem::GetAudioData()
{
	if (RuntimeAudioData != nullptr)
	{
		return RuntimeAudioData;
	}

	return DefaultAudioData.IsNull() ? nullptr : DefaultAudioData.LoadSynchronous();
}

USoundMix* UUOUAudioSubsystem::GetDefaultSoundMix()
{
	return DefaultSoundMix.IsNull() ? nullptr : DefaultSoundMix.LoadSynchronous();
}

USoundClass* UUOUAudioSubsystem::GetSoundClassForCategory(EUOUAudioCategory Category)
{
	switch (Category)
	{
	case EUOUAudioCategory::Master:
		return MasterSoundClass.IsNull() ? nullptr : MasterSoundClass.LoadSynchronous();
	case EUOUAudioCategory::BGM:
		return BGMSoundClass.IsNull() ? nullptr : BGMSoundClass.LoadSynchronous();
	case EUOUAudioCategory::SFX:
		return SFXSoundClass.IsNull() ? nullptr : SFXSoundClass.LoadSynchronous();
	case EUOUAudioCategory::UI:
		return UISoundClass.IsNull() ? nullptr : UISoundClass.LoadSynchronous();
	case EUOUAudioCategory::Ambience:
		return AmbienceSoundClass.IsNull() ? nullptr : AmbienceSoundClass.LoadSynchronous();
	default:
		return nullptr;
	}
}

const UUOUAudioSettingsSaveGame* UUOUAudioSubsystem::GetAudioSettings() const
{
	return AudioSettings != nullptr ? AudioSettings : GetDefault<UUOUAudioSettingsSaveGame>();
}

UUOUAudioSettingsSaveGame* UUOUAudioSubsystem::GetMutableAudioSettings()
{
	if (AudioSettings == nullptr)
	{
		LoadAudioSettings();
	}

	return AudioSettings;
}
