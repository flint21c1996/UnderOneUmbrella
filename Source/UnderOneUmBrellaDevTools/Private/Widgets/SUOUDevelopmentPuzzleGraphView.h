// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUDevelopmentPuzzleCheatGraphTypes.h"
#include "Widgets/SCompoundWidget.h"

class SCanvas;
class UUOUDevelopmentPuzzleCheatSubsystem;

DECLARE_DELEGATE_OneParam(FOnUOUPuzzleGraphNodeClicked, int32);
DECLARE_DELEGATE_TwoParams(FOnUOUPuzzleExternalInputClicked, int32, int32);

// ConditionGroup 노드와 실제 Result 연결을 열 기반 그래프로 표시하는 개발 전용 Slate 위젯입니다.
class SUOUDevelopmentPuzzleGraphView final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUOUDevelopmentPuzzleGraphView)
	{
	}
		SLATE_ARGUMENT(TWeakObjectPtr<UUOUDevelopmentPuzzleCheatSubsystem>, PuzzleCheatSubsystem)
		SLATE_EVENT(FOnUOUPuzzleGraphNodeClicked, OnNodeClicked)
		SLATE_EVENT(FOnUOUPuzzleExternalInputClicked, OnExternalInputClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Subsystem의 최신 그래프 스냅샷을 가져와 카드와 연결선 레이아웃을 다시 만듭니다.
	void RefreshGraph();

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	void RebuildLayout();
	FReply HandleNodeClicked(int32 NodeIndex);
	FReply HandleExternalInputClicked(int32 NodeIndex, int32 ExternalInputIndex);
	FText BuildNodeDetailText(int32 NodeIndex) const;
	FText GetNodeTitleText(int32 NodeIndex) const;
	FSlateColor GetNodeTitleColor(int32 NodeIndex) const;
	FText GetExternalInputButtonText(int32 NodeIndex, int32 ExternalInputIndex) const;
	FText GetExternalInputTooltipText(int32 NodeIndex, int32 ExternalInputIndex) const;
	FSlateColor GetExternalInputButtonColor(int32 NodeIndex, int32 ExternalInputIndex) const;
	bool IsExternalInputActionEnabled(int32 NodeIndex, int32 ExternalInputIndex) const;
	bool IsNodeActionEnabled() const;
	const FUOUDevelopmentPuzzleCheatGraphNode* FindNode(int32 NodeIndex) const;

	// 그래프 원본을 소유하는 현재 월드 Subsystem의 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentPuzzleCheatSubsystem> PuzzleCheatSubsystem;

	// 노드 카드 클릭 시 실행 책임을 가진 상위 HUD에 노드 인덱스를 전달합니다.
	FOnUOUPuzzleGraphNodeClicked OnNodeClicked;

	// 외부 입력 버튼 클릭을 실제 실행 책임을 가진 상위 HUD에 전달합니다.
	FOnUOUPuzzleExternalInputClicked OnExternalInputClicked;

	// 마지막 RefreshGraph 시점의 표시용 노드 스냅샷입니다.
	TArray<FUOUDevelopmentPuzzleCheatGraphNode> GraphNodes;

	// 마지막 RefreshGraph 시점의 표시용 연결 스냅샷입니다.
	TArray<FUOUDevelopmentPuzzleCheatGraphEdge> GraphEdges;

	// 노드 인덱스별 그래프 로컬 좌상단 위치입니다.
	TMap<int32, FVector2D> NodePositions;

	// 모든 카드가 들어갈 수 있도록 계산된 위젯 전체 크기입니다.
	FVector2D GraphDesiredSize = FVector2D(560.0, 220.0);

	// 계산된 위치에 Depth 제목, 노드 카드, 연결 라벨을 배치하는 Slate 캔버스입니다.
	TSharedPtr<SCanvas> GraphCanvas;
};
