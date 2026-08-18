// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AActor;
class SUOUDevelopmentPuzzleGraphView;
class SWrapBox;
class SVerticalBox;
class UClass;
class UUOUDevelopmentDebugControlSubsystem;
class UUOUDevelopmentDebugDrawSubsystem;
class UUOUDevelopmentPuzzleCheatSubsystem;
struct FUOUDevelopmentDebugActorEntry;
enum class EUOUDebugCategory : uint8;
enum class EUOUPlayerDebugFeature : uint8;

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

	void RebuildGraphRows();
	void RebuildDebugActorRows();
	void RebuildDebugActorClassFilters(const TArray<FUOUDevelopmentDebugActorEntry>& ActorEntries);
	FReply HandleToggleClicked();
	FReply HandleConditionTabClicked();
	FReply HandleDebugTabClicked();
	FReply HandleDebugToolsToggleClicked();
	FReply HandleDebugCategoryToggleClicked(EUOUDebugCategory Category);
	FReply HandlePlayerDebugFeatureToggleClicked(EUOUPlayerDebugFeature Feature);
	FReply HandleDebugActorRefreshClicked();
	FReply HandleDebugActorClearClicked();
	FReply HandleDebugActorClicked(TWeakObjectPtr<AActor> DebugActor);
	FReply HandleDebugActorClassFilterClicked(TWeakObjectPtr<UClass> DebugActorClass);
	FReply HandleRefreshClicked();
	FReply HandleCancelClicked();
	void HandleGraphNodeClicked(int32 TargetNodeIndex);
	void HandleExternalInputClicked(int32 NodeIndex, int32 ExternalInputIndex);
	EVisibility GetPanelVisibility() const;
	EVisibility GetConditionPageVisibility() const;
	EVisibility GetDebugPageVisibility() const;
	FText GetDebugStatusText() const;
	FText GetDebugToolsToggleText() const;
	FText GetDebugCategoryToggleText(EUOUDebugCategory Category) const;
	FText GetPlayerDebugFeatureToggleText(EUOUPlayerDebugFeature Feature) const;
	FText GetSelectedDebugActorsText() const;
	FText GetPlayerDebugInfoText() const;
	FText GetPerformanceDebugInfoText() const;
	FText GetVFXDebugInfoText() const;
	FText GetGraphStatusText() const;
	FSlateColor GetGraphStatusColor() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	bool IsCancelEnabled() const;
	bool IsDebugControlAvailable() const;
	bool DoesDebugActorPassClassFilter(const AActor* DebugActor) const;

	// 전체 및 Puzzle 카테고리 디버그 상태를 제어할 현재 월드 Subsystem의 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentDebugControlSubsystem> DebugControlSubsystem;

	// HUD에 표시할 런타임 디버그 정보를 생성하는 현재 월드 Subsystem의 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentDebugDrawSubsystem> DebugDrawSubsystem;

	// HUD를 소유한 현재 월드 Subsystem의 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentPuzzleCheatSubsystem> PuzzleCheatSubsystem;

	// 실제 Condition/Result 관계를 열과 연결선으로 표시하는 그래프 위젯입니다.
	TSharedPtr<SUOUDevelopmentPuzzleGraphView> PuzzleGraphView;

	// 현재 월드에서 선택 가능한 디버그 액터 버튼을 담는 동적 Slate 컨테이너입니다.
	TSharedPtr<SVerticalBox> DebugActorListBox;

	// 현재 월드에서 발견한 디버그 액터 클래스 필터 버튼을 담는 동적 Slate 컨테이너입니다.
	TSharedPtr<SWrapBox> DebugActorClassFilterBox;

	// HUD 액터 목록에 표시할 정확한 클래스를 저장하며, 비어 있으면 전체 클래스를 표시합니다.
	TWeakObjectPtr<UClass> SelectedDebugActorClassFilter;

	// 확장 패널에서 현재 표시 중인 최상위 페이지입니다.
	EActivePage ActivePage = EActivePage::Condition;

	// 토글 버튼을 제외한 메인 패널의 현재 표시 상태입니다.
	bool bPanelExpanded = false;
};
