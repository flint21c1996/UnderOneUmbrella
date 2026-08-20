// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUDevelopmentPuzzleCheatGraphTypes.generated.h"

class AActor;
class AUOUPuzzleConditionGroupActor;
class UUOUPuzzleConditionSourceComponent;

// ConditionGroup 입력 중 다른 ConditionGroup의 Result로 생산되지 않는 외부 입력 액터 캐시입니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLADEVTOOLS_API FUOUDevelopmentPuzzleCheatExternalInput
{
	GENERATED_BODY()

	// HUD에서 직접 해결할 외부 퍼즐 입력 액터입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	TObjectPtr<AActor> InputActor = nullptr;

	// 외부 입력 버튼에 표시할 액터 이름입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	FText DisplayName;

	// 이 외부 입력과 연결되어 현재 ConditionGroupComponent가 실제 평가하는 조건 소스 목록입니다.
	// 실제 소유권은 ConditionGroupComponent의 ResolvedConditionSources에 있고 이 배열은 HUD 동작을 위한 약한 참조입니다.
	TArray<TWeakObjectPtr<UUOUPuzzleConditionSourceComponent>> ConditionSources;
};

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

	// PuzzleConditionGroupComponent가 실제 평가 중인 전체 조건 소스입니다.
	// 외부 입력 여부와 관계없이 노드의 조건 상태를 HUD에 표시하기 위한 약한 참조입니다.
	TArray<TWeakObjectPtr<UUOUPuzzleConditionSourceComponent>> ConditionSources;

	// 각 ConditionSource가 제공한 논리 입력 액터 캐시입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	TArray<TObjectPtr<AActor>> InputActors;

	// InputActors 중 선행 ConditionGroup의 Result로 생산되지 않아 HUD에서 직접 해결할 수 있는 입력입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	TArray<FUOUDevelopmentPuzzleCheatExternalInput> ExternalInputs;

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

	// 실제 관계를 만든 Actor 또는 Specific Component입니다. 컴포넌트 단위 관계 판정과 HUD 라벨에 사용합니다.
	TWeakObjectPtr<UObject> RelationObject;
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
