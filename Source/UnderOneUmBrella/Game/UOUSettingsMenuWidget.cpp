// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUSettingsMenuWidget.h"

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

AUOUMenuPlayerController* UUOUSettingsMenuWidget::GetMenuPlayerController() const
{
	// WBP를 어떤 화면에서 열었는지는 Owning Player를 통해 구분합니다.
	return Cast<AUOUMenuPlayerController>(GetOwningPlayer());
}
