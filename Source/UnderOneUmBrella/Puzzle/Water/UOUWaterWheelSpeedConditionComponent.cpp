// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Water/UOUWaterWheelSpeedConditionComponent.h"

#include "GameFramework/Actor.h"
#include "Puzzle/Water/UOUWaterWheelRainConditionComponent.h"

namespace
{
	bool IsSpeedInRange(float Speed, float RangeEndA, float RangeEndB)
	{
		const float LowerSpeed = FMath::Min(RangeEndA, RangeEndB);
		const float UpperSpeed = FMath::Max(RangeEndA, RangeEndB);
		return Speed >= LowerSpeed && Speed <= UpperSpeed;
	}
}

UUOUWaterWheelSpeedConditionComponent::UUOUWaterWheelSpeedConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetAutoActivate(true);
}

void UUOUWaterWheelSpeedConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	Activate(true);
	SetComponentTickEnabled(true);
	RequiredSpeedDegreesPerSecond = FMath::Max(0.0f, RequiredSpeedDegreesPerSecond);
	MaximumSpeedDegreesPerSecond = FMath::Max(0.0f, MaximumSpeedDegreesPerSecond);
	RefreshConditionState();
}

void UUOUWaterWheelSpeedConditionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bRefreshEveryTick)
	{
		RefreshConditionState();
	}
}

FText UUOUWaterWheelSpeedConditionComponent::GetDebugSummaryText_Implementation() const
{
	const UUOUWaterWheelRainConditionComponent* WaterWheelCondition = ResolvedWaterWheelCondition.Get();
	const float CurrentSpeed = WaterWheelCondition != nullptr
		? WaterWheelCondition->CurrentRotationSpeedDegreesPerSecond
		: 0.0f;

	const TArray<FString> DebugLines = {
		FString::Printf(
			TEXT("Water Wheel Speed: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(TEXT("Observed: %s"), *GetNameSafe(WaterWheelCondition)),
		FString::Printf(
			TEXT("Speed: %.1f / %.1f-%.1f deg/s"),
			CurrentSpeed,
			RequiredSpeedDegreesPerSecond,
			MaximumSpeedDegreesPerSecond),
		FString::Printf(
			TEXT("Mode: %s"),
			*StaticEnum<EUOUWaterWheelSpeedConditionMode>()->GetNameStringByValue(
				static_cast<int64>(ConditionMode)))
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOUWaterWheelSpeedConditionComponent::GetPuzzleDebugInputActors_Implementation(
	TArray<AActor*>& OutInputActors) const
{
	if (ResolvedWaterWheelCondition != nullptr && ResolvedWaterWheelCondition->GetOwner() != nullptr)
	{
		OutInputActors.AddUnique(ResolvedWaterWheelCondition->GetOwner());
	}
	else if (ObservedWaterWheelCondition != nullptr && ObservedWaterWheelCondition->GetOwner() != nullptr)
	{
		OutInputActors.AddUnique(ObservedWaterWheelCondition->GetOwner());
	}
}

void UUOUWaterWheelSpeedConditionComponent::RefreshConditionState()
{
	RequiredSpeedDegreesPerSecond = FMath::Max(0.0f, RequiredSpeedDegreesPerSecond);
	MaximumSpeedDegreesPerSecond = FMath::Max(0.0f, MaximumSpeedDegreesPerSecond);
	ResolvedWaterWheelCondition = ResolveWaterWheelCondition();

	const bool bNextSatisfied = ResolvedWaterWheelCondition != nullptr
		&& EvaluateSpeedCondition(ResolvedWaterWheelCondition->CurrentRotationSpeedDegreesPerSecond);
	SetSatisfiedState(bNextSatisfied, true);
}

UUOUWaterWheelRainConditionComponent* UUOUWaterWheelSpeedConditionComponent::ResolveWaterWheelCondition() const
{
	if (ObservedWaterWheelCondition != nullptr)
	{
		return ObservedWaterWheelCondition.Get();
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	if (UActorComponent* ReferencedComponent = WaterWheelConditionReference.GetComponent(Owner))
	{
		if (UUOUWaterWheelRainConditionComponent* ReferencedWaterWheel =
			Cast<UUOUWaterWheelRainConditionComponent>(ReferencedComponent))
		{
			return ReferencedWaterWheel;
		}
	}

	return bAutoFindWaterWheelCondition
		? Owner->FindComponentByClass<UUOUWaterWheelRainConditionComponent>()
		: nullptr;
}

bool UUOUWaterWheelSpeedConditionComponent::EvaluateSpeedCondition(float CurrentSpeedDegreesPerSecond) const
{
	const float SafeRequiredSpeed = FMath::Max(0.0f, RequiredSpeedDegreesPerSecond);
	const float SafeMaximumSpeed = FMath::Max(0.0f, MaximumSpeedDegreesPerSecond);
	const float AbsoluteSpeed = FMath::Abs(CurrentSpeedDegreesPerSecond);
	const bool bPositiveDirection = CurrentSpeedDegreesPerSecond >= 0.0f;
	const bool bNegativeDirection = CurrentSpeedDegreesPerSecond <= 0.0f;

	switch (ConditionMode)
	{
	case EUOUWaterWheelSpeedConditionMode::PositiveSpeedAtLeast:
		return CurrentSpeedDegreesPerSecond >= SafeRequiredSpeed;
	case EUOUWaterWheelSpeedConditionMode::NegativeSpeedAtLeast:
		return CurrentSpeedDegreesPerSecond <= -SafeRequiredSpeed;
	case EUOUWaterWheelSpeedConditionMode::AbsoluteSpeedAtMost:
		return AbsoluteSpeed <= SafeRequiredSpeed;
	case EUOUWaterWheelSpeedConditionMode::PositiveSpeedAtMost:
		return bPositiveDirection && CurrentSpeedDegreesPerSecond <= SafeRequiredSpeed;
	case EUOUWaterWheelSpeedConditionMode::NegativeSpeedAtMost:
		return bNegativeDirection && -CurrentSpeedDegreesPerSecond <= SafeRequiredSpeed;
	case EUOUWaterWheelSpeedConditionMode::AbsoluteSpeedInRange:
		return IsSpeedInRange(AbsoluteSpeed, SafeRequiredSpeed, SafeMaximumSpeed);
	case EUOUWaterWheelSpeedConditionMode::PositiveSpeedInRange:
		return bPositiveDirection && IsSpeedInRange(CurrentSpeedDegreesPerSecond, SafeRequiredSpeed, SafeMaximumSpeed);
	case EUOUWaterWheelSpeedConditionMode::NegativeSpeedInRange:
		return bNegativeDirection && IsSpeedInRange(-CurrentSpeedDegreesPerSecond, SafeRequiredSpeed, SafeMaximumSpeed);
	case EUOUWaterWheelSpeedConditionMode::AbsoluteSpeedAtLeast:
	default:
		return AbsoluteSpeed >= SafeRequiredSpeed;
	}
}
