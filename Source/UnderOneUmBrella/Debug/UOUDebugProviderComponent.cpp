// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugProviderComponent.h"

#include "Debug/UOUDebugSubsystem.h"
#include "GameFramework/Actor.h"

UUOUDebugProviderComponent::UUOUDebugProviderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUDebugProviderComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>())
		{
			DebugSubsystem->RegisterDebugProvider(this);
		}
	}
}

void UUOUDebugProviderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>())
		{
			DebugSubsystem->UnregisterDebugProvider(this);
		}
	}

	Super::EndPlay(EndPlayReason);
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

