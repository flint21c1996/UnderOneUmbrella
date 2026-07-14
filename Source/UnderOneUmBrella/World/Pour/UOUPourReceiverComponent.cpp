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

bool UUOUPourReceiverComponent::CanAcceptPour_Implementation(const FUOUPourInputContext& Context) const
{
	return CanReceivePour() && Context.Volume > 0.0f;
}

FUOUPourReceiveResult UUOUPourReceiverComponent::TryReceivePour_Implementation(const FUOUPourInputContext& Context)
{
	FUOUPourReceiveResult Result;
	if (!CanAcceptPour_Implementation(Context))
	{
		return Result;
	}

	ReceivePourInput(Context);
	Result.bAccepted = true;
	Result.AcceptedVolume = Context.Volume;
	Result.ReceiverId = TEXT("PourReceiver");
	Result.ReceiverType = EUOUPourDropReceiverType::PurePourReceiver;
	Result.ReceiverObject = this;
	Result.ReceiverActor = GetOwner();
	return Result;
}

int32 UUOUPourReceiverComponent::GetPourReceivePriority_Implementation() const
{
	// 기존 TryDeliverWater의 수신 타입 검사 순서를 유지합니다.
	return 400;
}

bool UUOUPourReceiverComponent::CanAcceptPourAtLocation_Implementation(const FUOUPourInputContext& Context) const
{
	(void)Context;
	return false;
}
