// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueBoxWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/LocalPlayer.h"
#include "UI/UOUUISubsystem.h"

void UUOUDialogueBoxWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// BP 디자이너 미리보기에서도 실제 실행과 같은 크기와 9-Slice 설정을 확인할 수 있게 합니다.
	RefreshAdaptiveLayout();
}

void UUOUDialogueBoxWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshAdaptiveLayout();

	if (AdvanceButton != nullptr)
	{
		AdvanceButton->OnClicked.AddUniqueDynamic(this, &UUOUDialogueBoxWidget::HandleAdvanceButtonClicked);
	}

	SetDialogueBoxVisible(false);
}

void UUOUDialogueBoxWidget::RefreshAdaptiveLayout()
{
	if (DialoguePanel != nullptr)
	{
		DialoguePanel->SetPadding(ContentPadding);

		if (bApplyNineSliceBrush)
		{
			FSlateBrush AppliedBrush = NineSliceBrush;
			AppliedBrush.DrawAs = ESlateBrushDrawType::Box;
			DialoguePanel->SetBrush(AppliedBrush);
		}
	}
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
