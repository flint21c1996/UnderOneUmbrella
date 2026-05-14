// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UOUGameMode.generated.h"

UCLASS(minimalapi)
class AUOUGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUOUGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

private:
	bool IsTitleMap(const FString& MapName) const;
	bool IsTitleWorld() const;
};
