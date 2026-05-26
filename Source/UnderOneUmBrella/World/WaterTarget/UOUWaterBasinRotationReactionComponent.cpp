// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinRotationReactionComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

UUOUWaterBasinRotationReactionComponent::UUOUWaterBasinRotationReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	ValueSource = EUOUWaterBasinReactionValueSource::WaterFillRatio;
	CompareMode = EUOUWaterBasinReactionCompareMode::GreaterOrEqual;
	ThresholdValue = 0.0f;
}

void UUOUWaterBasinRotationReactionComponent::OnRegister()
{
	Super::OnRegister();

	CacheBaseRotationIfNeeded();
}

void UUOUWaterBasinRotationReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheBaseRotationIfNeeded();
}

void UUOUWaterBasinRotationReactionComponent::ResetRotationReaction(bool bResetObservedValue, bool bApplyBaseRotation)
{
	bHasCachedBaseRotation = false;
	CacheBaseRotationIfNeeded();

	if (bResetObservedValue)
	{
		bHasObservedValue = false;
		LastObservedValue = 0.0f;
	}

	CurrentAppliedAngleDegrees = 0.0f;
	if (bApplyBaseRotation)
	{
		ApplyRotationAngle(CurrentAppliedAngleDegrees);
	}
}

void UUOUWaterBasinRotationReactionComponent::OnWaterBasinReactionStateUpdated_Implementation(const FUOUWaterBasinReactionContext& Context)
{
	CacheBaseRotationIfNeeded();

	const float CurrentValue = Context.CurrentValue;
	if (!bHasObservedValue)
	{
		bHasObservedValue = true;
		LastObservedValue = CurrentValue;

		if (RotationMode == EUOUWaterBasinRotationReactionMode::AbsoluteByValue)
		{
			CurrentAppliedAngleDegrees = ClampRotationAngle(CurrentValue * DegreesPerValueUnit);
			ApplyRotationAngle(CurrentAppliedAngleDegrees);
		}
		return;
	}

	const bool bCanApplyRotation = !bRequireConditionSatisfied || Context.bIsSatisfied;
	if (!bCanApplyRotation)
	{
		LastObservedValue = CurrentValue;
		return;
	}

	if (RotationMode == EUOUWaterBasinRotationReactionMode::AbsoluteByValue)
	{
		CurrentAppliedAngleDegrees = ClampRotationAngle(CurrentValue * DegreesPerValueUnit);
		ApplyRotationAngle(CurrentAppliedAngleDegrees);
		LastObservedValue = CurrentValue;
		return;
	}

	const float DeltaValue = CurrentValue - LastObservedValue;
	LastObservedValue = CurrentValue;
	if (DeltaValue <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	CurrentAppliedAngleDegrees = ClampRotationAngle(CurrentAppliedAngleDegrees + (DeltaValue * DegreesPerValueUnit));
	ApplyRotationAngle(CurrentAppliedAngleDegrees);
}

USceneComponent* UUOUWaterBasinRotationReactionComponent::ResolveRotationTargetComponent() const
{
	if (IsValid(RotationTargetComponent))
	{
		return RotationTargetComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindRotationTargetComponent())
	{
		return FoundComponent;
	}

	return bUseOwnerRootWhenTargetMissing && GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
}

USceneComponent* UUOUWaterBasinRotationReactionComponent::FindRotationTargetComponent() const
{
	const AActor* Owner = GetOwner();
	if (!Owner || RotationTargetComponentName.IsNone())
	{
		return nullptr;
	}

	const FString TargetName = RotationTargetComponentName.ToString();
	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!IsValid(SceneComponent))
		{
			continue;
		}

		if (SceneComponent->GetFName() == RotationTargetComponentName
			|| SceneComponent->ComponentTags.Contains(RotationTargetComponentName)
			|| SceneComponent->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void UUOUWaterBasinRotationReactionComponent::CacheBaseRotationIfNeeded()
{
	if (bHasCachedBaseRotation)
	{
		return;
	}

	USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	if (!TargetComponent)
	{
		return;
	}

	BaseRelativeRotation = TargetComponent->GetRelativeRotation().Quaternion();
	BaseWorldRotation = TargetComponent->GetComponentQuat();
	bHasCachedBaseRotation = true;
}

void UUOUWaterBasinRotationReactionComponent::ApplyRotationAngle(float AngleDegrees)
{
	USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	if (!TargetComponent)
	{
		return;
	}

	const FVector SafeAxis = RotationAxis.GetSafeNormal();
	if (SafeAxis.IsNearlyZero())
	{
		return;
	}

	CacheBaseRotationIfNeeded();

	const FQuat RotationDelta(SafeAxis, FMath::DegreesToRadians(AngleDegrees));
	if (RotationSpace == EUOUWaterBasinRotationReactionSpace::World)
	{
		TargetComponent->SetWorldRotation((RotationDelta * BaseWorldRotation).Rotator());
		return;
	}

	TargetComponent->SetRelativeRotation((BaseRelativeRotation * RotationDelta).Rotator());
}

float UUOUWaterBasinRotationReactionComponent::ClampRotationAngle(float AngleDegrees) const
{
	if (!bClampRotationAngle)
	{
		return AngleDegrees;
	}

	const float MinAngle = FMath::Min(MinRotationAngleDegrees, MaxRotationAngleDegrees);
	const float MaxAngle = FMath::Max(MinRotationAngleDegrees, MaxRotationAngleDegrees);
	return FMath::Clamp(AngleDegrees, MinAngle, MaxAngle);
}
