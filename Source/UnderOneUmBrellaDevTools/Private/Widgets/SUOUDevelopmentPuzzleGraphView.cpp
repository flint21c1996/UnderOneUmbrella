// Copyright Epic Games, Inc. All Rights Reserved.

#include "Widgets/SUOUDevelopmentPuzzleGraphView.h"

#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UOUDevelopmentPuzzleCheatSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Text/STextBlock.h"

namespace UOUDevelopmentPuzzleGraphViewPrivate
{
	constexpr float CanvasPadding = 18.0f;
	constexpr float DepthHeaderHeight = 24.0f;
	constexpr float NodeWidth = 260.0f;
	constexpr float NodeHeight = 196.0f;
	constexpr float HorizontalGap = 130.0f;
	constexpr float VerticalGap = 24.0f;
	constexpr float EdgeThickness = 2.0f;
}

void SUOUDevelopmentPuzzleGraphView::Construct(const FArguments& InArgs)
{
	PuzzleCheatSubsystem = InArgs._PuzzleCheatSubsystem;
	OnNodeClicked = InArgs._OnNodeClicked;
	OnExternalInputClicked = InArgs._OnExternalInputClicked;
	SetVisibility(EVisibility::SelfHitTestInvisible);

	ChildSlot
	[
		SAssignNew(GraphCanvas, SCanvas)
	];

	RefreshGraph();
}

void SUOUDevelopmentPuzzleGraphView::RefreshGraph()
{
	GraphNodes.Reset();
	GraphEdges.Reset();

	if (const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get())
	{
		GraphNodes = Subsystem->GetPuzzleGraphNodesView();
		GraphEdges = Subsystem->GetPuzzleGraphEdgesView();
	}

	RebuildLayout();
}

FVector2D SUOUDevelopmentPuzzleGraphView::ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const
{
	return GraphDesiredSize;
}

int32 SUOUDevelopmentPuzzleGraphView::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	using namespace UOUDevelopmentPuzzleGraphViewPrivate;

	const FSlateBrush* LineBrush = FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox"));
	const FLinearColor LineColor(0.35f, 0.68f, 1.0f, 0.85f);
	const auto DrawSegment = [&](const FVector2D& Start, const FVector2D& End)
	{
		const bool bHorizontal = FMath::IsNearlyEqual(Start.Y, End.Y);
		const FVector2D Position(
			FMath::Min(Start.X, End.X) - (bHorizontal ? 0.0 : EdgeThickness * 0.5),
			FMath::Min(Start.Y, End.Y) - (bHorizontal ? EdgeThickness * 0.5 : 0.0));
		const FVector2D Size(
			bHorizontal ? FMath::Max(EdgeThickness, FMath::Abs(End.X - Start.X)) : EdgeThickness,
			bHorizontal ? EdgeThickness : FMath::Max(EdgeThickness, FMath::Abs(End.Y - Start.Y)));

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(Size),
				FSlateLayoutTransform(FVector2f(Position))),
			LineBrush,
			ESlateDrawEffect::None,
			LineColor);
	};

	for (const FUOUDevelopmentPuzzleCheatGraphEdge& Edge : GraphEdges)
	{
		const FVector2D* SourcePosition = NodePositions.Find(Edge.SourceNodeIndex);
		const FVector2D* TargetPosition = NodePositions.Find(Edge.TargetNodeIndex);
		if (SourcePosition == nullptr || TargetPosition == nullptr)
		{
			continue;
		}

		const FVector2D Start(
			SourcePosition->X + NodeWidth,
			SourcePosition->Y + NodeHeight * 0.5f);
		const FVector2D End(
			TargetPosition->X,
			TargetPosition->Y + NodeHeight * 0.5f);
		const float MiddleX = (Start.X + End.X) * 0.5f;

		DrawSegment(Start, FVector2D(MiddleX, Start.Y));
		DrawSegment(FVector2D(MiddleX, Start.Y), FVector2D(MiddleX, End.Y));
		DrawSegment(FVector2D(MiddleX, End.Y), End);
	}

	return SCompoundWidget::OnPaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId + 1,
		InWidgetStyle,
		bParentEnabled);
}

