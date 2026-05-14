// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UOUMenuPlayerController.generated.h"

class UUserWidget;
class UWorld;

UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AUOUMenuPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void OpenSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void CloseSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void ToggleSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void ReturnToTitle();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void ToggleTestSetting();

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	bool IsSettingsMenuOpen() const { return bSettingsMenuOpen; }

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	bool IsTestSettingEnabled() const { return bTestSettingEnabled; }

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	bool CanReturnToTitle() const { return bCanReturnToTitle; }

protected:
	void SetCanReturnToTitle(bool bInCanReturnToTitle) { bCanReturnToTitle = bInCanReturnToTitle; }

	virtual void ApplySettingsMenuInputMode(UUserWidget* InSettingsMenuWidget);
	virtual void RestoreInputModeAfterSettingsMenu();

private:
	UPROPERTY(EditDefaultsOnly, Config, Category = "Settings")
	TSoftClassPtr<UUserWidget> SettingsMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Settings")
	TSoftObjectPtr<UWorld> TitleLevel;

	UPROPERTY()
	TObjectPtr<UUserWidget> SettingsMenuWidget;

	bool bSettingsMenuOpen = false;
	bool bTestSettingEnabled = false;
	bool bCanReturnToTitle = false;
};
