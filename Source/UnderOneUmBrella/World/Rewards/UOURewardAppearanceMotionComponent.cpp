// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Rewards/UOURewardAppearanceMotionComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"

namespace
{
	FVector ConvertAppearanceWorldLocationToTargetSpace(
		const USceneComponent* TargetComponent,
		const FVector& WorldLocation)
	{
		const USceneComponent* AttachParent = TargetComponent != nullptr
			? TargetComponent->GetAttachParent()
			: nullptr;
		return AttachParent != nullptr
			? AttachParent->GetComponentTransform().InverseTransformPosition(WorldLocation)
			: WorldLocation;
	}

	FQuat ConvertAppearanceWorldRotationToTargetSpace(
		const USceneComponent* TargetComponent,
		const FQuat& WorldRotation)
	{
		const USceneComponent* AttachParent = TargetComponent != nullptr
			? TargetComponent->GetAttachParent()
			: nullptr;
		return AttachParent != nullptr
			? AttachParent->GetComponentQuat().Inverse() * WorldRotation
			: WorldRotation;
	}
}

UUOURewardAppearanceMotionComponent::UUOURewardAppearanceMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UUOURewardAppearanceMotionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bMotionPlaying)
	{
		return;
	}

	if (!MotionTarget.IsValid() || !ActiveMotionPath.IsValid())
	{
		FinishAppearanceMotion();
		return;
	}

	ElapsedTime += DeltaTime;
	const float SafeDuration = FMath::Max(MotionDuration, KINDA_SMALL_NUMBER);
	const float NormalizedTime = FMath::Clamp(ElapsedTime / SafeDuration, 0.0f, 1.0f);
	ApplyMotion(NormalizedTime);
	BroadcastPassedCues(FMath::Min(ElapsedTime, MotionDuration));
	if (NormalizedTime >= 1.0f)
	{
		FinishAppearanceMotion();
	}
}

void UUOURewardAppearanceMotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAppearanceMotion(false);
	Super::EndPlay(EndPlayReason);
}

bool UUOURewardAppearanceMotionComponent::StartAppearanceMotion(
	USceneComponent* TargetComponent,
	USplineComponent* MotionPath,
	const TArray<FUOURewardPresentationCue>& CueRequests)
{
	if (!bMotionEnabled
		|| !IsValid(TargetComponent)
		|| !IsValid(MotionPath)
		|| MotionPath->GetNumberOfSplinePoints() < 2
		|| bMotionPlaying)
	{
		return false;
	}

	MotionTarget = TargetComponent;
	ActiveMotionPath = MotionPath;
	FinalRelativeTransform = TargetComponent->GetRelativeTransform();
	const float SplineLength = MotionPath->GetSplineLength();
	EndPathRelativeLocation = ConvertAppearanceWorldLocationToTargetSpace(
		TargetComponent,
		MotionPath->GetLocationAtDistanceAlongSpline(
			SplineLength,
			ESplineCoordinateSpace::World));
	EndPathRelativeRotation = ConvertAppearanceWorldRotationToTargetSpace(
		TargetComponent,
		MotionPath->GetQuaternionAtDistanceAlongSpline(
			SplineLength,
			ESplineCoordinateSpace::World));
	ElapsedTime = 0.0f;
	bMotionPlaying = true;
	ActiveCueRequests = CueRequests;
	BuildCueSchedule();

	if (MotionDuration <= KINDA_SMALL_NUMBER)
	{
		ApplyMotion(1.0f);
		BroadcastPassedCues(0.0f);
		FinishAppearanceMotion();
		return true;
	}

	SetComponentTickEnabled(true);
	ApplyMotion(0.0f);
	BroadcastPassedCues(0.0f);
	return true;
}

