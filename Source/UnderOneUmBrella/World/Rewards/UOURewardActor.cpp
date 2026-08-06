// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Rewards/UOURewardActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Game/UOUPlayerProgressSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "Player/UOUCharacter.h"
#include "UI/UOUUISubsystem.h"
#include "World/Rewards/UOURewardCollectionMotionComponent.h"
#include "World/Rewards/UOURewardFeedbackComponent.h"

AUOURewardActor::AUOURewardActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	CollectionTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("CollectionTrigger"));
	CollectionTrigger->SetupAttachment(RootScene);
	CollectionTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollectionTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollectionTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollectionTrigger->SetGenerateOverlapEvents(true);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootScene);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);

	CollectionMotionPath = CreateDefaultSubobject<USplineComponent>(TEXT("CollectionMotionPath"));
	CollectionMotionPath->bEditableWhenInherited = true;
	CollectionMotionPath->SetupAttachment(RootScene);
	CollectionMotionPath->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollectionMotionPath->ClearSplinePoints(false);
	CollectionMotionPath->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
	CollectionMotionPath->AddSplinePoint(FVector(0.0f, 0.0f, 160.0f), ESplineCoordinateSpace::Local, true);
	CollectionMotionPath->SetClosedLoop(false);
	CollectionMotionPath->SetHiddenInGame(true);

	ObjectiveEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ObjectiveEffect"));
	ObjectiveEffect->SetupAttachment(VisualMesh);
	ObjectiveEffect->SetAutoActivate(true);

	RewardFeedbackComponent = CreateDefaultSubobject<UUOURewardFeedbackComponent>(TEXT("RewardFeedbackComponent"));
	RewardCollectionMotionComponent =
		CreateDefaultSubobject<UUOURewardCollectionMotionComponent>(TEXT("RewardCollectionMotionComponent"));
}

void AUOURewardActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyComponentSettings();
}

void AUOURewardActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyComponentSettings();
	if (VisualMesh != nullptr)
	{
		BaseVisualRelativeLocation = VisualMesh->GetRelativeLocation();
		BaseVisualRelativeRotation = VisualMesh->GetRelativeRotation();
	}

	CollectionTrigger->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&AUOURewardActor::HandleCollectionTriggerBeginOverlap);
	if (RewardFeedbackComponent != nullptr)
	{
		RewardFeedbackComponent->OnFeedbackFinished.AddUniqueDynamic(
			this,
			&AUOURewardActor::HandleRewardFeedbackFinished);
	}
	if (RewardCollectionMotionComponent != nullptr)
	{
		RewardCollectionMotionComponent->OnCollectionMotionFinished.AddUniqueDynamic(
			this,
			&AUOURewardActor::HandleCollectionMotionFinished);
		RewardCollectionMotionComponent->OnCollectionMotionCue.AddUniqueDynamic(
			this,
			&AUOURewardActor::HandleCollectionMotionCue);
	}
}

void AUOURewardActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CollectionTrigger != nullptr)
	{
		CollectionTrigger->OnComponentBeginOverlap.RemoveDynamic(
			this,
			&AUOURewardActor::HandleCollectionTriggerBeginOverlap);
	}

	if (RewardFeedbackComponent != nullptr)
	{
		RewardFeedbackComponent->OnFeedbackFinished.RemoveDynamic(
			this,
			&AUOURewardActor::HandleRewardFeedbackFinished);
	}

	if (RewardCollectionMotionComponent != nullptr)
	{
		RewardCollectionMotionComponent->OnCollectionMotionFinished.RemoveDynamic(
			this,
			&AUOURewardActor::HandleCollectionMotionFinished);
		RewardCollectionMotionComponent->OnCollectionMotionCue.RemoveDynamic(
			this,
			&AUOURewardActor::HandleCollectionMotionCue);
	}

	Super::EndPlay(EndPlayReason);
}

void AUOURewardActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bCollected || VisualMesh == nullptr)
	{
		return;
	}

	const float ElapsedTime = GetGameTimeSinceCreation();

	FVector RelativeLocation = BaseVisualRelativeLocation;
	if (bUseHoverMotion)
	{
		RelativeLocation.Z += FMath::Sin(ElapsedTime * HoverSpeed) * HoverAmplitude;
	}
	VisualMesh->SetRelativeLocation(RelativeLocation);

	FRotator RelativeRotation = BaseVisualRelativeRotation;
	if (bUseRotationMotion)
	{
		RelativeRotation += RotationSpeed * ElapsedTime;
	}
	VisualMesh->SetRelativeRotation(RelativeRotation);
}

bool AUOURewardActor::TryCollectReward(AActor* Collector)
{
	if (bCollected || !IsValidCollector(Collector))
	{
		return false;
	}

	// 이벤트 콜백에서 다시 수집을 요청하더라도 중복 처리되지 않도록 가장 먼저 잠급니다.
	bCollected = true;
	PendingCollector = Collector;
	DisableCollectionInteraction();

	bWaitingForRewardFeedback = RewardFeedbackComponent != nullptr;
	bWaitingForCollectionMotion = RewardCollectionMotionComponent != nullptr;
	BeginRewardFeedback();

	const TArray<FUOURewardPresentationCue> EmptyCueRequests;
	const TArray<FUOURewardPresentationCue>& CueRequests =
		RewardFeedbackComponent != nullptr
			? RewardFeedbackComponent->GetCueRequests()
			: EmptyCueRequests;

	if (bWaitingForCollectionMotion
		&& !RewardCollectionMotionComponent->StartCollectionMotion(
			VisualMesh,
			CollectionMotionPath,
			CueRequests))
	{
		bWaitingForCollectionMotion = false;
	}

	if (!bWaitingForCollectionMotion && RewardFeedbackComponent != nullptr)
	{
		RewardFeedbackComponent->CompleteFeedbackSequence();
	}

	TryCompleteCollection();
	return true;
}

