// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueBoxWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/LocalPlayer.h"
#include "UI/UOUUISubsystem.h"

void UUOUDialogueBoxWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AdvanceButton != nullptr)
	{
		AdvanceButton->OnClicked.AddUniqueDynamic(this, &UUOUDialogueBoxWidget::HandleAdvanceButtonClicked);
	}

	SetDialogueBoxVisible(false);
}

void UUOUDialogueBoxWidget::NativeDestruct()
{
	if (AdvanceButton != nullptr)
	{
		AdvanceButton->OnClicked.RemoveDynamic(this, &UUOUDialogueBoxWidget::HandleAdvanceButtonClicked);
	}

	Super::NativeDestruct();
}

void UUOUDialogueBoxWidget::ShowDialogueLine(AActor* SpeakerActor, const FUOUDialogueLine& Line)
{
	CurrentSpeakerActor = SpeakerActor;
	CurrentLine = Line;

	if (SpeakerNameText != nullptr)
	{
		SpeakerNameText->SetText(Line.SpeakerName);
	}

	if (DialogueText != nullptr)
	{
		DialogueText->SetText(Line.DialogueText);
	}

	SetDialogueBoxVisible(true);
	BP_OnDialogueLineShown(SpeakerActor, Line);
}

void UUOUDialogueBoxWidget::HideDialogueBox()
{
	CurrentSpeakerActor = nullptr;
	CurrentLine = FUOUDialogueLine();

	if (SpeakerNameText != nullptr)
	{
		SpeakerNameText->SetText(FText::GetEmpty());
	}

	if (DialogueText != nullptr)
	{
		DialogueText->SetText(FText::GetEmpty());
	}

	SetDialogueBoxVisible(false);
	BP_OnDialogueBoxHidden();
}

void UUOUDialogueBoxWidget::RequestAdvanceDialogue()
{
	OnAdvanceRequested.Broadcast();

	if (!bAdvanceDialogueOnButtonClick)
	{
		return;
	}

	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->AdvanceDialogue();
	}
}

void UUOUDialogueBoxWidget::HandleAdvanceButtonClicked()
{
	RequestAdvanceDialogue();
}

void UUOUDialogueBoxWidget::SetDialogueBoxVisible(bool bNewVisible)
{
	bDialogueBoxVisible = bNewVisible;

	UWidget* TargetWidget = DialogueRoot != nullptr ? DialogueRoot.Get() : static_cast<UWidget*>(this);
	if (TargetWidget != nullptr)
	{
		TargetWidget->SetVisibility(bNewVisible ? ShownVisibility : HiddenVisibility);
	}
}

UUOUUISubsystem* UUOUDialogueBoxWidget::GetUISubsystem() const
{
	ULocalPlayer* OwningLocalPlayer = GetOwningLocalPlayer();
	return OwningLocalPlayer != nullptr ? OwningLocalPlayer->GetSubsystem<UUOUUISubsystem>() : nullptr;
}