#if WITH_EDITOR
bool UUOURewardAppearanceMotionComponent::SynchronizeCueTimelineForEditor(
	const TArray<FUOURewardPresentationCue>& CueRequests)
{
	TMap<FGuid, EUOURewardMotionCueChannel> ActiveCueChannels;
	for (const FUOURewardPresentationCue& CueRequest : CueRequests)
	{
		if (CueRequest.RequestId.IsValid()
			&& !ActiveCueChannels.Contains(CueRequest.RequestId))
		{
			ActiveCueChannels.Add(CueRequest.RequestId, CueRequest.Channel);
		}
	}

	const float SafeMotionDuration = FMath::IsFinite(MotionDuration)
		? FMath::Max(0.0f, MotionDuration)
		: 0.0f;
	TSet<FGuid> AddedRequestIds;
	TArray<FUOURewardMotionCueTiming> SynchronizedTimeline;
	for (const FUOURewardMotionCueTiming& ExistingTiming : CueTimeline)
	{
		const EUOURewardMotionCueChannel* CueChannel =
			ActiveCueChannels.Find(ExistingTiming.RequestId);
		if (CueChannel == nullptr || AddedRequestIds.Contains(ExistingTiming.RequestId))
		{
			continue;
		}

		AddedRequestIds.Add(ExistingTiming.RequestId);
		FUOURewardMotionCueTiming& Timing =
			SynchronizedTimeline.Add_GetRef(ExistingTiming);
		Timing.TriggerTime = FMath::IsFinite(Timing.TriggerTime)
			? FMath::Clamp(Timing.TriggerTime, 0.0f, SafeMotionDuration)
			: 0.0f;
		if (*CueChannel == EUOURewardMotionCueChannel::Feedback
			|| !FMath::IsFinite(Timing.PresentationCloseTime)
			|| Timing.PresentationCloseTime < 0.0f)
		{
			Timing.PresentationCloseTime = -1.0f;
		}
		else
		{
			Timing.PresentationCloseTime = FMath::Clamp(
				Timing.PresentationCloseTime,
				Timing.TriggerTime,
				SafeMotionDuration);
		}
	}

	bool bChanged = CueTimeline.Num() != SynchronizedTimeline.Num();
	if (!bChanged)
	{
		for (int32 Index = 0; Index < CueTimeline.Num(); ++Index)
		{
			const FUOURewardMotionCueTiming& Existing = CueTimeline[Index];
			const FUOURewardMotionCueTiming& Synchronized = SynchronizedTimeline[Index];
			if (Existing.RequestId != Synchronized.RequestId
				|| Existing.TriggerTime != Synchronized.TriggerTime
				|| Existing.PresentationCloseTime != Synchronized.PresentationCloseTime)
			{
				bChanged = true;
				break;
			}
		}
	}
	if (!bChanged)
	{
		return false;
	}

	Modify();
	CueTimeline = MoveTemp(SynchronizedTimeline);
	MarkPackageDirty();
	return true;
}

void UUOURewardAppearanceMotionComponent::SetCueTriggerTimeForEditor(
	const FGuid& RequestId,
	float TriggerTime)
{
	if (!RequestId.IsValid())
	{
		return;
	}
	FUOURewardMotionCueTiming* Timing = CueTimeline.FindByPredicate(
		[&RequestId](const FUOURewardMotionCueTiming& Candidate)
		{
			return Candidate.RequestId == RequestId;
		});
	if (Timing == nullptr)
	{
		Timing = &CueTimeline.AddDefaulted_GetRef();
		Timing->RequestId = RequestId;
	}
	Timing->TriggerTime = FMath::Clamp(
		TriggerTime,
		0.0f,
		FMath::Max(0.0f, MotionDuration));
}

