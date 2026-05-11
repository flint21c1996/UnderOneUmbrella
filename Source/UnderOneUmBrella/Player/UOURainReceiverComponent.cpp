// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOURainReceiverComponent.h"

#include "Engine/World.h"

UUOURainReceiverComponent::UUOURainReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOURainReceiverComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxExposure = FMath::Max(0.0f, MaxExposure);
	NaturalDryRate = FMath::Max(0.0f, NaturalDryRate);
	DryingGraceTime = FMath::Max(0.0f, DryingGraceTime);
	SetExposure(InitialExposure);
}

void UUOURainReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentExposure <= 0.0f || NaturalDryRate <= 0.0f)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (World->GetTimeSeconds() - LastExposureTimestamp < DryingGraceTime)
	{
		return;
	}

	SetExposure(CurrentExposure - NaturalDryRate * DeltaTime);
}

void UUOURainReceiverComponent::ApplyRainExposure(float ExposureAmount)
{
	if (ExposureAmount <= 0.0f)
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		LastExposureTimestamp = World->GetTimeSeconds();
	}

	SetExposure(CurrentExposure + ExposureAmount);
}

void UUOURainReceiverComponent::ClearExposure()
{
	SetExposure(0.0f);
}

float UUOURainReceiverComponent::GetExposureRatio() const
{
	return MaxExposure > 0.0f ? CurrentExposure / MaxExposure : 0.0f;
}

void UUOURainReceiverComponent::SetExposure(float NewExposure)
{
	const float ClampedExposure = FMath::Clamp(NewExposure, 0.0f, MaxExposure);
	if (FMath::IsNearlyEqual(CurrentExposure, ClampedExposure))
	{
		CurrentExposure = ClampedExposure;
		return;
	}

	CurrentExposure = ClampedExposure;
	BroadcastExposureChanged();
}

void UUOURainReceiverComponent::BroadcastExposureChanged()
{
	OnRainExposureChanged.Broadcast(CurrentExposure, MaxExposure);
}
