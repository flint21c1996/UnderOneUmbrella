// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUUISubsystem.h"

#include "Game/UOUInGameHUDWidget.h"
#include "Player/UOUWaterContainerComponent.h"
#include "TimerManager.h"
#include "UI/UOUDialogueSourceComponent.h"

namespace
{
	FText NormalizeUIDialogueDisplayText(const FText& SourceText)
	{
		if (SourceText.IsEmpty())
		{
			return SourceText;
		}

		// 인라인 대사나 데이터 에셋에서도 \n을 적으면 실제 줄바꿈으로 보이게 맞춥니다.
		FString DisplayString = SourceText.ToString();
		DisplayString.ReplaceInline(TEXT("\\r\\n"), TEXT("\n"), ESearchCase::CaseSensitive);
		DisplayString.ReplaceInline(TEXT("\\n"), TEXT("\n"), ESearchCase::CaseSensitive);
		DisplayString.ReplaceInline(TEXT("\\t"), TEXT("\t"), ESearchCase::CaseSensitive);

		return FText::FromString(DisplayString);
	}

	FUOUDialogueLine NormalizeDialogueLineForDisplay(const FUOUDialogueLine& SourceLine)
	{
		FUOUDialogueLine DisplayLine = SourceLine;
		DisplayLine.BubbleText = NormalizeUIDialogueDisplayText(SourceLine.BubbleText);
		DisplayLine.DialogueText = NormalizeUIDialogueDisplayText(SourceLine.DialogueText);
		return DisplayLine;
	}
}

void UUOUUISubsystem::RegisterHUD(UUOUInGameHUDWidget* InHUDWidget)
{
	if (InHUDWidget == nullptr)
	{
		return;
	}

	RegisteredHUDWidget = InHUDWidget;
	BroadcastUmbrellaHUDState();
}

void UUOUUISubsystem::UnregisterHUD(UUOUInGameHUDWidget* InHUDWidget)
{
	if (RegisteredHUDWidget.Get() == InHUDWidget)
	{
		RegisteredHUDWidget.Reset();
	}
}

void UUOUUISubsystem::BindUmbrellaComponent(UUOUUmbrellaComponent* InUmbrellaComponent)
{
	if (BoundUmbrellaComponent.Get() == InUmbrellaComponent)
	{
		BroadcastUmbrellaHUDState();
		return;
	}

	UnbindUmbrellaComponent();

	BoundUmbrellaComponent = InUmbrellaComponent;
	if (BoundUmbrellaComponent.IsValid())
	{
		BoundUmbrellaComponent->OnUmbrellaStateChanged.AddDynamic(this, &UUOUUISubsystem::HandleUmbrellaStateChanged);
		BoundUmbrellaComponent->OnRainBlocked.AddDynamic(this, &UUOUUISubsystem::HandleUmbrellaRainBlocked);

		BoundWaterContainerComponent = BoundUmbrellaComponent->StoredWaterContainer.Get();
		if (!BoundWaterContainerComponent.IsValid())
		{
			BoundWaterContainerComponent = BoundUmbrellaComponent->GetOwner() != nullptr
				? BoundUmbrellaComponent->GetOwner()->FindComponentByClass<UUOUWaterContainerComponent>()
				: nullptr;
		}

		if (BoundWaterContainerComponent.IsValid())
		{
			BoundWaterContainerComponent->OnWaterAmountChanged.AddDynamic(this, &UUOUUISubsystem::HandleUmbrellaWaterChanged);
		}
	}

	BroadcastUmbrellaHUDState();
}

void UUOUUISubsystem::UnbindUmbrellaComponent()
{
	if (BoundUmbrellaComponent.IsValid())
	{
		BoundUmbrellaComponent->OnUmbrellaStateChanged.RemoveAll(this);
		BoundUmbrellaComponent->OnRainBlocked.RemoveAll(this);
	}

	if (BoundWaterContainerComponent.IsValid())
	{
		BoundWaterContainerComponent->OnWaterAmountChanged.RemoveAll(this);
	}

	BoundUmbrellaComponent.Reset();
	BoundWaterContainerComponent.Reset();
	LastRainBlockedAmount = 0.0f;
}

void UUOUUISubsystem::RefreshUmbrellaHUDState()
{
	BroadcastUmbrellaHUDState();
}

FUOUUmbrellaHUDState UUOUUISubsystem::GetCurrentUmbrellaHUDState() const
{
	return BuildUmbrellaHUDState();
}

