// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "UOUDialogueTriggerComponent.generated.h"

class UUOUDialogueSourceComponent;
class UUOUUmbrellaComponent;

// Overlap trigger that starts a dialogue source when the player enters a nearby area.
// It is intended for NPC reactions such as speaking only when the player approaches with an umbrella.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Trigger"))
class UNDERONEUMBRELLA_API UUOUDialogueTriggerComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UUOUDialogueTriggerComponent();

	virtual void BeginPlay() override;

	// Attempts to start the connected dialogue source using the given actor as the instigator.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Trigger")
	bool TryStartDialogue(AActor* InstigatorActor);

	// Clears the one-shot flag so this trigger can fire again.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Trigger")
	void ResetTrigger();

	// Dialogue source to play. If empty, the owner is searched for UOUDialogueSourceComponent.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Trigger")
	TObjectPtr<UUOUDialogueSourceComponent> DialogueSource = nullptr;

	// If true, only pawns can start the dialogue.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bOnlyPawn = true;

	// If true, this trigger fires once until ResetTrigger is called.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bTriggerOnce = true;

	// If true, the instigator must have an umbrella component and own an umbrella.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bRequireUmbrella = true;

	// If true, the umbrella must be opened or upside down.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules", meta = (EditCondition = "bRequireUmbrella"))
	bool bRequireOpenUmbrella = true;

	// If true, the umbrella must currently be blocking rain.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules", meta = (EditCondition = "bRequireUmbrella"))
	bool bRequireBlockingRain = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	bool bHasTriggered = false;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UUOUDialogueSourceComponent* ResolveDialogueSource() const;
	bool PassesInstigatorRules(AActor* InstigatorActor) const;
	UUOUUmbrellaComponent* FindUmbrellaComponent(AActor* InstigatorActor) const;
};
