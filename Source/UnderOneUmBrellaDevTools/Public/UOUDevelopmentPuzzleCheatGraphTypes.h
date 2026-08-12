// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUDevelopmentPuzzleCheatGraphTypes.generated.h"

class AActor;
class AUOUPuzzleConditionGroupActor;

// ConditionGroup 하나와 그래프에서 계산된 선행/후속 관계를 보관하는 런타임 캐시입니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLADEVTOOLS_API FUOUDevelopmentPuzzleCheatGraphNode
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	int32 NodeIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	TObjectPtr<AUOUPuzzleConditionGroupActor> PuzzleGroup = nullptr;

	// ConditionActors와 ConditionSource 참조에서 해석한 실제 입력 액터 캐시입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	TArray<TObjectPtr<AActor>> InputActors;

	// 만족 시 실행되는 유효한 Result 대상 액터 캐시입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	TArray<TObjectPtr<AActor>> ResultActors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	TArray<int32> PrerequisiteNodeIndices;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	TArray<int32> DependentNodeIndices;

	// 같은 깊이의 노드들은 서로의 완료를 요구하지 않으므로 이후 병렬 실행 묶음이 될 수 있습니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	int32 ExecutionDepth = INDEX_NONE;
};

// 두 ConditionGroup 사이의 방향과 그 관계를 만든 중간 액터를 기록합니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLADEVTOOLS_API FUOUDevelopmentPuzzleCheatGraphEdge
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	int32 SourceNodeIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	int32 TargetNodeIndex = INDEX_NONE;

	// Source의 Result이면서 Target의 Condition으로 사용되어 두 그룹을 연결한 액터입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	TObjectPtr<AActor> RelationActor = nullptr;
};

// 대상 노드까지 진행할 때 같은 깊이에서 함께 활성화할 노드 묶음입니다.
USTRUCT()
struct UNDERONEUMBRELLADEVTOOLS_API FUOUDevelopmentPuzzleCheatGraphExecutionWave
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	int32 ExecutionDepth = INDEX_NONE;

	UPROPERTY(Transient)
	TArray<int32> NodeIndices;
};

// 현재 병렬 묶음에서 결과 완료를 기다리는 노드별 시간 상태입니다.
USTRUCT()
struct UNDERONEUMBRELLADEVTOOLS_API FUOUDevelopmentPuzzleCheatActiveGraphNode
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	int32 NodeIndex = INDEX_NONE;

	UPROPERTY(Transient)
	double StartTimeSeconds = 0.0;

	UPROPERTY(Transient)
	float MinimumWaitSeconds = 0.0f;
};
