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
	// 설정창을 여는 주체는 HUD가 아니라 현재 플레이어 컨트롤러입니다.
	return Cast<AUOUMenuPlayerController>(GetOwningPlayer());
}
