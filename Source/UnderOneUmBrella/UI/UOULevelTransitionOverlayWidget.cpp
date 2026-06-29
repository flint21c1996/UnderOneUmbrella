// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOULevelTransitionOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

namespace
{
	constexpr float DefaultHorizontalPadding = 64.0f;
	constexpr float DefaultWrapTextAt = 900.0f;
}

void UUOULevelTransitionOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureFallbackWidgetTree();
	SetVisibility(VisibleOverlayVisibility);
	SetTransitionOpacity(0.0f);
}

void UUOULevelTransitionOverlayWidget::ApplyTransitionMessage(const FUOUTransitionMessageSettings& MessageSettings)
{
	EnsureFallbackWidgetTree();

	if (MessageText == nullptr)
	{
		return;
	}

	MessageText->SetText(MessageSettings.MessageText);
	MessageText->SetRenderOpacity(0.0f);
	MessageText->SetVisibility(MessageSettings.ShouldDisplay() ? ESlateVisibility::HitTestInvisible : HiddenMessageVisibility);
	MessageText->SetColorAndOpacity(FSlateColor(MessageSettings.TextColor));
	MessageText->SetAutoWrapText(true);
	MessageText->SetWrapTextAt(FMath::Max(100.0f, MessageSettings.WrapTextAt));

	FSlateFontInfo FontInfo = MessageText->GetFont();
	FontInfo.Size = FMath::Max(1, MessageSettings.FontSize);
	MessageText->SetFont(FontInfo);
}

void UUOULevelTransitionOverlayWidget::SetTransitionBackgroundColor(FLinearColor NewColor)
{
	EnsureFallbackWidgetTree();

	TransitionBackgroundColor = NewColor;
	ApplyTransitionBackgroundBrushColor();
}

void UUOULevelTransitionOverlayWidget::SetTransitionOpacity(float NewOpacity)
{
	const float ClampedOpacity = FMath::Clamp(NewOpacity, 0.0f, 1.0f);
	SetTransitionBackgroundOpacity(ClampedOpacity);
	SetTransitionMessageOpacity(ClampedOpacity);
	SetRenderOpacity(1.0f);
}

void UUOULevelTransitionOverlayWidget::SetTransitionBackgroundOpacity(float NewOpacity)
{
	EnsureFallbackWidgetTree();

	TransitionBackgroundOpacity = FMath::Clamp(NewOpacity, 0.0f, 1.0f);
	if (FadeBackground != nullptr)
	{
		ApplyTransitionBackgroundBrushColor();
	}
}

void UUOULevelTransitionOverlayWidget::SetTransitionMessageOpacity(float NewOpacity)
{
	EnsureFallbackWidgetTree();

	if (MessageText != nullptr)
	{
		MessageText->SetRenderOpacity(FMath::Clamp(NewOpacity, 0.0f, 1.0f));
	}
}

void UUOULevelTransitionOverlayWidget::ApplyTransitionBackgroundBrushColor()
{
	if (FadeBackground == nullptr)
	{
		return;
	}

	FLinearColor BrushColor = TransitionBackgroundColor;
	BrushColor.A *= TransitionBackgroundOpacity;
	FadeBackground->SetBrushColor(BrushColor);
}

void UUOULevelTransitionOverlayWidget::EnsureFallbackWidgetTree()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	if (WidgetTree->RootWidget != nullptr)
	{
		CacheTransitionWidgetsFromTree();
		return;
	}

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("TransitionRoot"));
	WidgetTree->RootWidget = RootOverlay;

	FadeBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FadeBackground"));
	FadeBackground->SetBrushColor(FLinearColor::Black);
	if (UOverlaySlot* BackgroundSlot = RootOverlay->AddChildToOverlay(FadeBackground))
	{
		BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
		BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
	}

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetAutoWrapText(true);
	MessageText->SetWrapTextAt(DefaultWrapTextAt);
	MessageText->SetVisibility(HiddenMessageVisibility);

	if (UOverlaySlot* MessageSlot = RootOverlay->AddChildToOverlay(MessageText))
	{
		MessageSlot->SetHorizontalAlignment(HAlign_Fill);
		MessageSlot->SetVerticalAlignment(VAlign_Center);
		MessageSlot->SetPadding(FMargin(DefaultHorizontalPadding, 0.0f));
	}
}

void UUOULevelTransitionOverlayWidget::CacheTransitionWidgetsFromTree()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	if (FadeBackground == nullptr)
	{
		FadeBackground = Cast<UBorder>(WidgetTree->FindWidget(TEXT("FadeBackground")));
	}

	if (MessageText == nullptr)
	{
		MessageText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("MessageText")));
	}

	if (FadeBackground != nullptr && MessageText != nullptr)
	{
		return;
	}

	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		if (FadeBackground == nullptr)
		{
			FadeBackground = Cast<UBorder>(Widget);
		}

		if (MessageText == nullptr)
		{
			MessageText = Cast<UTextBlock>(Widget);
		}

		if (FadeBackground != nullptr && MessageText != nullptr)
		{
			break;
		}
	}
}
