// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/UOUMenuPlayerController.h"
#include "Game/UOUStageSelectTypes.h"
#include "UOUMapSelectPlayerController.generated.h"

class AUOUStageSelectNodeActor;

UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUMapSelectPlayerController : public AUOUMenuPlayerController
{
	GENERATED_BODY()

public:
	AUOUMapSelectPlayerController();

	/** Deprecated migration path. Stage definitions now belong to a DataTable referenced by each node. */
	UFUNCTION(BlueprintPure, Category = "Stage Select", meta = (DeprecatedFunction, DeprecationMessage = "Use the StageRow on UOUStageSelectNodeActor."))
	TArray<FUOUStageDefinition> GetStages() const { return Stages; }

	UFUNCTION(BlueprintCallable, Category = "Stage Select", meta = (DeprecatedFunction, DeprecationMessage = "Call ActivateStage on UOUStageSelectNodeActor."))
	bool EnterStageByIndex(int32 StageIndex);

	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	bool EnterStage(const FUOUStageDefinition& Stage);

	/** Adds an overlapped node as an entry candidate. The most recently entered node has focus. */
	void RegisterStageSelectNode(AUOUStageSelectNodeActor* StageNode);

	/** Removes a node when the local player leaves its selection area. */
	void UnregisterStageSelectNode(AUOUStageSelectNodeActor* StageNode);

	UFUNCTION(BlueprintPure, Category = "Stage Select")
	AUOUStageSelectNodeActor* GetFocusedStageSelectNode() const;

	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	bool ConfirmFocusedStage();

	UFUNCTION(BlueprintPure, Category = "Stage Select")
	bool IsOpeningStage() const { return bIsOpeningStage; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void RestoreInputModeAfterSettingsMenu() override;

	/** Deprecated migration data. New stage definitions are authored in a DataTable. */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use the StageRow on UOUStageSelectNodeActor."))
	TArray<FUOUStageDefinition> Stages;

private:
	UPROPERTY(EditDefaultsOnly, Config, Category = "HUD", meta = (DisplayName = "스테이지 선택 HUD"))
	TSoftClassPtr<UUserWidget> MapSelectHUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MapSelectHUDWidget;

	void ApplyMapSelectInputMode();
	void HandleStageConfirmInput();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AUOUStageSelectNodeActor>> OverlappingStageNodes;

	bool bIsOpeningStage = false;
};
