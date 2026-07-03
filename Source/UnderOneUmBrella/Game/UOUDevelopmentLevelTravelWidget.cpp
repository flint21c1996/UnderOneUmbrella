// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOUDevelopmentLevelTravelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SlateWrapperTypes.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	constexpr TCHAR DefaultTitleLevelPath[] = TEXT("/Game/UOU/Maps/TitleMap.TitleMap");
}

void UUOUDevelopmentLevelTravelCommand::Initialize(
	UUOUDevelopmentLevelTravelWidget* InOwner,
	TSoftObjectPtr<UWorld> InTargetLevel)
{
	Owner = InOwner;
	TargetLevel = InTargetLevel;
}

void UUOUDevelopmentLevelTravelCommand::HandleClicked()
{
	if (Owner != nullptr)
	{
		Owner->TravelToLevel(TargetLevel);
	}
}

void UUOUDevelopmentLevelTravelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildWidgetTree();
}

void UUOUDevelopmentLevelTravelWidget::SetQuickLevels(const TArray<TSoftObjectPtr<UWorld>>& InQuickLevels)
{
	QuickLevels = InQuickLevels;
}

void UUOUDevelopmentLevelTravelWidget::SetTitleLevel(TSoftObjectPtr<UWorld> InTitleLevel)
{
	TitleLevel = InTitleLevel;
}

void UUOUDevelopmentLevelTravelWidget::TravelToLevel(TSoftObjectPtr<UWorld> TargetLevel)
{
	if (TargetLevel.IsNull())
	{
		return;
	}

	UUOULevelTransitionSubsystem* TransitionSubsystem = GetTransitionSubsystem();
	if (TransitionSubsystem == nullptr)
	{
		return;
	}

	TransitionSubsystem->RequestLevelTransitionFromWorld(
		GetWorld(),
		TargetLevel,
		MakeDevelopmentTransitionSettings());
}

