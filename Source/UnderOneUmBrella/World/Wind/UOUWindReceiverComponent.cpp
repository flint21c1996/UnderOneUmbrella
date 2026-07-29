// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Wind/UOUWindReceiverComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "World/Wind/UOUWindMotion.h"

namespace UOUWindReceiverPrivate
{
	bool IsUsableExposure(const FUOUWindExposureData& WindData)
	{
		return WindData.Acceleration > 0.0f
			&& WindData.MaximumAcceleration > 0.0f
			&& WindData.MaximumSpeed > 0.0f
			&& !WindData.Direction.IsNearlyZero();
	}
}

UUOUWindReceiverComponent::UUOUWindReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UUOUWindReceiverComponent::ReceiveWind_Implementation(const FUOUWindExposureData& WindData)
{
	if (!bReceiveWind
		|| !UOUWindReceiverPrivate::IsUsableExposure(WindData))
	{
		return;
	}

	LastWindData = WindData;
	LastAppliedCharacterAcceleration = 0.0f;
	LastUmbrellaStrengthMultiplier = 0.0f;
	LastGravityCancellationAcceleration = 0.0f;

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		ApplyWindToCharacter(Character, WindData);
		return;
	}

	ApplyWindToPhysicsBody(WindData);
}

void UUOUWindReceiverComponent::ApplyWindToCharacter(
	ACharacter* Character,
	const FUOUWindExposureData& WindData)
{
	if (Character == nullptr)
	{
		return;
	}

	bool bUmbrellaOpen = false;
	const float UmbrellaMultiplier =
		ResolveCharacterUmbrellaMultiplier(bUmbrellaOpen);
	LastUmbrellaStrengthMultiplier = UmbrellaMultiplier;
	if (UmbrellaMultiplier <= 0.0f)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent =
		Character->GetCharacterMovement();
	if (MovementComponent == nullptr
		|| MovementComponent->MovementMode == MOVE_None
		|| (bOnlyAffectFallingCharacter
			&& !MovementComponent->IsFalling()))
	{
		return;
	}

	const FVector WindDirection =
		WindData.Direction.GetSafeNormal();
	const float UmbrellaAdjustedAcceleration =
		FMath::Max(
			0.0f,
			WindData.Acceleration * UmbrellaMultiplier);
	const FVector RequestedWindAcceleration =
		UOUWindMotion::CalculateClampedAdditiveAcceleration(
			WindDirection * UmbrellaAdjustedAcceleration,
			WindDirection
				* FMath::Max(
					0.0f,
					AdditionalCharacterWindAcceleration),
			WindData.MaximumAcceleration);
	const float RequestedAcceleration =
		RequestedWindAcceleration.Size();
	if (RequestedAcceleration <= 0.0f)
	{
		return;
	}

	UpdateWindborneMovement(MovementComponent, WindData);

	const float AppliedAcceleration =
		UOUWindMotion::
			CalculateMagnitudeCappedDirectionalAcceleration(
				MovementComponent->Velocity,
				WindDirection,
				WindData.MaximumSpeed,
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
					* FMath::Max(
						0.0f,
						GravityCancellationMultiplier));
		LastGravityCancellationAcceleration =
			GravityCancellationAcceleration;
		TotalAcceleration +=
			FVector::UpVector
				* GravityCancellationAcceleration;
	}

	LastAppliedCharacterAcceleration = AppliedAcceleration;
	if (!TotalAcceleration.IsNearlyZero())
	{
		MovementComponent->AddForce(
			TotalAcceleration * MovementComponent->Mass);
	}
}

void UUOUWindReceiverComponent::ApplyWindToPhysicsBody(
	const FUOUWindExposureData& WindData)
{
	UPrimitiveComponent* TargetPrimitive =
		ResolveTargetPrimitive();
	if (TargetPrimitive != nullptr
		&& TargetPrimitive->IsSimulatingPhysics())
	{
		TargetPrimitive->AddForce(
			WindData.Direction.GetSafeNormal()
				* PhysicsForce
				* WindData.StrengthScale);
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

void UUOUWindReceiverComponent::UpdateWindborneMovement(
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
	const bool bChangingWindSegment =
		bCharacterWindActive
		&& (ActiveWindSourceActor.Get() != WindData.SourceActor
			|| ActiveWindReflectionIndex
				!= WindData.ReflectionIndex);
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
				WindData.InitialVelocityBoost,
				FMath::Min(
					WindData.MaximumEntrySpeed,
					WindData.MaximumSpeed),
				ExistingVelocityRetentionOnWindEntry)
				.GetClampedToMaxSize(
					FMath::Max(
						0.0f,
						WindData.MaximumSpeed));
	}
	else if (bChangingWindSegment)
	{
		MovementComponent->Velocity =
			UOUWindMotion::CalculateWindTransitionVelocity(
				MovementComponent->Velocity,
				WindData.Direction,
				WindData.MaximumSpeed,
				WindVelocityTransferRatio);
	}

	if (bEnteringWind || bChangingWindSegment)
	{
		LastWindEntrySpeed = FVector::DotProduct(
			MovementComponent->Velocity,
			WindData.Direction.GetSafeNormal());
	}
	ActiveWindSourceActor = WindData.SourceActor;
	ActiveWindReflectionIndex = WindData.ReflectionIndex;

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
	ActiveWindSourceActor.Reset();
	CachedBrakingDecelerationFalling = 0.0f;
	LastCharacterWindReceiveTime = -1.0f;
	LastWindEntrySpeed = 0.0f;
	ActiveWindReflectionIndex = INDEX_NONE;
	bCharacterWindActive = false;
	bHasCachedFallingBraking = false;
	SetComponentTickEnabled(false);
}
