// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/UOUAudioTriggerActor.h"

#include "Audio/UOUAudioSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

AUOUAudioTriggerActor::AUOUAudioTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootScene);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->SetBoxExtent(TriggerExtent);
}

void AUOUAudioTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyTriggerSettings();

	if (TriggerVolume != nullptr)
	{
		TriggerVolume->OnComponentBeginOverlap.RemoveDynamic(this, &AUOUAudioTriggerActor::HandleTriggerBeginOverlap);
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AUOUAudioTriggerActor::HandleTriggerBeginOverlap);
	}

	if (bPlayOnBeginPlay)
	{
		PlayAudioEvent();
	}
}

void AUOUAudioTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTriggerSettings();
}

bool AUOUAudioTriggerActor::PlayAudioEvent()
{
	if (AudioEventId.IsNone())
	{
		return false;
	}

	if (bPlayOnlyOnce && bHasPlayed)
	{
		return false;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	UUOUAudioSubsystem* AudioSubsystem = GameInstance != nullptr ? GameInstance->GetSubsystem<UUOUAudioSubsystem>() : nullptr;
	if (AudioSubsystem == nullptr)
	{
		return false;
	}

	const bool bPlayed = AudioSubsystem->PlayAudioEventAtLocation(AudioEventId, GetActorLocation());
	if (bPlayed)
	{
		bHasPlayed = true;
	}

	return bPlayed;
}

void AUOUAudioTriggerActor::ResetTrigger()
{
	bHasPlayed = false;
}

void AUOUAudioTriggerActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bPlayOnActorEnter || !ShouldAcceptTriggerActor(OtherActor))
	{
		return;
	}

	PlayAudioEvent();
}

void AUOUAudioTriggerActor::ApplyTriggerSettings()
{
	TriggerExtent.X = FMath::Max(0.0f, TriggerExtent.X);
	TriggerExtent.Y = FMath::Max(0.0f, TriggerExtent.Y);
	TriggerExtent.Z = FMath::Max(0.0f, TriggerExtent.Z);

	if (TriggerVolume == nullptr)
	{
		return;
	}

	TriggerVolume->SetBoxExtent(TriggerExtent);
	TriggerVolume->SetGenerateOverlapEvents(bPlayOnActorEnter);
	TriggerVolume->SetCollisionEnabled(bPlayOnActorEnter ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

bool AUOUAudioTriggerActor::ShouldAcceptTriggerActor(const AActor* OtherActor) const
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return false;
	}

	if (!bPlayerOnly)
	{
		return true;
	}

	const APawn* Pawn = Cast<APawn>(OtherActor);
	return Pawn != nullptr && Pawn->IsPlayerControlled();
}
