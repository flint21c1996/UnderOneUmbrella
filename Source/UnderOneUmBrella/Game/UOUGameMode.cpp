// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUGameMode.h"

#include "UOUTitlePlayerController.h"

#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Player/UOUCharacter.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
FString StripPIEPrefix(const FString& ShortMapName)
{
	static const FString Prefix(TEXT("UEDPIE_"));
	if (!ShortMapName.StartsWith(Prefix))
	{
		return ShortMapName;
	}

	for (int32 Index = Prefix.Len(); Index < ShortMapName.Len(); ++Index)
	{
		if (ShortMapName[Index] == TEXT('_'))
		{
			return ShortMapName.RightChop(Index + 1);
		}
	}

	return ShortMapName;
}
}

AUOUGameMode::AUOUGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/UOU/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AUOUGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (!IsTitleMap(MapName))
	{
		return;
	}

	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
	PlayerControllerClass = AUOUTitlePlayerController::StaticClass();
	HUDClass = nullptr;
	bStartPlayersAsSpectators = true;
}

UClass* AUOUGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return IsTitleWorld() ? nullptr : Super::GetDefaultPawnClassForController_Implementation(InController);
}

bool AUOUGameMode::IsTitleMap(const FString& MapName) const
{
	const FString ShortMapName = StripPIEPrefix(FPackageName::GetShortName(MapName));
	return ShortMapName.Equals(TEXT("TitleMap"), ESearchCase::IgnoreCase);
}

bool AUOUGameMode::IsTitleWorld() const
{
	const UWorld* World = GetWorld();
	return World != nullptr && IsTitleMap(World->GetMapName());
}
