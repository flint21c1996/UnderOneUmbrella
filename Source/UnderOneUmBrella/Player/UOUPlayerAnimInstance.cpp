// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUPlayerAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUPushPullInteractorComponent.h"
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
	UpdateLadderVariables();
	UpdatePushPullVariables();
	UpdateDerivedAnimationVariables();
}

void UUOUPlayerAnimInstance::CacheOwnerIfNeeded()
{
	AUOUCharacter* CurrentOwner = Cast<AUOUCharacter>(TryGetPawnOwner());

	if (OwnerCharacter != CurrentOwner)
	{
		OwnerCharacter = CurrentOwner;
		UmbrellaComponent = nullptr;
		LadderClimbComponent = nullptr;
		PushPullInteractorComponent = nullptr;
	}

	if (OwnerCharacter != nullptr && UmbrellaComponent == nullptr)
	{
		UmbrellaComponent = OwnerCharacter->FindComponentByClass<UUOUUmbrellaComponent>();
	}

	if (OwnerCharacter != nullptr && LadderClimbComponent == nullptr)
	{
		LadderClimbComponent = OwnerCharacter->FindComponentByClass<UUOULadderClimbComponent>();
	}

	if (OwnerCharacter != nullptr && PushPullInteractorComponent == nullptr)
	{
		PushPullInteractorComponent = OwnerCharacter->FindComponentByClass<UUOUPushPullInteractorComponent>();
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
	IsLightReflecting = UmbrellaComponent->IsLightReflecting();
}

void UUOUPlayerAnimInstance::UpdateDerivedAnimationVariables()
{
	UseUmbrellaAnim = HasUmbrella && IsUmbrellaOpen && !IsUmbrellaUpsideDown && !IsPouring && !IsClimbingLadder;
	UseFlippedUmbrellaAnim = HasUmbrella && IsUmbrellaUpsideDown && !IsPouring && !IsClimbingLadder;
}

void UUOUPlayerAnimInstance::UpdateLadderVariables()
{
	if (LadderClimbComponent == nullptr && OwnerCharacter != nullptr)
	{
		LadderClimbComponent = OwnerCharacter->FindComponentByClass<UUOULadderClimbComponent>();
	}

	if (LadderClimbComponent == nullptr)
	{
		ResetLadderVariables();
		return;
	}

	IsClimbingLadder = LadderClimbComponent->IsClimbing();
	LadderClimbState = LadderClimbComponent->GetClimbState();
	LadderClimbInput = LadderClimbComponent->GetClimbInput();
	LadderNormalizedHeight = LadderClimbComponent->GetNormalizedHeight();
}

void UUOUPlayerAnimInstance::UpdatePushPullVariables()
{
	if (PushPullInteractorComponent == nullptr && OwnerCharacter != nullptr)
	{
		PushPullInteractorComponent = OwnerCharacter->FindComponentByClass<UUOUPushPullInteractorComponent>();
	}

	if (PushPullInteractorComponent == nullptr)
	{
		ResetPushPullVariables();
		return;
	}

	// 크랭크는 전용 동작을 유지하고, 이동 입력을 사용하는 상자와 회전 거울에만 밀기/당기기 애니메이션을 적용합니다.
	IsPushPulling = PushPullInteractorComponent->GetGrabbedObject() != nullptr
		|| PushPullInteractorComponent->GetGrabbedMirror() != nullptr;
	if (!IsPushPulling)
	{
		PushPullBlendInput = 0.0f;
		return;
	}

	FVector MoveAxis = PushPullInteractorComponent->GetGrabbedMoveAxis();
	MoveAxis.Z = 0.0f;
	FVector CharacterForward = OwnerCharacter != nullptr
		? OwnerCharacter->GetActorForwardVector()
		: FVector::ZeroVector;
	CharacterForward.Z = 0.0f;

	// 캐릭터가 실제로 앞쪽으로 이동하면 밀기(+), 뒤쪽으로 이동하면 당기기(-)를 재생합니다.
	// 이 계산은 회전 거울의 반대편 손잡이에서 이동 축이 뒤집히는 경우도 자동으로 처리합니다.
	const float ForwardAlignment = FVector::DotProduct(
		CharacterForward.GetSafeNormal(),
		MoveAxis.GetSafeNormal());
	const float ForwardDirectionSign = FMath::IsNearlyZero(ForwardAlignment)
		? -1.0f
		: FMath::Sign(ForwardAlignment);
	PushPullBlendInput = FMath::Clamp(
		PushPullInteractorComponent->GetCurrentAxisInput() * ForwardDirectionSign,
		-1.0f,
		1.0f);
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
	IsLightReflecting = false;
}

void UUOUPlayerAnimInstance::ResetLadderVariables()
{
	IsClimbingLadder = false;
	LadderClimbState = EUOULadderClimbState::None;
	LadderClimbInput = 0.0f;
	LadderNormalizedHeight = 0.0f;
}

void UUOUPlayerAnimInstance::ResetPushPullVariables()
{
	IsPushPulling = false;
	PushPullBlendInput = 0.0f;
}
