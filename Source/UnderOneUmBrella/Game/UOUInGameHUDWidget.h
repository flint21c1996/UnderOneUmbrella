// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UOUUITypes.h"
#include "UOUInGameHUDWidget.generated.h"

class AUOUMenuPlayerController;
class UUOUDialogueBoxWidget;
class UUOUDialogueSourceComponent;
class UUOUUmbrellaStatusWidget;
class UUOUUISubsystem;
class UUserWidget;
class UWidgetComponent;

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

	UFUNCTION(BlueprintCallable, Category = "HUD|Level Travel")
	void ReturnToTitle();

	UFUNCTION(BlueprintCallable, Category = "HUD|Level Travel")
	void RestartCurrentStage();

	UFUNCTION(BlueprintCallable, Category = "HUD|Level Travel")
	void GoToNextLevel();

	UFUNCTION(BlueprintCallable, Category = "HUD|Level Travel")
	void GoToPreviousLevel();

	UFUNCTION(BlueprintPure, Category = "HUD|Level Travel")
	bool CanRestartCurrentStage() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Level Travel")
	bool CanReturnToTitle() const;

	// Finds the player's umbrella component and connects it to the HUD state stream.
	UFUNCTION(BlueprintCallable, Category = "HUD|Umbrella")
	void BindToPlayerUmbrella();

	// Called by the dialogue UI when the player advances to the next line.
	UFUNCTION(BlueprintCallable, Category = "HUD|Dialogue")
	void AdvanceDialogue();

	// HUD BP에 배치된 대화 박스 위젯을 직접 지정합니다.
	UFUNCTION(BlueprintCallable, Category = "HUD|Dialogue")
	void SetDialogueBoxWidget(UUOUDialogueBoxWidget* InDialogueBoxWidget);

	UFUNCTION(BlueprintCallable, Category = "HUD|Umbrella")
	void SetUmbrellaStatusWidget(UUOUUmbrellaStatusWidget* InUmbrellaStatusWidget);

	// Requests a title card from Blueprint or level triggers.
	UFUNCTION(BlueprintCallable, Category = "HUD|Title")
	void ShowTitle(const FUOUTitleDisplayData& TitleData);

	// Applies umbrella HUD data to C++ child widgets before optional Blueprint presentation logic runs.
	UFUNCTION(BlueprintCallable, Category = "HUD|Umbrella")
	void ApplyUmbrellaHUDState(const FUOUUmbrellaHUDState& State);

	// Blueprint event used to redraw umbrella icon, state, rain block, and water gauge.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "HUD|Umbrella")
	void HandleUmbrellaHUDStateChanged(const FUOUUmbrellaHUDState& State);

	// Blueprint event used once when dialogue starts, before individual lines are shown.
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "HUD|Dialogue")
	void BeginDialoguePresentation(AActor* SpeakerActor, UUOUDialogueSourceComponent* DialogueSource);
	virtual void BeginDialoguePresentation_Implementation(AActor* SpeakerActor, UUOUDialogueSourceComponent* DialogueSource);

	// NPC나 오브젝트 위의 WidgetComponent를 찾아 짧은 말풍선을 표시합니다.
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "HUD|Dialogue")
	void ShowNPCSpeechBubble(AActor* SpeakerActor, FText BubbleText, float Duration);

	// BP에서 추가 연출이 필요할 때 사용하는 선택 진입점입니다. 기본 말풍선 표시는 C++에서 먼저 처리합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "HUD|Dialogue")
	void BP_OnNPCSpeechBubbleRequested(AActor* SpeakerActor, const FText& BubbleText, float Duration);

	// Blueprint event used to display one bottom dialogue-box line.
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "HUD|Dialogue")
	void ShowDialogueLine(AActor* SpeakerActor, const FUOUDialogueLine& Line);
	virtual void ShowDialogueLine_Implementation(AActor* SpeakerActor, const FUOUDialogueLine& Line);

	// Blueprint event used to clear dialogue UI and related presentation.
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "HUD|Dialogue")
	void HideDialogue();
	virtual void HideDialogue_Implementation();

	// Blueprint event used to display a chapter, place, or stage title card.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "HUD|Title")
	void ShowTitleCard(const FUOUTitleDisplayData& TitleData);

private:
	AUOUMenuPlayerController* GetMenuPlayerController() const;
	UUOUUISubsystem* GetUISubsystem() const;
	UWidgetComponent* ResolveSpeechBubbleWidgetComponent(AActor* SpeakerActor) const;
	UUOUUmbrellaStatusWidget* ResolveUmbrellaStatusWidget();

	// WBP_InGameHUD 안에 같은 이름으로 배치하면 자동 연결됩니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD|Dialogue")
	TObjectPtr<UUOUDialogueBoxWidget> DialogueBoxWidget = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD|Umbrella")
	TObjectPtr<UUOUUmbrellaStatusWidget> UmbrellaStatusWidget = nullptr;
};
