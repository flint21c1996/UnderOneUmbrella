// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUSettingsMenuWidget.h"

#include "Audio/UOUAudioSubsystem.h"
#include "Components/Slider.h"
#include "Engine/GameInstance.h"
#include "UOUMenuPlayerController.h"

void UUOUSettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindAudioSliders();
	InitializeAudioSliderValues();
}

void UUOUSettingsMenuWidget::NativeDestruct()
{
	if (bAudioVolumeDirty)
	{
		SaveAudioSettings();
		bAudioVolumeDirty = false;
	}

	Super::NativeDestruct();
}

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

void UUOUSettingsMenuWidget::RestartCurrentStage()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->RestartCurrentStage();
	}
}

void UUOUSettingsMenuWidget::ToggleTestSetting()
{
	RestartCurrentStage();
}

bool UUOUSettingsMenuWidget::CanRestartCurrentStage() const
{
	const AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController();
	return MenuPlayerController != nullptr && MenuPlayerController->CanRestartCurrentStage();
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

void UUOUSettingsMenuWidget::BindAudioSliders()
{
	ConfigureAudioSlider(MasterVolumeSlider);
	ConfigureAudioSlider(BGMVolumeSlider);
	ConfigureAudioSlider(SFXVolumeSlider);
	ConfigureAudioSlider(UIVolumeSlider);
	ConfigureAudioSlider(AmbienceVolumeSlider);

	if (MasterVolumeSlider != nullptr)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UUOUSettingsMenuWidget::HandleMasterVolumeChanged);
	}

	if (BGMVolumeSlider != nullptr)
	{
		BGMVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UUOUSettingsMenuWidget::HandleBGMVolumeChanged);
	}

	if (SFXVolumeSlider != nullptr)
	{
		SFXVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UUOUSettingsMenuWidget::HandleSFXVolumeChanged);
	}

	if (UIVolumeSlider != nullptr)
	{
		UIVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UUOUSettingsMenuWidget::HandleUIVolumeChanged);
	}

	if (AmbienceVolumeSlider != nullptr)
	{
		AmbienceVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UUOUSettingsMenuWidget::HandleAmbienceVolumeChanged);
	}
}

void UUOUSettingsMenuWidget::InitializeAudioSliderValues()
{
	bUpdatingAudioSliderValues = true;

	SetAudioSliderValue(MasterVolumeSlider, EUOUAudioCategory::Master);
	SetAudioSliderValue(BGMVolumeSlider, EUOUAudioCategory::BGM);
	SetAudioSliderValue(SFXVolumeSlider, EUOUAudioCategory::SFX);
	SetAudioSliderValue(UIVolumeSlider, EUOUAudioCategory::UI);
	SetAudioSliderValue(AmbienceVolumeSlider, EUOUAudioCategory::Ambience);

	bUpdatingAudioSliderValues = false;
}

void UUOUSettingsMenuWidget::ConfigureAudioSlider(USlider* Slider) const
{
	if (Slider == nullptr)
	{
		return;
	}

	Slider->SetMinValue(0.0f);
	Slider->SetMaxValue(1.0f);
	Slider->SetStepSize(0.01f);
}

void UUOUSettingsMenuWidget::SetAudioSliderValue(USlider* Slider, EUOUAudioCategory Category)
{
	if (Slider == nullptr)
	{
		return;
	}

	Slider->SetValue(GetAudioVolume(Category));
}

void UUOUSettingsMenuWidget::HandleAudioSliderChanged(EUOUAudioCategory Category, float Value)
{
	if (bUpdatingAudioSliderValues)
	{
		return;
	}

	SetAudioVolume(Category, Value, false);
	bAudioVolumeDirty = true;
}

void UUOUSettingsMenuWidget::HandleMasterVolumeChanged(float Value)
{
	HandleAudioSliderChanged(EUOUAudioCategory::Master, Value);
}

void UUOUSettingsMenuWidget::HandleBGMVolumeChanged(float Value)
{
	HandleAudioSliderChanged(EUOUAudioCategory::BGM, Value);
}

void UUOUSettingsMenuWidget::HandleSFXVolumeChanged(float Value)
{
	HandleAudioSliderChanged(EUOUAudioCategory::SFX, Value);
}

void UUOUSettingsMenuWidget::HandleUIVolumeChanged(float Value)
{
	HandleAudioSliderChanged(EUOUAudioCategory::UI, Value);
}

void UUOUSettingsMenuWidget::HandleAmbienceVolumeChanged(float Value)
{
	HandleAudioSliderChanged(EUOUAudioCategory::Ambience, Value);
}
