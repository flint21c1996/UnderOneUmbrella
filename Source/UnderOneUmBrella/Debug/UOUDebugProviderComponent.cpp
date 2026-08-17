// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugProviderComponent.h"

#include "GameFramework/Actor.h"

UUOUDebugProviderComponent::UUOUDebugProviderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

EUOUDebugCategory UUOUDebugProviderComponent::GetDebugCategory_Implementation() const
{
	return DebugCategory;
}

bool UUOUDebugProviderComponent::IsDebugProviderEnabled_Implementation() const
{
	return bEnabled;
}

FText UUOUDebugProviderComponent::GetDebugDisplayName_Implementation() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}

	const AActor* Owner = GetOwner();
	return Owner != nullptr
		? FText::FromString(Owner->GetName())
		: FText::FromString(GetName());
}

FText UUOUDebugProviderComponent::GetDebugSummaryText_Implementation() const
{
	return SummaryText;
}

FVector UUOUDebugProviderComponent::GetDebugWorldLocation_Implementation() const
{
	const AActor* Owner = GetOwner();
	return Owner != nullptr
		? Owner->GetActorLocation() + WorldLocationOffset
		: WorldLocationOffset;
}

void UUOUDebugProviderComponent::GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const
{
	OutConnections.Reset();
}
