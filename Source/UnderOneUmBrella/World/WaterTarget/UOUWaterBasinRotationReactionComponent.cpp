// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinRotationReactionComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

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
	BindToWaterInputTarget();
}

void UUOUWaterBasinRotationReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromWaterInputTarget();

	Super::EndPlay(EndPlayReason);
}

void UUOUWaterBasinRotationReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateInterpolatedRotation(DeltaTime);
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

	TargetAngleDegrees = 0.0f;
	CurrentAppliedAngleDegrees = 0.0f;
	if (bApplyBaseRotation)
	{
		ApplyRotationAngle(CurrentAppliedAngleDegrees);
	}
}

void UUOUWaterBasinRotationReactionComponent::OnWaterBasinReactionStateUpdated_Implementation(const FUOUWaterBasinReactionContext& Context)
{
	CacheBaseRotationIfNeeded();
	BindToWaterInputTarget();

	const float CurrentValue = Context.CurrentValue;
	if (RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput)
	{
		LastObservedValue = CurrentValue;
		bHasObservedValue = true;
		return;
	}

	if (!bHasObservedValue)
	{
		bHasObservedValue = true;
		LastObservedValue = CurrentValue;

		if (RotationMode == EUOUWaterBasinRotationReactionMode::AbsoluteByValue)
		{
			SetTargetRotationAngle(CurrentValue * DegreesPerValueUnit);
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
		SetTargetRotationAngle(CurrentValue * DegreesPerValueUnit);
		LastObservedValue = CurrentValue;
		return;
	}

	const float DeltaValue = CurrentValue - LastObservedValue;
	LastObservedValue = CurrentValue;
	if (DeltaValue <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	SetTargetRotationAngle(TargetAngleDegrees + (DeltaValue * DegreesPerValueUnit));
}

void UUOUWaterBasinRotationReactionComponent::HandleWaterInputReceived(UUOUWaterBasinTargetComponent* Target, float InputVolume)
{
	if (RotationMode != EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput
		|| Target == nullptr
		|| InputVolume <= 0.0f)
	{
		return;
	}

	const FUOUWaterBasinReactionContext Context = GetLastReactionContext();
	if (bRequireConditionSatisfied && !Context.bIsSatisfied)
	{
		return;
	}

	SetTargetRotationAngle(TargetAngleDegrees + (InputVolume * DegreesPerInputVolume));
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

void UUOUWaterBasinRotationReactionComponent::BindToWaterInputTarget()
{
	UUOUWaterBasinTargetComponent* TargetComponent = nullptr;
	const FUOUWaterBasinReactionContext Context = GetLastReactionContext();
	if (IsValid(Context.WaterBasinTarget))
	{
		TargetComponent = Context.WaterBasinTarget;
	}
	else if (IsValid(Context.WaterTileActor))
	{
		TargetComponent = Context.WaterTileActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
	}
	else if (IsValid(TargetWaterTileActor))
	{
		TargetComponent = TargetWaterTileActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
	}

	if (BoundInputWaterBasinTarget == TargetComponent)
	{
		return;
	}

	UnbindFromWaterInputTarget();

	BoundInputWaterBasinTarget = TargetComponent;
	if (BoundInputWaterBasinTarget)
	{
		BoundInputWaterBasinTarget->OnWaterInputReceived.AddUniqueDynamic(this, &UUOUWaterBasinRotationReactionComponent::HandleWaterInputReceived);
	}
}

void UUOUWaterBasinRotationReactionComponent::UnbindFromWaterInputTarget()
{
	if (BoundInputWaterBasinTarget)
	{
		BoundInputWaterBasinTarget->OnWaterInputReceived.RemoveDynamic(this, &UUOUWaterBasinRotationReactionComponent::HandleWaterInputReceived);
		BoundInputWaterBasinTarget = nullptr;
	}
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

void UUOUWaterBasinRotationReactionComponent::SetTargetRotationAngle(float NewTargetAngleDegrees)
{
	TargetAngleDegrees = ClampRotationAngle(NewTargetAngleDegrees);
	if (bUseRotationInterpolation)
	{
		return;
	}

	CurrentAppliedAngleDegrees = TargetAngleDegrees;
	ApplyRotationAngle(CurrentAppliedAngleDegrees);
}

void UUOUWaterBasinRotationReactionComponent::UpdateInterpolatedRotation(float DeltaTime)
{
	if (!bUseRotationInterpolation)
	{
		return;
	}

	const float Duration = FMath::Max(DeltaTime, 0.0f);
	const float RotationSpeed = FMath::Max(RotationSpeedDegreesPerSecond, 0.0f);
	if (Duration <= 0.0f || RotationSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float NextAngle = FMath::FInterpConstantTo(
		CurrentAppliedAngleDegrees,
		TargetAngleDegrees,
		Duration,
		RotationSpeed);
	if (FMath::IsNearlyEqual(NextAngle, CurrentAppliedAngleDegrees, KINDA_SMALL_NUMBER))
	{
		return;
	}

	CurrentAppliedAngleDegrees = NextAngle;
	ApplyRotationAngle(CurrentAppliedAngleDegrees);
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
