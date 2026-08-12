// Copyright Epic Games, Inc. All Rights Reserved.

#include "Widgets/SUOUDevelopmentPuzzleCheatHUD.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"
#include "UOUDevelopmentDebugControlSubsystem.h"
#include "UOUDevelopmentDebugDrawSubsystem.h"
#include "UOUDevelopmentPuzzleCheatSubsystem.h"

void SUOUDevelopmentPuzzleCheatHUD::Construct(const FArguments& InArgs)
{
	DebugControlSubsystem = InArgs._DebugControlSubsystem;
	DebugDrawSubsystem = InArgs._DebugDrawSubsystem;
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
					.Text(FText::FromString(TEXT("개발 도구")))
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
							.Text(FText::FromString(TEXT("개발 도구")))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(0.0f, 0.0f, 3.0f, 0.0f)
							[
								SNew(SButton)
								.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleConditionTabClicked)
								.IsEnabled_Lambda([this]()
								{
									return ActivePage != EActivePage::Condition;
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Condition")))
								]
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(3.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SButton)
								.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleDebugTabClicked)
								.IsEnabled_Lambda([this]()
								{
									return ActivePage != EActivePage::Debug;
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Debug")))
								]
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(SVerticalBox)
							.Visibility(this, &SUOUDevelopmentPuzzleCheatHUD::GetConditionPageVisibility)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetStatusText)
								.ColorAndOpacity(this, &SUOUDevelopmentPuzzleCheatHUD::GetStatusColor)
								.AutoWrapText(true)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 10.0f, 0.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(0.0f, 0.0f, 3.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleRefreshClicked)
									.IsEnabled_Lambda([this]()
									{
										return PuzzleCheatSubsystem.IsValid()
											&& !PuzzleCheatSubsystem->IsSequenceRunning();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("새로고침")))
									]
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(3.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleNextClicked)
									.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsPuzzleActionEnabled)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("다음 퍼즐")))
									]
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(3.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleCancelClicked)
									.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsCancelEnabled)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("취소")))
									]
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 10.0f, 0.0f, 0.0f)
							[
								SNew(SBox)
								.MaxDesiredHeight(320.0f)
								[
									SNew(SScrollBox)
									+ SScrollBox::Slot()
									[
										SAssignNew(StepListBox, SVerticalBox)
									]
								]
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(SVerticalBox)
							.Visibility(this, &SUOUDevelopmentPuzzleCheatHUD::GetDebugPageVisibility)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SButton)
								.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleDebugToolsToggleClicked)
								.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsDebugControlAvailable)
								[
									SNew(STextBlock)
									.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetDebugToolsToggleText)
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(0.0f, 0.0f, 3.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleDebugCategoryToggleClicked, EUOUDebugCategory::Player)
									.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsDebugControlAvailable)
									[
										SNew(STextBlock)
										.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetDebugCategoryToggleText, EUOUDebugCategory::Player)
									]
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(3.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleDebugCategoryToggleClicked, EUOUDebugCategory::Puzzle)
									.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsDebugControlAvailable)
									[
										SNew(STextBlock)
										.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetDebugCategoryToggleText, EUOUDebugCategory::Puzzle)
									]
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(0.0f, 0.0f, 3.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleDebugCategoryToggleClicked, EUOUDebugCategory::NPC)
									.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsDebugControlAvailable)
									[
										SNew(STextBlock)
										.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetDebugCategoryToggleText, EUOUDebugCategory::NPC)
									]
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(3.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleDebugCategoryToggleClicked, EUOUDebugCategory::Interaction)
									.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsDebugControlAvailable)
									[
										SNew(STextBlock)
										.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetDebugCategoryToggleText, EUOUDebugCategory::Interaction)
									]
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(0.0f, 0.0f, 3.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleDebugCategoryToggleClicked, EUOUDebugCategory::VFX)
									.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsDebugControlAvailable)
									[
										SNew(STextBlock)
										.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetDebugCategoryToggleText, EUOUDebugCategory::VFX)
									]
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(3.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleDebugCategoryToggleClicked, EUOUDebugCategory::Performance)
									.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsDebugControlAvailable)
									[
										SNew(STextBlock)
										.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetDebugCategoryToggleText, EUOUDebugCategory::Performance)
									]
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 10.0f, 0.0f, 0.0f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")))
								.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.9f))
								.Padding(8.0f)
								[
									SNew(STextBlock)
									.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetPlayerDebugInfoText)
									.AutoWrapText(true)
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")))
								.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.9f))
								.Padding(8.0f)
								[
									SNew(STextBlock)
									.Text(this, &SUOUDevelopmentPuzzleCheatHUD::GetPerformanceDebugInfoText)
									.AutoWrapText(true)
								]
							]
						]
					]
				]
			]
		]
	];

	RebuildStepRows();
}

void SUOUDevelopmentPuzzleCheatHUD::TogglePanel()
{
	bPanelExpanded = !bPanelExpanded;
	if (bPanelExpanded)
	{
		RebuildStepRows();
	}
}

