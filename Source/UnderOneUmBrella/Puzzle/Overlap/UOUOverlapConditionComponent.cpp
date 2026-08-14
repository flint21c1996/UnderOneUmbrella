// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Overlap/UOUOverlapConditionComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UUOUOverlapConditionComponent::UUOUOverlapConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUOverlapConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	RequiredOverlapCount = FMath::Max(1, RequiredOverlapCount);
	SetSatisfiedState(bInitialSatisfied, false);

	ResolveOverlapVolume();
	BindOverlapVolume();
	RefreshOverlapState();
}

void UUOUOverlapConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindOverlapVolume();
	OverlapActorCounts.Reset();

	Super::EndPlay(EndPlayReason);
}

FText UUOUOverlapConditionComponent::GetDebugSummaryText_Implementation() const
{
	const TArray<FString> DebugLines = {
		FString::Printf(TEXT("Overlap Condition: %s"), IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(TEXT("Mode: %s"), *StaticEnum<EUOUOverlapConditionMode>()->GetNameStringByValue(static_cast<int64>(ConditionMode))),
		FString::Printf(TEXT("Targets: %d / Required: %d"), OverlappingTargetCount, RequiredOverlapCount),
		FString::Printf(TEXT("Last Entered: %s"), *GetNameSafe(LastEnteredActor))
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOUOverlapConditionComponent::GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const
{
	for (const TPair<TObjectPtr<AActor>, int32>& Pair : OverlapActorCounts)
	{
		AActor* OverlappingActor = Pair.Key.Get();
		if (IsValid(OverlappingActor))
		{
			OutInputActors.AddUnique(OverlappingActor);
		}
	}

	if (LastEnteredActor != nullptr)
	{
		OutInputActors.AddUnique(LastEnteredActor);
	}
}

void UUOUOverlapConditionComponent::RefreshOverlapState()
{
	TSet<AActor*> CurrentTargetActors;
	if (OverlapVolume != nullptr)
	{
		TArray<AActor*> CurrentOverlappingActors;
		OverlapVolume->GetOverlappingActors(CurrentOverlappingActors);

		for (AActor* CurrentOverlappingActor : CurrentOverlappingActors)
		{
			if (IsConditionTargetActor(CurrentOverlappingActor))
			{
				CurrentTargetActors.Add(CurrentOverlappingActor);
				if (!OverlapActorCounts.Contains(CurrentOverlappingActor))
				{
					OverlapActorCounts.Add(CurrentOverlappingActor, 1);
					if (LastEnteredActor == nullptr)
					{
						LastEnteredActor = CurrentOverlappingActor;
					}
				}
			}
		}
	}

	TArray<TObjectPtr<AActor>> MissingActors;
	for (const TPair<TObjectPtr<AActor>, int32>& Pair : OverlapActorCounts)
	{
		AActor* OverlappingActor = Pair.Key.Get();
		if (!IsValid(OverlappingActor)
			|| (OverlapVolume != nullptr && !CurrentTargetActors.Contains(OverlappingActor)))
		{
			MissingActors.Add(Pair.Key);
		}
	}

	for (AActor* MissingActor : MissingActors)
	{
		OverlapActorCounts.Remove(MissingActor);
	}

	RecalculateSatisfiedState();
}

void UUOUOverlapConditionComponent::ResetOverlapCondition()
{
	OverlapActorCounts.Reset();
	LastEnteredActor = nullptr;
	SetSatisfiedState(bInitialSatisfied, true);
	RefreshOverlapState();
}

void UUOUOverlapConditionComponent::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	RegisterOverlappingActor(OtherActor);
}

void UUOUOverlapConditionComponent::HandleEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	UnregisterOverlappingActor(OtherActor);
}

void UUOUOverlapConditionComponent::ResolveOverlapVolume()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (UActorComponent* ResolvedOverlapComponent = OverlapVolumeReference.GetComponent(Owner))
	{
		OverlapVolume = Cast<UPrimitiveComponent>(ResolvedOverlapComponent);
	}

	if (!bAutoFindOverlapVolume || OverlapVolume != nullptr)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent->GetFName() == PreferredOverlapVolumeName)
		{
			OverlapVolume = PrimitiveComponent;
			return;
		}
	}

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent->GetGenerateOverlapEvents())
		{
			OverlapVolume = PrimitiveComponent;
			return;
		}
	}
}

