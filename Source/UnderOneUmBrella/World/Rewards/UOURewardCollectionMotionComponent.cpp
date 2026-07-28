// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Rewards/UOURewardCollectionMotionComponent.h"

#include "Components/SceneComponent.h"

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

	if (!MotionTarget.IsValid())
	{
		FinishCollectionMotion();
		return;
	}

	ElapsedTime += DeltaTime;
	const float SafeDuration = FMath::Max(MotionDuration, KINDA_SMALL_NUMBER);
	const float NormalizedTime = FMath::Clamp(ElapsedTime / SafeDuration, 0.0f, 1.0f);
	ApplyMotion(NormalizedTime);

	if (NormalizedTime >= 1.0f)
	{
		FinishCollectionMotion();
	}
}

bool UUOURewardCollectionMotionComponent::StartCollectionMotion(USceneComponent* TargetComponent)
{
	if (!bMotionEnabled || !IsValid(TargetComponent) || bMotionPlaying)
	{
		return false;
	}

	MotionTarget = TargetComponent;
	StartRelativeTransform = TargetComponent->GetRelativeTransform();
	ElapsedTime = 0.0f;
	bMotionPlaying = true;

	if (MotionDuration <= KINDA_SMALL_NUMBER)
	{
		ApplyMotion(1.0f);
		FinishCollectionMotion();
		return true;
	}

	SetComponentTickEnabled(true);
	ApplyMotion(0.0f);
	return true;
}

bool UUOURewardCollectionMotionComponent::IsCollectionMotionPlaying() const
{
	return bMotionPlaying;
}

void UUOURewardCollectionMotionComponent::ApplyMotion(float NormalizedTime)
{
	USceneComponent* TargetComponent = MotionTarget.Get();
	if (TargetComponent == nullptr)
	{
		return;
	}

	const float EasedTime = FMath::InterpEaseInOut(
		0.0f,
		1.0f,
		FMath::Clamp(NormalizedTime, 0.0f, 1.0f),
		FMath::Max(1.0f, EaseExponent));

	FVector RelativeLocation = StartRelativeTransform.GetLocation();
	RelativeLocation += EndLocationOffset * EasedTime;
	RelativeLocation.Z += FMath::Sin(PI * EasedTime) * FMath::Max(0.0f, ArcHeight);

	const FRotator StartRotation = StartRelativeTransform.Rotator();
	const FRotator RelativeRotation = StartRotation + (EndRotationOffset * EasedTime);

	const float SafeEndScaleMultiplier = FMath::Max(0.0f, EndScaleMultiplier);
	const float ScaleMultiplier = FMath::Lerp(1.0f, SafeEndScaleMultiplier, EasedTime);
	const FVector RelativeScale = StartRelativeTransform.GetScale3D() * ScaleMultiplier;

	TargetComponent->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
	TargetComponent->SetRelativeScale3D(RelativeScale);
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
	SetComponentTickEnabled(false);

	OnCollectionMotionFinished.Broadcast();
}
