// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUBubbleConversationComponent.h"

#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UI/UOUUISubsystem.h"

UUOUBubbleConversationComponent::UUOUBubbleConversationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUBubbleConversationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bStartOnBeginPlay)
	{
		bActivated = true;
		StartConversation();
	}
}

void UUOUBubbleConversationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishConversation(false, false);
	Super::EndPlay(EndPlayReason);
}

bool UUOUBubbleConversationComponent::StartConversation()
{
	if (bIsPlaying)
	{
		if (!bRestartWhenActivated)
		{
			return false;
		}

		FinishConversation(false);
	}

	bActivated = true;
	SetActivationResultCompleted(false);
	SetDeactivationResultCompleted(false);

	if (GetUISubsystem() == nullptr || FindNextPlayableLineIndex(0) == INDEX_NONE)
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlaybackTimerHandle);
	}

	bIsPlaying = true;
	bIsPaused = false;
	CurrentLineIndex = INDEX_NONE;
	CurrentSpeakerActor.Reset();
	CurrentSpeechBubbleWidgetComponent.Reset();
	PlaybackPhase = EPlaybackPhase::None;
	PausedPhaseRemainingSeconds = 0.0f;

	OnConversationStarted.Broadcast();
	AdvanceToNextLine();
	return true;
}

void UUOUBubbleConversationComponent::StopConversation()
{
	FinishConversation(false);
}

void UUOUBubbleConversationComponent::PauseConversation()
{
	if (!bIsPlaying || bIsPaused)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		PausedPhaseRemainingSeconds = FMath::Max(
			0.0f,
			World->GetTimerManager().GetTimerRemaining(PlaybackTimerHandle));
		World->GetTimerManager().ClearTimer(PlaybackTimerHandle);
	}

	if (PlaybackPhase == EPlaybackPhase::ShowingLine)
	{
		HideCurrentSpeakerBubble(true);
	}

	bIsPaused = true;
}

void UUOUBubbleConversationComponent::ResumeConversation()
{
	if (!bIsPlaying || !bIsPaused)
	{
		return;
	}

	bIsPaused = false;
	if (PlaybackPhase == EPlaybackPhase::WaitingBeforeLine)
	{
		ScheduleShowCurrentLine(PausedPhaseRemainingSeconds);
		return;
	}

	if (PlaybackPhase == EPlaybackPhase::WaitingAfterLine)
	{
		ScheduleAdvance(PausedPhaseRemainingSeconds);
		return;
	}

	if (PlaybackPhase == EPlaybackPhase::ShowingLine)
	{
		const TArray<FUOUBubbleConversationLine>& Lines = GetResolvedLines();
		if (!Lines.IsValidIndex(CurrentLineIndex)
			|| !IsLinePlayable(Lines[CurrentLineIndex])
			|| PausedPhaseRemainingSeconds <= 0.0f)
		{
			AdvanceToNextLine();
			return;
		}

		const FUOUBubbleConversationLine& Line = Lines[CurrentLineIndex];
		UUOUUISubsystem* UISubsystem = GetUISubsystem();
		const bool bShown = UISubsystem != nullptr
			&& (CurrentSpeechBubbleWidgetComponent.IsValid()
				? UISubsystem->ShowSpeechBubbleOnComponent(
					CurrentSpeakerActor.Get(),
					CurrentSpeechBubbleWidgetComponent.Get(),
					Line.BubbleText,
					PausedPhaseRemainingSeconds,
					Line.PresentationStyle,
					Line.FontSizeOverride)
				: UISubsystem->ShowSpeechBubble(
					CurrentSpeakerActor.Get(),
					Line.BubbleText,
					PausedPhaseRemainingSeconds,
					Line.PresentationStyle));
		if (!bShown)
		{
			AdvanceToNextLine();
			return;
		}
		ScheduleBeginWaitingAfterLine(PausedPhaseRemainingSeconds);
	}
}

int32 UUOUBubbleConversationComponent::GetConversationLineCount() const
{
	return GetResolvedLines().Num();
}

AActor* UUOUBubbleConversationComponent::ResolveSpeakerActor(FName SpeakerId) const
{
	if (SpeakerId.IsNone())
	{
		return nullptr;
	}

	for (const FUOUBubbleConversationParticipant& Participant : Participants)
	{
		if (Participant.ParticipantId != SpeakerId)
		{
			continue;
		}

		if (UWidgetComponent* SpeechBubbleWidgetComponent = Cast<UWidgetComponent>(
			Participant.SpeechBubbleWidgetReference.GetComponent(GetOwner())))
		{
			return SpeechBubbleWidgetComponent->GetOwner();
		}

		if (IsValid(Participant.SpeakerActor))
		{
			return Participant.SpeakerActor.Get();
		}
	}

	return nullptr;
}

