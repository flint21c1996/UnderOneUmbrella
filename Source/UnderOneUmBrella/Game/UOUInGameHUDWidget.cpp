// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUInGameHUDWidget.h"

#include "UOUMenuPlayerController.h"

void UUOUInGameHUDWidget::OpenSettingsMenu()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->OpenSettingsMenu();
	}
}

bool UUOUInGameHUDWidget::IsSettingsMenuOpen() const
{
	const AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController();
	return MenuPlayerController != nullptr && MenuPlayerController->IsSettingsMenuOpen();
}

AUOUMenuPlayerController* UUOUInGameHUDWidget::GetMenuPlayerController() const
{
	return Cast<AUOUMenuPlayerController>(GetOwningPlayer());
}
