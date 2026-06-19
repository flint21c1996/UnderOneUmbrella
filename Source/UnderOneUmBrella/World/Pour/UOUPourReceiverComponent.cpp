// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Pour/UOUPourReceiverComponent.h"

UUOUPourReceiverComponent::UUOUPourReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUPourReceiverComponent::ReceivePourInput(const FUOUPourInputContext& PourContext)
{
	if (!CanReceivePour() || PourContext.Volume <= 0.0f)
	{
		return;
	}

	FUOUPourInputContext SanitizedContext = PourContext;
	SanitizedContext.Volume = FMath::Max(0.0f, SanitizedContext.Volume);
	SanitizedContext.Duration = FMath::Max(0.0f, SanitizedContext.Duration);
	SanitizedContext.WorldDirection = SanitizedContext.WorldDirection.GetSafeNormal();

	OnPourReceived.Broadcast(this, SanitizedContext);
}

bool UUOUPourReceiverComponent::CanReceivePour() const
{
	return bPourReceiverEnabled;
}
