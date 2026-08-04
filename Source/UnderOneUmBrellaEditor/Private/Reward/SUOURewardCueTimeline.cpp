// Copyright Epic Games, Inc. All Rights Reserved.

#include "Reward/SUOURewardCueTimeline.h"

#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "World/Rewards/UOURewardCollectionMotionComponent.h"
#include "World/Rewards/UOURewardFeedbackComponent.h"

#define LOCTEXT_NAMESPACE "UOURewardCueTimeline"

namespace UOURewardCueTimelinePrivate
{
	constexpr float LabelWidth = 155.0f;
	constexpr float RightPadding = 20.0f;
	constexpr float AxisY = 27.0f;
	constexpr float FirstLaneY = 54.0f;
	constexpr float LaneHeight = 29.0f;
	constexpr float MarkerWidth = 12.0f;
	constexpr float MarkerHeight = 20.0f;
	constexpr int32 TickDivisionCount = 4;
}

void SUOURewardCueTimeline::Construct(const FArguments& InArgs)
{
	MotionComponent = InArgs._MotionComponent;
	FeedbackComponent = InArgs._FeedbackComponent;
}

SUOURewardCueTimeline::~SUOURewardCueTimeline()
{
	ActiveTransaction.Reset();
}

FVector2D SUOURewardCueTimeline::ComputeDesiredSize(
	float LayoutScaleMultiplier) const
{
	const UUOURewardFeedbackComponent* Feedback = FeedbackComponent.Get();
	const int32 CueCount = Feedback != nullptr
		? Feedback->GetCueRequests().Num()
		: 0;
	const float DesiredHeight = FMath::Max(
		110.0f,
		UOURewardCueTimelinePrivate::FirstLaneY
			+ CueCount * UOURewardCueTimelinePrivate::LaneHeight
			+ 8.0f);
	return FVector2D(520.0f, DesiredHeight);
}

