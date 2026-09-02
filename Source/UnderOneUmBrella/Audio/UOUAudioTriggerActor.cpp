// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/UOUAudioTriggerActor.h"

#include "Audio/UOUAudioCueComponent.h"
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

	AudioCueComponent = CreateDefaultSubobject<UUOUAudioCueComponent>(TEXT("AudioCueComponent"));
	AudioCueComponent->SetupAttachment(RootScene);
}

void AUOUAudioTriggerActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		PlayAudioEvent();
		break;

	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		StopAudioEvent();
		break;

	case EOUUPuzzleResultAction::Toggle:
		if (bIsAudioPlaybackActive)
		{
			StopAudioEvent();
		}
		else
		{
			PlayAudioEvent();
		}
		break;

	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

void AUOUAudioTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyTriggerSettings();

	if (TriggerVolume != nullptr)
	{
		TriggerVolume->OnComponentBeginOverlap.RemoveDynamic(this, &AUOUAudioTriggerActor::HandleTriggerBeginOverlap);
		TriggerVolume->OnComponentEndOverlap.RemoveDynamic(this, &AUOUAudioTriggerActor::HandleTriggerEndOverlap);
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AUOUAudioTriggerActor::HandleTriggerBeginOverlap);
		TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AUOUAudioTriggerActor::HandleTriggerEndOverlap);
	}

	if (bPlayOnBeginPlay)
	{
		PlayAudioEvent();
	}
}

void AUOUAudioTriggerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bStopOnEndPlay)
	{
		StopAudioEvent();
	}

	Super::EndPlay(EndPlayReason);
}

void AUOUAudioTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTriggerSettings();
}

bool AUOUAudioTriggerActor::PlayAudioEvent()
{
	if (AudioCueId.IsNone() && AudioEventId.IsNone())
	{
		return false;
	}

	if (bPlayOnlyOnce && bHasPlayed)
	{
		return false;
	}

	bool bPlayed = false;
	const FName ResolvedAudioInstanceId = GetResolvedAudioInstanceId();
	if (!AudioCueId.IsNone()
		&& AudioCueComponent != nullptr
		&& AudioCueComponent->HasCue(AudioCueId))
	{
		bPlayed = AudioCueComponent->PlayCueWithInstance(AudioCueId, ResolvedAudioInstanceId, GetActorLocation());
	}

	if (!bPlayed && !AudioEventId.IsNone())
	{
		if (UUOUAudioSubsystem* AudioSubsystem = GetAudioSubsystem())
		{
			bPlayed = AudioSubsystem->PlayAudioEventInstance(AudioEventId, ResolvedAudioInstanceId, GetActorLocation());
		}
	}

	if (bPlayed)
	{
		bHasPlayed = true;
		bIsAudioPlaybackActive = true;
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

void AUOUAudioTriggerActor::HandleTriggerEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!bStopOnActorExit || !ShouldAcceptTriggerActor(OtherActor))
	{
		return;
	}

	StopAudioEvent();
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
	const bool bUseTriggerVolume = bPlayOnActorEnter || bStopOnActorExit;
	TriggerVolume->SetGenerateOverlapEvents(bUseTriggerVolume);
	TriggerVolume->SetCollisionEnabled(bUseTriggerVolume ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
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

bool AUOUAudioTriggerActor::StopAudioEvent(float OverrideFadeOutTime)
{
	bool bStopped = false;
	const FName ResolvedAudioInstanceId = GetResolvedAudioInstanceId();
	if (!AudioCueId.IsNone()
		&& AudioCueComponent != nullptr
		&& AudioCueComponent->HasCue(AudioCueId))
	{
		bStopped = AudioCueComponent->StopCueWithInstance(AudioCueId, ResolvedAudioInstanceId, OverrideFadeOutTime);
	}

	if (!bStopped && !AudioEventId.IsNone())
	{
		if (UUOUAudioSubsystem* AudioSubsystem = GetAudioSubsystem())
		{
			bStopped = AudioSubsystem->StopAudioEvent(AudioEventId, ResolvedAudioInstanceId, OverrideFadeOutTime);
		}
	}

	// 관리되지 않는 일회성 사운드는 이미 끝났을 수 있으므로 정지 성공 여부와 관계없이
	// 퍼즐 결과가 요청한 재생 상태는 비활성으로 갱신합니다.
	bIsAudioPlaybackActive = false;

	return bStopped;
}

FName AUOUAudioTriggerActor::GetResolvedAudioInstanceId() const
{
	return AudioInstanceId.IsNone() ? GetFName() : AudioInstanceId;
}

UUOUAudioSubsystem* AUOUAudioTriggerActor::GetAudioSubsystem() const
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	return GameInstance != nullptr ? GameInstance->GetSubsystem<UUOUAudioSubsystem>() : nullptr;
}
