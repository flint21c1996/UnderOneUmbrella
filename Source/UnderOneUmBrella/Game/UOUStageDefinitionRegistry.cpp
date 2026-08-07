// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOUStageDefinitionRegistry.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Game/UOUStageDefinitionSettings.h"
#include "Game/UOUStageSelectTypes.h"

UDataTable* FUOUStageDefinitionRegistry::LoadStageDefinitionTable()
{
	const UUOUStageDefinitionSettings* Settings = GetDefault<UUOUStageDefinitionSettings>();
	return Settings != nullptr
		? Settings->StageDefinitionTable.LoadSynchronous()
		: nullptr;
}

bool FUOUStageDefinitionRegistry::HasValidRowStruct(const UDataTable* StageDefinitionTable)
{
	return StageDefinitionTable != nullptr
		&& StageDefinitionTable->GetRowStruct() == FUOUStageDefinitionRow::StaticStruct();
}

const FUOUStageDefinitionRow* FUOUStageDefinitionRegistry::FindStageById(
	const FName StageId,
	const FString& Context)
{
	if (StageId.IsNone())
	{
		return nullptr;
	}

	UDataTable* StageDefinitionTable = LoadStageDefinitionTable();
	if (!HasValidRowStruct(StageDefinitionTable))
	{
		return nullptr;
	}

	return StageDefinitionTable->FindRow<FUOUStageDefinitionRow>(StageId, Context);
}

const FUOUStageDefinitionRow* FUOUStageDefinitionRegistry::FindStageByLevel(
	const UWorld* World,
	int32& OutMatchCount)
{
	OutMatchCount = 0;
	if (World == nullptr)
	{
		return nullptr;
	}

	UDataTable* StageDefinitionTable = LoadStageDefinitionTable();
	if (!HasValidRowStruct(StageDefinitionTable))
	{
		return nullptr;
	}

	const FString LevelPackageName = World->GetOutermost()->GetName();
	if (LevelPackageName.IsEmpty())
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

		if (Row->Level.ToSoftObjectPath().GetLongPackageName() == LevelPackageName)
		{
			MatchedRow = Row;
			++OutMatchCount;
		}
	}

	return OutMatchCount == 1 ? MatchedRow : nullptr;
}

TArray<FName> FUOUStageDefinitionRegistry::GetStageIds()
{
	TArray<FName> StageIds;
	StageIds.Add(NAME_None);

	UDataTable* StageDefinitionTable = LoadStageDefinitionTable();
	if (!HasValidRowStruct(StageDefinitionTable))
	{
		return StageIds;
	}

	for (const FName RowName : StageDefinitionTable->GetRowNames())
	{
		if (!RowName.IsNone())
		{
			StageIds.AddUnique(RowName);
		}
	}

	return StageIds;
}