int32 SUOURewardCueTimeline::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	using namespace UOURewardCueTimelinePrivate;

	const FVector2f LocalSize = AllottedGeometry.GetLocalSize();
	const float Width = LocalSize.X;
	const float TrackWidth = FMath::Max(1.0f, Width - LabelWidth - RightPadding);
	const FSlateBrush* WhiteBrush = FAppStyle::GetBrush(TEXT("WhiteBrush"));
	const FSlateFontInfo SmallFont = FAppStyle::GetFontStyle(TEXT("SmallFont"));
	const FLinearColor GridColor(0.32f, 0.32f, 0.32f, 0.65f);
	const FLinearColor TextColor(0.72f, 0.72f, 0.72f, 1.0f);

	const UUOURewardFeedbackComponent* Feedback = FeedbackComponent.Get();
	const int32 CueCount = Feedback != nullptr
		? Feedback->GetCueRequests().Num()
		: 0;
	const float GridBottom = CueCount > 0
		? GetLaneY(CueCount - 1) + MarkerHeight * 0.5f
		: AxisY + 46.0f;

	for (int32 TickIndex = 0; TickIndex <= TickDivisionCount; ++TickIndex)
	{
		const float Alpha = static_cast<float>(TickIndex)
			/ static_cast<float>(TickDivisionCount);
		const float TickX = LabelWidth + TrackWidth * Alpha;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(1.0f, GridBottom - AxisY),
				FSlateLayoutTransform(FVector2f(TickX, AxisY))),
			WhiteBrush,
			ESlateDrawEffect::None,
			GridColor);

		const float TickTime = GetMotionDuration() * Alpha;
		const FString TickLabel = FString::Printf(TEXT("%.2f"), TickTime);
		const float LabelX = FMath::Clamp(TickX - 18.0f, LabelWidth, Width - 42.0f);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(42.0f, 16.0f),
				FSlateLayoutTransform(FVector2f(LabelX, 4.0f))),
			TickLabel,
			SmallFont,
			ESlateDrawEffect::None,
			TextColor);
	}

	if (CueCount == 0)
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(FMath::Max(1.0f, Width - 24.0f), 20.0f),
				FSlateLayoutTransform(FVector2f(12.0f, 58.0f))),
			LOCTEXT("NoCueRequests", "FeedbackComponent에 Cue Request가 없습니다."),
			SmallFont,
			ESlateDrawEffect::None,
			TextColor);
		return LayerId + 2;
	}

	for (int32 CueIndex = 0; CueIndex < CueCount; ++CueIndex)
	{
		const float LaneY = GetLaneY(CueIndex);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(TrackWidth, 1.0f),
				FSlateLayoutTransform(FVector2f(LabelWidth, LaneY))),
			WhiteBrush,
			ESlateDrawEffect::None,
			GridColor);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(LabelWidth - 8.0f, 18.0f),
				FSlateLayoutTransform(FVector2f(4.0f, LaneY - 9.0f))),
			GetCueLabel(CueIndex),
			SmallFont,
			ESlateDrawEffect::None,
			TextColor);

		const float CueTime = GetCueTime(CueIndex);
		const float MarkerX = TimeToLocalX(CueTime, Width);
		const FUOURewardPresentationCue& Cue =
			Feedback->GetCueRequests()[CueIndex];
		const bool bPresentationCue =
			Cue.Channel == EUOURewardMotionCueChannel::Presentation;
		const FLinearColor PresentationColor(0.12f, 0.58f, 0.95f, 1.0f);
		const FLinearColor PresentationCloseColor(0.68f, 0.32f, 0.95f, 1.0f);
		FLinearColor MarkerColor = bPresentationCue
			? PresentationColor
			: FLinearColor(0.95f, 0.48f, 0.12f, 1.0f);
		if (CueIndex == DraggedCueIndex
			&& !bDraggingPresentationCloseMarker)
		{
			MarkerColor = FLinearColor::White;
		}

		float CloseTime = CueTime;
		float CloseMarkerX = MarkerX;
		if (bPresentationCue)
		{
			CloseTime = GetPresentationCloseTime(CueIndex);
			CloseMarkerX = TimeToLocalX(CloseTime, Width);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(FMath::Max(1.0f, CloseMarkerX - MarkerX), 5.0f),
					FSlateLayoutTransform(FVector2f(MarkerX, LaneY - 2.5f))),
				WhiteBrush,
				ESlateDrawEffect::None,
				FLinearColor(0.12f, 0.58f, 0.95f, 0.35f));
		}

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(MarkerWidth, MarkerHeight),
				FSlateLayoutTransform(FVector2f(
					MarkerX - MarkerWidth * 0.5f,
					LaneY - MarkerHeight * 0.5f))),
			WhiteBrush,
			ESlateDrawEffect::None,
			MarkerColor);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 3,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(56.0f, 18.0f),
				FSlateLayoutTransform(FVector2f(
					FMath::Min(MarkerX + 8.0f, Width - 56.0f),
					LaneY - 9.0f))),
			bPresentationCue
				? FString::Printf(TEXT("S %.2fs"), CueTime)
				: FString::Printf(TEXT("%.2fs"), CueTime),
			SmallFont,
			ESlateDrawEffect::None,
			MarkerColor);

		if (bPresentationCue)
		{
			FLinearColor CloseMarkerColor = PresentationCloseColor;
			if (CueIndex == DraggedCueIndex
				&& bDraggingPresentationCloseMarker)
			{
				CloseMarkerColor = FLinearColor::White;
			}

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 2,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(MarkerWidth, MarkerHeight),
					FSlateLayoutTransform(FVector2f(
						CloseMarkerX - MarkerWidth * 0.5f,
						LaneY - MarkerHeight * 0.5f))),
				WhiteBrush,
				ESlateDrawEffect::None,
				CloseMarkerColor);

			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 3,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(64.0f, 18.0f),
					FSlateLayoutTransform(FVector2f(
						FMath::Clamp(CloseMarkerX - 64.0f, LabelWidth, Width - 64.0f),
						LaneY - 9.0f))),
				FString::Printf(TEXT("O %.2fs"), CloseTime),
				SmallFont,
				ESlateDrawEffect::None,
				CloseMarkerColor);
		}
	}

	return LayerId + 4;
}

