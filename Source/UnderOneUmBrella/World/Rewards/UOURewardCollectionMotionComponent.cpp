// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Rewards/UOURewardCollectionMotionComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"

namespace
{
	FVector ConvertWorldLocationToTargetSpace(
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

	FQuat ConvertWorldRotationToTargetSpace(
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

UUOURewardCollectionMotionComponent::UUOURewardCollectionMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UUOURewardCollectionMotionComponent::TickComponent(
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
		FinishCollectionMotion();
		return;
	}

	ElapsedTime += DeltaTime;
	const float SafeDuration = FMath::Max(MotionDuration, KINDA_SMALL_NUMBER);
	const float NormalizedTime = FMath::Clamp(ElapsedTime / SafeDuration, 0.0f, 1.0f);
	ApplyMotion(NormalizedTime);
	BroadcastPassedCues(FMath::Min(ElapsedTime, MotionDuration));

	if (NormalizedTime >= 1.0f)
	{
		FinishCollectionMotion();
	}
}

bool UUOURewardCollectionMotionComponent::StartCollectionMotion(
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
	StartRelativeTransform = TargetComponent->GetRelativeTransform();
	StartPathRelativeLocation = ConvertWorldLocationToTargetSpace(
		TargetComponent,
		MotionPath->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World));
	StartPathRelativeRotation = ConvertWorldRotationToTargetSpace(
		TargetComponent,
		MotionPath->GetQuaternionAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World));
	ActiveCueRequests = CueRequests;
	ElapsedTime = 0.0f;
	bMotionPlaying = true;
	BuildCueSchedule();

	if (MotionDuration <= KINDA_SMALL_NUMBER)
	{
		ApplyMotion(1.0f);
		BroadcastPassedCues(0.0f);
		FinishCollectionMotion();
		return true;
	}

	SetComponentTickEnabled(true);
	ApplyMotion(0.0f);
	BroadcastPassedCues(0.0f);
	return true;
}

