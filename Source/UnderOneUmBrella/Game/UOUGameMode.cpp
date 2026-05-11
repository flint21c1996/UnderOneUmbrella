// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUGameMode.h"
#include "Player/UOUCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUOUGameMode::AUOUGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/UOU/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