bool AUOURewardActor::IsCollected() const
{
	return bCollected;
}

void AUOURewardActor::HandleCollectionTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	TryCollectReward(OtherActor);
}

void AUOURewardActor::ApplyComponentSettings()
{
	TriggerRadius = FMath::Max(0.0f, TriggerRadius);
	HoverAmplitude = FMath::Max(0.0f, HoverAmplitude);
	HoverSpeed = FMath::Max(0.0f, HoverSpeed);

	if (CollectionTrigger != nullptr)
	{
		CollectionTrigger->SetSphereRadius(TriggerRadius);
	}
}

void AUOURewardActor::DisableCollectionInteraction()
{
	SetActorTickEnabled(false);

	if (CollectionTrigger != nullptr)
	{
		CollectionTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollectionTrigger->SetGenerateOverlapEvents(false);
	}
}

void AUOURewardActor::HideCollectedVisual()
{
	if (VisualMesh != nullptr)
	{
		VisualMesh->SetVisibility(false, true);
		VisualMesh->SetHiddenInGame(true, true);
	}

	if (ObjectiveEffect != nullptr)
	{
		ObjectiveEffect->Deactivate();
		ObjectiveEffect->SetVisibility(false, true);
		ObjectiveEffect->SetHiddenInGame(true, true);
	}
}

void AUOURewardActor::BeginRewardFeedback()
{
	if (!bWaitingForRewardFeedback)
	{
		return;
	}

	AUOUCharacter* PlayerCharacter = Cast<AUOUCharacter>(PendingCollector.Get());
	if (RewardFeedbackComponent == nullptr)
	{
		bWaitingForRewardFeedback = false;
		TryCompleteCollection();
		return;
	}

	if (!RewardFeedbackComponent->BeginFeedback(
			PlayerCharacter,
			GetActorLocation()))
	{
		bWaitingForRewardFeedback = false;
		TryCompleteCollection();
		return;
	}

	TryCompleteCollection();
}

bool AUOURewardActor::RoutePresentationCueToUI(
	const FUOURewardPresentationCue& Cue)
{
	AUOUCharacter* PlayerCharacter = Cast<AUOUCharacter>(PendingCollector.Get());
	APlayerController* PlayerController = PlayerCharacter != nullptr
		? Cast<APlayerController>(PlayerCharacter->GetController())
		: nullptr;
	ULocalPlayer* LocalPlayer = PlayerController != nullptr
		? PlayerController->GetLocalPlayer()
		: nullptr;
	UUOUUISubsystem* UISubsystem = LocalPlayer != nullptr
		? LocalPlayer->GetSubsystem<UUOUUISubsystem>()
		: nullptr;
	if (UISubsystem == nullptr)
	{
		return false;
	}

	FUOURewardPresentationData PresentationData;
	PresentationData.RewardId = RewardId;
	return UISubsystem->ShowRewardPresentationCue(PresentationData, Cue);
}

void AUOURewardActor::TryCompleteCollection()
{
	if (bWaitingForRewardFeedback || bWaitingForCollectionMotion)
	{
		return;
	}

	HideCollectedVisual();
	CompleteCollection();
}

void AUOURewardActor::CompleteCollection()
{
	if (!bCollected || bCollectionCompleted)
	{
		return;
	}

	// 완료 상태를 이벤트보다 먼저 기록하여 UI나 저장 콜백의 재진입에서도 한 번만 발행합니다.
	bCollectionCompleted = true;
	AActor* Collector = PendingCollector.Get();
	PendingCollector = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UUOUPlayerProgressSubsystem* ProgressSubsystem =
				GameInstance->GetSubsystem<UUOUPlayerProgressSubsystem>())
			{
				ProgressSubsystem->RecordRewardCollected(RewardId);
			}
		}
	}

	OnRewardCollected.Broadcast(this, RewardId, Collector);
}

bool AUOURewardActor::IsValidCollector(const AActor* Candidate) const
{
	const AUOUCharacter* PlayerCharacter = Cast<AUOUCharacter>(Candidate);
	return PlayerCharacter != nullptr && PlayerCharacter->IsPlayerControlled();
}

void AUOURewardActor::HandleRewardFeedbackFinished()
{
	bWaitingForRewardFeedback = false;
	TryCompleteCollection();
}

void AUOURewardActor::HandleCollectionMotionFinished()
{
	bWaitingForCollectionMotion = false;
	if (RewardFeedbackComponent != nullptr)
	{
		RewardFeedbackComponent->CompleteFeedbackSequence();
	}
	TryCompleteCollection();
}

void AUOURewardActor::HandleCollectionMotionCue(const FUOURewardPresentationCue& Cue)
{
	switch (Cue.Channel)
	{
	case EUOURewardMotionCueChannel::Feedback:
		if (RewardFeedbackComponent != nullptr)
		{
			RewardFeedbackComponent->ExecuteFeedbackCue(Cue.FeedbackAction);
		}
		break;

	case EUOURewardMotionCueChannel::Presentation:
		{
			RoutePresentationCueToUI(Cue);
			break;
		}
	}
}