void UUOURewardAppearanceMotionComponent::SetPresentationCloseTimeForEditor(
	const FGuid& RequestId,
	float CloseTime)
{
	if (!RequestId.IsValid())
	{
		return;
	}
	FUOURewardMotionCueTiming* Timing = CueTimeline.FindByPredicate(
		[&RequestId](const FUOURewardMotionCueTiming& Candidate)
		{
			return Candidate.RequestId == RequestId;
		});
	if (Timing == nullptr)
	{
		Timing = &CueTimeline.AddDefaulted_GetRef();
		Timing->RequestId = RequestId;
	}
	const float SafeDuration = FMath::Max(0.0f, MotionDuration);
	Timing->PresentationCloseTime = FMath::Clamp(
		CloseTime,
		FMath::Clamp(Timing->TriggerTime, 0.0f, SafeDuration),
		SafeDuration);
}
#endif

void UUOURewardAppearanceMotionComponent::StopAppearanceMotion(bool bRestoreFinalTransform)
{
	if (bRestoreFinalTransform && MotionTarget.IsValid())
	{
		MotionTarget->SetRelativeTransform(FinalRelativeTransform);
	}

	ResetRuntimeState();
}

bool UUOURewardAppearanceMotionComponent::IsAppearanceMotionPlaying() const
{
	return bMotionPlaying;
}

void UUOURewardAppearanceMotionComponent::ApplyMotion(float NormalizedTime)
{
	USceneComponent* TargetComponent = MotionTarget.Get();
	USplineComponent* MotionPath = ActiveMotionPath.Get();
	if (TargetComponent == nullptr || MotionPath == nullptr)
	{
		return;
	}

	const float EasedTime = FMath::InterpEaseInOut(
		0.0f,
		1.0f,
		FMath::Clamp(NormalizedTime, 0.0f, 1.0f),
		FMath::Max(1.0f, EaseExponent));
	const float DistanceAlongSpline = MotionPath->GetSplineLength() * EasedTime;
	const FVector PathRelativeLocation = ConvertAppearanceWorldLocationToTargetSpace(
		TargetComponent,
		MotionPath->GetLocationAtDistanceAlongSpline(
			DistanceAlongSpline,
			ESplineCoordinateSpace::World));
	const FVector RelativeLocation =
		FinalRelativeTransform.GetLocation()
		+ (PathRelativeLocation - EndPathRelativeLocation);

	FQuat RelativeRotation = FinalRelativeTransform.GetRotation();
	if (bFollowSplineRotation)
	{
		const FQuat PathRelativeRotation = ConvertAppearanceWorldRotationToTargetSpace(
			TargetComponent,
			MotionPath->GetQuaternionAtDistanceAlongSpline(
				DistanceAlongSpline,
				ESplineCoordinateSpace::World));
		const FQuat SplineRotationDelta =
			PathRelativeRotation * EndPathRelativeRotation.Inverse();
		RelativeRotation = SplineRotationDelta * RelativeRotation;
	}

	const float RemainingRotationAlpha = 1.0f - EasedTime;
	const FRotator CurrentRotationOffset(
		StartRotationOffset.Pitch * RemainingRotationAlpha,
		StartRotationOffset.Yaw * RemainingRotationAlpha,
		StartRotationOffset.Roll * RemainingRotationAlpha);
	RelativeRotation *= CurrentRotationOffset.Quaternion();
	RelativeRotation.Normalize();

	const float ScaleMultiplier = FMath::Lerp(
		FMath::Max(0.0f, StartScaleMultiplier),
		1.0f,
		EasedTime);
	const FVector RelativeScale =
		FinalRelativeTransform.GetScale3D() * ScaleMultiplier;

	TargetComponent->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
	TargetComponent->SetRelativeScale3D(RelativeScale);
}