FReply SUOURewardCueTimeline::OnMouseButtonDown(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	const FVector2D LocalPosition =
		MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	bool bPresentationCloseMarker = false;
	const int32 CueIndex = FindMarkerAt(
		LocalPosition,
		MyGeometry.GetLocalSize().X,
		bPresentationCloseMarker);
	if (!BeginMarkerDrag(CueIndex, bPresentationCloseMarker))
	{
		return FReply::Unhandled();
	}

	UpdateDraggedMarker(LocalPosition.X, MyGeometry.GetLocalSize().X);
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SUOURewardCueTimeline::OnMouseMove(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	if (DraggedCueIndex == INDEX_NONE || !HasMouseCapture())
	{
		return FReply::Unhandled();
	}

	const FVector2D LocalPosition =
		MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	UpdateDraggedMarker(LocalPosition.X, MyGeometry.GetLocalSize().X);
	return FReply::Handled();
}

FReply SUOURewardCueTimeline::OnMouseButtonUp(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton
		|| DraggedCueIndex == INDEX_NONE)
	{
		return FReply::Unhandled();
	}

	EndMarkerDrag();
	return FReply::Handled().ReleaseMouseCapture();
}

void SUOURewardCueTimeline::OnMouseCaptureLost(
	const FCaptureLostEvent& CaptureLostEvent)
{
	EndMarkerDrag();
	SLeafWidget::OnMouseCaptureLost(CaptureLostEvent);
}

float SUOURewardCueTimeline::GetMotionDuration() const
{
	const UUOURewardCollectionMotionComponent* Motion = MotionComponent.Get();
	return Motion != nullptr
		? FMath::Max(0.0f, Motion->GetMotionDurationForEditor())
		: 0.0f;
}

float SUOURewardCueTimeline::GetCueTime(int32 CueIndex) const
{
	const UUOURewardCollectionMotionComponent* Motion = MotionComponent.Get();
	const UUOURewardFeedbackComponent* Feedback = FeedbackComponent.Get();
	if (Motion == nullptr
		|| Feedback == nullptr
		|| !Feedback->GetCueRequests().IsValidIndex(CueIndex))
	{
		return 0.0f;
	}

	const FUOURewardPresentationCue& Cue = Feedback->GetCueRequests()[CueIndex];
	if (Cue.RequestId.IsValid())
	{
		const FUOURewardMotionCueTiming* Timing =
			Motion->GetCueTimelineForEditor().FindByPredicate(
				[&Cue](const FUOURewardMotionCueTiming& Candidate)
				{
					return Candidate.RequestId == Cue.RequestId;
				});
		if (Timing != nullptr)
		{
			return FMath::Clamp(
				Timing->TriggerTime,
				0.0f,
				GetMotionDuration());
		}
	}

	return 0.0f;
}

float SUOURewardCueTimeline::GetPresentationCloseTime(int32 CueIndex) const
{
	const UUOURewardCollectionMotionComponent* Motion = MotionComponent.Get();
	const UUOURewardFeedbackComponent* Feedback = FeedbackComponent.Get();
	if (Motion == nullptr
		|| Feedback == nullptr
		|| !Feedback->GetCueRequests().IsValidIndex(CueIndex))
	{
		return GetMotionDuration();
	}

	const FUOURewardPresentationCue& Cue = Feedback->GetCueRequests()[CueIndex];
	if (Cue.RequestId.IsValid())
	{
		const FUOURewardMotionCueTiming* Timing =
			Motion->GetCueTimelineForEditor().FindByPredicate(
				[&Cue](const FUOURewardMotionCueTiming& Candidate)
				{
					return Candidate.RequestId == Cue.RequestId;
				});
		if (Timing != nullptr && Timing->PresentationCloseTime >= 0.0f)
		{
			return FMath::Clamp(
				Timing->PresentationCloseTime,
				GetCueTime(CueIndex),
				GetMotionDuration());
		}
	}

	return GetMotionDuration();
}

