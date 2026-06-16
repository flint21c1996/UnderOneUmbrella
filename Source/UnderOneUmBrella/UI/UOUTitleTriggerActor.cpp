// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUTitleTriggerActor.h"

#include "Components/BoxComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/UOUUISubsystem.h"

AUOUTitleTriggerActor::AUOUTitleTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);

	TriggerVolume->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	TriggerVolume->SetGenerateOverlapEvents(true);
}

void AUOUTitleTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerVolume != nullptr)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AUOUTitleTriggerActor::HandleTriggerBeginOverlap);
	}
}

void AUOUTitleTriggerActor::TriggerTitle(AActor* InstigatorActor)
{
	if (bTriggerOnce && bTriggered)
	{
		return;
	}

	UUOUUISubsystem* UISubsystem = GetUISubsystem(InstigatorActor);
	if (UISubsystem == nullptr)
	{
		return;
	}

	bTriggered = true;
	UISubsystem->ShowTitle(TitleData);
}

void AUOUTitleTriggerActor::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr)
	{
		return;
	}

	if (bOnlyPlayerPawn && Cast<APawn>(OtherActor) == nullptr)
	{
		return;
	}

	TriggerTitle(OtherActor);
}

UUOUUISubsystem* AUOUTitleTriggerActor::GetUISubsystem(AActor* InstigatorActor) const
{
	APlayerController* PlayerController = Cast<APlayerController>(InstigatorActor);
	if (PlayerController == nullptr)
	{
		const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor);
		PlayerController = InstigatorPawn != nullptr ? Cast<APlayerController>(InstigatorPawn->GetController()) : nullptr;
	}

	if (PlayerController == nullptr && GetWorld() != nullptr)
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}

	ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UUOUUISubsystem>() : nullptr;
}