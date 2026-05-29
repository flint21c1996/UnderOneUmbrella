// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformCarryComponent.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

UUOUFloorPlatformCarryComponent::UUOUFloorPlatformCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUFloorPlatformCarryComponent::SetDetectionBox(UBoxComponent* InDetectionBox)
{
	DetectionBox = InDetectionBox;
}

void UUOUFloorPlatformCarryComponent::AttachCarriedActors()
{
	DetachCarriedActors();

	if (!bCarryActorsOnMove || DetectionBox == nullptr)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	CollectCarryCandidateActors(OverlappingActors);

	for (AActor* CandidateActor : OverlappingActors)
	{
		if (!CanCarryActor(CandidateActor))
		{
			continue;
		}

		PrepareCarriedActorForAttach(CandidateActor);
		CandidateActor->AttachToActor(GetOwner(), FAttachmentTransformRules::KeepWorldTransform);
		CarriedActors.Add(CandidateActor);
	}
}

void UUOUFloorPlatformCarryComponent::AttachLastMovedActors()
{
	DetachCarriedActors();

	if (!bCarryActorsOnMove)
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& LastMovedActor : LastMovedActors)
	{
		AActor* CandidateActor = LastMovedActor.Get();
		if (!CanCarryActor(CandidateActor))
		{
			continue;
		}

		PrepareCarriedActorForAttach(CandidateActor);
		CandidateActor->AttachToActor(GetOwner(), FAttachmentTransformRules::KeepWorldTransform);
		CarriedActors.Add(CandidateActor);
	}
}

void UUOUFloorPlatformCarryComponent::CollectCarryCandidateActors(TArray<AActor*>& OutCandidateActors) const
{
	OutCandidateActors.Reset();

	AActor* OwnerActor = GetOwner();
	if (DetectionBox == nullptr || OwnerActor == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Destructible);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOUFloorPlatformCarryOverlap), false, OwnerActor);
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<FOverlapResult> OverlapResults;
	GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		DetectionBox->GetComponentLocation(),
		DetectionBox->GetComponentQuat(),
		ObjectQueryParams,
		FCollisionShape::MakeBox(DetectionBox->GetScaledBoxExtent()),
		QueryParams);

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* CandidateActor = OverlapResult.GetActor();
		if (CandidateActor == nullptr || OutCandidateActors.Contains(CandidateActor))
		{
			continue;
		}

		OutCandidateActors.Add(CandidateActor);
	}
}

void UUOUFloorPlatformCarryComponent::DetachCarriedActors()
{
	AActor* OwnerActor = GetOwner();
	for (AActor* CarriedActor : CarriedActors)
	{
		if (IsValid(CarriedActor) && CarriedActor->GetAttachParentActor() == OwnerActor)
		{
			CarriedActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	CarriedActors.Reset();
	RestoreCarriedPhysicsStates();
}

bool UUOUFloorPlatformCarryComponent::CanCarryActor(AActor* CandidateActor) const
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(CandidateActor) || CandidateActor == OwnerActor)
	{
		return false;
	}

	for (const TObjectPtr<AActor>& IgnoredActor : IgnoredCarryActors)
	{
		if (IgnoredActor.Get() == CandidateActor)
		{
			return false;
		}
	}

	for (const TSubclassOf<AActor>& IgnoredActorClass : IgnoredCarryActorClasses)
	{
		if (*IgnoredActorClass != nullptr && CandidateActor->IsA(IgnoredActorClass))
		{
			return false;
		}
	}

	if (CandidateActor->GetAttachParentActor() != nullptr && CandidateActor->GetAttachParentActor() != OwnerActor)
	{
		return false;
	}

	const bool bIsCharacter = CandidateActor->IsA<ACharacter>();
	if (bIsCharacter && !bCarryPlayerCharacters)
	{
		return false;
	}

	if (!bCarryPhysicsSimulatingActors && HasSimulatingPhysicsComponent(CandidateActor))
	{
		return false;
	}

	return MatchesCarryFilters(CandidateActor);
}

void UUOUFloorPlatformCarryComponent::PrepareCarriedActorForAttach(AActor* CandidateActor)
{
	if (!bPauseCarriedPhysicsDuringMove || CandidateActor == nullptr)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(CandidateActor);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent == nullptr || !PrimitiveComponent->IsSimulatingPhysics())
		{
			continue;
		}

		FUOUFloorPlatformCarriedPhysicsState PhysicsState;
		PhysicsState.Component = PrimitiveComponent;
		PhysicsState.bWasSimulatingPhysics = true;
		CarriedPhysicsStates.Add(PhysicsState);

		PrimitiveComponent->SetSimulatePhysics(false);
	}
}

void UUOUFloorPlatformCarryComponent::RestoreCarriedPhysicsStates()
{
	for (const FUOUFloorPlatformCarriedPhysicsState& PhysicsState : CarriedPhysicsStates)
	{
		UPrimitiveComponent* PrimitiveComponent = PhysicsState.Component.Get();
		if (PrimitiveComponent != nullptr)
		{
			PrimitiveComponent->SetSimulatePhysics(PhysicsState.bWasSimulatingPhysics);
		}
	}

	CarriedPhysicsStates.Reset();
}

bool UUOUFloorPlatformCarryComponent::HasSimulatingPhysicsComponent(AActor* CandidateActor) const
{
	if (CandidateActor == nullptr)
	{
		return false;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(CandidateActor);

	for (const UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent->IsSimulatingPhysics())
		{
			return true;
		}
	}

	return false;
}

bool UUOUFloorPlatformCarryComponent::MatchesCarryFilters(AActor* CandidateActor) const
{
	if (CandidateActor == nullptr)
	{
		return false;
	}

	const bool bHasClassFilter = CarryActorClasses.Num() > 0;
	const bool bHasTagFilter = CarryActorTags.Num() > 0;

	if (!bHasClassFilter && !bHasTagFilter)
	{
		return true;
	}

	for (const TSubclassOf<AActor>& CarryActorClass : CarryActorClasses)
	{
		if (*CarryActorClass != nullptr && CandidateActor->IsA(CarryActorClass))
		{
			return true;
		}
	}

	for (const FName& CarryActorTag : CarryActorTags)
	{
		if (!CarryActorTag.IsNone() && CandidateActor->ActorHasTag(CarryActorTag))
		{
			return true;
		}
	}

	return false;
}

void UUOUFloorPlatformCarryComponent::CacheLastMovedActors()
{
	LastMovedActors.Reset();

	for (AActor* CarriedActor : CarriedActors)
	{
		if (IsValid(CarriedActor))
		{
			LastMovedActors.Add(CarriedActor);
		}
	}
}

bool UUOUFloorPlatformCarryComponent::HasCarriedActors() const
{
	return CarriedActors.Num() > 0;
}