float SUOURewardCueTimeline::TimeToLocalX(float Time, float Width) const
{
	using namespace UOURewardCueTimelinePrivate;
	const float TrackWidth = FMath::Max(1.0f, Width - LabelWidth - RightPadding);
	const float Duration = GetMotionDuration();
	const float Alpha = Duration > KINDA_SMALL_NUMBER
		? FMath::Clamp(Time / Duration, 0.0f, 1.0f)
		: 0.0f;
	return LabelWidth + TrackWidth * Alpha;
}

float SUOURewardCueTimeline::LocalXToTime(float LocalX, float Width) const
{
	using namespace UOURewardCueTimelinePrivate;
	const float TrackWidth = FMath::Max(1.0f, Width - LabelWidth - RightPadding);
	const float Alpha = FMath::Clamp((LocalX - LabelWidth) / TrackWidth, 0.0f, 1.0f);
	return GetMotionDuration() * Alpha;
}

float SUOURewardCueTimeline::GetLaneY(int32 CueIndex) const
{
	return UOURewardCueTimelinePrivate::FirstLaneY
		+ CueIndex * UOURewardCueTimelinePrivate::LaneHeight;
}

int32 SUOURewardCueTimeline::FindMarkerAt(
	const FVector2D& LocalPosition,
	float Width,
	bool& bOutPresentationCloseMarker) const
{
	bOutPresentationCloseMarker = false;
	const UUOURewardFeedbackComponent* Feedback = FeedbackComponent.Get();
	if (Feedback == nullptr)
	{
		return INDEX_NONE;
	}

	for (int32 CueIndex = 0;
		CueIndex < Feedback->GetCueRequests().Num();
		++CueIndex)
	{
		const FUOURewardPresentationCue& Cue =
			Feedback->GetCueRequests()[CueIndex];
		if (Cue.Channel == EUOURewardMotionCueChannel::Presentation)
		{
			const float CloseMarkerX = TimeToLocalX(
				GetPresentationCloseTime(CueIndex),
				Width);
			if (FMath::Abs(LocalPosition.X - CloseMarkerX) <= 9.0f
				&& FMath::Abs(LocalPosition.Y - GetLaneY(CueIndex)) <= 12.0f)
			{
				bOutPresentationCloseMarker = true;
				return CueIndex;
			}
		}

		const float MarkerX = TimeToLocalX(GetCueTime(CueIndex), Width);
		if (FMath::Abs(LocalPosition.X - MarkerX) <= 9.0f
			&& FMath::Abs(LocalPosition.Y - GetLaneY(CueIndex)) <= 12.0f)
		{
			return CueIndex;
		}
	}

	return INDEX_NONE;
}

FText SUOURewardCueTimeline::GetCueLabel(int32 CueIndex) const
{
	const UUOURewardFeedbackComponent* Feedback = FeedbackComponent.Get();
	if (Feedback == nullptr
		|| !Feedback->GetCueRequests().IsValidIndex(CueIndex))
	{
		return FText::GetEmpty();
	}

	const FUOURewardPresentationCue& Cue = Feedback->GetCueRequests()[CueIndex];
	if (Cue.Channel == EUOURewardMotionCueChannel::Feedback)
	{
		const UEnum* ActionEnum = StaticEnum<EUOURewardFeedbackCueAction>();
		return ActionEnum != nullptr
			? ActionEnum->GetDisplayNameTextByValue(
				static_cast<int64>(Cue.FeedbackAction))
			: LOCTEXT("UnknownFeedbackAction", "Feedback Action");
	}

	return Cue.GetPresentationKey().IsNone()
		? LOCTEXT("UnsetPresentation", "Presentation (미설정)")
		: FText::FromName(Cue.GetPresentationKey());
}

