// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFadeTeleportTriggerActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

AUOUFadeTeleportTriggerActor::AUOUFadeTeleportTriggerActor()
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

void AUOUFadeTeleportTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyTriggerSettings();

	if (TriggerVolume != nullptr)
	{
		TriggerVolume->OnComponentBeginOverlap.RemoveDynamic(this, &AUOUFadeTeleportTriggerActor::HandleTriggerBeginOverlap);
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AUOUFadeTeleportTriggerActor::HandleTriggerBeginOverlap);
	}
}

void AUOUFadeTeleportTriggerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeOutTimerHandle);
		World->GetTimerManager().ClearTimer(BlackHoldTimerHandle);
		World->GetTimerManager().ClearTimer(FadeInTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AUOUFadeTeleportTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTriggerSettings();
}

bool AUOUFadeTeleportTriggerActor::TriggerTransition(AActor* InstigatorActor)
{
	if (bIsTransitioning || (bTriggerOnce && bHasTriggered))
	{
		return false;
	}

	if (!ShouldAcceptTriggerActor(InstigatorActor) || DestinationActor == nullptr)
	{
		return false;
	}

	APlayerController* PlayerController = ResolvePlayerController(InstigatorActor);
	if (PlayerController == nullptr || PlayerController->PlayerCameraManager == nullptr)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	bHasTriggered = true;
	bIsTransitioning = true;
	PendingTransitionActor = InstigatorActor;
	PendingPlayerController = PlayerController;

	const float SafeFadeOutDuration = FMath::Max(0.0f, FadeOutDuration);
	PlayerController->PlayerCameraManager->StartCameraFade(
		0.0f,
		1.0f,
		SafeFadeOutDuration,
		FadeColor,
		false,
		true);

	if (SafeFadeOutDuration <= 0.0f)
	{
		FinishFadeOut();
	}
	else
	{
		World->GetTimerManager().SetTimer(FadeOutTimerHandle, this, &AUOUFadeTeleportTriggerActor::FinishFadeOut, SafeFadeOutDuration, false);
	}

	return true;
}

void AUOUFadeTeleportTriggerActor::ResetTrigger()
{
	bHasTriggered = false;
}

void AUOUFadeTeleportTriggerActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	TriggerTransition(OtherActor);
}

void AUOUFadeTeleportTriggerActor::ApplyTriggerSettings()
{
	TriggerExtent.X = FMath::Max(0.0f, TriggerExtent.X);
	TriggerExtent.Y = FMath::Max(0.0f, TriggerExtent.Y);
	TriggerExtent.Z = FMath::Max(0.0f, TriggerExtent.Z);
	FadeOutDuration = FMath::Max(0.0f, FadeOutDuration);
	BlackHoldDuration = FMath::Max(0.0f, BlackHoldDuration);
	FadeInDuration = FMath::Max(0.0f, FadeInDuration);

	if (TriggerVolume == nullptr)
	{
		return;
	}

	TriggerVolume->SetBoxExtent(TriggerExtent);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
}

bool AUOUFadeTeleportTriggerActor::ShouldAcceptTriggerActor(const AActor* OtherActor) const
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

APlayerController* AUOUFadeTeleportTriggerActor::ResolvePlayerController(AActor* InstigatorActor) const
{
	APlayerController* PlayerController = Cast<APlayerController>(InstigatorActor);
	if (PlayerController != nullptr)
	{
		return PlayerController;
	}

	const APawn* Pawn = Cast<APawn>(InstigatorActor);
	if (Pawn != nullptr)
	{
		PlayerController = Cast<APlayerController>(Pawn->GetController());
	}

	if (PlayerController == nullptr && GetWorld() != nullptr)
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}

	return PlayerController;
}

void AUOUFadeTeleportTriggerActor::FinishFadeOut()
{
	TeleportPendingActor();

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		FinishTransition();
		return;
	}

	const float SafeBlackHoldDuration = FMath::Max(0.0f, BlackHoldDuration);
	if (SafeBlackHoldDuration <= 0.0f)
	{
		StartFadeIn();
	}
	else
	{
		World->GetTimerManager().SetTimer(BlackHoldTimerHandle, this, &AUOUFadeTeleportTriggerActor::StartFadeIn, SafeBlackHoldDuration, false);
	}
}

void AUOUFadeTeleportTriggerActor::StartFadeIn()
{
	APlayerController* PlayerController = PendingPlayerController.Get();
	if (PlayerController == nullptr || PlayerController->PlayerCameraManager == nullptr)
	{
		FinishTransition();
		return;
	}

	const float SafeFadeInDuration = FMath::Max(0.0f, FadeInDuration);
	PlayerController->PlayerCameraManager->StartCameraFade(
		1.0f,
		0.0f,
		SafeFadeInDuration,
		FadeColor,
		false,
		false);

	UWorld* World = GetWorld();
	if (World == nullptr || SafeFadeInDuration <= 0.0f)
	{
		FinishTransition();
	}
	else
	{
		World->GetTimerManager().SetTimer(FadeInTimerHandle, this, &AUOUFadeTeleportTriggerActor::FinishTransition, SafeFadeInDuration, false);
	}
}

void AUOUFadeTeleportTriggerActor::FinishTransition()
{
	PendingTransitionActor = nullptr;
	PendingPlayerController = nullptr;
	bIsTransitioning = false;
}

bool AUOUFadeTeleportTriggerActor::TeleportPendingActor()
{
	AActor* TargetActor = PendingTransitionActor.Get();
	AActor* Destination = DestinationActor.Get();
	if (TargetActor == nullptr || Destination == nullptr)
	{
		return false;
	}

	if (bStopMovementOnTeleport)
	{
		StopActorMovement(TargetActor);
	}

	const FVector DestinationLocation = Destination->GetActorLocation();
	const FRotator DestinationRotation = bUseDestinationRotation ? Destination->GetActorRotation() : TargetActor->GetActorRotation();
	const bool bTeleported = TargetActor->SetActorLocationAndRotation(
		DestinationLocation,
		DestinationRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	if (bTeleported && bUseDestinationRotation)
	{
		if (APlayerController* PlayerController = PendingPlayerController.Get())
		{
			PlayerController->SetControlRotation(DestinationRotation);
		}
	}

	return bTeleported;
}

void AUOUFadeTeleportTriggerActor::StopActorMovement(AActor* TargetActor) const
{
	ACharacter* Character = Cast<ACharacter>(TargetActor);
	if (Character == nullptr)
	{
		return;
	}

	if (UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement())
	{
		CharacterMovement->StopMovementImmediately();
	}
}
