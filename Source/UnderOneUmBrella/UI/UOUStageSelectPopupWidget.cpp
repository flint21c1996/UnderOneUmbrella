// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUStageSelectPopupWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UUOUStageSelectPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 데이터가 위젯 생성보다 먼저 적용된 경우에도 바인딩 완료 후 표시를 다시 맞춥니다.
	if (bHasStageData)
	{
		RefreshRewardProgressText();
		RefreshRewardStatusImage();
	}
}

void UUOUStageSelectPopupWidget::SetStageData(const FUOUStageDefinition& InStageData)
{
	StageData = InStageData;
	bHasStageData = true;

	RefreshRewardProgressText();
	RefreshRewardStatusImage();
	BP_OnStageDataChanged(StageData);
}

void UUOUStageSelectPopupWidget::RefreshRewardProgressText()
{
	if (!RewardProgressText)
	{
		return;
	}

	const FText RewardProgress = FText::Format(
		NSLOCTEXT("StageSelect", "RewardProgressFormat", "별 {0}/{1} · 부족 {2}"),
		FText::AsNumber(StageData.CollectedRewardCount),
		FText::AsNumber(StageData.TotalRewardCount),
		FText::AsNumber(StageData.MissingRewardCount));

	RewardProgressText->SetText(RewardProgress);
}

void UUOUStageSelectPopupWidget::RefreshRewardStatusImage()
{
	if (!RewardStatusImage)
	{
		return;
	}

	const FSlateBrush& RewardBrush = StageData.MissingRewardCount > 0
		? EmptyRewardBrush
		: CompletedRewardBrush;
	RewardStatusImage->SetBrush(RewardBrush);
}
