// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "UObject/SoftObjectPath.h"

namespace
{
constexpr TCHAR DefaultInGameHUDWidgetClassPath[] = TEXT("/Game/UOU/UI/WBP_InGameHUD.WBP_InGameHUD_C");
}

AUOUPlayerController::AUOUPlayerController()
{
	SetCanReturnToTitle(true);
}

void AUOUPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (InGameHUDWidgetClass.IsNull())
	{
		InGameHUDWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultInGameHUDWidgetClassPath));
		UE_LOG(LogTemp, Warning, TEXT("In-game HUD widget was not configured. Falling back to %s."), DefaultInGameHUDWidgetClassPath);
	}

	TSubclassOf<UUserWidget> LoadedInGameHUDWidgetClass = InGameHUDWidgetClass.LoadSynchronous();
	if (LoadedInGameHUDWidgetClass == nullptr)
	{
		InGameHUDWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultInGameHUDWidgetClassPath));
		LoadedInGameHUDWidgetClass = InGameHUDWidgetClass.LoadSynchronous();
	}

	if (LoadedInGameHUDWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load in-game HUD widget class."));
		return;
	}

	InGameHUDWidget = CreateWidget<UUserWidget>(this, LoadedInGameHUDWidgetClass);
	if (InGameHUDWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create in-game HUD widget."));
		return;
	}

	InGameHUDWidget->AddToViewport();
}

void AUOUPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
	{
		return;
	}

	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AUOUPlayerController::ToggleSettingsMenu);
}
