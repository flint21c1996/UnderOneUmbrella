// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Wind/UOUWindReceiverComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/UOUUmbrellaComponent.h"

UUOUWindReceiverComponent::UUOUWindReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UUOUWindReceiverComponent::ReceiveWind_Implementation(const FUOUWindExposureData& WindData)
{
	if (!bReceiveWind
		|| WindData.Acceleration <= 0.0f
		|| WindData.MaximumAcceleration <= 0.0f
		|| WindData.MaximumSpeed <= 0.0f
		|| WindData.Direction.IsNearlyZero())
	{
		return;
	}

	LastWindData = WindData;
	LastAppliedCharacterAcceleration = 0.0f;
	LastUmbrellaStrengthMultiplier = 0.0f;
	LastGravityCancellationAcceleration = 0.0f;

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		bool bUmbrellaOpen = false;
		const float UmbrellaMultiplier =
			ResolveCharacterUmbrellaMultiplier(bUmbrellaOpen);
		LastUmbrellaStrengthMultiplier = UmbrellaMultiplier;
		if (UmbrellaMultiplier <= 0.0f)
		{
			return;
		}

		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			if (MovementComponent->MovementMode == MOVE_None)
			{
				return;
			}

			if (bOnlyAffectFallingCharacter && !MovementComponent->IsFalling())
			{
				return;
			}

			const FVector WindDirection = WindData.Direction.GetSafeNormal();
			const float UmbrellaAdjustedAcceleration =
				FMath::Max(0.0f, WindData.Acceleration * UmbrellaMultiplier);
			const FVector RequestedWindAcceleration =
				UOUWindMotion::CalculateClampedAdditiveAcceleration(
					WindDirection * UmbrellaAdjustedAcceleration,
					WindDirection
						* FMath::Max(0.0f, AdditionalCharacterWindAcceleration),
					WindData.MaximumAcceleration);
			const float RequestedAcceleration = RequestedWindAcceleration.Size();
			if (RequestedAcceleration <= 0.0f)
			{
				return;
			}

			BeginWindborneMovement(MovementComponent, WindData);

			const float CurrentSpeedAlongWind =
				FVector::DotProduct(MovementComponent->Velocity, WindDirection);
			const float TargetSpeedAlongWind =
				FMath::Max(0.0f, WindData.MaximumSpeed);
			const float AppliedAcceleration =
				UOUWindMotion::CalculateDirectionalAcceleration(
					true,
					CurrentSpeedAlongWind,
					TargetSpeedAlongWind,
					RequestedAcceleration,
					WindData.DeltaTime);

			FVector TotalAcceleration =
				WindDirection * AppliedAcceleration;
			if (bUmbrellaOpen
				&& bCancelGravityWhileUmbrellaOpen
				&& MovementComponent->IsFalling())
			{
				const float GravityCancellationAcceleration =
					FMath::Max(
						0.0f,
						-MovementComponent->GetGravityZ()
							* FMath::Max(0.0f, GravityCancellationMultiplier));
				LastGravityCancellationAcceleration =
					GravityCancellationAcceleration;
				TotalAcceleration +=
					FVector::UpVector * GravityCancellationAcceleration;
			}

			LastAppliedCharacterAcceleration = AppliedAcceleration;

			if (!TotalAcceleration.IsNearlyZero())
			{
				MovementComponent->AddForce(
					TotalAcceleration * MovementComponent->Mass);
			}
		}
		return;
	}

	if (UPrimitiveComponent* TargetPrimitive = ResolveTargetPrimitive())
	{
		if (TargetPrimitive->IsSimulatingPhysics())
		{
			TargetPrimitive->AddForce(
				WindData.Direction.GetSafeNormal()
					* PhysicsForce
					* WindData.StrengthScale);
		}
	}
}

void UUOUWindReceiverComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UCharacterMovementComponent* MovementComponent =
		WindborneMovementComponent.Get();
	const UWorld* World = GetWorld();
	if (!bCharacterWindActive
		|| MovementComponent == nullptr
		|| World == nullptr)
	{
		RestoreFallingBraking();
		return;
	}

	constexpr float WindReceiveGracePeriod = 0.05f;
	const bool bWindReceiveExpired =
		World->GetTimeSeconds() - LastCharacterWindReceiveTime
		> WindReceiveGracePeriod;
	if (!MovementComponent->IsFalling() || bWindReceiveExpired)
	{
		RestoreFallingBraking();
	}
}

void UUOUWindReceiverComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	RestoreFallingBraking();
	Super::EndPlay(EndPlayReason);
}

FVector UUOUWindReceiverComponent::GetWindReceiverLocation_Implementation() const
{
	if (const UPrimitiveComponent* TargetPrimitive = ResolveTargetPrimitive())
	{
		return TargetPrimitive->GetComponentLocation();
	}

	const AActor* OwnerActor = GetOwner();
	return OwnerActor != nullptr ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
}

