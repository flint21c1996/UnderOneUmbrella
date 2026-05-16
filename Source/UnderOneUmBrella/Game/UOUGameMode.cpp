// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUGameMode.h"

#include "UOUPlayerController.h"

#include "Player/UOUCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUOUGameMode::AUOUGameMode()
{
	PlayerControllerClass = AUOUPlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/UOU/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