void UUOUUISubsystem::StartDialogue(UUOUDialogueSourceComponent* DialogueSource, AActor* InstigatorActor)
{
	if (DialogueSource == nullptr || DialogueSource->GetLineCount() <= 0)
	{
		return;
	}

	ClearDialogueTimer();
	ActiveDialogueSource = DialogueSource;
	ActiveDialogueInstigator = InstigatorActor;
	ActiveDialogueIndex = INDEX_NONE;

	AActor* SpeakerActor = DialogueSource->GetSpeakerActor();
	OnDialogueStarted.Broadcast(SpeakerActor, DialogueSource);
	if (RegisteredHUDWidget.IsValid())
	{
		RegisteredHUDWidget->BeginDialoguePresentation(SpeakerActor, DialogueSource);
	}

	AdvanceDialogue();
}

void UUOUUISubsystem::AdvanceDialogue()
{
	if (bHasPendingDialogueLine)
	{
		ClearDialogueTimer();
		ShowPendingDialogueLine();
		return;
	}

	if (!ActiveDialogueSource.IsValid())
	{
		FinishDialogue(false);
		return;
	}

	ClearDialogueTimer();
	++ActiveDialogueIndex;

	const FUOUDialogueLine* NextLine = ActiveDialogueSource->GetLine(ActiveDialogueIndex);
	if (NextLine == nullptr)
	{
		FinishDialogue(true);
		return;
	}

	FUOUDialogueLine DisplayLine = NormalizeDialogueLineForDisplay(*NextLine);
	if (DisplayLine.SpeakerName.IsEmpty())
	{
		DisplayLine.SpeakerName = ActiveDialogueSource->GetSpeakerName();
	}

	if (DisplayLine.bShowBubbleFirst && !DisplayLine.BubbleText.IsEmpty() && DisplayLine.BubbleDuration > 0.0f)
	{
		PendingDialogueLine = DisplayLine;
		bHasPendingDialogueLine = true;
		BroadcastDialogueBubble(DisplayLine);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(DialogueTimerHandle, this, &UUOUUISubsystem::ShowPendingDialogueLine, DisplayLine.BubbleDuration, false);
		}

		return;
	}

	BroadcastCurrentDialogueLine(DisplayLine);
}

void UUOUUISubsystem::EndDialogue()
{
	FinishDialogue(false);
}

void UUOUUISubsystem::FinishDialogue(bool bCompletedNaturally)
{
	ClearDialogueTimer();
	UUOUDialogueSourceComponent* CompletedDialogueSource = bCompletedNaturally
		? ActiveDialogueSource.Get()
		: nullptr;

	ActiveDialogueSource.Reset();
	ActiveDialogueInstigator.Reset();
	ActiveDialogueIndex = INDEX_NONE;
	bHasPendingDialogueLine = false;

	OnDialogueEnded.Broadcast();
	if (RegisteredHUDWidget.IsValid())
	{
		RegisteredHUDWidget->HideDialogue();
	}

	if (CompletedDialogueSource != nullptr)
	{
		OnDialogueCompleted.Broadcast(CompletedDialogueSource);
	}
}

bool UUOUUISubsystem::IsDialoguePlaying() const
{
	return ActiveDialogueSource.IsValid() && ActiveDialogueIndex != INDEX_NONE;
}

bool UUOUUISubsystem::StartBubbleOnlyDialogue(UUOUDialogueSourceComponent* DialogueSource)
{
	if (DialogueSource == nullptr
		|| !DialogueSource->IsDialogueBubbleEnabled()
		|| DialogueSource->GetLineCount() <= 0)
	{
		return false;
	}

	StopBubbleOnlyDialogueForSource(DialogueSource);
	BubbleOnlyPlaybackStates.Add(DialogueSource);
	return AdvanceBubbleOnlyDialogue(DialogueSource);
}

void UUOUUISubsystem::StopBubbleOnlyDialogue()
{
	if (UWorld* World = GetWorld())
	{
		for (TPair<TWeakObjectPtr<UUOUDialogueSourceComponent>, FUOUBubbleOnlyPlaybackState>& PlaybackEntry
			: BubbleOnlyPlaybackStates)
		{
			World->GetTimerManager().ClearTimer(PlaybackEntry.Value.TimerHandle);
		}
	}

	BubbleOnlyPlaybackStates.Reset();
}

