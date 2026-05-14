// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UOUInGameHUDWidget.generated.h"

class AUOUMenuPlayerController;

UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUInGameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OpenSettingsMenu();

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsSettingsMenuOpen() const;

private:
	AUOUMenuPlayerController* GetMenuPlayerController() const;
};
