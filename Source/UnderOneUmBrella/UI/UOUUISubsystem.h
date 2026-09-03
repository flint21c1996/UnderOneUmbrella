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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOUDialogueCompletedSignature,
	UUOUDialogueSourceComponent*,
	DialogueSource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUTitleRequestedSignature, const FUOUTitleDisplayData&, TitleData);
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

	// 마지막 대사까지 정상 진행한 DialogueSource를 알립니다. 취소성 EndDialogue 호출에는 발생하지 않습니다.
	UPROPERTY(BlueprintAssignable, Category = "UI|Dialogue")
	FUOUDialogueCompletedSignature OnDialogueCompleted;

	UPROPERTY(BlueprintAssignable, Category = "UI|Title")
	FUOUTitleRequestedSignature OnTitleRequested;

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

	// DialogueSource의 현재 상태에서 BubbleText만 순서대로 재생합니다.
	// 일반 대화 표시나 완료 이벤트와 독립적으로 동작합니다.
	UFUNCTION(BlueprintCallable, Category = "UI|Dialogue Bubble")
	bool StartBubbleOnlyDialogue(UUOUDialogueSourceComponent* DialogueSource);

	UFUNCTION(BlueprintCallable, Category = "UI|Dialogue Bubble")
	void StopBubbleOnlyDialogue();

	// 지정한 DialogueSource의 Bubble-only 재생만 중단하고 다른 NPC의 재생은 유지합니다.
	UFUNCTION(BlueprintCallable, Category = "UI|Dialogue Bubble")
	void StopBubbleOnlyDialogueForSource(UUOUDialogueSourceComponent* DialogueSource);

	UFUNCTION(BlueprintPure, Category = "UI|Dialogue Bubble")
	bool IsBubbleOnlyDialoguePlaying() const { return !BubbleOnlyPlaybackStates.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "UI|Dialogue Bubble")
	bool IsBubbleOnlyDialoguePlayingForSource(UUOUDialogueSourceComponent* DialogueSource) const;

	UFUNCTION(BlueprintCallable, Category = "UI|Title")
	void ShowTitle(const FUOUTitleDisplayData& TitleData);

	UFUNCTION(BlueprintCallable, Category = "UI|Reward")
	bool ShowRewardPresentationCue(
		const FUOURewardPresentationData& PresentationData,
		const FUOURewardPresentationCue& Cue);

	UFUNCTION(BlueprintCallable, Category = "UI|Reward")
	void NotifyRewardPresentationFinished(FName RewardId);

private:
	void FinishDialogue(bool bCompletedNaturally);

	UFUNCTION()
	void HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella);

	UFUNCTION()
	void HandleUmbrellaWaterChanged(float NewAmount, float MaxAmount);

	UFUNCTION()
	void HandleUmbrellaRainBlocked(float BlockedAmount);

	FUOUUmbrellaHUDState BuildUmbrellaHUDState() const;
	void BroadcastUmbrellaHUDState();
	void ClearDialogueTimer();
	void BroadcastCurrentDialogueLine(const FUOUDialogueLine& Line);
	void BroadcastDialogueBubble(const FUOUDialogueLine& Line);
	void BroadcastDialogueBoxLine(const FUOUDialogueLine& Line);
	void ClearBubbleOnlyDialogueTimer(UUOUDialogueSourceComponent* DialogueSource);

	UFUNCTION()
	void ShowPendingDialogueLine();

	bool AdvanceBubbleOnlyDialogue(TWeakObjectPtr<UUOUDialogueSourceComponent> DialogueSource);

	struct FUOUBubbleOnlyPlaybackState
	{
		// 이 Source에서 다음으로 검사할 Dialogue 줄 번호입니다.
		int32 NextLineIndex = 0;

		// 이 Source의 다음 Bubble 호출만 관리하는 독립 타이머입니다.
		FTimerHandle TimerHandle;
	};

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

	// NPC별 Bubble-only 재생 상태입니다. Source가 끝나거나 중단되면 해당 항목만 제거됩니다.
	TMap<TWeakObjectPtr<UUOUDialogueSourceComponent>, FUOUBubbleOnlyPlaybackState> BubbleOnlyPlaybackStates;
};
