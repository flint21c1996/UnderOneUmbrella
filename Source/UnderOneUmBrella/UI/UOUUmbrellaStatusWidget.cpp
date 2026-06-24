// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUUmbrellaStatusWidget.h"

#include "Components/Widget.h"

void UUOUUmbrellaStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyUmbrellaHUDState(CurrentState);
}

void UUOUUmbrellaStatusWidget::ApplyUmbrellaHUDState(const FUOUUmbrellaHUDState& State)
{
	CurrentState = State;

	RefreshVisibility();
	RefreshStateImages();

	BP_OnUmbrellaHUDStateApplied(CurrentState);
}

void UUOUUmbrellaStatusWidget::RefreshVisibility()
{
	bUmbrellaStatusVisible = CurrentState.bHasUmbrella || !bHideWhenNoUmbrella;

	if (UWidget* RootWidget = ResolveRootWidget())
	{
		SetWidgetVisible(RootWidget, bUmbrellaStatusVisible);
	}

	SetWidgetVisible(FrameImage, bUmbrellaStatusVisible);
}

void UUOUUmbrellaStatusWidget::RefreshStateImages()
{
	const bool bCanShowState = bUmbrellaStatusVisible && CurrentState.bHasUmbrella;

	SetWidgetVisible(ClosedUmbrellaImage, bCanShowState && CurrentState.UmbrellaVisualState == EUOUUmbrellaVisualState::Closed);
	SetWidgetVisible(OpenUmbrellaImage, bCanShowState && CurrentState.UmbrellaVisualState == EUOUUmbrellaVisualState::Open);
	SetWidgetVisible(ClosedReversedUmbrellaImage, bCanShowState && CurrentState.UmbrellaVisualState == EUOUUmbrellaVisualState::ClosedReversed);
	SetWidgetVisible(OpenReversedUmbrellaImage, bCanShowState && CurrentState.UmbrellaVisualState == EUOUUmbrellaVisualState::OpenReversed);
}

void UUOUUmbrellaStatusWidget::SetWidgetVisible(UWidget* Widget, bool bNewVisible) const
{
	if (Widget != nullptr)
	{
		Widget->SetVisibility(bNewVisible ? ShownVisibility : HiddenVisibility);
	}
}

UWidget* UUOUUmbrellaStatusWidget::ResolveRootWidget() const
{
	return UmbrellaStatusRoot != nullptr ? UmbrellaStatusRoot.Get() : const_cast<UUOUUmbrellaStatusWidget*>(this);
}
