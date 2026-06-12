// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUInGameHUDWidget.h"

#include "UOUMenuPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UI/UOUUISubsystem.h"

void UUOUInGameHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->RegisterHUD(this);
	}

	BindToPlayerUmbrella();
}

void UUOUInGameHUDWidget::NativeDestruct()
{
	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->UnregisterHUD(this);
	}

	Super::NativeDestruct();
}

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

void UUOUInGameHUDWidget::BindToPlayerUmbrella()
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	APawn* OwningPawn = OwningPlayerController != nullptr ? OwningPlayerController->GetPawn() : nullptr;
	if (OwningPawn == nullptr)
	{
		return;
	}

	UUOUUmbrellaComponent* UmbrellaComponent = OwningPawn->FindComponentByClass<UUOUUmbrellaComponent>();
	if (UmbrellaComponent == nullptr)
	{
		return;
	}

	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->BindUmbrellaComponent(UmbrellaComponent);
	}
}

void UUOUInGameHUDWidget::AdvanceDialogue()
{
	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->AdvanceDialogue();
	}
}

void UUOUInGameHUDWidget::ShowTitle(const FUOUTitleDisplayData& TitleData)
{
	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->ShowTitle(TitleData);
	}
}

AUOUMenuPlayerController* UUOUInGameHUDWidget::GetMenuPlayerController() const
{
	return Cast<AUOUMenuPlayerController>(GetOwningPlayer());
}

UUOUUISubsystem* UUOUInGameHUDWidget::GetUISubsystem() const
{
	ULocalPlayer* OwningLocalPlayer = GetOwningLocalPlayer();
	return OwningLocalPlayer != nullptr ? OwningLocalPlayer->GetSubsystem<UUOUUISubsystem>() : nullptr;
}