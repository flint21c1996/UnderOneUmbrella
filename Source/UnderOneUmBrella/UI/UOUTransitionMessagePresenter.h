// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/UOUTransitionMessageTypes.h"

class SWidget;
class UGameViewportClient;
class UWorld;

class FUOUTransitionMessagePresenter
{
public:
	void Show(UWorld* World, const FUOUTransitionMessageSettings& Settings, bool bShowBlackBackground = false);
	void Hide();
	void SetOpacity(float NewOpacity);

private:
	TWeakObjectPtr<UGameViewportClient> ViewportClient;
	TSharedPtr<SWidget> MessageWidget;
};
