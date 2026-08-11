// Copyright Epic Games, Inc. All Rights Reserved.

#include "Widgets/SUOUDevelopmentPuzzleCheatHUD.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SUOUDevelopmentPuzzleCheatHUD::Construct(const FArguments& InArgs)
{
	PuzzleCheatSubsystem = InArgs._PuzzleCheatSubsystem;

	ChildSlot
	[
		SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(16.0f)
		[
			SNew(SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleToggleClicked)
				.ContentPadding(FMargin(10.0f, 5.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("PUZZLE CHEAT")))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.Visibility(this, &SUOUDevelopmentPuzzleCheatHUD::GetPanelVisibility)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")))
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.92f))
				.Padding(12.0f)
				[
					SNew(SBox)
					.WidthOverride(360.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Puzzle Cheat Tool")))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Puzzle controls will be added in the next HUD stage.")))
						]
					]
				]
			]
		]
	];
}

void SUOUDevelopmentPuzzleCheatHUD::TogglePanel()
{
	bPanelExpanded = !bPanelExpanded;
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleToggleClicked()
{
	TogglePanel();
	return FReply::Handled();
}

EVisibility SUOUDevelopmentPuzzleCheatHUD::GetPanelVisibility() const
{
	return bPanelExpanded ? EVisibility::Visible : EVisibility::Collapsed;
}
