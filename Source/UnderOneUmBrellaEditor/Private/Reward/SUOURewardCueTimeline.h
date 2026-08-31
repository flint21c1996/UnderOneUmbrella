// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/Rewards/UOURewardCollectionMotionComponent.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "Widgets/SLeafWidget.h"

class FScopedTransaction;
class UUOURewardFeedbackComponent;

// MotionDuration 안에서 FeedbackComponent의 CueRequest 실행 시점을 편집하는 Slate 위젯입니다.
class SUOURewardCueTimeline : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SUOURewardCueTimeline) {}
		SLATE_ARGUMENT(UUOURewardCollectionMotionComponent*, MotionComponent)
		SLATE_ARGUMENT(EUOURewardMotionPhase, MotionPhase)
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
	const TArray<FUOURewardPresentationCue>& GetCueRequests() const;
	const TArray<FUOURewardMotionCueTiming>& GetCueTimeline() const;
	UObject* GetMotionObject() const;
	void SetCueTriggerTime(const FGuid& RequestId, float TriggerTime);
	void SetPresentationCloseTime(const FGuid& RequestId, float CloseTime);
	float GetCueTime(int32 CueIndex) const;
	// Presentation Cue에만 존재하는 두 번째 Outro 마커 시간을 반환합니다.
	float GetPresentationCloseTime(int32 CueIndex) const;
	float TimeToLocalX(float Time, float Width) const;
	float LocalXToTime(float LocalX, float Width) const;
	float GetLaneY(int32 CueIndex) const;
	int32 FindMarkerAt(
		const FVector2D& LocalPosition,
		float Width,
		bool& bOutPresentationCloseMarker) const;
	FText GetCueLabel(int32 CueIndex) const;

	bool BeginMarkerDrag(int32 CueIndex, bool bPresentationCloseMarker);
	void UpdateDraggedMarker(float LocalX, float Width);
	void EndMarkerDrag();

	TWeakObjectPtr<UUOURewardCollectionMotionComponent> MotionComponent;
	TWeakObjectPtr<UUOURewardFeedbackComponent> FeedbackComponent;
	EUOURewardMotionPhase MotionPhase = EUOURewardMotionPhase::Collection;
	TUniquePtr<FScopedTransaction> ActiveTransaction;
	int32 DraggedCueIndex = INDEX_NONE;
	// 같은 Presentation 행에서 시작 마커와 Outro 마커 중 무엇을 드래그하는지 구분합니다.
	bool bDraggingPresentationCloseMarker = false;
};
