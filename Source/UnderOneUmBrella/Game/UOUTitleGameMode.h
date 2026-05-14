// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UOUTitleGameMode.generated.h"

UCLASS()
class UNDERONEUMBRELLA_API AUOUTitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUOUTitleGameMode();

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
};
