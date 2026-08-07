// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Rewards/UOURewardActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Game/UOUPlayerProgressSubsystem.h"
#include "Game/UOUStageDefinitionSettings.h"
#include "Game/UOUStageSelectTypes.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "Player/UOUCharacter.h"
#include "UI/UOUUISubsystem.h"
#include "World/Rewards/UOURewardCollectionMotionComponent.h"
#include "World/Rewards/UOURewardFeedbackComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
	const FUOUStageDefinitionRow* FindStageDefinitionForRewardActor(
		const AUOURewardActor* RewardActor,
		int32& OutMatchCount)
	{
		OutMatchCount = 0;
		if (RewardActor == nullptr)
		{
			return nullptr;
		}

		const UUOUStageDefinitionSettings* Settings = GetDefault<UUOUStageDefinitionSettings>();
		UDataTable* StageDefinitionTable = Settings != nullptr
			? Settings->StageDefinitionTable.LoadSynchronous()
			: nullptr;
		if (StageDefinitionTable == nullptr
			|| StageDefinitionTable->GetRowStruct() != FUOUStageDefinitionRow::StaticStruct())
		{
			return nullptr;
		}

		const UWorld* OwningWorld = RewardActor->GetWorld();
		const FString OwningLevelPackageName = OwningWorld != nullptr
			? OwningWorld->GetOutermost()->GetName()
			: FString();
		if (OwningLevelPackageName.IsEmpty())
		{
			return nullptr;
		}

		const FUOUStageDefinitionRow* MatchedRow = nullptr;
		for (const TPair<FName, uint8*>& RowPair : StageDefinitionTable->GetRowMap())
		{
			const FUOUStageDefinitionRow* Row =
				reinterpret_cast<const FUOUStageDefinitionRow*>(RowPair.Value);
			if (Row == nullptr || Row->Level.IsNull())
			{
				continue;
			}

			if (Row->Level.ToSoftObjectPath().GetLongPackageName() == OwningLevelPackageName)
			{
				MatchedRow = Row;
				++OutMatchCount;
			}
		}

		return OutMatchCount == 1 ? MatchedRow : nullptr;
	}
}

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

TArray<FName> AUOURewardActor::GetAvailableRewardIds() const
{
	TArray<FName> AvailableRewardIds;
	AvailableRewardIds.Add(NAME_None);

	int32 MatchingStageCount = 0;
	const FUOUStageDefinitionRow* StageDefinition =
		FindStageDefinitionForRewardActor(this, MatchingStageCount);
	if (StageDefinition == nullptr)
	{
		return AvailableRewardIds;
	}

	for (const FName AvailableRewardId : StageDefinition->RewardIds)
	{
		if (!AvailableRewardId.IsNone())
		{
			AvailableRewardIds.AddUnique(AvailableRewardId);
		}
	}

	return AvailableRewardIds;
}

#if WITH_EDITOR
EDataValidationResult AUOURewardActor::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	bool bIsValid = ParentResult != EDataValidationResult::Invalid;
	auto AddValidationError = [&Context, &bIsValid](const FText& ErrorMessage)
	{
		Context.AddError(ErrorMessage);
		bIsValid = false;
	};

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return ParentResult;
	}

	const UUOUStageDefinitionSettings* Settings = GetDefault<UUOUStageDefinitionSettings>();
	UDataTable* StageDefinitionTable = Settings != nullptr
		? Settings->StageDefinitionTable.LoadSynchronous()
		: nullptr;
	if (StageDefinitionTable == nullptr)
	{
		AddValidationError(FText::FromString(
			TEXT("Project Settings에 Stage Definition DataTable이 설정되지 않았습니다.")));
		return EDataValidationResult::Invalid;
	}

	if (StageDefinitionTable->GetRowStruct() != FUOUStageDefinitionRow::StaticStruct())
	{
		AddValidationError(FText::FromString(
			TEXT("Stage Definition DataTable의 RowStruct가 FUOUStageDefinitionRow가 아닙니다.")));
		return EDataValidationResult::Invalid;
	}

	int32 MatchingStageCount = 0;
	const FUOUStageDefinitionRow* StageDefinition =
		FindStageDefinitionForRewardActor(this, MatchingStageCount);
	if (MatchingStageCount == 0)
	{
		AddValidationError(FText::FromString(
			TEXT("현재 레벨을 Level로 지정한 Stage Definition 행이 없습니다.")));
	}
	else if (MatchingStageCount > 1)
	{
		AddValidationError(FText::FromString(
			TEXT("현재 레벨을 참조하는 Stage Definition 행이 두 개 이상입니다.")));
	}
	else if (RewardId.IsNone())
	{
		AddValidationError(FText::FromString(TEXT("RewardId가 선택되지 않았습니다.")));
	}
	else if (StageDefinition == nullptr || !StageDefinition->RewardIds.Contains(RewardId))
	{
		AddValidationError(FText::Format(
			FText::FromString(TEXT("RewardId '{0}'가 현재 스테이지의 RewardIds에 없습니다.")),
			FText::FromName(RewardId)));
	}

	if (!RewardId.IsNone())
	{
		const UWorld* OwningWorld = GetWorld();
		if (OwningWorld != nullptr)
		{
			for (TActorIterator<AUOURewardActor> RewardIterator(OwningWorld); RewardIterator; ++RewardIterator)
			{
				const AUOURewardActor* OtherReward = *RewardIterator;
				if (OtherReward != this && OtherReward->RewardId == RewardId)
				{
					AddValidationError(FText::Format(
						FText::FromString(TEXT("RewardId '{0}'를 Actor '{1}'도 사용하고 있습니다.")),
						FText::FromName(RewardId),
						FText::FromString(OtherReward->GetActorLabel())));
					break;
				}
			}
		}
	}

	return bIsValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif

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