bool SUOURewardCueTimeline::BeginMarkerDrag(
	int32 CueIndex,
	bool bPresentationCloseMarker)
{
	UUOURewardCollectionMotionComponent* Motion = MotionComponent.Get();
	UUOURewardFeedbackComponent* Feedback = FeedbackComponent.Get();
	if (Motion == nullptr
		|| Feedback == nullptr
		|| !Feedback->GetCueRequests().IsValidIndex(CueIndex))
	{
		return false;
	}

	const FUOURewardPresentationCue& SelectedCue =
		Feedback->GetCueRequests()[CueIndex];
	const bool bCanDragCloseMarker =
		bPresentationCloseMarker
		&& SelectedCue.Channel == EUOURewardMotionCueChannel::Presentation;
	const float CurrentTime = bCanDragCloseMarker
		? GetPresentationCloseTime(CueIndex)
		: GetCueTime(CueIndex);
	ActiveTransaction = MakeUnique<FScopedTransaction>(
		bCanDragCloseMarker
			? LOCTEXT(
				"MovePresentationCloseMarkerTransaction",
				"Move Reward Presentation Outro Marker")
			: LOCTEXT("MoveCueMarkerTransaction", "Move Reward Cue Marker"));
	Motion->Modify();
	Feedback->Modify();

	TArray<FUOURewardPresentationCue>& CueRequests =
		Feedback->GetMutableCueRequestsForEditor();
	FUOURewardPresentationCue& Cue = CueRequests[CueIndex];
	bool bRequestIdDuplicated = false;
	for (int32 OtherCueIndex = 0;
		OtherCueIndex < CueRequests.Num();
		++OtherCueIndex)
	{
		if (OtherCueIndex != CueIndex
			&& Cue.RequestId.IsValid()
			&& CueRequests[OtherCueIndex].RequestId == Cue.RequestId)
		{
			bRequestIdDuplicated = true;
			break;
		}
	}
	if (!Cue.RequestId.IsValid() || bRequestIdDuplicated)
	{
		Cue.RequestId = FGuid::NewGuid();
	}

	if (bCanDragCloseMarker)
	{
		Motion->SetPresentationCloseTimeForEditor(Cue.RequestId, CurrentTime);
	}
	else
	{
		Motion->SetCueTriggerTimeForEditor(Cue.RequestId, CurrentTime);
	}
	DraggedCueIndex = CueIndex;
	bDraggingPresentationCloseMarker = bCanDragCloseMarker;
	Invalidate(EInvalidateWidgetReason::Paint);
	return true;
}

void SUOURewardCueTimeline::UpdateDraggedMarker(float LocalX, float Width)
{
	UUOURewardCollectionMotionComponent* Motion = MotionComponent.Get();
	UUOURewardFeedbackComponent* Feedback = FeedbackComponent.Get();
	if (Motion == nullptr
		|| Feedback == nullptr
		|| !Feedback->GetCueRequests().IsValidIndex(DraggedCueIndex))
	{
		return;
	}

	const FGuid RequestId =
		Feedback->GetCueRequests()[DraggedCueIndex].RequestId;
	if (bDraggingPresentationCloseMarker)
	{
		Motion->SetPresentationCloseTimeForEditor(
			RequestId,
			LocalXToTime(LocalX, Width));
	}
	else
	{
		const FUOURewardPresentationCue& Cue =
			Feedback->GetCueRequests()[DraggedCueIndex];
		const float RequestedTime = LocalXToTime(LocalX, Width);
		const float ClampedTime =
			Cue.Channel == EUOURewardMotionCueChannel::Presentation
				? FMath::Min(
					RequestedTime,
					GetPresentationCloseTime(DraggedCueIndex))
				: RequestedTime;
		Motion->SetCueTriggerTimeForEditor(
			RequestId,
			ClampedTime);
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SUOURewardCueTimeline::EndMarkerDrag()
{
	if (DraggedCueIndex == INDEX_NONE)
	{
		return;
	}

	if (UUOURewardCollectionMotionComponent* Motion = MotionComponent.Get())
	{
		Motion->MarkPackageDirty();
	}
	if (UUOURewardFeedbackComponent* Feedback = FeedbackComponent.Get())
	{
		Feedback->MarkPackageDirty();
	}

	DraggedCueIndex = INDEX_NONE;
	bDraggingPresentationCloseMarker = false;
	ActiveTransaction.Reset();
	Invalidate(EInvalidateWidgetReason::Paint);
}

#undef LOCTEXT_NAMESPACE
