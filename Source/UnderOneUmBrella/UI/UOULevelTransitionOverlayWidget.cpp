// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOULevelTransitionOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"

namespace
{
	constexpr float DefaultHorizontalPadding = 64.0f;
	constexpr float DefaultImageBottomPadding = 50.0f;
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

	if (MessageImage != nullptr)
	{
		if (MessageSettings.MessageImage != nullptr)
		{
			const FVector2D SafeImageDesiredSize(
				FMath::Max(1.0f, MessageSettings.ImageDesiredSize.X),
				FMath::Max(1.0f, MessageSettings.ImageDesiredSize.Y));
			MessageImage->SetBrushFromTexture(MessageSettings.MessageImage, true);
			MessageImage->SetDesiredSizeOverride(SafeImageDesiredSize);
			MessageImage->SetRenderOpacity(0.0f);
			MessageImage->SetVisibility(ESlateVisibility::HitTestInvisible);

			const bool bTextAlreadyStackedBelowImage = Cast<UVerticalBoxSlot>(MessageText->Slot) != nullptr;
			MessageText->SetRenderTranslation(bTextAlreadyStackedBelowImage
				? FVector2D::ZeroVector
				: FVector2D(0.0f, SafeImageDesiredSize.Y * 0.5f + DefaultImageBottomPadding));
		}
		else
		{
			MessageImage->SetVisibility(HiddenMessageVisibility);
			MessageText->SetRenderTranslation(FVector2D::ZeroVector);
		}
	}
	else
	{
		MessageText->SetRenderTranslation(FVector2D::ZeroVector);
	}

	const bool bHasMessageText = !MessageSettings.MessageText.IsEmpty();
	MessageText->SetText(MessageSettings.MessageText);
	MessageText->SetRenderOpacity(0.0f);
	MessageText->SetVisibility(bHasMessageText ? ESlateVisibility::HitTestInvisible : HiddenMessageVisibility);
	MessageText->SetColorAndOpacity(FSlateColor(MessageSettings.TextColor));
	MessageText->SetAutoWrapText(false);
	MessageText->SetWrapTextAt(0.0f);

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

	if (MessageImage != nullptr)
	{
		MessageImage->SetRenderOpacity(FMath::Clamp(NewOpacity, 0.0f, 1.0f));
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

	UVerticalBox* MessageStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MessageStack"));
	if (UOverlaySlot* StackSlot = RootOverlay->AddChildToOverlay(MessageStack))
	{
		StackSlot->SetHorizontalAlignment(HAlign_Fill);
		StackSlot->SetVerticalAlignment(VAlign_Center);
		StackSlot->SetPadding(FMargin(DefaultHorizontalPadding, 0.0f));
	}

	MessageImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MessageImage"));
	MessageImage->SetVisibility(HiddenMessageVisibility);
	if (UVerticalBoxSlot* ImageSlot = MessageStack->AddChildToVerticalBox(MessageImage))
	{
		ImageSlot->SetHorizontalAlignment(HAlign_Center);
		ImageSlot->SetVerticalAlignment(VAlign_Center);
		ImageSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, DefaultImageBottomPadding));
	}

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetAutoWrapText(false);
	MessageText->SetWrapTextAt(0.0f);
	MessageText->SetVisibility(HiddenMessageVisibility);

	if (UVerticalBoxSlot* MessageSlot = MessageStack->AddChildToVerticalBox(MessageText))
	{
		MessageSlot->SetHorizontalAlignment(HAlign_Center);
		MessageSlot->SetVerticalAlignment(VAlign_Center);
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

	if (MessageImage == nullptr)
	{
		MessageImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("MessageImage")));
	}

	if (FadeBackground != nullptr && MessageText != nullptr && MessageImage != nullptr)
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

		if (MessageImage == nullptr)
		{
			MessageImage = Cast<UImage>(Widget);
		}

		if (FadeBackground != nullptr && MessageText != nullptr && MessageImage != nullptr)
		{
			break;
		}
	}
}
