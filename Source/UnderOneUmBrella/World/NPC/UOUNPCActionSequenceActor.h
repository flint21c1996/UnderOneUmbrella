// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/NPC/UOUNPCActionTypes.h"
#include "UOUNPCActionSequenceActor.generated.h"

class AUOUNPCCharacter;
class USceneComponent;

UCLASS(Blueprintable, meta = (DisplayName = "UOU NPC Action Sequence"))
class AUOUNPCActionSequenceActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUNPCActionSequenceActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Sequence")
	TObjectPtr<USceneComponent> RootScene = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Sequence")
	TObjectPtr<AUOUNPCCharacter> TargetNPC = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Sequence")
	TArray<FUOUNPCActionRequest> ActivateActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Sequence")
	TArray<FUOUNPCActionRequest> DeactivateActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Sequence")
	bool bActivateOnBeginPlay = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	bool bActivated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	bool bRunningSequence = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	bool bRunningDeactivateSequence = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	int32 CurrentActionIndex = INDEX_NONE;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Activate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Toggle();

	UFUNCTION(BlueprintCallable, Category = "NPC|Sequence")
	void StopSequence();

protected:
	UPROPERTY()
	TArray<FUOUNPCActionRequest> CurrentSequenceActions;

	void StartSequence(const TArray<FUOUNPCActionRequest>& Actions, bool bDeactivateSequence);
	void RunCurrentAction();
	void FinishSequence();
	void BindToTargetNPC();
	void UnbindFromTargetNPC();

	UFUNCTION()
	void HandleNPCActionCompleted(AUOUNPCCharacter* NPC, UObject* ActionSource);
};
