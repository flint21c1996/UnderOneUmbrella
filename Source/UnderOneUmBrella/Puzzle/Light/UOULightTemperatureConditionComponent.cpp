// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Light/UOULightTemperatureConditionComponent.h"

#include "GameFramework/Actor.h"
#include "World/Light/UOULightExposureReceiverComponent.h"

UUOULightTemperatureConditionComponent::UUOULightTemperatureConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOULightTemperatureConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (DeactivateTemperature > ActivateTemperature)
	{
		Swap(DeactivateTemperature, ActivateTemperature);
	}

	ResolveLightReceiver();
	SubscribeLightReceiver();
	RefreshSatisfiedState();
}

void UUOULightTemperatureConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeLightReceiver();
	Super::EndPlay(EndPlayReason);
}

void UUOULightTemperatureConditionComponent::HandleTemperatureChanged(float NewTemperature, float PreviousTemperature)
{
	RefreshSatisfiedState();
}

void UUOULightTemperatureConditionComponent::ResolveLightReceiver()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (HasLightReceiverReference())
	{
		LightReceiver = Cast<UUOULightExposureReceiverComponent>(LightReceiverReference.GetComponent(Owner));
	}

	if (LightReceiver == nullptr && bAutoFindLightReceiver)
	{
		LightReceiver = Owner->FindComponentByClass<UUOULightExposureReceiverComponent>();
	}
}

void UUOULightTemperatureConditionComponent::SubscribeLightReceiver()
{
	if (LightReceiver == nullptr)
	{
		return;
	}

	LightReceiver->OnTemperatureChanged.RemoveDynamic(this, &UUOULightTemperatureConditionComponent::HandleTemperatureChanged);
	LightReceiver->OnTemperatureChanged.AddDynamic(this, &UUOULightTemperatureConditionComponent::HandleTemperatureChanged);
}

void UUOULightTemperatureConditionComponent::UnsubscribeLightReceiver()
{
	if (LightReceiver != nullptr)
	{
		LightReceiver->OnTemperatureChanged.RemoveDynamic(this, &UUOULightTemperatureConditionComponent::HandleTemperatureChanged);
	}
}

void UUOULightTemperatureConditionComponent::RefreshSatisfiedState()
{
	if (LightReceiver == nullptr)
	{
		CurrentTemperature = 0.0f;
		SetSatisfiedState(false, true);
		return;
	}

	CurrentTemperature = LightReceiver->CurrentTemperature;
	if (!bIsSatisfied && CurrentTemperature >= ActivateTemperature)
	{
		SetSatisfiedState(true, true);
		return;
	}

	if (bIsSatisfied && CurrentTemperature <= DeactivateTemperature)
	{
		SetSatisfiedState(false, true);
	}
}

bool UUOULightTemperatureConditionComponent::HasLightReceiverReference() const
{
	return LightReceiverReference.ComponentProperty != NAME_None ||
		!LightReceiverReference.PathToComponent.IsEmpty() ||
		LightReceiverReference.OverrideComponent.IsValid();
}