void SUOUDevelopmentPuzzleGraphView::RebuildLayout()
{
	using namespace UOUDevelopmentPuzzleGraphViewPrivate;

	NodePositions.Reset();
	GraphDesiredSize = FVector2D(560.0, 220.0);
	if (!GraphCanvas.IsValid())
	{
		return;
	}

	GraphCanvas->ClearChildren();
	if (GraphNodes.IsEmpty())
	{
		GraphCanvas->AddSlot()
		.Position(FVector2D(CanvasPadding, CanvasPadding))
		.Size(FVector2D(500.0, 28.0))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("ConditionGroup 관계를 찾지 못했습니다.")))
		];
		return;
	}

	int32 MaximumValidDepth = INDEX_NONE;
	for (const FUOUDevelopmentPuzzleCheatGraphNode& Node : GraphNodes)
	{
		if (Node.ExecutionDepth != INDEX_NONE)
		{
			MaximumValidDepth = FMath::Max(MaximumValidDepth, Node.ExecutionDepth);
		}
	}
	const int32 InvalidDepthColumn = MaximumValidDepth + 1;

	TMap<int32, TArray<int32>> NodeIndicesByDepth;
	for (const FUOUDevelopmentPuzzleCheatGraphNode& Node : GraphNodes)
	{
		const int32 LayoutDepth = Node.ExecutionDepth == INDEX_NONE
			? InvalidDepthColumn
			: Node.ExecutionDepth;
		NodeIndicesByDepth.FindOrAdd(LayoutDepth).Add(Node.NodeIndex);
	}

	TArray<int32> LayoutDepths;
	NodeIndicesByDepth.GetKeys(LayoutDepths);
	LayoutDepths.Sort();

	int32 MaximumRowCount = 1;
	for (int32 ColumnIndex = 0; ColumnIndex < LayoutDepths.Num(); ++ColumnIndex)
	{
		const int32 LayoutDepth = LayoutDepths[ColumnIndex];
		TArray<int32>& NodeIndices = NodeIndicesByDepth.FindChecked(LayoutDepth);
		NodeIndices.Sort([this](int32 LeftIndex, int32 RightIndex)
		{
			const FUOUDevelopmentPuzzleCheatGraphNode* LeftNode = FindNode(LeftIndex);
			const FUOUDevelopmentPuzzleCheatGraphNode* RightNode = FindNode(RightIndex);
			return LeftNode != nullptr && RightNode != nullptr
				? LeftNode->DisplayName.ToString() < RightNode->DisplayName.ToString()
				: LeftIndex < RightIndex;
		});
		MaximumRowCount = FMath::Max(MaximumRowCount, NodeIndices.Num());

		const float ColumnX = CanvasPadding + ColumnIndex * (NodeWidth + HorizontalGap);
		const bool bInvalidDepth = LayoutDepth == InvalidDepthColumn;
		const FString DepthText = bInvalidDepth
			? TEXT("순환 / Depth 오류")
			: FString::Printf(TEXT("Depth %d"), LayoutDepth);
		GraphCanvas->AddSlot()
		.Position(FVector2D(ColumnX, CanvasPadding))
		.Size(FVector2D(NodeWidth, DepthHeaderHeight))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")))
			.BorderBackgroundColor(bInvalidDepth
				? FLinearColor(0.45f, 0.12f, 0.10f, 0.95f)
				: FLinearColor(0.10f, 0.10f, 0.12f, 0.95f))
			.Padding(FMargin(6.0f, 3.0f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(DepthText))
			]
		];

		for (int32 RowIndex = 0; RowIndex < NodeIndices.Num(); ++RowIndex)
		{
			const int32 NodeIndex = NodeIndices[RowIndex];
			const FUOUDevelopmentPuzzleCheatGraphNode* Node = FindNode(NodeIndex);
			TSharedRef<SVerticalBox> ExternalInputButtons = SNew(SVerticalBox);
			if (Node != nullptr && !Node->ExternalInputs.IsEmpty())
			{
				ExternalInputButtons->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 5.0f, 0.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("외부 입력")))
					.ColorAndOpacity(FLinearColor(0.72f, 0.78f, 0.88f, 1.0f))
				];

				for (int32 ExternalInputIndex = 0;
					ExternalInputIndex < Node->ExternalInputs.Num();
					++ExternalInputIndex)
				{
					ExternalInputButtons->AddSlot()
					.AutoHeight()
					.Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.IsFocusable(false)
						.OnClicked(
							this,
							&SUOUDevelopmentPuzzleGraphView::HandleExternalInputClicked,
							NodeIndex,
							ExternalInputIndex)
						.IsEnabled(
							this,
							&SUOUDevelopmentPuzzleGraphView::IsExternalInputActionEnabled,
							NodeIndex,
							ExternalInputIndex)
						.ButtonColorAndOpacity(
							this,
							&SUOUDevelopmentPuzzleGraphView::GetExternalInputButtonColor,
							NodeIndex,
							ExternalInputIndex)
						.ContentPadding(FMargin(5.0f, 2.0f))
						[
							SNew(STextBlock)
							.Text(
								this,
								&SUOUDevelopmentPuzzleGraphView::GetExternalInputButtonText,
								NodeIndex,
								ExternalInputIndex)
							.Justification(ETextJustify::Center)
						]
					];
				}
			}
			else
			{
				ExternalInputButtons->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 5.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("외부 입력: 없음")))
					.ColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f, 1.0f))
				];
			}

			const FVector2D NodePosition(
				ColumnX,
				CanvasPadding + DepthHeaderHeight + 10.0f + RowIndex * (NodeHeight + VerticalGap));
			NodePositions.Add(NodeIndex, NodePosition);

			GraphCanvas->AddSlot()
			.Position(NodePosition)
			.Size(FVector2D(NodeWidth, NodeHeight))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")))
				.BorderBackgroundColor(FLinearColor(0.055f, 0.06f, 0.075f, 0.98f))
				.Padding(8.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SButton)
						.IsFocusable(false)
						.OnClicked(this, &SUOUDevelopmentPuzzleGraphView::HandleNodeClicked, NodeIndex)
						.IsEnabled(this, &SUOUDevelopmentPuzzleGraphView::IsNodeActionEnabled)
						.ContentPadding(FMargin(4.0f, 3.0f))
						[
							SNew(STextBlock)
							.Text(this, &SUOUDevelopmentPuzzleGraphView::GetNodeTitleText, NodeIndex)
							.ColorAndOpacity(this, &SUOUDevelopmentPuzzleGraphView::GetNodeTitleColor, NodeIndex)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(BuildNodeDetailText(NodeIndex))
						.AutoWrapText(true)
						.ColorAndOpacity(FLinearColor(0.78f, 0.78f, 0.78f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						ExternalInputButtons
					]
				]
			];
		}
	}

	TMap<uint64, int32> ParallelEdgeCounts;
	for (const FUOUDevelopmentPuzzleCheatGraphEdge& Edge : GraphEdges)
	{
		const FVector2D* SourcePosition = NodePositions.Find(Edge.SourceNodeIndex);
		const FVector2D* TargetPosition = NodePositions.Find(Edge.TargetNodeIndex);
		if (SourcePosition == nullptr || TargetPosition == nullptr)
		{
			continue;
		}

		const uint64 EdgePairKey = (static_cast<uint64>(static_cast<uint32>(Edge.SourceNodeIndex)) << 32)
			| static_cast<uint32>(Edge.TargetNodeIndex);
		const int32 EdgeOffsetIndex = ParallelEdgeCounts.FindOrAdd(EdgePairKey)++;
		const FVector2D LabelPosition(
			(SourcePosition->X + NodeWidth + TargetPosition->X) * 0.5f - 58.0f,
			(SourcePosition->Y + TargetPosition->Y) * 0.5f + NodeHeight * 0.5f
				+ EdgeOffsetIndex * 20.0f - 10.0f);
		const AActor* RelationActor = Edge.RelationActor.Get();
		const FString RelationName = RelationActor != nullptr
			? RelationActor->GetName()
			: TEXT("직접 연결");

		GraphCanvas->AddSlot()
		.Position(LabelPosition)
		.Size(FVector2D(116.0f, 20.0f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")))
			.BorderBackgroundColor(FLinearColor(0.08f, 0.13f, 0.20f, 0.96f))
			.Padding(FMargin(4.0f, 1.0f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("→ %s"), *RelationName)))
				.Justification(ETextJustify::Center)
			]
		];
	}

	GraphDesiredSize = FVector2D(
		FMath::Max(560.0f, CanvasPadding * 2.0f
			+ LayoutDepths.Num() * NodeWidth
			+ FMath::Max(0, LayoutDepths.Num() - 1) * HorizontalGap),
		FMath::Max(220.0f, CanvasPadding * 2.0f + DepthHeaderHeight + 10.0f
			+ MaximumRowCount * NodeHeight
			+ FMath::Max(0, MaximumRowCount - 1) * VerticalGap));
}