UWidgetComponent* UUOUBubbleConversationComponent::ResolveSpeakerBubbleWidgetComponent(FName SpeakerId) const
{
	if (SpeakerId.IsNone())
	{
		return nullptr;
	}

	for (const FUOUBubbleConversationParticipant& Participant : Participants)
	{
		if (Participant.ParticipantId != SpeakerId)
		{
			continue;
		}

		return Cast<UWidgetComponent>(
			Participant.SpeechBubbleWidgetReference.GetComponent(GetOwner()));
	}

	return nullptr;
}

void UUOUBubbleConversationComponent::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		bActivated = true;
		StartConversation();
		break;
	case EOUUPuzzleResultAction::Deactivate:
		bActivated = false;
		FinishConversation(false);
		SetActivationResultCompleted(false);
		SetDeactivationResultCompleted(true);
		break;
	case EOUUPuzzleResultAction::Pause:
		PauseConversation();
		break;
	case EOUUPuzzleResultAction::Resume:
		ResumeConversation();
		break;
	case EOUUPuzzleResultAction::Toggle:
		if (bIsPlaying)
		{
			bActivated = false;
			FinishConversation(false);
		}
		else
		{
			bActivated = true;
			StartConversation();
		}
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

bool UUOUBubbleConversationComponent::IsPuzzleResultCompleted_Implementation(EOUUPuzzleResultAction Action) const
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		return bHasCompletedResultSinceLastActivation;
	case EOUUPuzzleResultAction::Deactivate:
		return bHasCompletedResultSinceLastDeactivation;
	case EOUUPuzzleResultAction::None:
	case EOUUPuzzleResultAction::Pause:
	case EOUUPuzzleResultAction::Resume:
	case EOUUPuzzleResultAction::Toggle:
	default:
		return false;
	}
}

FOnUOUPuzzleResultCompletionStateChangedNativeSignature*
UUOUBubbleConversationComponent::GetPuzzleResultCompletionStateChangedEvent()
{
	return &OnPuzzleResultCompletionStateChanged;
}

const TArray<FUOUBubbleConversationLine>& UUOUBubbleConversationComponent::GetResolvedLines() const
{
	return ConversationData != nullptr && ConversationData->HasValidLines()
		? ConversationData->Lines
		: InlineLines;
}

int32 UUOUBubbleConversationComponent::FindNextPlayableLineIndex(int32 StartIndex) const
{
	const TArray<FUOUBubbleConversationLine>& Lines = GetResolvedLines();
	for (int32 LineIndex = FMath::Max(0, StartIndex); LineIndex < Lines.Num(); ++LineIndex)
	{
		if (IsLinePlayable(Lines[LineIndex]))
		{
			return LineIndex;
		}
	}

	return INDEX_NONE;
}

bool UUOUBubbleConversationComponent::IsLinePlayable(const FUOUBubbleConversationLine& Line) const
{
	return !Line.SpeakerId.IsNone()
		&& !Line.BubbleText.IsEmpty()
		&& Line.BubbleDuration > 0.0f
		&& ResolveSpeakerActor(Line.SpeakerId) != nullptr;
}

UUOUUISubsystem* UUOUBubbleConversationComponent::GetUISubsystem() const
{
	APlayerController* PlayerController = GetWorld() != nullptr
		? GetWorld()->GetFirstPlayerController()
		: nullptr;
	ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UUOUUISubsystem>() : nullptr;
}

void UUOUBubbleConversationComponent::AdvanceToNextLine()
{
	if (!bIsPlaying || bIsPaused)
	{
		return;
	}

	if (bHidePreviousBubbleWhenAdvancing)
	{
		HideCurrentSpeakerBubble(true);
	}

	const int32 NextLineIndex = FindNextPlayableLineIndex(CurrentLineIndex + 1);
	if (NextLineIndex == INDEX_NONE)
	{
		FinishConversation(true);
		return;
	}

	CurrentLineIndex = NextLineIndex;
	CurrentSpeakerActor = ResolveSpeakerActor(GetResolvedLines()[CurrentLineIndex].SpeakerId);
	CurrentSpeechBubbleWidgetComponent = ResolveSpeakerBubbleWidgetComponent(
		GetResolvedLines()[CurrentLineIndex].SpeakerId);
	PlaybackPhase = EPlaybackPhase::WaitingBeforeLine;
	ScheduleShowCurrentLine(GetResolvedLines()[CurrentLineIndex].DelayBefore);
}

