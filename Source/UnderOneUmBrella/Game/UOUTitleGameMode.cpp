// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUTitleGameMode.h"

#include "UOUTitlePlayerController.h"

AUOUTitleGameMode::AUOUTitleGameMode()
{
	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
	PlayerControllerClass = AUOUTitlePlayerController::StaticClass();
	HUDClass = nullptr;
	bStartPlayersAsSpectators = true;
}

UClass* AUOUTitleGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return nullptr;
}
