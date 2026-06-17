// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUPlayerAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUUmbrellaComponent.h"

void UUOUPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheOwnerIfNeeded();
}

void UUOUPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	CacheOwnerIfNeeded();
	UpdateMovementVariables();
	UpdateUmbrellaVariables();
	UpdateDerivedAnimationVariables();
}

void UUOUPlayerAnimInstance::CacheOwnerIfNeeded()
{
	AUOUCharacter* CurrentOwner = Cast<AUOUCharacter>(TryGetPawnOwner());

	if (OwnerCharacter != CurrentOwner)
	{
		OwnerCharacter = CurrentOwner;
		UmbrellaComponent = nullptr;
	}

	if (OwnerCharacter != nullptr && UmbrellaComponent == nullptr)
	{
		UmbrellaComponent = OwnerCharacter->FindComponentByClass<UUOUUmbrellaComponent>();
	}
}

void UUOUPlayerAnimInstance::UpdateMovementVariables()
{
	if (OwnerCharacter == nullptr)
	{
		ResetMovementVariables();
		return;
	}

	const FVector Velocity = OwnerCharacter->GetVelocity();
	Speed = Velocity.Size2D();
	VerticalSpeed = Velocity.Z;

	const UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement();
	IsInAir = CharacterMovement != nullptr && CharacterMovement->IsFalling();
}

void UUOUPlayerAnimInstance::UpdateUmbrellaVariables()
{
	if (UmbrellaComponent == nullptr && OwnerCharacter != nullptr)
	{
		UmbrellaComponent = OwnerCharacter->FindComponentByClass<UUOUUmbrellaComponent>();
	}

	if (UmbrellaComponent == nullptr)
	{
		ResetUmbrellaVariables();
		return;
	}

	HasUmbrella = UmbrellaComponent->HasUmbrella();
	IsUmbrellaOpen = UmbrellaComponent->IsOpen();
	IsUmbrellaUpsideDown = UmbrellaComponent->IsUpsideDown();
	IsPouring = UmbrellaComponent->IsPouring();
}

void UUOUPlayerAnimInstance::UpdateDerivedAnimationVariables()
{
	UseUmbrellaAnim = HasUmbrella && IsUmbrellaOpen && !IsUmbrellaUpsideDown && !IsPouring;
	UseFlippedUmbrellaAnim = HasUmbrella && IsUmbrellaUpsideDown && !IsPouring;
}

void UUOUPlayerAnimInstance::ResetMovementVariables()
{
	Speed = 0.0f;
	VerticalSpeed = 0.0f;
	IsInAir = false;
}

void UUOUPlayerAnimInstance::ResetUmbrellaVariables()
{
	HasUmbrella = false;
	IsUmbrellaOpen = false;
	IsUmbrellaUpsideDown = false;
	IsPouring = false;
}