void UUOUOverlapConditionComponent::BindOverlapVolume()
{
	if (OverlapVolume == nullptr)
	{
		return;
	}

	OverlapVolume->SetGenerateOverlapEvents(true);
	OverlapVolume->OnComponentBeginOverlap.RemoveDynamic(this, &UUOUOverlapConditionComponent::HandleBeginOverlap);
	OverlapVolume->OnComponentEndOverlap.RemoveDynamic(this, &UUOUOverlapConditionComponent::HandleEndOverlap);
	OverlapVolume->OnComponentBeginOverlap.AddDynamic(this, &UUOUOverlapConditionComponent::HandleBeginOverlap);
	OverlapVolume->OnComponentEndOverlap.AddDynamic(this, &UUOUOverlapConditionComponent::HandleEndOverlap);
}

void UUOUOverlapConditionComponent::UnbindOverlapVolume()
{
	if (OverlapVolume == nullptr)
	{
		return;
	}

	OverlapVolume->OnComponentBeginOverlap.RemoveDynamic(this, &UUOUOverlapConditionComponent::HandleBeginOverlap);
	OverlapVolume->OnComponentEndOverlap.RemoveDynamic(this, &UUOUOverlapConditionComponent::HandleEndOverlap);
}

void UUOUOverlapConditionComponent::RegisterOverlappingActor(AActor* OtherActor)
{
	if (!IsConditionTargetActor(OtherActor))
	{
		return;
	}

	int32& ContactCount = OverlapActorCounts.FindOrAdd(OtherActor);
	++ContactCount;
	LastEnteredActor = OtherActor;
	OnTargetEntered.Broadcast(OtherActor);

	RecalculateSatisfiedState();
}

void UUOUOverlapConditionComponent::UnregisterOverlappingActor(AActor* OtherActor)
{
	if (OtherActor == nullptr)
	{
		return;
	}

	int32* ContactCount = OverlapActorCounts.Find(OtherActor);
	if (ContactCount == nullptr)
	{
		return;
	}

	--(*ContactCount);
	if (*ContactCount <= 0)
	{
		OverlapActorCounts.Remove(OtherActor);
		OnTargetExited.Broadcast(OtherActor);
	}

	RecalculateSatisfiedState();
}

void UUOUOverlapConditionComponent::RecalculateSatisfiedState()
{
	OverlappingTargetCount = OverlapActorCounts.Num();

	if (ConditionMode == EUOUOverlapConditionMode::LatchOnEnter && IsSatisfied())
	{
		return;
	}

	SetSatisfiedState(OverlappingTargetCount >= RequiredOverlapCount, true);
}

bool UUOUOverlapConditionComponent::IsConditionTargetActor(AActor* OtherActor) const
{
	if (!IsValid(OtherActor))
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	if (bIgnoreOwner && OtherActor == Owner)
	{
		return false;
	}

	if (TargetActors.Num() > 0)
	{
		return TargetActors.Contains(OtherActor)
			&& MatchesClassFilter(OtherActor)
			&& MatchesTagFilter(OtherActor);
	}

	if (bUsePlayerPawnWhenTargetActorsEmpty)
	{
		const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		return OtherActor == PlayerPawn
			&& MatchesClassFilter(OtherActor)
			&& MatchesTagFilter(OtherActor);
	}

	return MatchesClassFilter(OtherActor) && MatchesTagFilter(OtherActor);
}

bool UUOUOverlapConditionComponent::MatchesClassFilter(AActor* OtherActor) const
{
	if (TargetActorClasses.Num() == 0)
	{
		return true;
	}

	for (const TSubclassOf<AActor>& TargetActorClass : TargetActorClasses)
	{
		if (*TargetActorClass != nullptr && OtherActor->IsA(TargetActorClass))
		{
			return true;
		}
	}

	return false;
}

bool UUOUOverlapConditionComponent::MatchesTagFilter(AActor* OtherActor) const
{
	if (TargetActorTags.Num() == 0)
	{
		return true;
	}

	for (const FName& TargetActorTag : TargetActorTags)
	{
		if (!TargetActorTag.IsNone() && OtherActor->ActorHasTag(TargetActorTag))
		{
			return true;
		}
	}

	return false;
}
