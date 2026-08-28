// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUSpeechBubbleWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableText.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	bool SetSpeechBubbleWidgetText(UWidget* Widget, const FText& BubbleText)
	{
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			TextBlock->SetText(BubbleText);
			return true;
		}

		if (URichTextBlock* RichTextBlock = Cast<URichTextBlock>(Widget))
		{
			RichTextBlock->SetText(BubbleText);
			return true;
		}

		if (UEditableTextBox* EditableTextBox = Cast<UEditableTextBox>(Widget))
		{
			EditableTextBox->SetText(BubbleText);
			return true;
		}

		if (UEditableText* EditableText = Cast<UEditableText>(Widget))
		{
			EditableText->SetText(BubbleText);
			return true;
		}

		if (UMultiLineEditableTextBox* MultiLineEditableTextBox = Cast<UMultiLineEditableTextBox>(Widget))
		{
			MultiLineEditableTextBox->SetText(BubbleText);
			return true;
		}

		if (UMultiLineEditableText* MultiLineEditableText = Cast<UMultiLineEditableText>(Widget))
		{
			MultiLineEditableText->SetText(BubbleText);
			return true;
		}

		return false;
	}
}

void UUOUSpeechBubbleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	HideBubbleImmediately();
}

void UUOUSpeechBubbleWidget::NativeDestruct()
{
	if (UWorld* World = GetTimerWorld())
	{
		World->GetTimerManager().ClearTimer(AutoHideTimerHandle);
	}

	Super::NativeDestruct();
}

void UUOUSpeechBubbleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (FadeState == EFadeState::FadingIn)
	{
		FadeElapsedTime += InDeltaTime;
		const float Alpha = FadeInDuration > 0.0f ? FMath::Clamp(FadeElapsedTime / FadeInDuration, 0.0f, 1.0f) : 1.0f;
		SetBubbleOpacity(Alpha);

		if (Alpha >= 1.0f)
		{
			FadeState = EFadeState::Visible;
			FadeElapsedTime = 0.0f;
		}
	}
	else if (FadeState == EFadeState::FadingOut)
	{
		FadeElapsedTime += InDeltaTime;
		const float Alpha = FadeOutDuration > 0.0f ? 1.0f - FMath::Clamp(FadeElapsedTime / FadeOutDuration, 0.0f, 1.0f) : 0.0f;
		SetBubbleOpacity(Alpha);

		if (Alpha <= 0.0f)
		{
			HideBubbleImmediately();
		}
	}
}

void UUOUSpeechBubbleWidget::ShowBubble(FText BubbleText, double Duration)
{
	SetBubbleText(BubbleText);

	if (UWorld* World = GetTimerWorld())
	{
		World->GetTimerManager().ClearTimer(AutoHideTimerHandle);

		if (Duration >= 0.0)
		{
			World->GetTimerManager().SetTimer(
				AutoHideTimerHandle,
				this,
				&UUOUSpeechBubbleWidget::StartFadeOut,
				static_cast<float>(Duration) + FadeInDuration,
				false);
		}
	}

	SetBubbleVisible(true);
	FadeElapsedTime = 0.0f;

	if (FadeInDuration <= 0.0f)
	{
		SetBubbleOpacity(1.0f);
		FadeState = EFadeState::Visible;
		return;
	}

	SetBubbleOpacity(0.0f);
	FadeState = EFadeState::FadingIn;
}

void UUOUSpeechBubbleWidget::HideBubble()
{
	if (UWorld* World = GetTimerWorld())
	{
		World->GetTimerManager().ClearTimer(AutoHideTimerHandle);
	}

	if (FadeState == EFadeState::Hidden || FadeState == EFadeState::FadingOut)
	{
		return;
	}

	FadeElapsedTime = 0.0f;

	if (FadeOutDuration <= 0.0f)
	{
		HideBubbleImmediately();
		return;
	}

	SetBubbleVisible(true);
	FadeState = EFadeState::FadingOut;
}

void UUOUSpeechBubbleWidget::HideBubbleImmediately()
{
	if (UWorld* World = GetTimerWorld())
	{
		World->GetTimerManager().ClearTimer(AutoHideTimerHandle);
	}

	SetBubbleOpacity(0.0f);
	SetBubbleVisible(false);
	FadeElapsedTime = 0.0f;
	FadeState = EFadeState::Hidden;
}

void UUOUSpeechBubbleWidget::StartFadeOut()
{
	HideBubble();
}

void UUOUSpeechBubbleWidget::SetBubbleVisible(bool bNewVisible)
{
	if (UWidget* TargetWidget = ResolveBubbleRootWidget())
	{
		TargetWidget->SetVisibility(bNewVisible ? ShownVisibility : HiddenVisibility);
	}
}

void UUOUSpeechBubbleWidget::SetBubbleOpacity(float NewOpacity)
{
	if (UWidget* TargetWidget = ResolveBubbleRootWidget())
	{
		TargetWidget->SetRenderOpacity(NewOpacity);
	}
}

bool UUOUSpeechBubbleWidget::SetBubbleText(const FText& BubbleText) const
{
	return SetSpeechBubbleWidgetText(ResolveBubbleTextWidget(), BubbleText);
}

UWidget* UUOUSpeechBubbleWidget::ResolveBubbleTextWidget() const
{
	if (WidgetTree == nullptr)
	{
		return nullptr;
	}

	if (UWidget* NamedWidget = WidgetTree->FindWidget(FName(TEXT("TXT_BubbleText"))))
	{
		return NamedWidget;
	}

	UWidget* FirstSupportedTextWidget = nullptr;
	WidgetTree->ForEachWidget([&FirstSupportedTextWidget](UWidget* Widget)
	{
		if (FirstSupportedTextWidget == nullptr
			&& (Widget->IsA<UTextBlock>()
				|| Widget->IsA<URichTextBlock>()
				|| Widget->IsA<UEditableTextBox>()
				|| Widget->IsA<UEditableText>()
				|| Widget->IsA<UMultiLineEditableTextBox>()
				|| Widget->IsA<UMultiLineEditableText>()))
		{
			FirstSupportedTextWidget = Widget;
		}
	});

	return FirstSupportedTextWidget;
}

UWidget* UUOUSpeechBubbleWidget::ResolveBubbleRootWidget() const
{
	if (WidgetTree != nullptr)
	{
		if (UWidget* FoundRoot = WidgetTree->FindWidget(FName(TEXT("BubbleRoot"))))
		{
			return FoundRoot;
		}
	}

	return const_cast<UUOUSpeechBubbleWidget*>(this);
}

UWorld* UUOUSpeechBubbleWidget::GetTimerWorld() const
{
	return GetWorld();
}
