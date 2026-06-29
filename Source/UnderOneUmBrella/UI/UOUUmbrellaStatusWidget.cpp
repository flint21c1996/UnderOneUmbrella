// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUUmbrellaStatusWidget.h"

#include "Components/SizeBox.h"
#include "Components/Widget.h"

void UUOUUmbrellaStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveStoredWaterFillClipFullWidth();
	ApplyUmbrellaHUDState(CurrentState);
}

void UUOUUmbrellaStatusWidget::ApplyUmbrellaHUDState(const FUOUUmbrellaHUDState& State)
{
	CurrentState = State;

	RefreshVisibility();
	RefreshStateImages();
	RefreshStoredWater();

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

void UUOUUmbrellaStatusWidget::RefreshStoredWater()
{
	bStoredWaterVisible = ShouldShowStoredWater();
	const float FillRatio = FMath::Clamp(CurrentState.StoredWaterRatio, 0.0f, 1.0f);

	SetWidgetVisible(StoredWaterRoot, bStoredWaterVisible);
	SetWidgetVisible(StoredWaterBackImage, bStoredWaterVisible);
	SetWidgetVisible(StoredWaterFillClip, bStoredWaterVisible);
	SetWidgetVisible(StoredWaterFill, bStoredWaterVisible);
	SetWidgetVisible(StoredWaterFrameImage, bStoredWaterVisible);
	SetWidgetVisible(StoredWaterEffectImage, bStoredWaterVisible);

	if (StoredWaterFillClip != nullptr && bResizeStoredWaterFillClip)
	{
		StoredWaterFillClip->SetWidthOverride(ResolvedStoredWaterFillClipFullWidth * FillRatio);
		StoredWaterFillClip->SetClipping(EWidgetClipping::ClipToBounds);
	}

	BP_OnStoredWaterPresentationChanged(FillRatio, CurrentState.StoredWater, CurrentState.MaxStoredWater, bStoredWaterVisible);
}

void UUOUUmbrellaStatusWidget::ResolveStoredWaterFillClipFullWidth()
{
	ResolvedStoredWaterFillClipFullWidth = FMath::Max(0.0f, StoredWaterFillClipFullWidth);

	if (StoredWaterFillClip != nullptr && StoredWaterFillClip->IsWidthOverride())
	{
		const float DesignerWidthOverride = StoredWaterFillClip->GetWidthOverride();
		if (DesignerWidthOverride > 0.0f)
		{
			ResolvedStoredWaterFillClipFullWidth = DesignerWidthOverride;
			StoredWaterFillClipFullWidth = DesignerWidthOverride;
		}
	}
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

bool UUOUUmbrellaStatusWidget::ShouldShowStoredWater() const
{
	if (!bUmbrellaStatusVisible || !CurrentState.bHasUmbrella)
	{
		return false;
	}

	if (!bShowStoredWaterOnlyWhenOpenReversed)
	{
		return true;
	}

	return CurrentState.UmbrellaVisualState == EUOUUmbrellaVisualState::OpenReversed;
}
