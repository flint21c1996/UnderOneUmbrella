// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/NPC/UOUNPCActionTypes.h"
#include "UOUNPCActionRequestActor.generated.h"

class AUOUNPCCharacter;
class USceneComponent;

UCLASS(Blueprintable, meta = (DisplayName = "UOU NPC Action Request"))
class AUOUNPCActionRequestActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUNPCActionRequestActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Action")
	TObjectPtr<USceneComponent> RootScene = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action")
	TObjectPtr<AUOUNPCCharacter> TargetNPC = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action")
	FUOUNPCActionRequest ActionRequest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action")
	bool bActivateOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action")
	bool bClearNPCActionOnDeactivate = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	bool bActivated = false;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Activate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Toggle();
};
