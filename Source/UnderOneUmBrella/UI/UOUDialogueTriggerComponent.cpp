// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueTriggerComponent.h"

#include "GameFramework/Pawn.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UI/UOUDialogueSourceComponent.h"

UUOUDialogueTriggerComponent::UUOUDialogueTriggerComponent()
{
	InitSphereRadius(180.0f);
	SetCollisionProfileName(TEXT("Trigger"));
	SetGenerateOverlapEvents(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUDialogueTriggerComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UUOUDialogueTriggerComponent::HandleBeginOverlap);
}

bool UUOUDialogueTriggerComponent::TryStartDialogue(AActor* InstigatorActor)
{
	if (bTriggerOnce && bHasTriggered)
	{
		return false;
	}

	if (!PassesInstigatorRules(InstigatorActor))
	{
		return false;
	}

	UUOUDialogueSourceComponent* Source = ResolveDialogueSource();
	if (Source == nullptr || !Source->StartDialogue(InstigatorActor))
	{
		return false;
	}

	bHasTriggered = true;
	return true;
}

void UUOUDialogueTriggerComponent::ResetTrigger()
{
	bHasTriggered = false;
}

void UUOUDialogueTriggerComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryStartDialogue(OtherActor);
}

UUOUDialogueSourceComponent* UUOUDialogueTriggerComponent::ResolveDialogueSource() const
{
	if (DialogueSource != nullptr)
	{
		return DialogueSource;
	}

	AActor* OwnerActor = GetOwner();
	return OwnerActor != nullptr ? OwnerActor->FindComponentByClass<UUOUDialogueSourceComponent>() : nullptr;
}

bool UUOUDialogueTriggerComponent::PassesInstigatorRules(AActor* InstigatorActor) const
{
	if (InstigatorActor == nullptr)
	{
		return false;
	}

	if (bOnlyPawn && Cast<APawn>(InstigatorActor) == nullptr)
	{
		return false;
	}

	if (!bRequireUmbrella)
	{
		return true;
	}

	const UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent(InstigatorActor);
	if (UmbrellaComponent == nullptr || !UmbrellaComponent->HasUmbrella())
	{
		return false;
	}

	if (bRequireOpenUmbrella && !UmbrellaComponent->IsOpen() && !UmbrellaComponent->IsUpsideDown())
	{
		return false;
	}

	if (bRequireBlockingRain && !UmbrellaComponent->IsBlockingRain())
	{
		return false;
	}

	return true;
}

UUOUUmbrellaComponent* UUOUDialogueTriggerComponent::FindUmbrellaComponent(AActor* InstigatorActor) const
{
	if (InstigatorActor == nullptr)
	{
		return nullptr;
	}

	return InstigatorActor->FindComponentByClass<UUOUUmbrellaComponent>();
}
