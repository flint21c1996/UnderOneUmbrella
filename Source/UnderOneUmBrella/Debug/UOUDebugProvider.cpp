// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugProvider.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

namespace UOUDebugProviderPrivate
{
	const AActor* ResolveOwnerActor(const UObject* ProviderObject)
	{
		if (const AActor* Actor = Cast<AActor>(ProviderObject))
		{
			return Actor;
		}

		const UActorComponent* Component = Cast<UActorComponent>(ProviderObject);
		return Component != nullptr ? Component->GetOwner() : nullptr;
	}
}

EUOUDebugCategory IUOUDebugProvider::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::System;
}

bool IUOUDebugProvider::IsDebugProviderEnabled_Implementation() const
{
	return true;
}

FText IUOUDebugProvider::GetDebugDisplayName_Implementation() const
{
	const UObject* ProviderObject = _getUObject();
	const AActor* OwnerActor = UOUDebugProviderPrivate::ResolveOwnerActor(ProviderObject);
	return FText::FromString(GetNameSafe(OwnerActor != nullptr ? OwnerActor : ProviderObject));
}

FText IUOUDebugProvider::GetDebugSummaryText_Implementation() const
{
	return FText::GetEmpty();
}

FVector IUOUDebugProvider::GetDebugWorldLocation_Implementation() const
{
	const AActor* OwnerActor = UOUDebugProviderPrivate::ResolveOwnerActor(_getUObject());
	return OwnerActor != nullptr ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
}

void IUOUDebugProvider::GetDebugConnections_Implementation(
	TArray<FUOUDebugConnection>& OutConnections) const
{
	OutConnections.Reset();
}