FReply SUOUDevelopmentPuzzleGraphView::HandleNodeClicked(int32 NodeIndex)
{
	OnNodeClicked.ExecuteIfBound(NodeIndex);
	return FReply::Handled();
}

FReply SUOUDevelopmentPuzzleGraphView::HandleExternalInputClicked(
	int32 NodeIndex,
	int32 ExternalInputIndex)
{
	OnExternalInputClicked.ExecuteIfBound(NodeIndex, ExternalInputIndex);
	return FReply::Handled();
}

FText SUOUDevelopmentPuzzleGraphView::BuildNodeDetailText(int32 NodeIndex) const
{
	const FUOUDevelopmentPuzzleCheatGraphNode* Node = FindNode(NodeIndex);
	if (Node == nullptr)
	{
		return FText::GetEmpty();
	}

	TArray<FString> ResultNames;
	for (const TObjectPtr<AActor>& ResultActor : Node->ResultActors)
	{
		if (IsValid(ResultActor.Get()))
		{
			ResultNames.AddUnique(ResultActor->GetName());
		}
	}

	const FString ResultText = ResultNames.IsEmpty()
		? TEXT("없음")
		: FString::Join(ResultNames, TEXT(", "));
	return FText::FromString(FString::Printf(TEXT("결과: %s"), *ResultText));
}

