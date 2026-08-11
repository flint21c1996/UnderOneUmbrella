// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AUOUPuzzleConditionGroupActor;
class SVerticalBox;
class UUOUDevelopmentPuzzleCheatSubsystem;

// 퍼즐 치트 컨트롤을 담을 개발 전용 Viewport 오버레이의 기본 껍데기입니다.
class SUOUDevelopmentPuzzleCheatHUD final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUOUDevelopmentPuzzleCheatHUD)
	{
	}
		SLATE_ARGUMENT(TWeakObjectPtr<UUOUDevelopmentPuzzleCheatSubsystem>, PuzzleCheatSubsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// 작은 토글 버튼 아래의 HUD 패널을 확장하거나 접습니다.
	void TogglePanel();

	// 현재 패널의 확장 상태를 반환합니다.
	bool IsPanelExpanded() const { return bPanelExpanded; }

private:
	void RebuildStepRows();
	FReply HandleToggleClicked();
	FReply HandleRefreshClicked();
	FReply HandleNextClicked();
	FReply HandleCancelClicked();
	FReply HandleStepClicked(int32 TargetStepOrder);
	EVisibility GetPanelVisibility() const;
	FText GetStatusText() const;
	bool IsPuzzleActionEnabled() const;
	bool IsCancelEnabled() const;

	// HUD를 소유한 현재 월드 Subsystem의 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentPuzzleCheatSubsystem> PuzzleCheatSubsystem;

	// 현재 수집된 퍼즐 Step 버튼을 동적으로 담는 Slate 컨테이너입니다.
	TSharedPtr<SVerticalBox> StepListBox;

	// 토글 버튼을 제외한 메인 패널의 현재 표시 상태입니다.
	bool bPanelExpanded = false;
};
