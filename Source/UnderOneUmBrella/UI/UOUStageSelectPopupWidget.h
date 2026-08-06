// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/UOUStageSelectTypes.h"
#include "UOUStageSelectPopupWidget.generated.h"

/**
 * C++ data boundary for a stage-select popup.
 * Blueprint owns the layout and presentation while this class owns the stage data applied to it.
 */
UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUStageSelectPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Stores the selected stage data and asks the Blueprint presentation to refresh. */
	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	void SetStageData(const FUOUStageDefinition& InStageData);

	UFUNCTION(BlueprintPure, Category = "Stage Select")
	const FUOUStageDefinition& GetStageData() const { return StageData; }

	UFUNCTION(BlueprintPure, Category = "Stage Select")
	bool HasStageData() const { return bHasStageData; }

protected:
	/** Most recently applied stage data. Layout and animations remain owned by the WBP. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stage Select")
	FUOUStageDefinition StageData;

	/** Implement this event in WBP_StageSelectPopup to update text, images, and animations. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Stage Select", meta = (DisplayName = "On Stage Data Changed"))
	void BP_OnStageDataChanged(const FUOUStageDefinition& NewStageData);

private:
	UPROPERTY(Transient)
	bool bHasStageData = false;
};
