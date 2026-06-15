// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformCarryComponent.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
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
	for (const TEnumAsByte<ECollisionChannel>& CarryObjectChannel : AdditionalCarryObjectChannels)
	{
		ObjectQueryParams.AddObjectTypesToQuery(CarryObjectChannel.GetValue());
	}

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
	RestoreCarriedMobilityStates();
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

	if (CandidateActor->GetAttachParentActor() != nullptr)
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

	if (HasStaticMobilityComponent(CandidateActor) && bIgnoreStaticMobilityActors && !bForceMovableBeforeAttach)
	{
		return false;
	}

	return MatchesCarryFilters(CandidateActor);
}

void UUOUFloorPlatformCarryComponent::PrepareCarriedActorForAttach(AActor* CandidateActor)
{
	PrepareCarriedActorMobilityForAttach(CandidateActor);

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
		PhysicsState.bLockXTranslation = PrimitiveComponent->BodyInstance.bLockXTranslation;
		PhysicsState.bLockYTranslation = PrimitiveComponent->BodyInstance.bLockYTranslation;
		PhysicsState.bLockZTranslation = PrimitiveComponent->BodyInstance.bLockZTranslation;
		PhysicsState.bLockXRotation = PrimitiveComponent->BodyInstance.bLockXRotation;
		PhysicsState.bLockYRotation = PrimitiveComponent->BodyInstance.bLockYRotation;
		PhysicsState.bLockZRotation = PrimitiveComponent->BodyInstance.bLockZRotation;
		CarriedPhysicsStates.Add(PhysicsState);

		PrimitiveComponent->SetSimulatePhysics(false);
	}
}

void UUOUFloorPlatformCarryComponent::PrepareCarriedActorMobilityForAttach(AActor* CandidateActor)
{
	if (!bForceMovableBeforeAttach || CandidateActor == nullptr)
	{
		return;
	}

	TInlineComponentArray<USceneComponent*> SceneComponents(CandidateActor);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent == nullptr || SceneComponent->Mobility == EComponentMobility::Movable)
		{
			continue;
		}

		FUOUFloorPlatformCarriedMobilityState MobilityState;
		MobilityState.Component = SceneComponent;
		MobilityState.Mobility = SceneComponent->Mobility;
		CarriedMobilityStates.Add(MobilityState);

		SceneComponent->SetMobility(EComponentMobility::Movable);
	}
}

void UUOUFloorPlatformCarryComponent::RestoreCarriedPhysicsStates()
{
	for (const FUOUFloorPlatformCarriedPhysicsState& PhysicsState : CarriedPhysicsStates)
	{
		UPrimitiveComponent* PrimitiveComponent = PhysicsState.Component.Get();
		if (PrimitiveComponent != nullptr)
		{
			const FTransform CarriedWorldTransform = PrimitiveComponent->GetComponentTransform();
			if (PhysicsState.bWasSimulatingPhysics)
			{
				// 잠긴 물리 축이 예전 바디 위치를 되살리지 않도록 복구 순간에는 임시로 이동 잠금을 풉니다.
				PrimitiveComponent->BodyInstance.bLockXTranslation = false;
				PrimitiveComponent->BodyInstance.bLockYTranslation = false;
				PrimitiveComponent->BodyInstance.bLockZTranslation = false;
			}

			PrimitiveComponent->SetSimulatePhysics(PhysicsState.bWasSimulatingPhysics);
			if (PhysicsState.bWasSimulatingPhysics)
			{
				// 물리를 다시 켤 때 기존 물리 바디 위치로 튀지 않도록 최종 운반 위치를 물리 바디에 다시 적용합니다.
				PrimitiveComponent->SetWorldTransform(CarriedWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
				PrimitiveComponent->BodyInstance.bLockXTranslation = PhysicsState.bLockXTranslation;
				PrimitiveComponent->BodyInstance.bLockYTranslation = PhysicsState.bLockYTranslation;
				PrimitiveComponent->BodyInstance.bLockZTranslation = PhysicsState.bLockZTranslation;
				PrimitiveComponent->BodyInstance.bLockXRotation = PhysicsState.bLockXRotation;
				PrimitiveComponent->BodyInstance.bLockYRotation = PhysicsState.bLockYRotation;
				PrimitiveComponent->BodyInstance.bLockZRotation = PhysicsState.bLockZRotation;
				PrimitiveComponent->RecreatePhysicsState();
				PrimitiveComponent->SetWorldTransform(CarriedWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
				PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
				PrimitiveComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			}
		}
	}

	CarriedPhysicsStates.Reset();
}

void UUOUFloorPlatformCarryComponent::RestoreCarriedMobilityStates()
{
	for (const FUOUFloorPlatformCarriedMobilityState& MobilityState : CarriedMobilityStates)
	{
		USceneComponent* SceneComponent = MobilityState.Component.Get();
		if (SceneComponent != nullptr && SceneComponent->Mobility != MobilityState.Mobility)
		{
			SceneComponent->SetMobility(MobilityState.Mobility);
		}
	}

	CarriedMobilityStates.Reset();
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

bool UUOUFloorPlatformCarryComponent::HasStaticMobilityComponent(AActor* CandidateActor) const
{
	if (CandidateActor == nullptr)
	{
		return false;
	}

	TInlineComponentArray<USceneComponent*> SceneComponents(CandidateActor);
	for (const USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent != nullptr && SceneComponent->Mobility == EComponentMobility::Static)
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
