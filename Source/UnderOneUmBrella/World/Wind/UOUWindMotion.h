// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace UOUWindMotion
{
	inline FVector CalculateClampedAdditiveAcceleration(
		const FVector& WindAcceleration,
		const FVector& AdditionalAcceleration,
		float MaximumAcceleration)
	{
		if (MaximumAcceleration <= 0.0f)
		{
			return FVector::ZeroVector;
		}

		return (WindAcceleration + AdditionalAcceleration)
			.GetClampedToMaxSize(MaximumAcceleration);
	}

	inline float CalculateMagnitudeCappedDirectionalAcceleration(
		const FVector& CurrentVelocity,
		const FVector& AccelerationDirection,
		float MaximumSpeed,
		float RequestedAcceleration,
		float DeltaTime)
	{
		const FVector SafeDirection =
			AccelerationDirection.GetSafeNormal();
		const float SafeMaximumSpeed =
			FMath::Max(0.0f, MaximumSpeed);
		const float SafeRequestedAcceleration =
			FMath::Max(0.0f, RequestedAcceleration);
		if (SafeDirection.IsNearlyZero()
			|| SafeMaximumSpeed <= 0.0f
			|| SafeRequestedAcceleration <= 0.0f
			|| DeltaTime <= 0.0f)
		{
			return 0.0f;
		}

		const float CurrentSpeedAlongDirection =
			FVector::DotProduct(CurrentVelocity, SafeDirection);
		const float PerpendicularSpeedSquared =
			FMath::Max(
				0.0f,
				CurrentVelocity.SizeSquared()
					- FMath::Square(CurrentSpeedAlongDirection));
		const float AvailableParallelSpeedSquared =
			FMath::Square(SafeMaximumSpeed)
				- PerpendicularSpeedSquared;
		if (AvailableParallelSpeedSquared <= 0.0f)
		{
			return 0.0f;
		}

		const float MaximumVelocityDelta =
			FMath::Max(
				0.0f,
				FMath::Sqrt(AvailableParallelSpeedSquared)
					- CurrentSpeedAlongDirection);
		return FMath::Min(
			SafeRequestedAcceleration,
			MaximumVelocityDelta
				/ FMath::Max(
					DeltaTime,
					KINDA_SMALL_NUMBER));
	}

	inline FVector CalculateWindTransitionVelocity(
		const FVector& PreviousVelocity,
		const FVector& NewWindDirection,
		float MaximumSpeed,
		float VelocityTransferRatio)
	{
		const FVector SafeNewWindDirection =
			NewWindDirection.GetSafeNormal();
		const float ClampedSpeed =
			FMath::Min(
				PreviousVelocity.Size(),
				FMath::Max(0.0f, MaximumSpeed));
		if (SafeNewWindDirection.IsNearlyZero())
		{
			return PreviousVelocity.GetSafeNormal() * ClampedSpeed;
		}

		const float SafeTransferRatio =
			FMath::Clamp(
				VelocityTransferRatio,
				0.0f,
				1.0f);
		const FVector PreviousDirection =
			PreviousVelocity.GetSafeNormal();
		FVector BlendedDirection =
			FMath::Lerp(
				PreviousDirection,
				SafeNewWindDirection,
				SafeTransferRatio);
		if (!BlendedDirection.Normalize())
		{
			BlendedDirection =
				SafeTransferRatio >= 0.5f
					? SafeNewWindDirection
					: PreviousDirection;
		}

		return BlendedDirection * ClampedSpeed;
	}

	inline FVector CalculateWindEntryVelocity(
		const FVector& PreviousVelocity,
		const FVector& WindDirection,
		float MinimumEntrySpeed,
		float FallingMomentumConversion,
		float InitialVelocityBoost,
		float MaximumEntrySpeed,
		float ExistingVelocityRetention)
	{
		const FVector RetainedVelocity =
			PreviousVelocity
			* FMath::Clamp(ExistingVelocityRetention, 0.0f, 1.0f);
		const FVector SafeWindDirection = WindDirection.GetSafeNormal();
		if (SafeWindDirection.IsNearlyZero())
		{
			return RetainedVelocity;
		}

		const float WindEntrySpeed = FMath::Clamp(
			FMath::Max(
				FMath::Max(0.0f, MinimumEntrySpeed),
				FMath::Abs(PreviousVelocity.Z)
					* FMath::Max(0.0f, FallingMomentumConversion))
				+ FMath::Max(0.0f, InitialVelocityBoost),
			0.0f,
			FMath::Max(0.0f, MaximumEntrySpeed));

		return RetainedVelocity + SafeWindDirection * WindEntrySpeed;
	}
}
