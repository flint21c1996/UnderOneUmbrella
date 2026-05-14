// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UOUSettingsMenuWidget.generated.h"

class AUOUMenuPlayerController;

UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUSettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void CloseSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ReturnToTitle();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ToggleTestSetting();

	UFUNCTION(BlueprintPure, Category = "Settings")
	bool IsTestSettingEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	bool CanReturnToTitle() const;

private:
	AUOUMenuPlayerController* GetMenuPlayerController() const;
};
