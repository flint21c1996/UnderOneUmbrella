// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUMenuPlayerController.h"
#include "UOUTitlePlayerController.generated.h"

class UWorld;
class UUserWidget;

UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUTitlePlayerController : public AUOUMenuPlayerController
{
	GENERATED_BODY()

public:
	AUOUTitlePlayerController();

	UFUNCTION(BlueprintCallable, Category = "Title")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Title")
	void QuitGame();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void RestoreInputModeAfterSettingsMenu() override;

private:
	void ApplyTitleMenuInputMode();

	UPROPERTY(EditDefaultsOnly, Config, Category = "Title")
	TSoftObjectPtr<UWorld> TestLevel;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Title")
	TSoftClassPtr<UUserWidget> TitleMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> TitleMenuWidget;

	bool bIsOpeningLevel = false;
};
