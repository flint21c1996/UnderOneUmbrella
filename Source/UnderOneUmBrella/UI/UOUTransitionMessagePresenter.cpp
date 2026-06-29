// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUTransitionMessagePresenter.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	constexpr float HorizontalMessagePadding = 64.0f;
	constexpr float MessageShadowAlpha = 0.55f;
	constexpr float MinWrapTextAt = 100.0f;
}

void FUOUTransitionMessagePresenter::Show(UWorld* World, const FUOUTransitionMessageSettings& Settings, bool bShowBlackBackground)
{
	Hide();

	if (World == nullptr || (!Settings.ShouldDisplay() && !bShowBlackBackground))
	{
		return;
	}

	UGameViewportClient* TargetViewportClient = World->GetGameViewport();
	if (TargetViewportClient == nullptr && GEngine != nullptr)
	{
		TargetViewportClient = GEngine->GameViewport;
	}

	if (TargetViewportClient == nullptr)
	{
		return;
	}

	const int32 SafeFontSize = FMath::Max(1, Settings.FontSize);
	const float SafeWrapTextAt = FMath::Max(MinWrapTextAt, Settings.WrapTextAt);

	TSharedRef<SOverlay> Overlay = SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible);

	if (bShowBlackBackground)
	{
		Overlay->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FSlateColor(FLinearColor::Black))
		];
	}

	if (Settings.ShouldDisplay())
	{
		Overlay->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.Padding(FMargin(HorizontalMessagePadding, 0.0f))
		[
			SNew(STextBlock)
			.Text(Settings.MessageText)
			.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), SafeFontSize))
			.ColorAndOpacity(FSlateColor(Settings.TextColor))
			.ShadowOffset(FVector2D(1.5f, 1.5f))
			.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, MessageShadowAlpha))
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
			.WrapTextAt(SafeWrapTextAt)
		];
	}

	MessageWidget = Overlay;

	TargetViewportClient->AddViewportWidgetContent(MessageWidget.ToSharedRef(), Settings.ViewportZOrder);
	ViewportClient = TargetViewportClient;
}

void FUOUTransitionMessagePresenter::Hide()
{
	UGameViewportClient* TargetViewportClient = ViewportClient.Get();
	if (TargetViewportClient != nullptr && MessageWidget.IsValid())
	{
		TargetViewportClient->RemoveViewportWidgetContent(MessageWidget.ToSharedRef());
	}

	MessageWidget.Reset();
	ViewportClient.Reset();
}

void FUOUTransitionMessagePresenter::SetOpacity(float NewOpacity)
{
	if (MessageWidget.IsValid())
	{
		MessageWidget->SetRenderOpacity(FMath::Clamp(NewOpacity, 0.0f, 1.0f));
	}
}
