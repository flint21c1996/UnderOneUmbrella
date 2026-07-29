// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/UOUUITypes.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "UOUUISubsystem.generated.h"

class UUOUDialogueSourceComponent;
class UUOUInGameHUDWidget;
class UUOUWaterContainerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUUmbrellaHUDStateChangedSignature, const FUOUUmbrellaHUDState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUOUDialogueStartedSignature, AActor*, SpeakerActor, UUOUDialogueSourceComponent*, DialogueSource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUOUDialogueBubbleRequestedSignature, AActor*, SpeakerActor, const FText&, BubbleText, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUOUDialogueLineStartedSignature, AActor*, SpeakerActor, const FUOUDialogueLine&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOUDialogueEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUTitleRequestedSignature, const FUOUTitleDisplayData&, TitleData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUOURewardPresentationCueRequestedSignature,
	const FUOURewardPresentationData&,
	PresentationData,
	const FUOURewardPresentationCue&,
	Cue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOURewardPresentationFinishedSignature,
	FName,
	RewardId);

// Local player subsystem that bridges gameplay UI requests to the in-game HUD widget.
// UMG owns the visuals, while this class owns event routing and dialogue progression.
UCLASS()
class UNDERONEUMBRELLA_API UUOUUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "UI|Umbrella")
	FUOUUmbrellaHUDStateChangedSignature OnUmbrellaHUDStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI|Dialogue")
	FUOUDialogueStartedSignature OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "UI|Dialogue")
	FUOUDialogueBubbleRequestedSignature OnDialogueBubbleRequested;

	UPROPERTY(BlueprintAssignable, Category = "UI|Dialogue")
	FUOUDialogueLineStartedSignature OnDialogueLineStarted;

	UPROPERTY(BlueprintAssignable, Category = "UI|Dialogue")
	FUOUDialogueEndedSignature OnDialogueEnded;

	UPROPERTY(BlueprintAssignable, Category = "UI|Title")
	FUOUTitleRequestedSignature OnTitleRequested;

	UPROPERTY(BlueprintAssignable, Category = "UI|Reward")
	FUOURewardPresentationCueRequestedSignature OnRewardPresentationCueRequested;

	UPROPERTY(BlueprintAssignable, Category = "UI|Reward")
	FUOURewardPresentationFinishedSignature OnRewardPresentationFinished;

	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	void RegisterHUD(UUOUInGameHUDWidget* InHUDWidget);

	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	void UnregisterHUD(UUOUInGameHUDWidget* InHUDWidget);

	UFUNCTION(BlueprintCallable, Category = "UI|Umbrella")
	void BindUmbrellaComponent(UUOUUmbrellaComponent* InUmbrellaComponent);

	UFUNCTION(BlueprintCallable, Category = "UI|Umbrella")
	void UnbindUmbrellaComponent();

	UFUNCTION(BlueprintCallable, Category = "UI|Umbrella")
	void RefreshUmbrellaHUDState();

	UFUNCTION(BlueprintPure, Category = "UI|Umbrella")
	FUOUUmbrellaHUDState GetCurrentUmbrellaHUDState() const;

	UFUNCTION(BlueprintCallable, Category = "UI|Dialogue")
	void StartDialogue(UUOUDialogueSourceComponent* DialogueSource, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category = "UI|Dialogue")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "UI|Dialogue")
	void EndDialogue();

	UFUNCTION(BlueprintPure, Category = "UI|Dialogue")
	bool IsDialoguePlaying() const;

	UFUNCTION(BlueprintPure, Category = "UI|Dialogue")
	int32 GetActiveDialogueIndex() const { return ActiveDialogueIndex; }

	UFUNCTION(BlueprintCallable, Category = "UI|Title")
	void ShowTitle(const FUOUTitleDisplayData& TitleData);

	UFUNCTION(BlueprintCallable, Category = "UI|Reward")
	void ShowRewardPresentationCue(
		const FUOURewardPresentationData& PresentationData,
		const FUOURewardPresentationCue& Cue);

	UFUNCTION(BlueprintCallable, Category = "UI|Reward")
	void NotifyRewardPresentationFinished(FName RewardId);

private:
	UFUNCTION()
	void HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella);

	UFUNCTION()
	void HandleUmbrellaWaterChanged(float NewAmount, float MaxAmount);

	UFUNCTION()
	void HandleUmbrellaRainBlocked(float BlockedAmount);

	FUOUUmbrellaHUDState BuildUmbrellaHUDState() const;
	void BroadcastUmbrellaHUDState();
	void ClearDialogueTimer();
	void ScheduleAutoAdvanceIfNeeded(const FUOUDialogueLine& Line);
	void BroadcastCurrentDialogueLine(const FUOUDialogueLine& Line);
	void BroadcastDialogueBubble(const FUOUDialogueLine& Line);
	void BroadcastDialogueBoxLine(const FUOUDialogueLine& Line);

	UFUNCTION()
	void ShowPendingDialogueLine();

	TWeakObjectPtr<UUOUInGameHUDWidget> RegisteredHUDWidget;
	TWeakObjectPtr<UUOUUmbrellaComponent> BoundUmbrellaComponent;
	TWeakObjectPtr<UUOUWaterContainerComponent> BoundWaterContainerComponent;
	TWeakObjectPtr<UUOUDialogueSourceComponent> ActiveDialogueSource;
	TWeakObjectPtr<AActor> ActiveDialogueInstigator;

	FTimerHandle DialogueTimerHandle;
	FUOUDialogueLine PendingDialogueLine;
	int32 ActiveDialogueIndex = INDEX_NONE;
	float LastRainBlockedAmount = 0.0f;
	bool bHasPendingDialogueLine = false;
};
