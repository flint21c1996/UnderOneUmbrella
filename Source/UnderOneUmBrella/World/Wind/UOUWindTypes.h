// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUWindTypes.generated.h"

class AActor;

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

	inline float CalculateCappedAcceleration(
		float CurrentSpeedAlongWind,
		float TargetSpeedAlongWind,
		float MaximumAcceleration,
		float DeltaTime)
	{
		const float RemainingSpeed =
			FMath::Max(0.0f, TargetSpeedAlongWind - CurrentSpeedAlongWind);
		if (RemainingSpeed <= 0.0f || MaximumAcceleration <= 0.0f)
		{
			return 0.0f;
		}

		return FMath::Min(
			MaximumAcceleration,
			RemainingSpeed / FMath::Max(DeltaTime, KINDA_SMALL_NUMBER));
	}

	inline float CalculateDirectionalAcceleration(
		bool bLimitSpeed,
		float CurrentSpeedAlongWind,
		float TargetSpeedAlongWind,
		float RequestedAcceleration,
		float DeltaTime)
	{
		const float SafeRequestedAcceleration =
			FMath::Max(0.0f, RequestedAcceleration);
		if (!bLimitSpeed)
		{
			return SafeRequestedAcceleration;
		}

		return CalculateCappedAcceleration(
			CurrentSpeedAlongWind,
			TargetSpeedAlongWind,
			SafeRequestedAcceleration,
			DeltaTime);
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

		const float CurrentSpeedSquared =
			CurrentVelocity.SizeSquared();
		const float CurrentSpeedAlongDirection =
			FVector::DotProduct(
				CurrentVelocity,
				SafeDirection);
		const float Discriminant =
			FMath::Square(CurrentSpeedAlongDirection)
			+ FMath::Square(SafeMaximumSpeed)
			- CurrentSpeedSquared;
		if (Discriminant <= 0.0f)
		{
			return 0.0f;
		}

		const float MaximumVelocityDelta =
			FMath::Max(
				0.0f,
				-CurrentSpeedAlongDirection
					+ FMath::Sqrt(Discriminant));
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
		if (SafeNewWindDirection.IsNearlyZero())
		{
			return PreviousVelocity.GetClampedToMaxSize(
				FMath::Max(0.0f, MaximumSpeed));
		}

		const float SafeTransferRatio =
			FMath::Clamp(
				VelocityTransferRatio,
				0.0f,
				1.0f);
		const float PreviousSpeed = PreviousVelocity.Size();
		const FVector PreviousDirection =
			PreviousVelocity.GetSafeNormal();
		FVector BlendedDirection =
			PreviousDirection * (1.0f - SafeTransferRatio)
			+ SafeNewWindDirection * SafeTransferRatio;
		if (!BlendedDirection.Normalize())
		{
			BlendedDirection =
				SafeTransferRatio >= 0.5f
					? SafeNewWindDirection
					: PreviousDirection;
		}

		return BlendedDirection
			* FMath::Min(
				PreviousSpeed,
				FMath::Max(0.0f, MaximumSpeed));
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

// 주기형 바람의 현재 ON/OFF 상태와 다음 전환까지 남은 시간입니다.
USTRUCT(BlueprintType)
struct FUOUWindPulseRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind|Pulse")
	bool bIsBlowing = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind|Pulse", meta = (Units = "s"))
	float TimeRemaining = 0.0f;

	void Reset(bool bStartWithWind, float OnDuration, float OffDuration)
	{
		bIsBlowing = bStartWithWind;
		TimeRemaining = bIsBlowing
			? FMath::Max(OnDuration, KINDA_SMALL_NUMBER)
			: FMath::Max(OffDuration, KINDA_SMALL_NUMBER);
	}

	// 한 프레임에 여러 구간을 지나더라도 최종 상태와 남은 시간을 안정적으로 계산합니다.
	bool Advance(float DeltaTime, float OnDuration, float OffDuration)
	{
		if (DeltaTime <= 0.0f)
		{
			return false;
		}

		const float SafeOnDuration = FMath::Max(OnDuration, KINDA_SMALL_NUMBER);
		const float SafeOffDuration = FMath::Max(OffDuration, KINDA_SMALL_NUMBER);
		const bool bPreviousBlowing = bIsBlowing;
		TimeRemaining -= DeltaTime;

		constexpr int32 MaxTransitionsPerAdvance = 128;
		int32 TransitionCount = 0;
		while (TimeRemaining <= 0.0f && TransitionCount < MaxTransitionsPerAdvance)
		{
			bIsBlowing = !bIsBlowing;
			TimeRemaining += bIsBlowing ? SafeOnDuration : SafeOffDuration;
			++TransitionCount;
		}

		if (TimeRemaining <= 0.0f)
		{
			Reset(bIsBlowing, SafeOnDuration, SafeOffDuration);
		}

		return bIsBlowing != bPreviousBlowing;
	}
};

// 런타임에 계산된 하나의 직선 바람 구간입니다.
USTRUCT(BlueprintType)
struct FUOUWindPathSegment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector End = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	float Strength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	int32 ReflectionIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<AActor> HitActor = nullptr;

	float GetLength() const
	{
		return FVector::Distance(Start, End);
	}
};

// 바람 수신체 하나에 전달되는 방향과 실제 가속도 정보입니다.
USTRUCT(BlueprintType)
struct FUOUWindExposureData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector ClosestPointOnPath = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind", meta = (Units = "cm/s^2"))
	float Acceleration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind", meta = (Units = "cm/s^2"))
	float MaximumAcceleration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind", meta = (Units = "cm/s"))
	float MaximumSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind", meta = (Units = "cm/s"))
	float MinimumEntrySpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	float FallingMomentumConversion = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind", meta = (Units = "cm/s"))
	float InitialVelocityBoost = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind", meta = (Units = "cm/s"))
	float MaximumEntrySpeed = 0.0f;

	// 물리 오브젝트 힘에 사용하는 0~1 기반 경로 세기입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	float StrengthScale = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	float DeltaTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	int32 ReflectionIndex = 0;
};
