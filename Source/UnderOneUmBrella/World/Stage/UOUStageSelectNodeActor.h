// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUStageSelectNodeActor.generated.h"

class USceneComponent;

UCLASS(meta = (DisplayName = "UOU Stage Select Node"))
class UNDERONEUMBRELLA_API AUOUStageSelectNodeActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUStageSelectNodeActor();

	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	bool ActivateStage();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Select", meta = (ClampMin = "0"))
	int32 StageIndex = 0;
};