void UUOUUISubsystem::StopBubbleOnlyDialogueForSource(UUOUDialogueSourceComponent* DialogueSource)
{
	if (DialogueSource == nullptr)
	{
		return;
	}

	ClearBubbleOnlyDialogueTimer(DialogueSource);
	BubbleOnlyPlaybackStates.Remove(DialogueSource);
}

bool UUOUUISubsystem::IsBubbleOnlyDialoguePlayingForSource(
	UUOUDialogueSourceComponent* DialogueSource) const
{
	return DialogueSource != nullptr && BubbleOnlyPlaybackStates.Contains(DialogueSource);
}

void UUOUUISubsystem::ShowTitle(const FUOUTitleDisplayData& TitleData)
{
	OnTitleRequested.Broadcast(TitleData);
	if (RegisteredHUDWidget.IsValid())
	{
		RegisteredHUDWidget->ShowTitleCard(TitleData);
	}
}

bool UUOUUISubsystem::ShowRewardPresentationCue(
	const FUOURewardPresentationData& PresentationData,
	const FUOURewardPresentationCue& Cue)
{
	if (RegisteredHUDWidget.IsValid())
	{
		return RegisteredHUDWidget->ProcessRewardPresentationCue(
			PresentationData,
			Cue);
	}

	return false;
}

void UUOUUISubsystem::NotifyRewardPresentationFinished(FName RewardId)
{
	if (RewardId.IsNone())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Reward Presentation finished without a valid RewardId."));
		return;
	}

	OnRewardPresentationFinished.Broadcast(RewardId);
}

void UUOUUISubsystem::HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella)
{
	BroadcastUmbrellaHUDState();
}

void UUOUUISubsystem::HandleUmbrellaWaterChanged(float NewAmount, float MaxAmount)
{
	BroadcastUmbrellaHUDState();
}

void UUOUUISubsystem::HandleUmbrellaRainBlocked(float BlockedAmount)
{
	LastRainBlockedAmount = BlockedAmount;
	BroadcastUmbrellaHUDState();
}

FUOUUmbrellaHUDState UUOUUISubsystem::BuildUmbrellaHUDState() const
{
	FUOUUmbrellaHUDState State;
	if (!BoundUmbrellaComponent.IsValid())
	{
		return State;
	}

	State.bHasUmbrella = BoundUmbrellaComponent->HasUmbrella();
	State.UmbrellaState = BoundUmbrellaComponent->CurrentState;
	State.UmbrellaVisualState = BoundUmbrellaComponent->GetCurrentVisualState();
	State.StoredWater = BoundUmbrellaComponent->GetCurrentStoredWater();
	State.PlayerRainAmount = BoundUmbrellaComponent->GetCurrentPlayerRainAmount();
	State.bBlockingRain = BoundUmbrellaComponent->IsBlockingRain() || LastRainBlockedAmount > 0.0f;

	if (BoundWaterContainerComponent.IsValid())
	{
		State.MaxStoredWater = BoundWaterContainerComponent->MaxAmount;
		State.StoredWaterRatio = BoundWaterContainerComponent->GetFillRatio();
	}
	else
	{
		State.MaxStoredWater = 0.0f;
		State.StoredWaterRatio = 0.0f;
	}

	return State;
}

void UUOUUISubsystem::BroadcastUmbrellaHUDState()
{
	const FUOUUmbrellaHUDState State = BuildUmbrellaHUDState();
	OnUmbrellaHUDStateChanged.Broadcast(State);

	if (RegisteredHUDWidget.IsValid())
	{
		RegisteredHUDWidget->ApplyUmbrellaHUDState(State);
		RegisteredHUDWidget->HandleUmbrellaHUDStateChanged(State);
	}
}

void UUOUUISubsystem::ClearDialogueTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DialogueTimerHandle);
	}
}

void UUOUUISubsystem::ClearBubbleOnlyDialogueTimer(UUOUDialogueSourceComponent* DialogueSource)
{
	FUOUBubbleOnlyPlaybackState* PlaybackState = DialogueSource != nullptr
		? BubbleOnlyPlaybackStates.Find(DialogueSource)
		: nullptr;
	if (PlaybackState != nullptr)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PlaybackState->TimerHandle);
		}
	}
}

void UUOUUISubsystem::BroadcastCurrentDialogueLine(const FUOUDialogueLine& Line)
{
	BroadcastDialogueBubble(Line);
	BroadcastDialogueBoxLine(Line);
}