void UUOURewardAppearanceMotionComponent::BuildCueSchedule()
{
	ActiveCueTimeline.Reset();
	NextCueIndex = 0;
	const float SafeDuration = FMath::Max(0.0f, MotionDuration);
	for (int32 CueIndex = 0; CueIndex < ActiveCueRequests.Num(); ++CueIndex)
	{
		const FUOURewardPresentationCue& Cue = ActiveCueRequests[CueIndex];
		const FUOURewardMotionCueTiming* ConfiguredTiming = Cue.RequestId.IsValid()
			? CueTimeline.FindByPredicate(
				[&Cue](const FUOURewardMotionCueTiming& Timing)
				{
					return Timing.RequestId == Cue.RequestId;
				})
			: nullptr;
		const float ShowTime = FMath::Clamp(
			ConfiguredTiming != nullptr ? ConfiguredTiming->TriggerTime : 0.0f,
			0.0f,
			SafeDuration);
		FActiveCueTiming& ShowTiming = ActiveCueTimeline.AddDefaulted_GetRef();
		ShowTiming.CueIndex = CueIndex;
		ShowTiming.TriggerTime = ShowTime;

		if (Cue.Channel == EUOURewardMotionCueChannel::Presentation)
		{
			const float CloseTime = ConfiguredTiming != nullptr
				&& ConfiguredTiming->PresentationCloseTime >= 0.0f
				? ConfiguredTiming->PresentationCloseTime
				: SafeDuration;
			FActiveCueTiming& CloseTiming = ActiveCueTimeline.AddDefaulted_GetRef();
			CloseTiming.CueIndex = CueIndex;
			CloseTiming.TriggerTime = FMath::Clamp(CloseTime, ShowTime, SafeDuration);
			CloseTiming.bPresentationClose = true;
		}
	}

	ActiveCueTimeline.Sort(
		[](const FActiveCueTiming& Left, const FActiveCueTiming& Right)
		{
			if (!FMath::IsNearlyEqual(Left.TriggerTime, Right.TriggerTime))
			{
				return Left.TriggerTime < Right.TriggerTime;
			}
			if (Left.CueIndex != Right.CueIndex)
			{
				return Left.CueIndex < Right.CueIndex;
			}
			return !Left.bPresentationClose && Right.bPresentationClose;
		});
}

void UUOURewardAppearanceMotionComponent::BroadcastPassedCues(float CurrentTime)
{
	while (ActiveCueTimeline.IsValidIndex(NextCueIndex))
	{
		const FActiveCueTiming& Timing = ActiveCueTimeline[NextCueIndex];
		if (!ActiveCueRequests.IsValidIndex(Timing.CueIndex))
		{
			++NextCueIndex;
			continue;
		}
		if (Timing.TriggerTime > FMath::Max(0.0f, CurrentTime) + KINDA_SMALL_NUMBER)
		{
			break;
		}

		++NextCueIndex;
		const FUOURewardPresentationCue& Cue = ActiveCueRequests[Timing.CueIndex];
		if (Cue.Channel == EUOURewardMotionCueChannel::Feedback
			|| (Cue.PresentationRow.DataTable != nullptr
				&& !Cue.GetPresentationKey().IsNone()))
		{
			FUOURewardPresentationCue CueEvent = Cue;
			CueEvent.PresentationPhase = Timing.bPresentationClose
				? EUOURewardPresentationCuePhase::Close
				: EUOURewardPresentationCuePhase::Show;
			OnAppearanceMotionCue.Broadcast(CueEvent);
		}
	}
}

void UUOURewardAppearanceMotionComponent::FinishAppearanceMotion()
{
	if (!bMotionPlaying)
	{
		return;
	}

	if (MotionTarget.IsValid())
	{
		MotionTarget->SetRelativeTransform(FinalRelativeTransform);
	}
	ResetRuntimeState();
	OnAppearanceMotionFinished.Broadcast();
}

void UUOURewardAppearanceMotionComponent::ResetRuntimeState()
{
	bMotionPlaying = false;
	ElapsedTime = 0.0f;
	MotionTarget.Reset();
	ActiveMotionPath.Reset();
	ActiveCueRequests.Reset();
	ActiveCueTimeline.Reset();
	NextCueIndex = 0;
	SetComponentTickEnabled(false);
}
