// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUMenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPath.h"

namespace
{
constexpr TCHAR DefaultSettingsMenuWidgetClassPath[] = TEXT("/Game/UOU/UI/WBP_SettingsMenu.WBP_SettingsMenu_C");
constexpr TCHAR DefaultTitleLevelPath[] = TEXT("/Game/UOU/Maps/TitleMap.TitleMap");
}

AUOUMenuPlayerController::AUOUMenuPlayerController()
{
}

void AUOUMenuPlayerController::OpenSettingsMenu()
{
	if (bSettingsMenuOpen)
	{
		return;
	}

	if (SettingsMenuWidgetClass.IsNull())
	{
		SettingsMenuWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultSettingsMenuWidgetClassPath));
		UE_LOG(LogTemp, Warning, TEXT("Settings menu widget was not configured. Falling back to %s."), DefaultSettingsMenuWidgetClassPath);
	}

	TSubclassOf<UUserWidget> LoadedSettingsMenuWidgetClass = SettingsMenuWidgetClass.LoadSynchronous();
	if (LoadedSettingsMenuWidgetClass == nullptr)
	{
		SettingsMenuWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultSettingsMenuWidgetClassPath));
		LoadedSettingsMenuWidgetClass = SettingsMenuWidgetClass.LoadSynchronous();
	}

	if (LoadedSettingsMenuWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load settings menu widget class."));
		return;
	}

	SettingsMenuWidget = CreateWidget<UUserWidget>(this, LoadedSettingsMenuWidgetClass);
	if (SettingsMenuWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create settings menu widget."));
		return;
	}

	SettingsMenuWidget->AddToViewport(100);
	bSettingsMenuOpen = true;
	ApplySettingsMenuInputMode(SettingsMenuWidget);
}

void AUOUMenuPlayerController::CloseSettingsMenu()
{
	if (!bSettingsMenuOpen)
	{
		return;
	}

	if (SettingsMenuWidget != nullptr)
	{
		SettingsMenuWidget->RemoveFromParent();
		SettingsMenuWidget = nullptr;
	}

	bSettingsMenuOpen = false;
	RestoreInputModeAfterSettingsMenu();
}

void AUOUMenuPlayerController::ToggleSettingsMenu()
{
	if (bSettingsMenuOpen)
	{
		CloseSettingsMenu();
	}
	else
	{
		OpenSettingsMenu();
	}
}

void AUOUMenuPlayerController::ReturnToTitle()
{
	if (!bCanReturnToTitle)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnToTitle was requested, but this controller does not allow it."));
		return;
	}

	if (TitleLevel.IsNull())
	{
		TitleLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultTitleLevelPath));
		UE_LOG(LogTemp, Warning, TEXT("Title level was not configured. Falling back to %s."), DefaultTitleLevelPath);
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TitleLevel);
}

void AUOUMenuPlayerController::ToggleTestSetting()
{
	bTestSettingEnabled = !bTestSettingEnabled;
	UE_LOG(LogTemp, Log, TEXT("Test setting is now %s."), bTestSettingEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void AUOUMenuPlayerController::ApplySettingsMenuInputMode(UUserWidget* InSettingsMenuWidget)
{
	if (InSettingsMenuWidget != nullptr)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InSettingsMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}

	bShowMouseCursor = true;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
}

void AUOUMenuPlayerController::RestoreInputModeAfterSettingsMenu()
{
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
}
