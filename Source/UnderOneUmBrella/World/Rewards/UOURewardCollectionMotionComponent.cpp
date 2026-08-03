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

		FActiveCueTiming& ActiveTiming = ActiveCueTimeline.AddDefaulted_GetRef();
		ActiveTiming.CueIndex = CueIndex;
		ActiveTiming.TriggerTime = ConfiguredTiming != nullptr
			? ConfiguredTiming->TriggerTime
			: 0.0f;
	}

	ActiveCueTimeline.Sort(
		[](const FActiveCueTiming& Left, const FActiveCueTiming& Right)
		{
			return FMath::IsNearlyEqual(Left.TriggerTime, Right.TriggerTime)
				? Left.CueIndex < Right.CueIndex
				: Left.TriggerTime < Right.TriggerTime;
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
			OnCollectionMotionCue.Broadcast(Cue);
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
