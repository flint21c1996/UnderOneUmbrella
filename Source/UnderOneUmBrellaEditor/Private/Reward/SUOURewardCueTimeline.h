// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class FScopedTransaction;
class UUOURewardCollectionMotionComponent;
class UUOURewardFeedbackComponent;

// MotionDuration 안에서 FeedbackComponent의 CueRequest 실행 시점을 편집하는 Slate 위젯입니다.
class SUOURewardCueTimeline : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SUOURewardCueTimeline) {}
		SLATE_ARGUMENT(UUOURewardCollectionMotionComponent*, MotionComponent)
		SLATE_ARGUMENT(UUOURewardFeedbackComponent*, FeedbackComponent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SUOURewardCueTimeline() override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FReply OnMouseButtonDown(
		const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(
		const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(
		const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

private:
	float GetMotionDuration() const;
	float GetCueTime(int32 CueIndex) const;
	float TimeToLocalX(float Time, float Width) const;
	float LocalXToTime(float LocalX, float Width) const;
	float GetLaneY(int32 CueIndex) const;
	int32 FindMarkerAt(const FVector2D& LocalPosition, float Width) const;
	FText GetCueLabel(int32 CueIndex) const;

	bool BeginMarkerDrag(int32 CueIndex);
	void UpdateDraggedMarker(float LocalX, float Width);
	void EndMarkerDrag();

	TWeakObjectPtr<UUOURewardCollectionMotionComponent> MotionComponent;
	TWeakObjectPtr<UUOURewardFeedbackComponent> FeedbackComponent;
	TUniquePtr<FScopedTransaction> ActiveTransaction;
	int32 DraggedCueIndex = INDEX_NONE;
};
