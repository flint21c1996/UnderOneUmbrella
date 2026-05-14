// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUTitlePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"

AUOUTitlePlayerController::AUOUTitlePlayerController()
	: TestLevel(FSoftObjectPath(TEXT("/Game/UOU/Maps/TempMap.TempMap")))
{
	bShowMouseCursor = false;

	static ConstructorHelpers::FClassFinder<UUserWidget> TitleMenuBPClass(TEXT("/Game/UOU/UI/WBP_TitleMenu"));
	if (TitleMenuBPClass.Succeeded())
	{
		TitleMenuWidgetClass = TitleMenuBPClass.Class;
	}
}

void AUOUTitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (TitleMenuWidgetClass != nullptr)
	{
		TitleMenuWidget = CreateWidget<UUserWidget>(this, TitleMenuWidgetClass);
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