void UUOUBubbleConversationComponent::ShowCurrentLine()
{
	if (!bIsPlaying || bIsPaused)
	{
		return;
	}

	const TArray<FUOUBubbleConversationLine>& Lines = GetResolvedLines();
	if (!Lines.IsValidIndex(CurrentLineIndex) || !IsLinePlayable(Lines[CurrentLineIndex]))
	{
		AdvanceToNextLine();
		return;
	}

	const FUOUBubbleConversationLine& Line = Lines[CurrentLineIndex];
	CurrentSpeakerActor = ResolveSpeakerActor(Line.SpeakerId);
	CurrentSpeechBubbleWidgetComponent = ResolveSpeakerBubbleWidgetComponent(Line.SpeakerId);
	UUOUUISubsystem* UISubsystem = GetUISubsystem();
	const bool bShown = UISubsystem != nullptr
		&& (CurrentSpeechBubbleWidgetComponent.IsValid()
			? UISubsystem->ShowSpeechBubbleOnComponent(
				CurrentSpeakerActor.Get(),
				CurrentSpeechBubbleWidgetComponent.Get(),
				Line.BubbleText,
				Line.BubbleDuration,
				Line.PresentationStyle,
				Line.FontSizeOverride)
			: UISubsystem->ShowSpeechBubble(
				CurrentSpeakerActor.Get(),
				Line.BubbleText,
				Line.BubbleDuration,
				Line.PresentationStyle));
	if (!bShown)
	{
		AdvanceToNextLine();
		return;
	}

	PlaybackPhase = EPlaybackPhase::ShowingLine;
	OnConversationLineStarted.Broadcast(CurrentLineIndex, Line.SpeakerId, CurrentSpeakerActor.Get());
	ScheduleBeginWaitingAfterLine(Line.BubbleDuration);
}

void UUOUBubbleConversationComponent::BeginWaitingAfterLine()
{
	if (!bIsPlaying || bIsPaused)
	{
		return;
	}

	const TArray<FUOUBubbleConversationLine>& Lines = GetResolvedLines();
	if (!Lines.IsValidIndex(CurrentLineIndex))
	{
		AdvanceToNextLine();
		return;
	}

	// 표시 시간이 끝나면 즉시 제거하지 않고 위젯의 FadeOut을 시작합니다.
	HideCurrentSpeakerBubble(false);
	PlaybackPhase = EPlaybackPhase::WaitingAfterLine;
	ScheduleAdvance(FMath::Max(0.0f, Lines[CurrentLineIndex].DelayAfter));
}

void UUOUBubbleConversationComponent::ScheduleShowCurrentLine(float DelaySeconds)
{
	if (DelaySeconds <= 0.0f)
	{
		ShowCurrentLine();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PlaybackTimerHandle,
			this,
			&UUOUBubbleConversationComponent::ShowCurrentLine,
			DelaySeconds,
			false);
	}
}

void UUOUBubbleConversationComponent::ScheduleBeginWaitingAfterLine(float DelaySeconds)
{
	if (DelaySeconds <= 0.0f)
	{
		BeginWaitingAfterLine();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PlaybackTimerHandle,
			this,
			&UUOUBubbleConversationComponent::BeginWaitingAfterLine,
			DelaySeconds,
			false);
	}
}

void UUOUBubbleConversationComponent::ScheduleAdvance(float DelaySeconds)
{
	if (DelaySeconds <= 0.0f)
	{
		AdvanceToNextLine();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PlaybackTimerHandle,
			this,
			&UUOUBubbleConversationComponent::AdvanceToNextLine,
			DelaySeconds,
			false);
	}
}

void UUOUBubbleConversationComponent::FinishConversation(bool bCompletedNaturally, bool bBroadcastFinished)
{
	const bool bWasPlaying = bIsPlaying;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlaybackTimerHandle);
	}

	HideCurrentSpeakerBubble(true);
	bIsPlaying = false;
	bIsPaused = false;
	CurrentLineIndex = INDEX_NONE;
	CurrentSpeakerActor.Reset();
	CurrentSpeechBubbleWidgetComponent.Reset();
	PlaybackPhase = EPlaybackPhase::None;
	PausedPhaseRemainingSeconds = 0.0f;

	if (bCompletedNaturally && bActivated)
	{
		SetActivationResultCompleted(true);
	}

	if (bWasPlaying && bBroadcastFinished)
	{
		OnConversationFinished.Broadcast(bCompletedNaturally);
	}
}

void UUOUBubbleConversationComponent::HideCurrentSpeakerBubble(bool bImmediately) const
{
	if (CurrentSpeakerActor.IsValid())
	{
		if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
		{
			if (CurrentSpeechBubbleWidgetComponent.IsValid())
			{
				UISubsystem->HideSpeechBubbleOnComponent(
					CurrentSpeakerActor.Get(),
					CurrentSpeechBubbleWidgetComponent.Get(),
					bImmediately);
			}
			else
			{
				UISubsystem->HideSpeechBubble(CurrentSpeakerActor.Get(), bImmediately);
			}
		}
	}
}

void UUOUBubbleConversationComponent::SetActivationResultCompleted(bool bNewCompleted)
{
	if (bHasCompletedResultSinceLastActivation == bNewCompleted)
	{
		return;
	}

	bHasCompletedResultSinceLastActivation = bNewCompleted;
	OnPuzzleResultCompletionStateChanged.Broadcast(EOUUPuzzleResultAction::Activate, bNewCompleted);
}

void UUOUBubbleConversationComponent::SetDeactivationResultCompleted(bool bNewCompleted)
{
	if (bHasCompletedResultSinceLastDeactivation == bNewCompleted)
	{
		return;
	}

	bHasCompletedResultSinceLastDeactivation = bNewCompleted;
	OnPuzzleResultCompletionStateChanged.Broadcast(EOUUPuzzleResultAction::Deactivate, bNewCompleted);
}
