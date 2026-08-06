// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/UOUStageSelectTypes.h"
#include "GameFramework/Actor.h"
#include "UOUStageSelectNodeActor.generated.h"

class USceneComponent;
class UUOUStageSelectAreaComponent;
class APawn;

UCLASS(meta = (DisplayName = "UOU Stage Select Node"))
class UNDERONEUMBRELLA_API AUOUStageSelectNodeActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUStageSelectNodeActor();

	/** Resolves this node's DataTable row. StageRow.RowName becomes the stable StageId. */
	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	bool GetStageDefinition(FUOUStageDefinition& OutStageDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	bool ActivateStage();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select")
	TObjectPtr<UUOUStageSelectAreaComponent> StageSelectArea = nullptr;

	/** Set the DataTable once on the BP defaults, then select a different row on each placed node. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Select")
	FDataTableRowHandle StageRow;

	/** Kept temporarily so existing Blueprint graphs can be migrated without becoming invalid. */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use StageRow instead."))
	int32 StageIndex = 0;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandlePlayerEnteredStageArea(APawn* PlayerPawn);

	UFUNCTION()
	void HandlePlayerExitedStageArea(APawn* PlayerPawn);
};
