// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UOUTitlePlayerController.generated.h"

class UWorld;
class UUserWidget;

UCLASS()
class UNDERONEUMBRELLA_API AUOUTitlePlayerController : public APlayerController
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

private:
	UPROPERTY(EditDefaultsOnly, Category = "Title")
	TSoftObjectPtr<UWorld> TestLevel;

	UPROPERTY(EditDefaultsOnly, Category = "Title")
	TSubclassOf<UUserWidget> TitleMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> TitleMenuWidget;

	bool bIsOpeningLevel = false;
};
