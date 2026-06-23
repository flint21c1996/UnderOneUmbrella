// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUPlayerInteractionExecutorComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UUOUPlayerInteractionExecutorComponent::UUOUPlayerInteractionExecutorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUPlayerInteractionExecutorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearMontageDelegate();
	InputBlockRequestCounts.Empty();
	Super::EndPlay(EndPlayReason);
}

bool UUOUPlayerInteractionExecutorComponent::TryStartInteraction(
	UObject* InteractionSource,
	const FUOUPlayerInteractionRequest& InteractionRequest)
{
	if (InteractionSource == nullptr || bInteractionActive)
	{
		return false;
	}

	bInteractionActive = true;
	ActiveInteractionSource = InteractionSource;
	bBlockInputWhileActive = InteractionRequest.bBlockPlayerInputDuringInteraction;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (InteractionRequest.bStopMovementOnStart && OwnerCharacter != nullptr)
	{
		StopOwnerMovementImmediately();
	}

	if (!InteractionRequest.HasPlayerMontage())
	{
		FinishActiveInteraction(false);
		return true;
	}

	if (OwnerCharacter == nullptr || OwnerCharacter->GetMesh() == nullptr)
	{
		FinishActiveInteraction(true);
		return false;
	}

	UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance == nullptr)
	{
		FinishActiveInteraction(true);
		return false;
	}

	const float PlayRate = FMath::Max(0.01f, InteractionRequest.MontagePlayRate);
	const float Duration = OwnerCharacter->PlayAnimMontage(
		InteractionRequest.PlayerMontage.Get(),
		PlayRate,
		InteractionRequest.MontageStartSection);
	if (Duration <= 0.0f)
	{
		FinishActiveInteraction(true);
		return false;
	}

	ActiveMontage = InteractionRequest.PlayerMontage;
	ActiveAnimInstance = AnimInstance;

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &UUOUPlayerInteractionExecutorComponent::HandleInteractionMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveMontage);
	return true;
}

void UUOUPlayerInteractionExecutorComponent::CancelActiveInteraction()
{
	if (!bInteractionActive)
	{
		return;
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (ActiveMontage != nullptr)
		{
			OwnerCharacter->StopAnimMontage(ActiveMontage);
			if (!bInteractionActive)
			{
				return;
			}
		}
	}

	FinishActiveInteraction(true);
}

bool UUOUPlayerInteractionExecutorComponent::IsInteractionActiveFor(UObject* InteractionSource) const
{
	return bInteractionActive && ActiveInteractionSource == InteractionSource;
}

bool UUOUPlayerInteractionExecutorComponent::ShouldBlockPlayerInput() const
{
	return (bInteractionActive && bBlockInputWhileActive) || HasExternalPlayerInputBlock();
}

void UUOUPlayerInteractionExecutorComponent::RequestPlayerInputBlock(UObject* BlockSource, bool bStopMovementImmediately)
{
	if (BlockSource == nullptr)
	{
		return;
	}

	const TWeakObjectPtr<UObject> SourceKey(BlockSource);
	int32& RequestCount = InputBlockRequestCounts.FindOrAdd(SourceKey);
	++RequestCount;

	if (bStopMovementImmediately)
	{
		StopOwnerMovementImmediately();
	}
}

void UUOUPlayerInteractionExecutorComponent::ReleasePlayerInputBlock(UObject* BlockSource)
{
	if (BlockSource == nullptr)
	{
		return;
	}

	const TWeakObjectPtr<UObject> SourceKey(BlockSource);
	int32* RequestCount = InputBlockRequestCounts.Find(SourceKey);
	if (RequestCount == nullptr)
	{
		return;
	}

	--(*RequestCount);
	if (*RequestCount <= 0)
	{
		InputBlockRequestCounts.Remove(SourceKey);
	}
}

bool UUOUPlayerInteractionExecutorComponent::IsPlayerInputBlockedBy(UObject* BlockSource) const
{
	if (BlockSource == nullptr)
	{
		return false;
	}

	const TWeakObjectPtr<UObject> SourceKey(BlockSource);
	const int32* RequestCount = InputBlockRequestCounts.Find(SourceKey);
	return RequestCount != nullptr && *RequestCount > 0;
}

UUOUPlayerInteractionExecutorComponent* UUOUPlayerInteractionExecutorComponent::FindLocalPlayerExecutor(
	const UObject* WorldContextObject,
	int32 PlayerIndex)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, PlayerIndex);
	const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	return PlayerPawn != nullptr ? PlayerPawn->FindComponentByClass<UUOUPlayerInteractionExecutorComponent>() : nullptr;
}

void UUOUPlayerInteractionExecutorComponent::FinishActiveInteraction(bool bInterrupted)
{
	UObject* FinishedInteractionSource = ActiveInteractionSource.Get();

	ClearMontageDelegate();
	bInteractionActive = false;
	bBlockInputWhileActive = false;
	ActiveInteractionSource = nullptr;
	ActiveMontage = nullptr;
	ActiveAnimInstance.Reset();

	OnInteractionFinished.Broadcast(FinishedInteractionSource, bInterrupted);
}

void UUOUPlayerInteractionExecutorComponent::ClearMontageDelegate()
{
	UAnimInstance* AnimInstance = ActiveAnimInstance.Get();
	if (AnimInstance == nullptr || ActiveMontage == nullptr)
	{
		return;
	}

	FOnMontageEnded EmptyDelegate;
	AnimInstance->Montage_SetEndDelegate(EmptyDelegate, ActiveMontage);
}

void UUOUPlayerInteractionExecutorComponent::StopOwnerMovementImmediately() const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
}

bool UUOUPlayerInteractionExecutorComponent::HasExternalPlayerInputBlock() const
{
	for (const TPair<TWeakObjectPtr<UObject>, int32>& RequestPair : InputBlockRequestCounts)
	{
		if (RequestPair.Key.IsValid() && RequestPair.Value > 0)
		{
			return true;
		}
	}

	return false;
}

void UUOUPlayerInteractionExecutorComponent::HandleInteractionMontageEnded(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	if (Montage != ActiveMontage)
	{
		return;
	}

	FinishActiveInteraction(bInterrupted);
}