FText SUOUDevelopmentPuzzleGraphView::GetNodeTitleText(int32 NodeIndex) const
{
	const FUOUDevelopmentPuzzleCheatGraphNode* Node = FindNode(NodeIndex);
	if (Node == nullptr)
	{
		return FText::FromString(TEXT("[오류] 유효하지 않은 노드"));
	}

	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	const bool bActive = Subsystem != nullptr && Subsystem->IsGraphNodeActive(NodeIndex);
	const AUOUPuzzleConditionGroupActor* PuzzleGroup = Node->PuzzleGroup.Get();
	const TCHAR* StateText = bActive
		? TEXT("실행 중")
		: (IsValid(PuzzleGroup) && PuzzleGroup->IsSatisfied() ? TEXT("완료") : TEXT("대기"));
	return FText::FromString(FString::Printf(
		TEXT("[%s] %s"),
		StateText,
		*Node->DisplayName.ToString()));
}

FSlateColor SUOUDevelopmentPuzzleGraphView::GetNodeTitleColor(int32 NodeIndex) const
{
	const FUOUDevelopmentPuzzleCheatGraphNode* Node = FindNode(NodeIndex);
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	if (Node != nullptr && Subsystem != nullptr && Subsystem->IsGraphNodeActive(NodeIndex))
	{
		return FSlateColor(FLinearColor(1.0f, 0.8f, 0.2f, 1.0f));
	}

	const AUOUPuzzleConditionGroupActor* PuzzleGroup = Node != nullptr
		? Node->PuzzleGroup.Get()
		: nullptr;
	return FSlateColor(IsValid(PuzzleGroup) && PuzzleGroup->IsSatisfied()
		? FLinearColor(0.25f, 1.0f, 0.35f, 1.0f)
		: FLinearColor::White);
}

