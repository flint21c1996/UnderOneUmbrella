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
#include "Game/UOUStageDefinitionRegistry.h"
#include "Game/UOUStageSelectTypes.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "Player/UOUCharacter.h"
#include "Puzzle/Reward/UOURewardCollectedConditionComponent.h"
#include "UI/UOUUISubsystem.h"
#include "World/Rewards/UOURewardCollectionMotionComponent.h"
#include "World/Rewards/UOURewardFeedbackComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

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

	AppearanceMotionPath = CreateDefaultSubobject<USplineComponent>(TEXT("AppearanceMotionPath"));
	AppearanceMotionPath->bEditableWhenInherited = true;
	AppearanceMotionPath->SetupAttachment(RootScene);
	AppearanceMotionPath->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AppearanceMotionPath->ClearSplinePoints(false);
	AppearanceMotionPath->AddSplinePoint(FVector(0.0f, 0.0f, 300.0f), ESplineCoordinateSpace::Local, false);
	AppearanceMotionPath->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, true);
	AppearanceMotionPath->SetClosedLoop(false);
	AppearanceMotionPath->SetHiddenInGame(true);

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
	RewardCollectedConditionComponent =
		CreateDefaultSubobject<UUOURewardCollectedConditionComponent>(TEXT("RewardCollectedConditionComponent"));
	RewardCollectionMotionComponent =
		CreateDefaultSubobject<UUOURewardCollectionMotionComponent>(TEXT("RewardCollectionMotionComponent"));
}

TArray<FName> AUOURewardActor::GetAvailableRewardIds() const
{
	TArray<FName> AvailableRewardIds;
	AvailableRewardIds.Add(NAME_None);

	int32 MatchingStageCount = 0;
	const FUOUStageDefinitionRow* StageDefinition =
		FUOUStageDefinitionRegistry::FindStageByLevel(GetWorld(), MatchingStageCount);
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

	UDataTable* StageDefinitionTable =
		FUOUStageDefinitionRegistry::LoadStageDefinitionTable();
	if (StageDefinitionTable == nullptr)
	{
		AddValidationError(FText::FromString(
			TEXT("Project Settings에 Stage Definition DataTable이 설정되지 않았습니다.")));
		return EDataValidationResult::Invalid;
	}

	if (!FUOUStageDefinitionRegistry::HasValidRowStruct(StageDefinitionTable))
	{
		AddValidationError(FText::FromString(
			TEXT("Stage Definition DataTable의 RowStruct가 FUOUStageDefinitionRow가 아닙니다.")));
		return EDataValidationResult::Invalid;
	}

	int32 MatchingStageCount = 0;
	const FUOUStageDefinitionRow* StageDefinition =
		FUOUStageDefinitionRegistry::FindStageByLevel(GetWorld(), MatchingStageCount);
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
		RewardCollectionMotionComponent->OnAppearanceMotionFinished.AddUniqueDynamic(
			this,
			&AUOURewardActor::HandleAppearanceMotionFinished);
		RewardCollectionMotionComponent->OnAppearanceMotionCue.AddUniqueDynamic(
			this,
			&AUOURewardActor::HandleAppearanceMotionCue);
		RewardCollectionMotionComponent->OnCollectionMotionFinished.AddUniqueDynamic(
			this,
			&AUOURewardActor::HandleCollectionMotionFinished);
		RewardCollectionMotionComponent->OnCollectionMotionCue.AddUniqueDynamic(
			this,
			&AUOURewardActor::HandleCollectionMotionCue);
	}

	// 기본값과 같은 상태도 BeginPlay에서 실제 컴포넌트에 적용되도록 반대 상태에서 전환합니다.
	bRewardActive = !bStartActive;
	SetRewardActive(bStartActive);
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
		RewardCollectionMotionComponent->OnAppearanceMotionFinished.RemoveDynamic(
			this,
			&AUOURewardActor::HandleAppearanceMotionFinished);
		RewardCollectionMotionComponent->OnAppearanceMotionCue.RemoveDynamic(
			this,
			&AUOURewardActor::HandleAppearanceMotionCue);
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

	if (!bRewardActive || bRewardAppearanceInProgress || bCollected || VisualMesh == nullptr)
	{
		return;
	}

	const float ElapsedTime = FMath::Max(0.0f, GetGameTimeSinceCreation() - IdleMotionStartTime);

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
	if (!bRewardActive || bRewardAppearanceInProgress || bCollected || !IsValidCollector(Collector))
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

bool AUOURewardActor::IsCollectionCompleted() const
{
	return bCollectionCompleted;
}

