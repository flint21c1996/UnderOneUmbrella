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
		FinishMotion();
		return;
	}

	ElapsedTime += DeltaTime;
	const float PhaseDuration = GetMotionDuration(ActiveMotionPhase);
	const float SafeDuration = FMath::Max(PhaseDuration, KINDA_SMALL_NUMBER);
	const float NormalizedTime = FMath::Clamp(ElapsedTime / SafeDuration, 0.0f, 1.0f);
	ApplyMotion(NormalizedTime);
	BroadcastPassedCues(FMath::Min(ElapsedTime, PhaseDuration));

	if (NormalizedTime >= 1.0f)
	{
		FinishMotion();
	}
}

void UUOURewardCollectionMotionComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ResetRuntimeState();
	Super::EndPlay(EndPlayReason);
}

bool UUOURewardCollectionMotionComponent::StartAppearanceMotion(
	USceneComponent* TargetComponent,
	USplineComponent* MotionPath,
	const TArray<FUOURewardPresentationCue>& CueRequests)
{
	return StartMotion(
		EUOURewardMotionPhase::Appearance,
		TargetComponent,
		MotionPath,
		CueRequests);
}

void UUOURewardCollectionMotionComponent::StopAppearanceMotion(
	bool bRestoreFinalTransform)
{
	if (!bMotionPlaying || ActiveMotionPhase != EUOURewardMotionPhase::Appearance)
	{
		return;
	}

	if (bRestoreFinalTransform && MotionTarget.IsValid())
	{
		MotionTarget->SetRelativeTransform(ReferenceRelativeTransform);
	}
	ResetRuntimeState();
}

bool UUOURewardCollectionMotionComponent::IsAppearanceMotionPlaying() const
{
	return bMotionPlaying
		&& ActiveMotionPhase == EUOURewardMotionPhase::Appearance;
}

bool UUOURewardCollectionMotionComponent::StartCollectionMotion(
	USceneComponent* TargetComponent,
	USplineComponent* MotionPath,
	const TArray<FUOURewardPresentationCue>& CueRequests)
{
	return StartMotion(
		EUOURewardMotionPhase::Collection,
		TargetComponent,
		MotionPath,
		CueRequests);
}

bool UUOURewardCollectionMotionComponent::StartMotion(
	EUOURewardMotionPhase Phase,
	USceneComponent* TargetComponent,
	USplineComponent* MotionPath,
	const TArray<FUOURewardPresentationCue>& CueRequests)
{
	if (!IsMotionEnabled(Phase)
		|| !IsValid(TargetComponent)
		|| !IsValid(MotionPath)
		|| MotionPath->GetNumberOfSplinePoints() < 2
		|| bMotionPlaying)
	{
		return false;
	}

	ActiveMotionPhase = Phase;
	MotionTarget = TargetComponent;
	ActiveMotionPath = MotionPath;
	ReferenceRelativeTransform = TargetComponent->GetRelativeTransform();
	const float AnchorDistance = Phase == EUOURewardMotionPhase::Appearance
		? MotionPath->GetSplineLength()
		: 0.0f;
	AnchorPathRelativeLocation = ConvertWorldLocationToTargetSpace(
		TargetComponent,
		MotionPath->GetLocationAtDistanceAlongSpline(
			AnchorDistance,
			ESplineCoordinateSpace::World));
	AnchorPathRelativeRotation = ConvertWorldRotationToTargetSpace(
		TargetComponent,
		MotionPath->GetQuaternionAtDistanceAlongSpline(
			AnchorDistance,
			ESplineCoordinateSpace::World));
	ActiveCueRequests = CueRequests;
	ElapsedTime = 0.0f;
	bMotionPlaying = true;
	BuildCueSchedule();

	const float PhaseDuration = GetMotionDuration(ActiveMotionPhase);
	if (PhaseDuration <= KINDA_SMALL_NUMBER)
	{
		ApplyMotion(1.0f);
		BroadcastPassedCues(0.0f);
		FinishMotion();
		return true;
	}

	SetComponentTickEnabled(true);
	ApplyMotion(0.0f);
	BroadcastPassedCues(0.0f);
	return true;
}

bool UUOURewardCollectionMotionComponent::IsMotionEnabled(
	EUOURewardMotionPhase Phase) const
{
	return Phase == EUOURewardMotionPhase::Appearance
		? bAppearanceMotionEnabled
		: bMotionEnabled;
}

float UUOURewardCollectionMotionComponent::GetMotionDuration(
	EUOURewardMotionPhase Phase) const
{
	return Phase == EUOURewardMotionPhase::Appearance
		? AppearanceMotionDuration
		: MotionDuration;
}

float UUOURewardCollectionMotionComponent::GetEaseExponent(
	EUOURewardMotionPhase Phase) const
{
	return Phase == EUOURewardMotionPhase::Appearance
		? AppearanceEaseExponent
		: EaseExponent;
}

bool UUOURewardCollectionMotionComponent::ShouldFollowSplineRotation(
	EUOURewardMotionPhase Phase) const
{
	return Phase == EUOURewardMotionPhase::Appearance
		? bFollowAppearanceSplineRotation
		: bFollowSplineRotation;
}