#if WITH_EDITOR
bool UUOURewardCollectionMotionComponent::SynchronizeCueTimelineForEditor(
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
	SynchronizedTimeline.Reserve(
		FMath::Min(CueTimeline.Num(), ActiveCueChannels.Num()));

	for (const FUOURewardMotionCueTiming& ExistingTiming : CueTimeline)
	{
		const EUOURewardMotionCueChannel* CueChannel =
			ActiveCueChannels.Find(ExistingTiming.RequestId);
		if (CueChannel == nullptr
			|| AddedRequestIds.Contains(ExistingTiming.RequestId))
		{
			continue;
		}

		AddedRequestIds.Add(ExistingTiming.RequestId);
		FUOURewardMotionCueTiming& SynchronizedTiming =
			SynchronizedTimeline.Add_GetRef(ExistingTiming);
		SynchronizedTiming.TriggerTime = FMath::IsFinite(ExistingTiming.TriggerTime)
			? FMath::Clamp(ExistingTiming.TriggerTime, 0.0f, SafeMotionDuration)
			: 0.0f;

		if (*CueChannel == EUOURewardMotionCueChannel::Feedback
			|| !FMath::IsFinite(ExistingTiming.PresentationCloseTime)
			|| ExistingTiming.PresentationCloseTime < 0.0f)
		{
			SynchronizedTiming.PresentationCloseTime = -1.0f;
		}
		else
		{
			SynchronizedTiming.PresentationCloseTime = FMath::Clamp(
				ExistingTiming.PresentationCloseTime,
				SynchronizedTiming.TriggerTime,
				SafeMotionDuration);
		}
	}

	bool bChanged = CueTimeline.Num() != SynchronizedTimeline.Num();
	if (!bChanged)
	{
		for (int32 TimingIndex = 0;
			TimingIndex < CueTimeline.Num();
			++TimingIndex)
		{
			const FUOURewardMotionCueTiming& ExistingTiming =
				CueTimeline[TimingIndex];
			const FUOURewardMotionCueTiming& SynchronizedTiming =
				SynchronizedTimeline[TimingIndex];
			if (ExistingTiming.RequestId != SynchronizedTiming.RequestId
				|| ExistingTiming.TriggerTime != SynchronizedTiming.TriggerTime
				|| ExistingTiming.PresentationCloseTime
					!= SynchronizedTiming.PresentationCloseTime)
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

void UUOURewardCollectionMotionComponent::SetCueTriggerTimeForEditor(
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

void UUOURewardCollectionMotionComponent::SetPresentationCloseTimeForEditor(
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

	const float SafeMotionDuration = FMath::Max(0.0f, MotionDuration);
	const float MinimumCloseTime = FMath::Clamp(
		Timing->TriggerTime,
		0.0f,
		SafeMotionDuration);
	Timing->PresentationCloseTime = FMath::Clamp(
		CloseTime,
		MinimumCloseTime,
		SafeMotionDuration);
}
#endif

void UUOURewardCollectionMotionComponent::ApplyMotion(float NormalizedTime)
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
	const FVector PathRelativeLocation = ConvertWorldLocationToTargetSpace(
		TargetComponent,
		MotionPath->GetLocationAtDistanceAlongSpline(
			DistanceAlongSpline,
			ESplineCoordinateSpace::World));
	const FVector RelativeLocation =
		StartRelativeTransform.GetLocation()
		+ (PathRelativeLocation - StartPathRelativeLocation);

	FQuat RelativeRotation = StartRelativeTransform.GetRotation();
	if (bFollowSplineRotation)
	{
		const FQuat PathRelativeRotation = ConvertWorldRotationToTargetSpace(
			TargetComponent,
			MotionPath->GetQuaternionAtDistanceAlongSpline(
				DistanceAlongSpline,
				ESplineCoordinateSpace::World));
		const FQuat SplineRotationDelta =
			PathRelativeRotation * StartPathRelativeRotation.Inverse();
		RelativeRotation = SplineRotationDelta * RelativeRotation;
	}

	const FRotator CurrentAdditionalRotation(
		AdditionalRotation.Pitch * EasedTime,
		AdditionalRotation.Yaw * EasedTime,
		AdditionalRotation.Roll * EasedTime);
	RelativeRotation *= CurrentAdditionalRotation.Quaternion();
	RelativeRotation.Normalize();

	const float SafeEndScaleMultiplier = FMath::Max(0.0f, EndScaleMultiplier);
	const float ScaleMultiplier = FMath::Lerp(1.0f, SafeEndScaleMultiplier, EasedTime);
	const FVector RelativeScale = StartRelativeTransform.GetScale3D() * ScaleMultiplier;

	TargetComponent->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
	TargetComponent->SetRelativeScale3D(RelativeScale);
}

void UUOURewardCollectionMotionComponent::BuildCueSchedule()
{
	ActiveCueTimeline.Reset();
	NextCueIndex = 0;
	const float SafeMotionDuration = FMath::Max(0.0f, MotionDuration);

	for (int32 CueIndex = 0; CueIndex < ActiveCueRequests.Num(); ++CueIndex)
	{
		const FUOURewardPresentationCue& Cue = ActiveCueRequests[CueIndex];
		const FUOURewardMotionCueTiming* ConfiguredTiming =
			Cue.RequestId.IsValid()
				? CueTimeline.FindByPredicate(
					[&Cue](const FUOURewardMotionCueTiming& Timing)
					{
						return Timing.RequestId == Cue.RequestId;
					})
				: nullptr;

		const float ShowTime = FMath::Clamp(
			ConfiguredTiming != nullptr ? ConfiguredTiming->TriggerTime : 0.0f,
			0.0f,
			SafeMotionDuration);
		FActiveCueTiming& ShowTiming = ActiveCueTimeline.AddDefaulted_GetRef();
		ShowTiming.CueIndex = CueIndex;
		ShowTiming.TriggerTime = ShowTime;

		if (Cue.Channel == EUOURewardMotionCueChannel::Presentation)
		{
			const float ConfiguredCloseTime =
				ConfiguredTiming != nullptr
					&& ConfiguredTiming->PresentationCloseTime >= 0.0f
					? ConfiguredTiming->PresentationCloseTime
					: SafeMotionDuration;
			FActiveCueTiming& CloseTiming =
				ActiveCueTimeline.AddDefaulted_GetRef();
			CloseTiming.CueIndex = CueIndex;
			CloseTiming.TriggerTime = FMath::Clamp(
				ConfiguredCloseTime,
				ShowTime,
				SafeMotionDuration);
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

void UUOURewardCollectionMotionComponent::BroadcastPassedCues(float CurrentTime)
{
	const float SafeMotionDuration = FMath::Max(0.0f, MotionDuration);
	const float SafeCurrentTime = FMath::Max(0.0f, CurrentTime);

	while (ActiveCueTimeline.IsValidIndex(NextCueIndex))
	{
		const FActiveCueTiming& Timing = ActiveCueTimeline[NextCueIndex];
		if (!ActiveCueRequests.IsValidIndex(Timing.CueIndex))
		{
			++NextCueIndex;
			continue;
		}

		const FUOURewardPresentationCue& Cue =
			ActiveCueRequests[Timing.CueIndex];
		const float SafeTriggerTime =
			FMath::Clamp(Timing.TriggerTime, 0.0f, SafeMotionDuration);
		if (SafeTriggerTime > SafeCurrentTime + KINDA_SMALL_NUMBER)
		{
			break;
		}

		++NextCueIndex;
		if (Cue.Channel == EUOURewardMotionCueChannel::Feedback
			|| (Cue.PresentationRow.DataTable != nullptr
				&& !Cue.GetPresentationKey().IsNone()))
		{
			FUOURewardPresentationCue CueEvent = Cue;
			CueEvent.PresentationPhase = Timing.bPresentationClose
				? EUOURewardPresentationCuePhase::Close
				: EUOURewardPresentationCuePhase::Show;
			OnCollectionMotionCue.Broadcast(CueEvent);
		}
	}
}

void UUOURewardCollectionMotionComponent::FinishCollectionMotion()
{
	if (!bMotionPlaying)
	{
		return;
	}

	bMotionPlaying = false;
	ElapsedTime = 0.0f;
	MotionTarget.Reset();
	ActiveMotionPath.Reset();
	ActiveCueRequests.Reset();
	ActiveCueTimeline.Reset();
	NextCueIndex = 0;
	SetComponentTickEnabled(false);

	OnCollectionMotionFinished.Broadcast();
}
