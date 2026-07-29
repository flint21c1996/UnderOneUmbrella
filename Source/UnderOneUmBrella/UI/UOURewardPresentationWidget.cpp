// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOURewardPresentationWidget.h"

#include "Engine/World.h"

void UUOURewardPresentationWidget::NativeDestruct()
{
	ClearAutoCloseTimer();
	Super::NativeDestruct();
}

bool UUOURewardPresentationWidget::InitializePresentation(
	const FUOURewardPresentationData& InPresentationData)
{
	if (PresentationState != EUOURewardPresentationWidgetState::Uninitialized)
	{
		return false;
	}

	PresentationData = InPresentationData;
	PresentationState = EUOURewardPresentationWidgetState::Ready;
	bPresentationHoldStarted = false;
	ReceivePresentationInitialized(PresentationData);
	return true;
}

bool UUOURewardPresentationWidget::StartPresentation()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Ready)
	{
		return false;
	}

	PresentationState = EUOURewardPresentationWidgetState::Presenting;
	ReceivePresentationStarted();
	return true;
}

bool UUOURewardPresentationWidget::HandlePresentationCue(
	const FUOURewardPresentationCue& Cue)
{
	if (PresentationState != EUOURewardPresentationWidgetState::Presenting)
	{
		return false;
	}

	ReceivePresentationCue(Cue);
	return true;
}

bool UUOURewardPresentationWidget::BeginPresentationHold()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Presenting
		|| bPresentationHoldStarted)
	{
		return false;
	}

	ClearAutoCloseTimer();

	const float SafeDisplayDuration =
		FMath::Max(0.0f, PresentationData.DisplayDuration);
	if (SafeDisplayDuration <= KINDA_SMALL_NUMBER)
	{
		bPresentationHoldStarted = true;
		return true;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	bPresentationHoldStarted = true;
	World->GetTimerManager().SetTimer(
		AutoCloseTimerHandle,
		this,
		&UUOURewardPresentationWidget::HandleAutoCloseTimerElapsed,
		SafeDisplayDuration,
		false);
	return true;
}

bool UUOURewardPresentationWidget::RequestClose()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Presenting)
	{
		return false;
	}

	ClearAutoCloseTimer();
	PresentationState = EUOURewardPresentationWidgetState::Closing;
	ReceivePresentationCloseRequested();
	return true;
}

bool UUOURewardPresentationWidget::FinishPresentation()
{
	if (PresentationState == EUOURewardPresentationWidgetState::Uninitialized
		|| PresentationState == EUOURewardPresentationWidgetState::Finished)
	{
		return false;
	}

	ClearAutoCloseTimer();
	PresentationState = EUOURewardPresentationWidgetState::Finished;
	OnPresentationFinished.Broadcast(this);
	return true;
}

bool UUOURewardPresentationWidget::ResetPresentation()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Finished)
	{
		return false;
	}

	ClearAutoCloseTimer();
	PresentationData = FUOURewardPresentationData();
	PresentationState = EUOURewardPresentationWidgetState::Uninitialized;
	bPresentationHoldStarted = false;
	ReceivePresentationReset();
	return true;
}

EUOURewardPresentationWidgetState
UUOURewardPresentationWidget::GetPresentationState() const
{
	return PresentationState;
}

bool UUOURewardPresentationWidget::IsPresentationActive() const
{
	return PresentationState == EUOURewardPresentationWidgetState::Presenting
		|| PresentationState == EUOURewardPresentationWidgetState::Closing;
}

void UUOURewardPresentationWidget::ClearAutoCloseTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoCloseTimerHandle);
	}
}

void UUOURewardPresentationWidget::HandleAutoCloseTimerElapsed()
{
	RequestClose();
}