void UUOUDevelopmentLevelTravelWidget::BuildWidgetTree()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	QuickLevelCommands.Reset();
	LevelNameTextBox = nullptr;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("DevelopmentLevelTravelRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("DevelopmentLevelTravelPanel"));
	PanelBorder->SetPadding(FMargin(10.0f));
	PanelBorder->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.022f, 0.82f));

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBorder);
	PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	PanelSlot->SetAlignment(FVector2D(0.0f, 0.0f));
	PanelSlot->SetPosition(FVector2D(16.0f, 72.0f));
	PanelSlot->SetSize(FVector2D(320.0f, 420.0f));

	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("DevelopmentLevelTravelContent"));
	PanelBorder->SetContent(MainBox);

	UTextBlock* TitleText = CreateTextBlock(FText::FromString(TEXT("Dev Level Travel")), 18);
	if (UVerticalBoxSlot* TitleSlot = MainBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UHorizontalBox* PrimaryRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("DevelopmentLevelPrimaryRow"));
	if (UVerticalBoxSlot* PrimaryRowSlot = MainBox->AddChildToVerticalBox(PrimaryRow))
	{
		PrimaryRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	UButton* RestartButton = CreateButton(FText::FromString(TEXT("Restart")));
	RestartButton->OnClicked.AddDynamic(this, &UUOUDevelopmentLevelTravelWidget::HandleRestartClicked);
	if (UHorizontalBoxSlot* RestartSlot = PrimaryRow->AddChildToHorizontalBox(RestartButton))
	{
		RestartSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		RestartSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UButton* NextButton = CreateButton(FText::FromString(TEXT("Next")));
	NextButton->OnClicked.AddDynamic(this, &UUOUDevelopmentLevelTravelWidget::HandleNextClicked);
	if (UHorizontalBoxSlot* NextSlot = PrimaryRow->AddChildToHorizontalBox(NextButton))
	{
		NextSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		NextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UButton* TitleButton = CreateButton(FText::FromString(TEXT("Title")));
	TitleButton->OnClicked.AddDynamic(this, &UUOUDevelopmentLevelTravelWidget::HandleTitleClicked);
	if (UHorizontalBoxSlot* TitleButtonSlot = PrimaryRow->AddChildToHorizontalBox(TitleButton))
	{
		TitleButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UHorizontalBox* OpenNameRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("DevelopmentLevelOpenNameRow"));
	if (UVerticalBoxSlot* OpenNameRowSlot = MainBox->AddChildToVerticalBox(OpenNameRow))
	{
		OpenNameRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	LevelNameTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(
		UEditableTextBox::StaticClass(),
		TEXT("DevelopmentLevelNameTextBox"));
	LevelNameTextBox->SetHintText(FText::FromString(TEXT("Level name or path")));
	if (UHorizontalBoxSlot* TextBoxSlot = OpenNameRow->AddChildToHorizontalBox(LevelNameTextBox))
	{
		TextBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		TextBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UButton* OpenNameButton = CreateButton(FText::FromString(TEXT("Open")));
	OpenNameButton->OnClicked.AddDynamic(this, &UUOUDevelopmentLevelTravelWidget::HandleOpenByNameClicked);
	if (UHorizontalBoxSlot* OpenButtonSlot = OpenNameRow->AddChildToHorizontalBox(OpenNameButton))
	{
		OpenButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UTextBlock* QuickTitleText = CreateTextBlock(FText::FromString(TEXT("Quick Levels")), 14);
	if (UVerticalBoxSlot* QuickTitleSlot = MainBox->AddChildToVerticalBox(QuickTitleText))
	{
		QuickTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	USizeBox* QuickListSizeBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("DevelopmentQuickLevelListSize"));
	QuickListSizeBox->SetHeightOverride(250.0f);
	if (UVerticalBoxSlot* QuickListSizeSlot = MainBox->AddChildToVerticalBox(QuickListSizeBox))
	{
		QuickListSizeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UScrollBox* QuickLevelList = WidgetTree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(),
		TEXT("DevelopmentQuickLevelList"));
	QuickListSizeBox->SetContent(QuickLevelList);
	RebuildQuickLevelButtons(QuickLevelList);
}

void UUOUDevelopmentLevelTravelWidget::RebuildQuickLevelButtons(UScrollBox* QuickLevelList)
{
	if (QuickLevelList == nullptr)
	{
		return;
	}

	for (const TSoftObjectPtr<UWorld>& QuickLevel : QuickLevels)
	{
		if (QuickLevel.IsNull())
		{
			continue;
		}

		const FSoftObjectPath LevelPath = QuickLevel.ToSoftObjectPath();
		FString Label = LevelPath.GetAssetName();
		if (Label.IsEmpty())
		{
			Label = LevelPath.ToString();
		}

		UButton* QuickLevelButton = CreateButton(FText::FromString(Label));
		UUOUDevelopmentLevelTravelCommand* Command = NewObject<UUOUDevelopmentLevelTravelCommand>(this);
		Command->Initialize(this, QuickLevel);
		QuickLevelCommands.Add(Command);
		QuickLevelButton->OnClicked.AddDynamic(Command, &UUOUDevelopmentLevelTravelCommand::HandleClicked);
		QuickLevelList->AddChild(QuickLevelButton);
	}
}

UTextBlock* UUOUDevelopmentLevelTravelWidget::CreateTextBlock(const FText& Text, int32 FontSize) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(Text);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	TextBlock->SetFont(FontInfo);
	return TextBlock;
}

UButton* UUOUDevelopmentLevelTravelWidget::CreateButton(const FText& Label) const
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

	UTextBlock* LabelText = CreateTextBlock(Label, 12);
	LabelText->SetJustification(ETextJustify::Center);
	Button->AddChild(LabelText);
	return Button;
}

UUOULevelTransitionSubsystem* UUOUDevelopmentLevelTravelWidget::GetTransitionSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance != nullptr ? GameInstance->GetSubsystem<UUOULevelTransitionSubsystem>() : nullptr;
}

FUOULevelTransitionSettings UUOUDevelopmentLevelTravelWidget::MakeDevelopmentTransitionSettings() const
{
	FUOULevelTransitionSettings Settings;
	Settings.FadeOutDuration = 0.15f;
	Settings.BlackHoldDuration = 0.02f;
	Settings.FadeInDuration = 0.15f;
	Settings.bUseCurrentMapExitSettings = false;
	Settings.bUseLoadedMapEnterSettings = false;
	return Settings;
}

void UUOUDevelopmentLevelTravelWidget::HandleRestartClicked()
{
	if (UUOULevelTransitionSubsystem* TransitionSubsystem = GetTransitionSubsystem())
	{
		TransitionSubsystem->RestartCurrentLevelFromWorld(GetWorld(), MakeDevelopmentTransitionSettings());
	}
}

void UUOUDevelopmentLevelTravelWidget::HandleNextClicked()
{
	if (UUOULevelTransitionSubsystem* TransitionSubsystem = GetTransitionSubsystem())
	{
		TransitionSubsystem->RequestNextLevelFromWorld(GetWorld(), MakeDevelopmentTransitionSettings());
	}
}

void UUOUDevelopmentLevelTravelWidget::HandleTitleClicked()
{
	TSoftObjectPtr<UWorld> TargetTitleLevel = TitleLevel;
	if (TargetTitleLevel.IsNull())
	{
		TargetTitleLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultTitleLevelPath));
	}

	TravelToLevel(TargetTitleLevel);
}

void UUOUDevelopmentLevelTravelWidget::HandleOpenByNameClicked()
{
	if (LevelNameTextBox == nullptr)
	{
		return;
	}

	FString LevelName = LevelNameTextBox->GetText().ToString().TrimStartAndEnd();
	if (LevelName.IsEmpty())
	{
		return;
	}

	if (UUOULevelTransitionSubsystem* TransitionSubsystem = GetTransitionSubsystem())
	{
		TransitionSubsystem->RequestLevelTransitionByNameFromWorld(
			GetWorld(),
			FName(*LevelName),
			MakeDevelopmentTransitionSettings());
	}
}