void UUOUUISubsystem::BroadcastDialogueBubble(const FUOUDialogueLine& Line)
{
	const FUOUDialogueLine DisplayLine = NormalizeDialogueLineForDisplay(Line);
	AActor* SpeakerActor = ActiveDialogueSource.IsValid() ? ActiveDialogueSource->GetSpeakerActor() : nullptr;
	if (ActiveDialogueSource.IsValid() && ActiveDialogueSource->IsDialogueBubbleEnabled() && !DisplayLine.BubbleText.IsEmpty())
	{
		OnDialogueBubbleRequested.Broadcast(SpeakerActor, DisplayLine.BubbleText, DisplayLine.BubbleDuration);
		if (RegisteredHUDWidget.IsValid())
		{
			RegisteredHUDWidget->ShowNPCSpeechBubbleStyled(
				SpeakerActor,
				DisplayLine.BubbleText,
				DisplayLine.BubbleDuration,
				DisplayLine.PresentationStyle);
		}
	}
}

void UUOUUISubsystem::BroadcastDialogueBoxLine(const FUOUDialogueLine& Line)
{
	const FUOUDialogueLine DisplayLine = NormalizeDialogueLineForDisplay(Line);
	AActor* SpeakerActor = ActiveDialogueSource.IsValid() ? ActiveDialogueSource->GetSpeakerActor() : nullptr;
	OnDialogueLineStarted.Broadcast(SpeakerActor, DisplayLine);
	if (RegisteredHUDWidget.IsValid())
	{
		RegisteredHUDWidget->ShowDialogueLine(SpeakerActor, DisplayLine);
	}
}

void UUOUUISubsystem::ShowPendingDialogueLine()
{
	if (!bHasPendingDialogueLine)
	{
		return;
	}

	const FUOUDialogueLine LineToShow = PendingDialogueLine;
	bHasPendingDialogueLine = false;

	BroadcastDialogueBoxLine(LineToShow);
}

bool UUOUUISubsystem::AdvanceBubbleOnlyDialogue(
	TWeakObjectPtr<UUOUDialogueSourceComponent> WeakDialogueSource)
{
	UUOUDialogueSourceComponent* DialogueSource = WeakDialogueSource.Get();
	if (DialogueSource == nullptr || !DialogueSource->IsDialogueBubbleEnabled())
	{
		BubbleOnlyPlaybackStates.Remove(WeakDialogueSource);
		return false;
	}

	FUOUBubbleOnlyPlaybackState* PlaybackState = BubbleOnlyPlaybackStates.Find(WeakDialogueSource);
	if (PlaybackState == nullptr)
	{
		return false;
	}

	ClearBubbleOnlyDialogueTimer(DialogueSource);
	while (PlaybackState->NextLineIndex >= 0
		&& PlaybackState->NextLineIndex < DialogueSource->GetLineCount())
	{
		const FUOUDialogueLine* SourceLine = DialogueSource->GetLine(PlaybackState->NextLineIndex++);
		if (SourceLine == nullptr)
		{
			continue;
		}

		const FUOUDialogueLine DisplayLine = NormalizeDialogueLineForDisplay(*SourceLine);
		if (DisplayLine.BubbleText.IsEmpty() || DisplayLine.BubbleDuration <= 0.0f)
		{
			continue;
		}

		AActor* SpeakerActor = DialogueSource->GetSpeakerActor();
		OnDialogueBubbleRequested.Broadcast(SpeakerActor, DisplayLine.BubbleText, DisplayLine.BubbleDuration);
		if (RegisteredHUDWidget.IsValid())
		{
			RegisteredHUDWidget->ShowNPCSpeechBubbleStyled(
				SpeakerActor,
				DisplayLine.BubbleText,
				DisplayLine.BubbleDuration,
				DisplayLine.PresentationStyle);
		}

		if (UWorld* World = GetWorld())
		{
			const FTimerDelegate AdvanceDelegate = FTimerDelegate::CreateWeakLambda(
				this,
				[this, WeakDialogueSource]()
				{
					AdvanceBubbleOnlyDialogue(WeakDialogueSource);
				});
			World->GetTimerManager().SetTimer(
				PlaybackState->TimerHandle,
				AdvanceDelegate,
				DisplayLine.BubbleDuration,
				false);
		}
		return true;
	}

	StopBubbleOnlyDialogueForSource(DialogueSource);
	return false;
}