void SUOUDevelopmentPuzzleCheatHUD::RebuildStepRows()
{
	if (!StepListBox.IsValid())
	{
		return;
	}

	StepListBox->ClearChildren();
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	if (Subsystem == nullptr)
	{
		StepListBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("퍼즐 치트 Subsystem을 사용할 수 없습니다.")))
		];
		return;
	}

	const TArray<FUOUDevelopmentPuzzleCheatStep> Steps = Subsystem->GetPuzzleSteps();
	if (Steps.IsEmpty())
	{
		StepListBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("태그가 설정된 퍼즐 Step을 찾지 못했습니다.")))
		];
		return;
	}

	for (const FUOUDevelopmentPuzzleCheatStep& Step : Steps)
	{
		const int32 StepOrder = Step.StepOrder;
		const FText DisplayName = Step.DisplayName;
		const float DelaySeconds = Step.DelayAfterActivationSeconds;
		const TWeakObjectPtr<AUOUPuzzleConditionGroupActor> PuzzleGroup = Step.PuzzleGroup.Get();

		StepListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SButton)
			.OnClicked(this, &SUOUDevelopmentPuzzleCheatHUD::HandleStepClicked, StepOrder)
			.IsEnabled(this, &SUOUDevelopmentPuzzleCheatHUD::IsPuzzleActionEnabled)
			.ContentPadding(FMargin(8.0f, 5.0f))
			[
				SNew(STextBlock)
				.ColorAndOpacity_Lambda([PuzzleGroup]()
				{
					const bool bSatisfied = PuzzleGroup.IsValid() && PuzzleGroup->IsSatisfied();
					return FSlateColor(bSatisfied
						? FLinearColor(0.25f, 1.0f, 0.35f, 1.0f)
						: FLinearColor::White);
				})
				.Text_Lambda([StepOrder, DisplayName, DelaySeconds, PuzzleGroup]()
				{
					const bool bSatisfied = PuzzleGroup.IsValid() && PuzzleGroup->IsSatisfied();
					return FText::FromString(FString::Printf(
						TEXT("[%03d] %s  |  %s  |  %.2fs"),
						StepOrder,
						*DisplayName.ToString(),
						bSatisfied ? TEXT("완료") : TEXT("대기"),
						DelaySeconds));
				})
			]
		];
	}
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleToggleClicked()
{
	if (UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get())
	{
		Subsystem->ToggleCheatHUD();
	}
	else
	{
		TogglePanel();
	}
	return FReply::Handled();
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleConditionTabClicked()
{
	ActivePage = EActivePage::Condition;
	return FReply::Handled();
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleDebugTabClicked()
{
	ActivePage = EActivePage::Debug;
	return FReply::Handled();
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleDebugToolsToggleClicked()
{
	if (UUOUDevelopmentDebugControlSubsystem* Subsystem = DebugControlSubsystem.Get())
	{
		Subsystem->SetDebugToolsEnabled(!Subsystem->IsDebugToolsEnabled());
	}
	return FReply::Handled();
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleDebugCategoryToggleClicked(EUOUDebugCategory Category)
{
	if (UUOUDevelopmentDebugControlSubsystem* Subsystem = DebugControlSubsystem.Get())
	{
		const bool bNewEnabled = !Subsystem->IsDebugCategoryEnabled(Category);
		Subsystem->SetDebugCategoryEnabled(Category, bNewEnabled);
	}
	return FReply::Handled();
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleRefreshClicked()
{
	if (UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get())
	{
		Subsystem->RefreshPuzzleSequence();
		RebuildStepRows();
	}
	return FReply::Handled();
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleNextClicked()
{
	if (UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get())
	{
		Subsystem->AdvanceToNextPuzzle();
	}
	return FReply::Handled();
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleCancelClicked()
{
	if (UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get())
	{
		Subsystem->CancelPendingSequence();
	}
	return FReply::Handled();
}

FReply SUOUDevelopmentPuzzleCheatHUD::HandleStepClicked(int32 TargetStepOrder)
{
	if (UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get())
	{
		Subsystem->AdvanceThroughStep(TargetStepOrder);
	}
	return FReply::Handled();
}

EVisibility SUOUDevelopmentPuzzleCheatHUD::GetPanelVisibility() const
{
	return bPanelExpanded ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SUOUDevelopmentPuzzleCheatHUD::GetConditionPageVisibility() const
{
	return ActivePage == EActivePage::Condition ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SUOUDevelopmentPuzzleCheatHUD::GetDebugPageVisibility() const
{
	return ActivePage == EActivePage::Debug ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SUOUDevelopmentPuzzleCheatHUD::GetDebugToolsToggleText() const
{
	const UUOUDevelopmentDebugControlSubsystem* Subsystem = DebugControlSubsystem.Get();
	if (Subsystem == nullptr)
	{
		return FText::FromString(TEXT("전체 디버그: 사용 불가"));
	}

	return FText::FromString(Subsystem->IsDebugToolsEnabled()
		? TEXT("전체 디버그: ON")
		: TEXT("전체 디버그: OFF"));
}

FText SUOUDevelopmentPuzzleCheatHUD::GetDebugCategoryToggleText(EUOUDebugCategory Category) const
{
	const TCHAR* CategoryName = TEXT("Unknown");
	switch (Category)
	{
	case EUOUDebugCategory::Player:
		CategoryName = TEXT("Player");
		break;
	case EUOUDebugCategory::NPC:
		CategoryName = TEXT("NPC");
		break;
	case EUOUDebugCategory::Puzzle:
		CategoryName = TEXT("Puzzle");
		break;
	case EUOUDebugCategory::Interaction:
		CategoryName = TEXT("Interaction");
		break;
	case EUOUDebugCategory::VFX:
		CategoryName = TEXT("VFX");
		break;
	case EUOUDebugCategory::Performance:
		CategoryName = TEXT("Performance");
		break;
	case EUOUDebugCategory::System:
	default:
		break;
	}

	const UUOUDevelopmentDebugControlSubsystem* Subsystem = DebugControlSubsystem.Get();
	if (Subsystem == nullptr)
	{
		return FText::FromString(FString::Printf(TEXT("%s: 사용 불가"), CategoryName));
	}

	return FText::FromString(FString::Printf(
		TEXT("%s: %s"),
		CategoryName,
		Subsystem->IsDebugCategoryEnabled(Category) ? TEXT("ON") : TEXT("OFF")));
}

FText SUOUDevelopmentPuzzleCheatHUD::GetPlayerDebugInfoText() const
{
	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem = DebugControlSubsystem.Get();
	const UUOUDevelopmentDebugDrawSubsystem* DrawSubsystem = DebugDrawSubsystem.Get();
	if (ControlSubsystem == nullptr || DrawSubsystem == nullptr)
	{
		return FText::FromString(TEXT("Player debug information is unavailable."));
	}

	if (!ControlSubsystem->IsDebugToolsEnabled()
		|| !ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::Player))
	{
		return FText::FromString(TEXT("Player debug information is disabled."));
	}

	const FString& DebugText = DrawSubsystem->GetPlayerDebugText();
	return FText::FromString(DebugText.IsEmpty()
		? TEXT("Waiting for player debug information...")
		: DebugText);
}

FText SUOUDevelopmentPuzzleCheatHUD::GetPerformanceDebugInfoText() const
{
	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem = DebugControlSubsystem.Get();
	const UUOUDevelopmentDebugDrawSubsystem* DrawSubsystem = DebugDrawSubsystem.Get();
	if (ControlSubsystem == nullptr || DrawSubsystem == nullptr)
	{
		return FText::FromString(TEXT("Performance debug information is unavailable."));
	}

	if (!ControlSubsystem->IsDebugToolsEnabled()
		|| !ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::Performance))
	{
		return FText::FromString(TEXT("Performance debug information is disabled."));
	}

	const FString& DebugText = DrawSubsystem->GetPerformanceDebugText();
	return FText::FromString(DebugText.IsEmpty()
		? TEXT("Waiting for performance debug information...")
		: DebugText);
}

FText SUOUDevelopmentPuzzleCheatHUD::GetStatusText() const
{
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	if (Subsystem == nullptr)
	{
		return FText::FromString(TEXT("Subsystem 사용 불가"));
	}

	const FString StatePrefix = Subsystem->IsSequenceRunning() ? TEXT("실행 중") : TEXT("준비");
	return FText::FromString(FString::Printf(
		TEXT("%s | %s"),
		*StatePrefix,
		*Subsystem->LastStatusMessage));
}

FSlateColor SUOUDevelopmentPuzzleCheatHUD::GetStatusColor() const
{
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	if (Subsystem == nullptr || !Subsystem->IsPuzzleSequenceValid())
	{
		return FSlateColor(FLinearColor(1.0f, 0.25f, 0.2f, 1.0f));
	}

	if (Subsystem->IsSequenceRunning())
	{
		return FSlateColor(FLinearColor(1.0f, 0.8f, 0.2f, 1.0f));
	}

	return FSlateColor(FLinearColor(0.25f, 1.0f, 0.35f, 1.0f));
}

bool SUOUDevelopmentPuzzleCheatHUD::IsPuzzleActionEnabled() const
{
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	return Subsystem != nullptr
		&& Subsystem->IsPuzzleSequenceValid()
		&& !Subsystem->IsSequenceRunning();
}

bool SUOUDevelopmentPuzzleCheatHUD::IsCancelEnabled() const
{
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	return Subsystem != nullptr && Subsystem->IsSequenceRunning();
}

bool SUOUDevelopmentPuzzleCheatHUD::IsDebugControlAvailable() const
{
	return DebugControlSubsystem.IsValid();
}
