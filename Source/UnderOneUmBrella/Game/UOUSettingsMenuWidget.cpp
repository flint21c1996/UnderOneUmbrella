// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUSettingsMenuWidget.h"

#include "Audio/UOUAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "UOUMenuPlayerController.h"

void UUOUSettingsMenuWidget::CloseSettingsMenu()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->CloseSettingsMenu();
	}
}

void UUOUSettingsMenuWidget::ReturnToTitle()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->ReturnToTitle();
	}
}

void UUOUSettingsMenuWidget::ToggleTestSetting()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->ToggleTestSetting();
	}
}

bool UUOUSettingsMenuWidget::IsTestSettingEnabled() const
{
	const AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController();
	return MenuPlayerController != nullptr && MenuPlayerController->IsTestSettingEnabled();
}

bool UUOUSettingsMenuWidget::CanReturnToTitle() const
{
	const AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController();
	return MenuPlayerController != nullptr && MenuPlayerController->CanReturnToTitle();
}

void UUOUSettingsMenuWidget::SetAudioVolume(EUOUAudioCategory Category, float Volume, bool bSaveImmediately)
{
	if (UUOUAudioSubsystem* AudioSubsystem = GetAudioSubsystem())
	{
		AudioSubsystem->SetCategoryVolume(Category, Volume, bSaveImmediately);
	}
}

float UUOUSettingsMenuWidget::GetAudioVolume(EUOUAudioCategory Category) const
{
	const UUOUAudioSubsystem* AudioSubsystem = GetAudioSubsystem();
	return AudioSubsystem != nullptr ? AudioSubsystem->GetCategoryVolume(Category) : 1.0f;
}

float UUOUSettingsMenuWidget::GetEffectiveAudioVolume(EUOUAudioCategory Category) const
{
	const UUOUAudioSubsystem* AudioSubsystem = GetAudioSubsystem();
	return AudioSubsystem != nullptr ? AudioSubsystem->GetEffectiveCategoryVolume(Category) : 1.0f;
}

void UUOUSettingsMenuWidget::SaveAudioSettings()
{
	if (UUOUAudioSubsystem* AudioSubsystem = GetAudioSubsystem())
	{
		AudioSubsystem->SaveAudioSettings();
	}
}

AUOUMenuPlayerController* UUOUSettingsMenuWidget::GetMenuPlayerController() const
{
	// WBP를 어떤 화면에서 열었는지는 Owning Player를 통해 구분합니다.
	return Cast<AUOUMenuPlayerController>(GetOwningPlayer());
}

UUOUAudioSubsystem* UUOUSettingsMenuWidget::GetAudioSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UUOUAudioSubsystem>();
	}

	return nullptr;
}
