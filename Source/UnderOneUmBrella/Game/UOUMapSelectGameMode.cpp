// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOUMapSelectGameMode.h"

#include "Game/UOUMapSelectPlayerController.h"
#include "UObject/ConstructorHelpers.h"

AUOUMapSelectGameMode::AUOUMapSelectGameMode()
{
	PlayerControllerClass = AUOUMapSelectPlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/UOU/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
