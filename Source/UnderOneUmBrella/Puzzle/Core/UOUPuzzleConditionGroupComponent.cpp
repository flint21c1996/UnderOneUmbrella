// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Core/UOUPuzzleConditionGroupComponent.h"

#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"

UUOUPuzzleConditionGroupComponent::UUOUPuzzleConditionGroupComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUPuzzleConditionGroupComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveConditionSources();
	SubscribeConditions();
	RefreshSatisfiedState(true);
}

void UUOUPuzzleConditionGroupComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeConditions();
	Super::EndPlay(EndPlayReason);
}

void UUOUPuzzleConditionGroupComponent::RefreshNow()
{
	UnsubscribeConditions();
	ResolveConditionSources();
	SubscribeConditions();
	RefreshSatisfiedState(true);
}

bool UUOUPuzzleConditionGroupComponent::IsSatisfied() const
{
	return bIsSatisfied;
}

int32 UUOUPuzzleConditionGroupComponent::GetConditionCount() const
{
	return ResolvedConditionSources.Num();
}

int32 UUOUPuzzleConditionGroupComponent::GetSatisfiedCount() const
{
	int32 SatisfiedCount = 0;
	for (const UUOUPuzzleConditionSourceComponent* ConditionSource : ResolvedConditionSources)
	{
		if (ConditionSource != nullptr && ConditionSource->IsSatisfied())
		{
			++SatisfiedCount;
		}
	}

	return SatisfiedCount;
}

void UUOUPuzzleConditionGroupComponent::SetExternalConditionSources(
	const TArray<UUOUPuzzleConditionSourceComponent*>& NewConditionSources)
{
	ExternalConditionSources.Reset();

	for (UUOUPuzzleConditionSourceComponent* ConditionSource : NewConditionSources)
	{
		if (ConditionSource != nullptr)
		{
			ExternalConditionSources.AddUnique(ConditionSource);
		}
	}
}

void UUOUPuzzleConditionGroupComponent::ClearExternalConditionSources()
{
	ExternalConditionSources.Reset();
}

void UUOUPuzzleConditionGroupComponent::HandleConditionChanged(bool bNewSatisfied)
{
	RefreshSatisfiedState(true);
}

void UUOUPuzzleConditionGroupComponent::ResolveConditionSources()
{
	ResolvedConditionSources.Reset();

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	for (const FComponentReference& ConditionReference : ConditionSourceReferences)
	{
		if (UActorComponent* Component = ConditionReference.GetComponent(Owner))
		{
			if (UUOUPuzzleConditionSourceComponent* ConditionSource = Cast<UUOUPuzzleConditionSourceComponent>(Component))
			{
				ResolvedConditionSources.AddUnique(ConditionSource);
			}
		}
	}

	for (UUOUPuzzleConditionSourceComponent* ConditionSource : ExternalConditionSources)
	{
		if (ConditionSource != nullptr)
		{
			ResolvedConditionSources.AddUnique(ConditionSource);
		}
	}

	if (bAutoCollectLocalConditionSources)
	{
		TInlineComponentArray<UUOUPuzzleConditionSourceComponent*> LocalConditionSources(Owner);
		for (UUOUPuzzleConditionSourceComponent* ConditionSource : LocalConditionSources)
		{
			if (ConditionSource != nullptr)
			{
				ResolvedConditionSources.AddUnique(ConditionSource);
			}
		}
	}
}

void UUOUPuzzleConditionGroupComponent::SubscribeConditions()
{
	for (UUOUPuzzleConditionSourceComponent* ConditionSource : ResolvedConditionSources)
	{
		if (ConditionSource == nullptr)
		{
			continue;
		}

		ConditionSource->OnConditionChanged.RemoveDynamic(this, &UUOUPuzzleConditionGroupComponent::HandleConditionChanged);
		ConditionSource->OnConditionChanged.AddDynamic(this, &UUOUPuzzleConditionGroupComponent::HandleConditionChanged);
	}
}

void UUOUPuzzleConditionGroupComponent::UnsubscribeConditions()
{
	for (UUOUPuzzleConditionSourceComponent* ConditionSource : ResolvedConditionSources)
	{
		if (ConditionSource != nullptr)
		{
			ConditionSource->OnConditionChanged.RemoveDynamic(this, &UUOUPuzzleConditionGroupComponent::HandleConditionChanged);
		}
	}
}

void UUOUPuzzleConditionGroupComponent::RefreshSatisfiedState(bool bBroadcastEvents)
{
	const bool bNextSatisfied = AreAllConditionsSatisfied();
	if (bIsSatisfied == bNextSatisfied)
	{
		return;
	}

	bIsSatisfied = bNextSatisfied;
	if (!bBroadcastEvents)
	{
		return;
	}

	OnStateChanged.Broadcast(bIsSatisfied);
	if (bIsSatisfied)
	{
		OnSatisfied.Broadcast();
		return;
	}

	OnUnsatisfied.Broadcast();
}

bool UUOUPuzzleConditionGroupComponent::AreAllConditionsSatisfied() const
{
	if (ResolvedConditionSources.Num() == 0)
	{
		return false;
	}

	for (const UUOUPuzzleConditionSourceComponent* ConditionSource : ResolvedConditionSources)
	{
		if (ConditionSource == nullptr || !ConditionSource->IsSatisfied())
		{
			return false;
		}
	}

	return true;
}
