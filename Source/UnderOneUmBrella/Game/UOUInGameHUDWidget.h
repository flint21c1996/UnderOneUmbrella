// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UOUUITypes.h"
#include "UOUInGameHUDWidget.generated.h"

class AUOUMenuPlayerController;
class UUOUDialogueSourceComponent;
class UUOUUISubsystem;

// C++ entry point for the in-game HUD Blueprint.
// It forwards gameplay UI events to UMG while keeping the existing settings menu hook.
UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUInGameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OpenSettingsMenu();

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsSettingsMenuOpen() const;

	// Finds the player's umbrella component and connects it to the HUD state stream.
	UFUNCTION(BlueprintCallable, Category = "HUD|Umbrella")
	void BindToPlayerUmbrella();

	// Called by the dialogue UI when the player advances to the next line.
	UFUNCTION(BlueprintCallable, Category = "HUD|Dialogue")
	void AdvanceDialogue();

	// Requests a title card from Blueprint or level triggers.
	UFUNCTION(BlueprintCallable, Category = "HUD|Title")
	void ShowTitle(const FUOUTitleDisplayData& TitleData);

	// Blueprint event used to redraw umbrella icon, state, rain block, and water gauge.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "HUD|Umbrella")
	void HandleUmbrellaHUDStateChanged(const FUOUUmbrellaHUDState& State);

	// Blueprint event used once when dialogue starts, before individual lines are shown.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "HUD|Dialogue")
	void BeginDialoguePresentation(AActor* SpeakerActor, UUOUDialogueSourceComponent* DialogueSource);

	// Blueprint event used to spawn a short speech bubble above an NPC or object.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "HUD|Dialogue")
	void ShowNPCSpeechBubble(AActor* SpeakerActor, const FText& BubbleText, float Duration);

	// Blueprint event used to display one bottom dialogue-box line.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "HUD|Dialogue")
	void ShowDialogueLine(AActor* SpeakerActor, const FUOUDialogueLine& Line);

	// Blueprint event used to clear dialogue UI and related presentation.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "HUD|Dialogue")
	void HideDialogue();

	// Blueprint event used to display a chapter, place, or stage title card.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "HUD|Title")
	void ShowTitleCard(const FUOUTitleDisplayData& TitleData);

private:
	AUOUMenuPlayerController* GetMenuPlayerController() const;
	UUOUUISubsystem* GetUISubsystem() const;
};
