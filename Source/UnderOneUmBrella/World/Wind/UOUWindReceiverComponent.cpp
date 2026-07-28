// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Wind/UOUWindReceiverComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/UOUUmbrellaComponent.h"

UUOUWindReceiverComponent::UUOUWindReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUWindReceiverComponent::ReceiveWind_Implementation(const FUOUWindExposureData& WindData)
{
	if (!bReceiveWind || WindData.Strength <= 0.0f || WindData.Direction.IsNearlyZero())
	{
		return;
	}

	LastWindData = WindData;

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		const float UmbrellaMultiplier = ResolveCharacterUmbrellaMultiplier();
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

			const FVector Acceleration =
				WindData.Direction.GetSafeNormal()
				* CharacterAcceleration
				* WindData.Strength
				* UmbrellaMultiplier;
			MovementComponent->AddForce(Acceleration * MovementComponent->Mass);
		}
		return;
	}

	if (UPrimitiveComponent* TargetPrimitive = ResolveTargetPrimitive())
	{
		if (TargetPrimitive->IsSimulatingPhysics())
		{
			TargetPrimitive->AddForce(
				WindData.Direction.GetSafeNormal() * PhysicsForce * WindData.Strength);
		}
	}
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

float UUOUWindReceiverComponent::ResolveCharacterUmbrellaMultiplier() const
{
	const AActor* OwnerActor = GetOwner();
	const UUOUUmbrellaComponent* UmbrellaComponent =
		OwnerActor != nullptr
		? OwnerActor->FindComponentByClass<UUOUUmbrellaComponent>()
		: nullptr;
	const bool bUmbrellaOpen = UmbrellaComponent != nullptr && UmbrellaComponent->IsOpen();

	if (bRequireOpenUmbrella && !bUmbrellaOpen)
	{
		return 0.0f;
	}

	return bUmbrellaOpen
		? FMath::Max(0.0f, OpenUmbrellaStrengthMultiplier)
		: FMath::Max(0.0f, ClosedUmbrellaStrengthMultiplier);
}
