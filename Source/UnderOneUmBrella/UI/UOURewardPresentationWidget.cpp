// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOURewardPresentationWidget.h"

bool UUOURewardPresentationWidget::InitializePresentation(
	const FUOURewardPresentationData& InPresentationData)
{
	if (PresentationState != EUOURewardPresentationWidgetState::Uninitialized)
	{
		return false;
	}

	PresentationData = InPresentationData;
	PresentationState = EUOURewardPresentationWidgetState::Ready;
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

bool UUOURewardPresentationWidget::RequestClose()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Presenting)
	{
		return false;
	}

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

	PresentationData = FUOURewardPresentationData();
	PresentationState = EUOURewardPresentationWidgetState::Uninitialized;
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