void UUOUWindReceiverComponent::SetReceiveWind(bool bNewReceiveWind)
{
	bReceiveWind = bNewReceiveWind;
	if (!bReceiveWind)
	{
		RestoreFallingBraking();
	}
}

bool UUOUWindReceiverComponent::ApplyWindborneSteeringInput(
	const FVector2D& MovementInput,
	float MovementYaw)
{
	UCharacterMovementComponent* MovementComponent =
		WindborneMovementComponent.Get();
	if (!bCharacterWindActive
		|| MovementComponent == nullptr
		|| !MovementComponent->IsFalling())
	{
		return false;
	}

	const FVector2D ClampedInput =
		MovementInput.GetClampedToMaxSize(1.0f);
	if (!ClampedInput.IsNearlyZero())
	{
		const FRotator YawRotation(0.0f, MovementYaw, 0.0f);
		const FVector CameraRightDirection =
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		const FVector SteeringDirection =
			CameraRightDirection * ClampedInput.X
			+ FVector::UpVector * ClampedInput.Y;
		const FVector SteeringAcceleration =
			SteeringDirection
			* FMath::Max(0.0f, WindborneSteeringAcceleration);
		MovementComponent->AddForce(
			SteeringAcceleration * MovementComponent->Mass);
	}

	return true;
}

UPrimitiveComponent* UUOUWindReceiverComponent::ResolveTargetPrimitive() const
{
	if (UActorComponent* ReferencedComponent = TargetPrimitiveReference.GetComponent(GetOwner()))
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ReferencedComponent))
		{
			return PrimitiveComponent;
		}
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return nullptr;
	}

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
	{
		return RootPrimitive;
	}

	return OwnerActor->FindComponentByClass<UPrimitiveComponent>();
}

float UUOUWindReceiverComponent::ResolveCharacterUmbrellaMultiplier(
	bool& bOutUmbrellaOpen) const
{
	bOutUmbrellaOpen = false;
	const AActor* OwnerActor = GetOwner();
	const UUOUUmbrellaComponent* UmbrellaComponent =
		OwnerActor != nullptr
		? OwnerActor->FindComponentByClass<UUOUUmbrellaComponent>()
		: nullptr;
	bOutUmbrellaOpen = UmbrellaComponent != nullptr && UmbrellaComponent->IsOpen();

	if (bRequireOpenUmbrella && !bOutUmbrellaOpen)
	{
		return 0.0f;
	}

	return bOutUmbrellaOpen
		? FMath::Max(0.0f, OpenUmbrellaStrengthMultiplier)
		: FMath::Max(0.0f, ClosedUmbrellaStrengthMultiplier);
}

void UUOUWindReceiverComponent::BeginWindborneMovement(
	UCharacterMovementComponent* MovementComponent,
	const FUOUWindExposureData& WindData)
{
	if (MovementComponent == nullptr)
	{
		return;
	}

	if (bCharacterWindActive
		&& WindborneMovementComponent.Get() != MovementComponent)
	{
		RestoreFallingBraking();
	}

	const bool bEnteringWind = !bCharacterWindActive;
	WindborneMovementComponent = MovementComponent;
	bCharacterWindActive = true;

	if (bEnteringWind)
	{
		MovementComponent->Velocity =
			UOUWindMotion::CalculateWindEntryVelocity(
				MovementComponent->Velocity,
				WindData.Direction,
				WindData.MinimumEntrySpeed,
				WindData.FallingMomentumConversion,
				FMath::Min(
					WindData.MaximumEntrySpeed,
					WindData.MaximumSpeed),
				bResetVerticalVelocityOnWindEntry);
		LastWindEntrySpeed = FVector::DotProduct(
			MovementComponent->Velocity,
			WindData.Direction.GetSafeNormal());
	}

	if (bSuppressFallingBrakingWhileWindborne
		&& !bHasCachedFallingBraking)
	{
		CachedBrakingDecelerationFalling =
			MovementComponent->BrakingDecelerationFalling;
		bHasCachedFallingBraking = true;
	}

	if (bSuppressFallingBrakingWhileWindborne)
	{
		MovementComponent->BrakingDecelerationFalling = 0.0f;
	}
	if (const UWorld* World = GetWorld())
	{
		LastCharacterWindReceiveTime = World->GetTimeSeconds();
	}
	SetComponentTickEnabled(true);
}

void UUOUWindReceiverComponent::RestoreFallingBraking()
{
	if (bHasCachedFallingBraking)
	{
		if (UCharacterMovementComponent* MovementComponent =
			WindborneMovementComponent.Get())
		{
			MovementComponent->BrakingDecelerationFalling =
				CachedBrakingDecelerationFalling;
		}
	}

	WindborneMovementComponent.Reset();
	CachedBrakingDecelerationFalling = 0.0f;
	LastCharacterWindReceiveTime = -1.0f;
	LastWindEntrySpeed = 0.0f;
	bCharacterWindActive = false;
	bHasCachedFallingBraking = false;
	SetComponentTickEnabled(false);
}
