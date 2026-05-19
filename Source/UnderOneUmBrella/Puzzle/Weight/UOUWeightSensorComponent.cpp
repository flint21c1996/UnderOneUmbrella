// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUWeightSensorComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleWeightSource.h"

UUOUWeightSensorComponent::UUOUWeightSensorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UUOUWeightSensorComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveSensorVolume();
	BindSensorVolume();
	RefreshCurrentWeight();
}

void UUOUWeightSensorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSensorVolume();
	OverlapActorCounts.Reset();
	Super::EndPlay(EndPlayReason);
}

void UUOUWeightSensorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshCurrentWeight();
}

float UUOUWeightSensorComponent::GetPuzzleWeight() const
{
	return CurrentWeight;
}

void UUOUWeightSensorComponent::RefreshCurrentWeight()
{
	float TotalWeight = 0.0f;
	TArray<TObjectPtr<AActor>> MissingActors;

	for (const TPair<TObjectPtr<AActor>, int32>& Pair : OverlapActorCounts)
	{
		AActor* OtherActor = Pair.Key.Get();
		if (OtherActor == nullptr)
		{
			MissingActors.Add(Pair.Key);
			continue;
		}

		TotalWeight += ResolveActorWeight(OtherActor);
	}

	for (AActor* MissingActor : MissingActors)
	{
		OverlapActorCounts.Remove(MissingActor);
	}

	OverlappingActorCount = OverlapActorCounts.Num();
	if (FMath::IsNearlyEqual(CurrentWeight, TotalWeight))
	{
		CurrentWeight = TotalWeight;
		return;
	}

	CurrentWeight = TotalWeight;
	OnWeightChanged.Broadcast(CurrentWeight);
}

void UUOUWeightSensorComponent::GetOverlappingActors(TArray<AActor*>& OutActors) const
{
	for (const TPair<TObjectPtr<AActor>, int32>& Pair : OverlapActorCounts)
	{
		AActor* OverlappingActor = Pair.Key.Get();
		if (IsValid(OverlappingActor))
		{
			OutActors.AddUnique(OverlappingActor);
		}
	}
}

void UUOUWeightSensorComponent::HandleSensorBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	RegisterOverlappingActor(OtherActor);
}

void UUOUWeightSensorComponent::HandleSensorEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	UnregisterOverlappingActor(OtherActor);
}

void UUOUWeightSensorComponent::ResolveSensorVolume()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (UActorComponent* ResolvedSensorComponent = SensorVolumeReference.GetComponent(Owner))
	{
		SensorVolume = Cast<UPrimitiveComponent>(ResolvedSensorComponent);
	}

	if (!bAutoFindSensorVolume || SensorVolume != nullptr)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent->GetFName() == PreferredSensorVolumeName)
		{
			SensorVolume = PrimitiveComponent;
			return;
		}
	}

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent->GetGenerateOverlapEvents())
		{
			SensorVolume = PrimitiveComponent;
			return;
		}
	}
}

void UUOUWeightSensorComponent::BindSensorVolume()
{
	if (SensorVolume == nullptr)
	{
		return;
	}

	SensorVolume->SetGenerateOverlapEvents(true);
	SensorVolume->OnComponentBeginOverlap.RemoveDynamic(this, &UUOUWeightSensorComponent::HandleSensorBeginOverlap);
	SensorVolume->OnComponentEndOverlap.RemoveDynamic(this, &UUOUWeightSensorComponent::HandleSensorEndOverlap);
	SensorVolume->OnComponentBeginOverlap.AddDynamic(this, &UUOUWeightSensorComponent::HandleSensorBeginOverlap);
	SensorVolume->OnComponentEndOverlap.AddDynamic(this, &UUOUWeightSensorComponent::HandleSensorEndOverlap);
}

void UUOUWeightSensorComponent::UnbindSensorVolume()
{
	if (SensorVolume == nullptr)
	{
		return;
	}

	SensorVolume->OnComponentBeginOverlap.RemoveDynamic(this, &UUOUWeightSensorComponent::HandleSensorBeginOverlap);
	SensorVolume->OnComponentEndOverlap.RemoveDynamic(this, &UUOUWeightSensorComponent::HandleSensorEndOverlap);
}

void UUOUWeightSensorComponent::RegisterOverlappingActor(AActor* OtherActor)
{
	AActor* Owner = GetOwner();
	if (OtherActor == nullptr || OtherActor == Owner)
	{
		return;
	}

	int32& ContactCount = OverlapActorCounts.FindOrAdd(OtherActor);
	++ContactCount;
}

void UUOUWeightSensorComponent::UnregisterOverlappingActor(AActor* OtherActor)
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
	}
}

float UUOUWeightSensorComponent::ResolveActorWeight(const AActor* OtherActor) const
{
	if (OtherActor == nullptr)
	{
		return 0.0f;
	}

	TInlineComponentArray<UActorComponent*> ActorComponents(OtherActor);
	for (UActorComponent* ActorComponent : ActorComponents)
	{
		if (ActorComponent == nullptr || ActorComponent == this)
		{
			continue;
		}

		if (const IUOUPuzzleWeightSource* WeightSource = Cast<IUOUPuzzleWeightSource>(ActorComponent))
		{
			return FMath::Max(0.0f, WeightSource->GetPuzzleWeight());
		}
	}

	float PrimitiveMassTotal = 0.0f;
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(OtherActor);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent->IsSimulatingPhysics())
		{
			PrimitiveMassTotal += PrimitiveComponent->GetMass();
		}
	}

	return FMath::Max(0.0f, PrimitiveMassTotal);
}