FText SUOUDevelopmentPuzzleGraphView::GetExternalInputButtonText(
	int32 NodeIndex,
	int32 ExternalInputIndex) const
{
	const FUOUDevelopmentPuzzleCheatGraphNode* Node = FindNode(NodeIndex);
	if (Node == nullptr || !Node->ExternalInputs.IsValidIndex(ExternalInputIndex))
	{
		return FText::FromString(TEXT("[오류] 유효하지 않은 외부 입력"));
	}

	const FUOUDevelopmentPuzzleCheatExternalInput& ExternalInput =
		Node->ExternalInputs[ExternalInputIndex];
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	const bool bSatisfied = Subsystem != nullptr
		&& Subsystem->IsExternalInputSatisfied(NodeIndex, ExternalInputIndex);
	const FString InputName = ExternalInput.DisplayName.IsEmpty()
		? GetNameSafe(ExternalInput.InputActor.Get())
		: ExternalInput.DisplayName.ToString();
	return FText::FromString(FString::Printf(
		TEXT("[%s] %s"),
		bSatisfied ? TEXT("완료") : TEXT("대기"),
		*InputName));
}

FSlateColor SUOUDevelopmentPuzzleGraphView::GetExternalInputButtonColor(
	int32 NodeIndex,
	int32 ExternalInputIndex) const
{
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	const bool bSatisfied = Subsystem != nullptr
		&& Subsystem->IsExternalInputSatisfied(NodeIndex, ExternalInputIndex);
	return FSlateColor(bSatisfied
		? FLinearColor(0.12f, 0.55f, 0.20f, 1.0f)
		: FLinearColor(0.16f, 0.32f, 0.52f, 1.0f));
}

bool SUOUDevelopmentPuzzleGraphView::IsExternalInputActionEnabled(
	int32 NodeIndex,
	int32 ExternalInputIndex) const
{
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	const FUOUDevelopmentPuzzleCheatGraphNode* Node = FindNode(NodeIndex);
	if (Subsystem == nullptr
		|| Subsystem->IsGraphExecutionActive()
		|| Node == nullptr
		|| !Node->ExternalInputs.IsValidIndex(ExternalInputIndex))
	{
		return false;
	}

	const FUOUDevelopmentPuzzleCheatExternalInput& ExternalInput =
		Node->ExternalInputs[ExternalInputIndex];
	return IsValid(ExternalInput.InputActor.Get())
		&& !ExternalInput.ConditionSources.IsEmpty()
		&& !Subsystem->IsExternalInputSatisfied(NodeIndex, ExternalInputIndex);
}

bool SUOUDevelopmentPuzzleGraphView::IsNodeActionEnabled() const
{
	const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = PuzzleCheatSubsystem.Get();
	return Subsystem != nullptr
		&& Subsystem->IsPuzzleGraphValid()
		&& !Subsystem->IsGraphExecutionActive();
}

const FUOUDevelopmentPuzzleCheatGraphNode* SUOUDevelopmentPuzzleGraphView::FindNode(int32 NodeIndex) const
{
	return GraphNodes.FindByPredicate(
		[NodeIndex](const FUOUDevelopmentPuzzleCheatGraphNode& Node)
		{
			return Node.NodeIndex == NodeIndex;
		});
}
