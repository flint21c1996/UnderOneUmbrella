// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AUOUPuzzleConditionGroupActor;
class AActor;
class SVerticalBox;
class UUOUDevelopmentDebugControlSubsystem;
class UUOUDevelopmentDebugDrawSubsystem;
class UUOUDevelopmentPuzzleCheatSubsystem;
enum class EUOUDebugCategory : uint8;

// 퍼즐 치트 컨트롤을 담을 개발 전용 Viewport 오버레이의 기본 껍데기입니다.
class SUOUDevelopmentPuzzleCheatHUD final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUOUDevelopmentPuzzleCheatHUD)
	{
	}
		SLATE_ARGUMENT(TWeakObjectPtr<UUOUDevelopmentDebugControlSubsystem>, DebugControlSubsystem)
		SLATE_ARGUMENT(TWeakObjectPtr<UUOUDevelopmentDebugDrawSubsystem>, DebugDrawSubsystem)
		SLATE_ARGUMENT(TWeakObjectPtr<UUOUDevelopmentPuzzleCheatSubsystem>, PuzzleCheatSubsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// 작은 토글 버튼 아래의 HUD 패널을 확장하거나 접습니다.
	void TogglePanel();

	// 현재 패널의 확장 상태를 반환합니다.
	bool IsPanelExpanded() const { return bPanelExpanded; }

private:
	enum class EActivePage : uint8
	{
		Condition,
		Debug
	};

	void RebuildStepRows();
	void RebuildDebugActorRows();
	FReply HandleToggleClicked();
	FReply HandleConditionTabClicked();
	FReply HandleDebugTabClicked();
	FReply HandleDebugToolsToggleClicked();
	FReply HandleDebugCategoryToggleClicked(EUOUDebugCategory Category);
	FReply HandleDebugActorRefreshClicked();
	FReply HandleDebugActorClearClicked();
	FReply HandleDebugActorClicked(TWeakObjectPtr<AActor> DebugActor);
	FReply HandleRefreshClicked();
	FReply HandleNextClicked();
	FReply HandleCancelClicked();
	FReply HandleStepClicked(int32 TargetStepOrder);
	EVisibility GetPanelVisibility() const;
	EVisibility GetConditionPageVisibility() const;
	EVisibility GetDebugPageVisibility() const;
	FText GetDebugStatusText() const;
	FText GetDebugToolsToggleText() const;
	FText GetDebugCategoryToggleText(EUOUDebugCategory Category) const;
	FText GetSelectedDebugActorText() const;
	FText GetPlayerDebugInfoText() const;
	FText GetPerformanceDebugInfoText() const;
	FText GetVFXDebugInfoText() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	bool IsPuzzleActionEnabled() const;
	bool IsCancelEnabled() const;
	bool IsDebugControlAvailable() const;

	// 전체 및 Puzzle 카테고리 디버그 상태를 제어할 현재 월드 Subsystem의 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentDebugControlSubsystem> DebugControlSubsystem;

	// HUD에 표시할 런타임 디버그 정보를 생성하는 현재 월드 Subsystem의 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentDebugDrawSubsystem> DebugDrawSubsystem;

	// HUD를 소유한 현재 월드 Subsystem의 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentPuzzleCheatSubsystem> PuzzleCheatSubsystem;

	// 현재 수집된 퍼즐 Step 버튼을 동적으로 담는 Slate 컨테이너입니다.
	TSharedPtr<SVerticalBox> StepListBox;

	// 현재 월드에서 선택 가능한 디버그 액터 버튼을 담는 동적 Slate 컨테이너입니다.
	TSharedPtr<SVerticalBox> DebugActorListBox;

	// 확장 패널에서 현재 표시 중인 최상위 페이지입니다.
	EActivePage ActivePage = EActivePage::Condition;

	// 토글 버튼을 제외한 메인 패널의 현재 표시 상태입니다.
	bool bPanelExpanded = false;
};
