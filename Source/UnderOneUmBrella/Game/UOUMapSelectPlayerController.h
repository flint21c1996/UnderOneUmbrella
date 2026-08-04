// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/UOUMenuPlayerController.h"
#include "Game/UOUStageSelectTypes.h"
#include "UOUMapSelectPlayerController.generated.h"

class UUserWidget;

UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUMapSelectPlayerController : public AUOUMenuPlayerController
{
	GENERATED_BODY()

public:
	AUOUMapSelectPlayerController();

	UFUNCTION(BlueprintPure, Category = "Stage Select")
	TArray<FUOUStageDefinition> GetStages() const { return Stages; }

	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	bool EnterStageByIndex(int32 StageIndex);

	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	bool EnterStage(const FUOUStageDefinition& Stage);

	UFUNCTION(BlueprintPure, Category = "Stage Select")
	bool IsOpeningStage() const { return bIsOpeningStage; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void RestoreInputModeAfterSettingsMenu() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Select")
	TArray<FUOUStageDefinition> Stages;

private:
	void ApplyMapSelectInputMode();

	UPROPERTY(EditDefaultsOnly, Config, Category = "Stage Select")
	TSoftClassPtr<UUserWidget> StageSelectWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> StageSelectWidget = nullptr;

	bool bIsOpeningStage = false;
};
