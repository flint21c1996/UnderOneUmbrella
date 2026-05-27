// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/UOUAudioSettingsSaveGame.h"

float UUOUAudioSettingsSaveGame::GetCategoryVolume(EUOUAudioCategory Category) const
{
	switch (Category)
	{
	case EUOUAudioCategory::Master:
		return MasterVolume;
	case EUOUAudioCategory::BGM:
		return BGMVolume;
	case EUOUAudioCategory::SFX:
		return SFXVolume;
	case EUOUAudioCategory::UI:
		return UIVolume;
	case EUOUAudioCategory::Ambience:
		return AmbienceVolume;
	default:
		return 1.0f;
	}
}

void UUOUAudioSettingsSaveGame::SetCategoryVolume(EUOUAudioCategory Category, float Volume)
{
	const float ClampedVolume = FMath::Clamp(Volume, 0.0f, 1.0f);

	switch (Category)
	{
	case EUOUAudioCategory::Master:
		MasterVolume = ClampedVolume;
		break;
	case EUOUAudioCategory::BGM:
		BGMVolume = ClampedVolume;
		break;
	case EUOUAudioCategory::SFX:
		SFXVolume = ClampedVolume;
		break;
	case EUOUAudioCategory::UI:
		UIVolume = ClampedVolume;
		break;
	case EUOUAudioCategory::Ambience:
		AmbienceVolume = ClampedVolume;
		break;
	default:
		break;
	}
}
