// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOUPlayerProgressSubsystem.h"

#include "Game/UOUPlayerProgressSaveGame.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogUOUPlayerProgress);

namespace
{
constexpr TCHAR DefaultProgressSaveSlotName[] = TEXT("UOUPlayerProgress");
}

void UUOUPlayerProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadProgress();
}

void UUOUPlayerProgressSubsystem::Deinitialize()
{
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
