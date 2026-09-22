#include "World/Stage/UOUIllusionTraversalProbe.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	FVector BrakeWalkingVelocity(const UCharacterMovementComponent* Movement, FVector Velocity, float DeltaSeconds, float Friction)
	{
		// 엔진 지상 제동과 같은 분할 적분을 사용한다. 보호된 엔진 함수를 호출하려고 이동 모드를 바꾸지 않는다.
		const FVector Before = Velocity;
		const float Drag = FMath::Max(0.0f, Friction) * FMath::Max(0.0f, Movement->BrakingFrictionFactor);
		const float Deceleration = FMath::Max(0.0f, Movement->BrakingDecelerationWalking);
		const FVector ReverseAcceleration = -Deceleration * Velocity.GetSafeNormal();
		const float MaxStep = FMath::Clamp(Movement->BrakingSubStepTime, 1.0f / 75.0f, 1.0f / 20.0f);
		float Remaining = DeltaSeconds;
		while (Remaining >= 0.000001f)
		{
			const float Step = Remaining > MaxStep && Drag > 0 ? FMath::Min(MaxStep, Remaining * 0.5f) : Remaining;
			Remaining -= Step;
			Velocity += (-Drag * Velocity + ReverseAcceleration) * Step;
			if (FVector::DotProduct(Velocity, Before) <= 0) return FVector::ZeroVector;
		}
		// 엔진의 저속 제동 종료 기준을 맞춰 거의 정지한 상태에서 계속 미끄러지지 않게 한다.
		return Velocity.SizeSquared() <= UE_KINDA_SMALL_NUMBER || (Deceleration > 0 && Velocity.SizeSquared() <= 100.0)
			? FVector::ZeroVector : Velocity;
	}
}

FVector AUOUIllusionTraversalProbe::CalculateWalkingVelocity(UCharacterMovementComponent* Movement,
	const FVector& Input, float DeltaSeconds)
{
	// 기본 지상 보행의 마찰·가속·제동 설정을 공유한다. 위치의 깊이 보정은 속도에 섞지 않는다.
	if (!Movement || DeltaSeconds <= 0) return Movement ? Movement->Velocity : FVector::ZeroVector;
	const FVector SavedVelocity = Movement->Velocity;
	Movement->Velocity.Z = 0;
	const FVector Intent = FVector(Input.X, Input.Y, 0).GetClampedToMaxSize(1.0);
	const FVector Acceleration = Intent * Movement->GetMaxAcceleration();
	const float MaxSpeed = FMath::Max(Movement->MinAnalogWalkSpeed, Movement->MaxWalkSpeed * Intent.Size());
	const float Friction = FMath::Max(0.0f, Movement->GroundFriction);
	const bool bOverMax = Movement->Velocity.SizeSquared() > FMath::Square(MaxSpeed) * 1.01f;
	if (Intent.IsNearlyZero() || bOverMax)
	{
		const FVector BeforeBraking = Movement->Velocity;
		Movement->Velocity = BrakeWalkingVelocity(Movement, Movement->Velocity, DeltaSeconds,
			Movement->bUseSeparateBrakingFriction ? Movement->BrakingFriction : Friction);
		if (bOverMax && Movement->Velocity.SizeSquared() < FMath::Square(MaxSpeed)
			&& FVector::DotProduct(Acceleration, BeforeBraking) > 0)
			Movement->Velocity = BeforeBraking.GetSafeNormal() * MaxSpeed;
	}
	else
	{
		// 방향 전환도 속도를 새로 0으로 만들지 않고 일반 보행과 같은 마찰로 처리한다.
		const FVector DesiredDirectionVelocity = Intent.GetSafeNormal() * Movement->Velocity.Size();
		Movement->Velocity -= (Movement->Velocity - DesiredDirectionVelocity) * FMath::Min(DeltaSeconds * Friction, 1.0f);
	}
	if (!Intent.IsNearlyZero())
	{
		const double Limit = FMath::Max(static_cast<double>(MaxSpeed), Movement->Velocity.Size());
		Movement->Velocity = (Movement->Velocity + Acceleration * DeltaSeconds).GetClampedToMaxSize(Limit);
	}
	const FVector Result = Movement->Velocity;
	// 실제 속도 반영은 이동이 확정된 뒤 한 번만 한다.
	Movement->Velocity = SavedVelocity;
	return Result;
}
