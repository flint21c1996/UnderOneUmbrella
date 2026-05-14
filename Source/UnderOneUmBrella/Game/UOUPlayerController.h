// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUMenuPlayerController.h"
#include "UOUPlayerController.generated.h"

class UUserWidget;

UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUPlayerController : public AUOUMenuPlayerController
{
	GENERATED_BODY()

public:
	AUOUPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditDefaultsOnly, Config, Category = "HUD")
	TSoftClassPtr<UUserWidget> InGameHUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InGameHUDWidget;
};