void AUOURewardActor::SetRewardActive(bool bNewActive)
{
	// 수집이 시작된 뒤에는 조건 변화가 수집 연출이나 완료 처리를 되돌리지 않게 합니다.
	if (bCollected || bRewardActive == bNewActive)
	{
		return;
	}

	bRewardActive = bNewActive;
	bRewardAppearanceInProgress = false;
	SetActorTickEnabled(false);

	if (CollectionTrigger != nullptr)
	{
		// 등장 연출이 끝날 때까지 플레이어가 Reward를 먼저 수집하지 못하게 합니다.
		CollectionTrigger->SetGenerateOverlapEvents(false);
		CollectionTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (VisualMesh != nullptr)
	{
		VisualMesh->SetVisibility(bRewardActive, true);
		VisualMesh->SetHiddenInGame(!bRewardActive, true);
	}

	if (ObjectiveEffect != nullptr)
	{
		ObjectiveEffect->SetVisibility(bRewardActive, true);
		ObjectiveEffect->SetHiddenInGame(!bRewardActive, true);
		if (bRewardActive)
		{
			if (!ObjectiveEffect->IsActive())
			{
				ObjectiveEffect->Activate(true);
			}
		}
		else
		{
			ObjectiveEffect->Deactivate();
		}
	}

	if (!bRewardActive)
	{
		bRewardAppearanceInProgress = false;
		bWaitingForAppearanceMotion = false;
		bWaitingForAppearanceFeedback = false;
		if (RewardCollectionMotionComponent != nullptr)
		{
			RewardCollectionMotionComponent->StopAppearanceMotion(true);
		}
		if (RewardFeedbackComponent != nullptr
			&& RewardFeedbackComponent->IsFeedbackPlaying())
		{
			RewardFeedbackComponent->CancelFeedback();
		}
		if (VisualMesh != nullptr)
		{
			// 재활성화 때 이전 idle 흔들림 위치가 새 등장 도착점으로 누적되지 않게 합니다.
			VisualMesh->SetRelativeLocation(BaseVisualRelativeLocation);
			VisualMesh->SetRelativeRotation(BaseVisualRelativeRotation);
		}
		return;
	}

	BeginRewardAppearance();
}

bool AUOURewardActor::IsRewardActive() const
{
	return bRewardActive && !bRewardAppearanceInProgress && !bCollected;
}

void AUOURewardActor::BeginRewardAppearance()
{
	bRewardAppearanceInProgress = true;
	bWaitingForAppearanceMotion = false;
	bWaitingForAppearanceFeedback = false;

	AUOUCharacter* PlayerCharacter = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			PlayerCharacter = Cast<AUOUCharacter>(PlayerController->GetPawn());
		}
	}

	if (RewardFeedbackComponent != nullptr)
	{
		bWaitingForAppearanceFeedback =
			RewardFeedbackComponent->BeginAppearanceFeedback(
				PlayerCharacter,
				GetActorLocation());
	}

	const TArray<FUOURewardPresentationCue> EmptyCueRequests;
	const TArray<FUOURewardPresentationCue>& CueRequests =
		RewardFeedbackComponent != nullptr
			? RewardFeedbackComponent->GetAppearanceCueRequests()
			: EmptyCueRequests;

	bWaitingForAppearanceMotion = RewardCollectionMotionComponent != nullptr;
	if (bWaitingForAppearanceMotion
		&& !RewardCollectionMotionComponent->StartAppearanceMotion(
			VisualMesh,
			AppearanceMotionPath,
			CueRequests))
	{
		bWaitingForAppearanceMotion = false;
	}

	if (!bWaitingForAppearanceMotion
		&& bWaitingForAppearanceFeedback
		&& RewardFeedbackComponent != nullptr)
	{
		RewardFeedbackComponent->CompleteFeedbackSequence();
	}

	TryCompleteRewardAppearance();
}

void AUOURewardActor::TryCompleteRewardAppearance()
{
	if (!bRewardAppearanceInProgress
		|| bWaitingForAppearanceMotion
		|| bWaitingForAppearanceFeedback)
	{
		return;
	}

	CompleteRewardAppearance();
}

void AUOURewardActor::CompleteRewardAppearance()
{
	if (!bRewardActive || bCollected)
	{
		return;
	}

	bRewardAppearanceInProgress = false;
	if (VisualMesh != nullptr)
	{
		// 등장 연출이 VisualMesh의 기준 Transform을 바꿨다면 그 위치에서 idle 움직임을 이어갑니다.
		BaseVisualRelativeLocation = VisualMesh->GetRelativeLocation();
		BaseVisualRelativeRotation = VisualMesh->GetRelativeRotation();
	}
	IdleMotionStartTime = GetGameTimeSinceCreation();

	SetActorTickEnabled(true);
	if (CollectionTrigger != nullptr)
	{
		CollectionTrigger->SetGenerateOverlapEvents(true);
		CollectionTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

bool AUOURewardActor::IsRewardAppearanceInProgress() const
{
	return bRewardAppearanceInProgress;
}

void AUOURewardActor::HandleAppearanceMotionFinished()
{
	bWaitingForAppearanceMotion = false;
	if (bWaitingForAppearanceFeedback && RewardFeedbackComponent != nullptr)
	{
		RewardFeedbackComponent->CompleteFeedbackSequence();
	}
	TryCompleteRewardAppearance();
}

void AUOURewardActor::HandleAppearanceMotionCue(
	const FUOURewardPresentationCue& Cue)
{
	ExecuteMotionCue(Cue);
}

void AUOURewardActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		SetRewardActive(true);
		break;

	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		SetRewardActive(false);
		break;

	case EOUUPuzzleResultAction::Toggle:
		SetRewardActive(!IsRewardActive());
		break;

	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
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
	bRewardActive = false;
	bRewardAppearanceInProgress = false;
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
	if (PlayerController == nullptr && GetWorld() != nullptr)
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}
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
	if (bRewardAppearanceInProgress)
	{
		bWaitingForAppearanceFeedback = false;
		TryCompleteRewardAppearance();
		return;
	}

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
	ExecuteMotionCue(Cue);
}

void AUOURewardActor::ExecuteMotionCue(const FUOURewardPresentationCue& Cue)
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
