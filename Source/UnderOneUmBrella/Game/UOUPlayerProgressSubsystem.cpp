// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOUPlayerProgressSubsystem.h"

#include "Game/UOUPlayerProgressSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY(LogUOUPlayerProgress);

namespace
{
constexpr TCHAR DefaultProgressSaveSlotName[] = TEXT("UOUPlayerProgress");
}

void UUOUPlayerProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadProgress();
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UUOUPlayerProgressSubsystem::HandlePostLoadMapWithWorld);
}

void UUOUPlayerProgressSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	ClearStageAttempt();
	ProgressSave = nullptr;

	Super::Deinitialize();
}

void UUOUPlayerProgressSubsystem::GetStageRewardCounts(
	const FName StageId,
	const TArray<FName>& ValidRewardIds,
	int32& OutCollectedRewardCount,
	int32& OutTotalRewardCount,
	int32& OutMissingRewardCount) const
{
	OutCollectedRewardCount = 0;
	OutTotalRewardCount = ValidRewardIds.Num();
	OutMissingRewardCount = OutTotalRewardCount;

	const FUOUStageProgressRecord* StageProgress = FindStageProgress(StageId);
	if (StageProgress == nullptr)
	{
		return;
	}

	TSet<FName> CountedRewardIds;
	for (const FName CollectedRewardId : StageProgress->CollectedRewardIds)
	{
		if (!CollectedRewardId.IsNone() && ValidRewardIds.Contains(CollectedRewardId))
		{
			CountedRewardIds.Add(CollectedRewardId);
		}
	}

	OutCollectedRewardCount = FMath::Min(CountedRewardIds.Num(), OutTotalRewardCount);
	OutMissingRewardCount = FMath::Max(0, OutTotalRewardCount - OutCollectedRewardCount);
}

bool UUOUPlayerProgressSubsystem::IsStageCompleted(const FName StageId) const
{
	const FUOUStageProgressRecord* StageProgress = FindStageProgress(StageId);
	return StageProgress != nullptr && StageProgress->bCompleted;
}

bool UUOUPlayerProgressSubsystem::BeginStageAttempt(
	const FName StageId,
	const TSoftObjectPtr<UWorld> StageLevel,
	const TArray<FName>& ValidRewardIds)
{
	if (StageId.IsNone() || StageLevel.IsNull())
	{
		UE_LOG(
			LogUOUPlayerProgress,
			Warning,
			TEXT("Cannot begin a stage attempt without a valid StageId and level."));
		return false;
	}

	const FString StagePackageName = StageLevel.ToSoftObjectPath().GetLongPackageName();
	const FName StageLevelName(*FPackageName::GetShortName(StagePackageName));
	if (StageLevelName.IsNone())
	{
		return false;
	}

	TSet<FName> ValidRewardIdSet;
	for (const FName RewardId : ValidRewardIds)
	{
		if (!RewardId.IsNone())
		{
			ValidRewardIdSet.Add(RewardId);
		}
	}

	ActiveStageId = StageId;
	ActiveStageLevelName = StageLevelName;
	ExpectedRewardIds = MoveTemp(ValidRewardIdSet);
	PendingRewardIds.Reset();
	return true;
}

bool UUOUPlayerProgressSubsystem::RecordRewardCollected(const FName RewardId)
{
	if (ActiveStageId.IsNone() || RewardId.IsNone())
	{
		return false;
	}

	if (!ExpectedRewardIds.Contains(RewardId))
	{
		UE_LOG(
			LogUOUPlayerProgress,
			Warning,
			TEXT("RewardId '%s' is not registered for active stage '%s'."),
			*RewardId.ToString(),
			*ActiveStageId.ToString());
		return false;
	}

	if (PendingRewardIds.Contains(RewardId))
	{
		return false;
	}

	PendingRewardIds.Add(RewardId);
	return true;
}

