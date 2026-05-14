// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUTitlePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/SoftObjectPath.h"

namespace
{
constexpr TCHAR DefaultTestLevelPath[] = TEXT("/Game/UOU/Maps/TempMap.TempMap");
constexpr TCHAR DefaultTitleMenuWidgetClassPath[] = TEXT("/Game/UOU/UI/WBP_TitleMenu.WBP_TitleMenu_C");
}

AUOUTitlePlayerController::AUOUTitlePlayerController()
{
	bShowMouseCursor = false;
}

void AUOUTitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (TestLevel.IsNull())
	{
		TestLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultTestLevelPath));
		UE_LOG(LogTemp, Warning, TEXT("Title test level was not configured. Falling back to %s."), DefaultTestLevelPath);
	}

	if (TitleMenuWidgetClass.IsNull())
	{
		TitleMenuWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultTitleMenuWidgetClassPath));
		UE_LOG(LogTemp, Warning, TEXT("Title menu widget was not configured. Falling back to %s."), DefaultTitleMenuWidgetClassPath);
	}

	TSubclassOf<UUserWidget> LoadedTitleMenuWidgetClass = TitleMenuWidgetClass.LoadSynchronous();
	if (LoadedTitleMenuWidgetClass == nullptr)
	{
		TitleMenuWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultTitleMenuWidgetClassPath));
		LoadedTitleMenuWidgetClass = TitleMenuWidgetClass.LoadSynchronous();
	}

	if (LoadedTitleMenuWidgetClass != nullptr)
	{
		TitleMenuWidget = CreateWidget<UUserWidget>(this, LoadedTitleMenuWidgetClass);
		if (TitleMenuWidget != nullptr)
		{
			TitleMenuWidget->AddToViewport();

			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(TitleMenuWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load title menu widget class."));
		SetInputMode(FInputModeGameOnly());
	}

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
}

void AUOUTitlePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
	{
		return;
	}

	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AUOUTitlePlayerController::StartGame);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AUOUTitlePlayerController::StartGame);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AUOUTitlePlayerController::QuitGame);
	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AUOUTitlePlayerController::QuitGame);
}

void AUOUTitlePlayerController::StartGame()
{
	if (bIsOpeningLevel || TestLevel.IsNull())
	{
		return;
	}

	bIsOpeningLevel = true;
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TestLevel);
}

void AUOUTitlePlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
