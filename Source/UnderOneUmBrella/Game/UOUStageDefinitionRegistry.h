// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDataTable;
class UWorld;
struct FUOUStageDefinitionRow;

/**
 * 프로젝트 설정에 등록된 스테이지 정의 DataTable을 공통 방식으로 조회합니다.
 * 설정값은 소유하지 않으며, Settings에 저장된 테이블을 읽는 책임만 가집니다.
 */
class UNDERONEUMBRELLA_API FUOUStageDefinitionRegistry final
{
public:
	/** Settings에 등록된 DataTable을 동기 로드합니다. 설정되지 않았다면 nullptr을 반환합니다. */
	static UDataTable* LoadStageDefinitionTable();

	/** DataTable이 FUOUStageDefinitionRow 구조를 사용하는지 확인합니다. */
	static bool HasValidRowStruct(const UDataTable* StageDefinitionTable);

	/** StageId와 같은 RowName을 가진 스테이지 정의를 찾습니다. */
	static const FUOUStageDefinitionRow* FindStageById(
		FName StageId,
		const FString& Context);

	/** 현재 레벨을 참조하는 스테이지 정의를 찾고, 일치하는 행의 수를 함께 반환합니다. */
	static const FUOUStageDefinitionRow* FindStageByLevel(
		const UWorld* World,
		int32& OutMatchCount);

	/** Stage Select Node의 선택 목록으로 사용할 모든 StageId를 반환합니다. */
	static TArray<FName> GetStageIds();
};