bool UUOUPlayerProgressSubsystem::CommitCurrentStage()
{
	if (ActiveStageId.IsNone())
	{
		UE_LOG(LogUOUPlayerProgress, Warning, TEXT("Cannot commit progress without an active stage."));
		return false;
	}

	if (ProgressSave == nullptr)
	{
		ProgressSave = CreateDefaultProgress();
	}

	if (ProgressSave == nullptr)
	{
		return false;
	}

	const bool bHadExistingProgress = ProgressSave->StageProgress.Contains(ActiveStageId);
	FUOUStageProgressRecord PreviousProgress;
	if (bHadExistingProgress)
	{
		PreviousProgress = ProgressSave->StageProgress.FindChecked(ActiveStageId);
	}

	FUOUStageProgressRecord& StageProgress = ProgressSave->StageProgress.FindOrAdd(ActiveStageId);
	TArray<FName> MergedRewardIds;
	MergedRewardIds.Reserve(StageProgress.CollectedRewardIds.Num() + PendingRewardIds.Num());
	for (const FName CollectedRewardId : StageProgress.CollectedRewardIds)
	{
		if (!CollectedRewardId.IsNone())
		{
			MergedRewardIds.AddUnique(CollectedRewardId);
		}
	}

	for (const FName PendingRewardId : PendingRewardIds)
	{
		if (!PendingRewardId.IsNone() && ExpectedRewardIds.Contains(PendingRewardId))
		{
			MergedRewardIds.AddUnique(PendingRewardId);
		}
	}

	StageProgress.CollectedRewardIds = MoveTemp(MergedRewardIds);
	StageProgress.bCompleted = true;

	if (!SaveProgress())
	{
		if (bHadExistingProgress)
		{
			ProgressSave->StageProgress.FindOrAdd(ActiveStageId) = MoveTemp(PreviousProgress);
		}
		else
		{
			ProgressSave->StageProgress.Remove(ActiveStageId);
		}
		return false;
	}

	PendingRewardIds.Reset();
	return true;
}

bool UUOUPlayerProgressSubsystem::SaveProgress()
{
	if (ProgressSave == nullptr)
	{
		ProgressSave = CreateDefaultProgress();
	}

	if (ProgressSave == nullptr)
	{
		return false;
	}

	if (SaveSlotName.IsEmpty())
	{
		SaveSlotName = DefaultProgressSaveSlotName;
	}

	if (!UGameplayStatics::SaveGameToSlot(ProgressSave, SaveSlotName, SaveUserIndex))
	{
		UE_LOG(
			LogUOUPlayerProgress,
			Warning,
			TEXT("Failed to save player progress to slot '%s'."),
			*SaveSlotName);
		return false;
	}

	return true;
}

void UUOUPlayerProgressSubsystem::ReloadProgress()
{
	LoadProgress();
}

UUOUPlayerProgressSaveGame* UUOUPlayerProgressSubsystem::CreateDefaultProgress() const
{
	return Cast<UUOUPlayerProgressSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UUOUPlayerProgressSaveGame::StaticClass()));
}

void UUOUPlayerProgressSubsystem::LoadProgress()
{
	if (SaveSlotName.IsEmpty())
	{
		SaveSlotName = DefaultProgressSaveSlotName;
	}

	ProgressSave = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		ProgressSave = Cast<UUOUPlayerProgressSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
		if (ProgressSave == nullptr)
		{
			UE_LOG(
				LogUOUPlayerProgress,
				Warning,
				TEXT("Player progress slot '%s' had an unexpected type. Defaults will be used."),
				*SaveSlotName);
		}
	}

	if (ProgressSave == nullptr)
	{
		ProgressSave = CreateDefaultProgress();
	}
}

const FUOUStageProgressRecord* UUOUPlayerProgressSubsystem::FindStageProgress(const FName StageId) const
{
	if (ProgressSave == nullptr || StageId.IsNone())
	{
		return nullptr;
	}

	return ProgressSave->StageProgress.Find(StageId);
}

void UUOUPlayerProgressSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (LoadedWorld == nullptr || !LoadedWorld->IsGameWorld())
	{
		return;
	}

	PendingRewardIds.Reset();
	if (ActiveStageId.IsNone())
	{
		return;
	}

	const FString LoadedMapName = UWorld::RemovePIEPrefix(LoadedWorld->GetMapName());
	const FName LoadedLevelName(*FPackageName::GetShortName(LoadedMapName));
	if (LoadedLevelName != ActiveStageLevelName)
	{
		ClearStageAttempt();
	}
}

void UUOUPlayerProgressSubsystem::ClearStageAttempt()
{
	ActiveStageId = NAME_None;
	ActiveStageLevelName = NAME_None;
	ExpectedRewardIds.Reset();
	PendingRewardIds.Reset();
}