const TArray<FUOURewardMotionCueTiming>&
UUOURewardCollectionMotionComponent::GetCueTimeline(
	EUOURewardMotionPhase Phase) const
{
	return Phase == EUOURewardMotionPhase::Appearance
		? AppearanceCueTimeline
		: CueTimeline;
}

TArray<FUOURewardMotionCueTiming>&
UUOURewardCollectionMotionComponent::GetMutableCueTimeline(
	EUOURewardMotionPhase Phase)
{
	return Phase == EUOURewardMotionPhase::Appearance
		? AppearanceCueTimeline
		: CueTimeline;
}

#if WITH_EDITOR
float UUOURewardCollectionMotionComponent::GetMotionDurationForEditor(
	EUOURewardMotionPhase Phase) const
{
	return GetMotionDuration(Phase);
}

const TArray<FUOURewardMotionCueTiming>&
UUOURewardCollectionMotionComponent::GetCueTimelineForEditor(
	EUOURewardMotionPhase Phase) const
{
	return GetCueTimeline(Phase);
}

bool UUOURewardCollectionMotionComponent::SynchronizeCueTimelineForEditor(
	EUOURewardMotionPhase Phase,
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

	const float PhaseDuration = GetMotionDuration(Phase);
	const float SafeMotionDuration = FMath::IsFinite(PhaseDuration)
		? FMath::Max(0.0f, PhaseDuration)
		: 0.0f;
	TArray<FUOURewardMotionCueTiming>& PhaseCueTimeline =
		GetMutableCueTimeline(Phase);
	TSet<FGuid> AddedRequestIds;
	TArray<FUOURewardMotionCueTiming> SynchronizedTimeline;
	SynchronizedTimeline.Reserve(
		FMath::Min(PhaseCueTimeline.Num(), ActiveCueChannels.Num()));

	for (const FUOURewardMotionCueTiming& ExistingTiming : PhaseCueTimeline)
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

	bool bChanged = PhaseCueTimeline.Num() != SynchronizedTimeline.Num();
	if (!bChanged)
	{
		for (int32 TimingIndex = 0;
			TimingIndex < PhaseCueTimeline.Num();
			++TimingIndex)
		{
			const FUOURewardMotionCueTiming& ExistingTiming =
				PhaseCueTimeline[TimingIndex];
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
	PhaseCueTimeline = MoveTemp(SynchronizedTimeline);
	MarkPackageDirty();
	return true;
}

void UUOURewardCollectionMotionComponent::SetCueTriggerTimeForEditor(
	EUOURewardMotionPhase Phase,
	const FGuid& RequestId,
	float TriggerTime)
{
	if (!RequestId.IsValid())
	{
		return;
	}

	TArray<FUOURewardMotionCueTiming>& PhaseCueTimeline =
		GetMutableCueTimeline(Phase);
	FUOURewardMotionCueTiming* Timing = PhaseCueTimeline.FindByPredicate(
		[&RequestId](const FUOURewardMotionCueTiming& Candidate)
		{
			return Candidate.RequestId == RequestId;
		});
	if (Timing == nullptr)
	{
		Timing = &PhaseCueTimeline.AddDefaulted_GetRef();
		Timing->RequestId = RequestId;
	}

	Timing->TriggerTime = FMath::Clamp(
		TriggerTime,
		0.0f,
		FMath::Max(0.0f, GetMotionDuration(Phase)));
}

void UUOURewardCollectionMotionComponent::SetPresentationCloseTimeForEditor(
	EUOURewardMotionPhase Phase,
	const FGuid& RequestId,
	float CloseTime)
{
	if (!RequestId.IsValid())
	{
		return;
	}

	TArray<FUOURewardMotionCueTiming>& PhaseCueTimeline =
		GetMutableCueTimeline(Phase);
	FUOURewardMotionCueTiming* Timing = PhaseCueTimeline.FindByPredicate(
		[&RequestId](const FUOURewardMotionCueTiming& Candidate)
		{
			return Candidate.RequestId == RequestId;
		});
	if (Timing == nullptr)
	{
		Timing = &PhaseCueTimeline.AddDefaulted_GetRef();
		Timing->RequestId = RequestId;
	}

	const float SafeMotionDuration = FMath::Max(0.0f, GetMotionDuration(Phase));
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
		FMath::Max(1.0f, GetEaseExponent(ActiveMotionPhase)));
	const float DistanceAlongSpline = MotionPath->GetSplineLength() * EasedTime;
	const FVector PathRelativeLocation = ConvertWorldLocationToTargetSpace(
		TargetComponent,
		MotionPath->GetLocationAtDistanceAlongSpline(
			DistanceAlongSpline,
			ESplineCoordinateSpace::World));
	const FVector RelativeLocation =
		ReferenceRelativeTransform.GetLocation()
		+ (PathRelativeLocation - AnchorPathRelativeLocation);

	FQuat RelativeRotation = ReferenceRelativeTransform.GetRotation();
	if (ShouldFollowSplineRotation(ActiveMotionPhase))
	{
		const FQuat PathRelativeRotation = ConvertWorldRotationToTargetSpace(
			TargetComponent,
			MotionPath->GetQuaternionAtDistanceAlongSpline(
				DistanceAlongSpline,
				ESplineCoordinateSpace::World));
		const FQuat SplineRotationDelta =
			PathRelativeRotation * AnchorPathRelativeRotation.Inverse();
		RelativeRotation = SplineRotationDelta * RelativeRotation;
	}

	const bool bAppearance =
		ActiveMotionPhase == EUOURewardMotionPhase::Appearance;
	const float RotationAlpha = bAppearance ? 1.0f - EasedTime : EasedTime;
	const FRotator& ConfiguredRotation = bAppearance
		? AppearanceStartRotationOffset
		: AdditionalRotation;
	const FRotator CurrentAdditionalRotation(
		ConfiguredRotation.Pitch * RotationAlpha,
		ConfiguredRotation.Yaw * RotationAlpha,
		ConfiguredRotation.Roll * RotationAlpha);
	RelativeRotation *= CurrentAdditionalRotation.Quaternion();
	RelativeRotation.Normalize();

	const float StartScaleMultiplier = bAppearance
		? FMath::Max(0.0f, AppearanceStartScaleMultiplier)
		: 1.0f;
	const float TargetScaleMultiplier = bAppearance
		? 1.0f
		: FMath::Max(0.0f, EndScaleMultiplier);
	const float ScaleMultiplier = FMath::Lerp(
		StartScaleMultiplier,
		TargetScaleMultiplier,
		EasedTime);
	const FVector RelativeScale =
		ReferenceRelativeTransform.GetScale3D() * ScaleMultiplier;

	TargetComponent->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
	TargetComponent->SetRelativeScale3D(RelativeScale);
}

void UUOURewardCollectionMotionComponent::BuildCueSchedule()
{
	ActiveCueTimeline.Reset();
	NextCueIndex = 0;
	const float SafeMotionDuration = FMath::Max(
		0.0f,
		GetMotionDuration(ActiveMotionPhase));
	const TArray<FUOURewardMotionCueTiming>& PhaseCueTimeline =
		GetCueTimeline(ActiveMotionPhase);

	for (int32 CueIndex = 0; CueIndex < ActiveCueRequests.Num(); ++CueIndex)
	{
		const FUOURewardPresentationCue& Cue = ActiveCueRequests[CueIndex];
		const FUOURewardMotionCueTiming* ConfiguredTiming = Cue.RequestId.IsValid()
			? PhaseCueTimeline.FindByPredicate(
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
			const float ConfiguredCloseTime = ConfiguredTiming != nullptr
				&& ConfiguredTiming->PresentationCloseTime >= 0.0f
				? ConfiguredTiming->PresentationCloseTime
				: SafeMotionDuration;
			FActiveCueTiming& CloseTiming = ActiveCueTimeline.AddDefaulted_GetRef();
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
	const float SafeMotionDuration = FMath::Max(
		0.0f,
		GetMotionDuration(ActiveMotionPhase));
	const float SafeCurrentTime = FMath::Max(0.0f, CurrentTime);

	while (ActiveCueTimeline.IsValidIndex(NextCueIndex))
	{
		const FActiveCueTiming& Timing = ActiveCueTimeline[NextCueIndex];
		if (!ActiveCueRequests.IsValidIndex(Timing.CueIndex))
		{
			++NextCueIndex;
			continue;
		}

		const FUOURewardPresentationCue& Cue = ActiveCueRequests[Timing.CueIndex];
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
			if (ActiveMotionPhase == EUOURewardMotionPhase::Appearance)
			{
				OnAppearanceMotionCue.Broadcast(CueEvent);
			}
			else
			{
				OnCollectionMotionCue.Broadcast(CueEvent);
			}
		}
	}
}

void UUOURewardCollectionMotionComponent::FinishMotion()
{
	if (!bMotionPlaying)
	{
		return;
	}

	const EUOURewardMotionPhase FinishedPhase = ActiveMotionPhase;
	if (FinishedPhase == EUOURewardMotionPhase::Appearance
		&& MotionTarget.IsValid())
	{
		MotionTarget->SetRelativeTransform(ReferenceRelativeTransform);
	}
	ResetRuntimeState();

	if (FinishedPhase == EUOURewardMotionPhase::Appearance)
	{
		OnAppearanceMotionFinished.Broadcast();
	}
	else
	{
		OnCollectionMotionFinished.Broadcast();
	}
}

void UUOURewardCollectionMotionComponent::ResetRuntimeState()
{
	bMotionPlaying = false;
	ElapsedTime = 0.0f;
	MotionTarget.Reset();
	ActiveMotionPath.Reset();
	ActiveCueRequests.Reset();
	ActiveCueTimeline.Reset();
	ActiveMotionPhase = EUOURewardMotionPhase::Collection;
	NextCueIndex = 0;
	SetComponentTickEnabled(false);
}
