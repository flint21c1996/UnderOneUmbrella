// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUStageSelectPopupWidget.h"

void UUOUStageSelectPopupWidget::SetStageData(const FUOUStageDefinition& InStageData)
{
	StageData = InStageData;
	bHasStageData = true;

	BP_OnStageDataChanged(StageData);
}
