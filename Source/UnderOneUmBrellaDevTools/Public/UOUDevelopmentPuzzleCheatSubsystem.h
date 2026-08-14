// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "UOUDevelopmentPuzzleCheatGraphTypes.h"
#include "UOUDevelopmentPuzzleCheatSubsystem.generated.h"

class AUOUPuzzleConditionGroupActor;
class SUOUDevelopmentPuzzleCheatHUD;
class UGameViewportClient;
class UUOUPuzzleConditionSourceComponent;

// Development 및 Internal Shipping 빌드에서만 존재하는 퍼즐 관계 그래프 진행 도구입니다.
UCLASS()
class UNDERONEUMBRELLADEVTOOLS_API UUOUDevelopmentPuzzleCheatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// 실제 Condition/Result 참조를 바탕으로 치트 진행용 관계 그래프를 다시 수집하고 검증합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle Cheat|Graph")
	bool RefreshPuzzleGraph();

	UFUNCTION(BlueprintPure, Category = "Puzzle Cheat|Graph")
	TArray<FUOUDevelopmentPuzzleCheatGraphNode> GetPuzzleGraphNodes() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle Cheat|Graph")
	TArray<FUOUDevelopmentPuzzleCheatGraphEdge> GetPuzzleGraphEdges() const;

	const TArray<FUOUDevelopmentPuzzleCheatGraphNode>& GetPuzzleGraphNodesView() const
	{
		return PuzzleGraphNodes;
	}

	const TArray<FUOUDevelopmentPuzzleCheatGraphEdge>& GetPuzzleGraphEdgesView() const
	{
		return PuzzleGraphEdges;
	}

	UFUNCTION(BlueprintPure, Category = "Puzzle Cheat|Graph")
	bool IsPuzzleGraphValid() const { return bPuzzleGraphValid; }

	// 대상 그래프 노드의 미완료 선행 관계를 Wave로 계산해 대상 노드까지 진행합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle Cheat|Graph")
	bool AdvanceThroughGraphNode(int32 TargetNodeIndex);

	UFUNCTION(BlueprintPure, Category = "Puzzle Cheat|Graph")
	bool IsGraphExecutionActive() const { return bGraphExecutionActive; }

	// 지정한 노드가 현재 병렬 실행 Wave에 포함되어 있는지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle Cheat|Graph")
	bool IsGraphNodeActive(int32 NodeIndex) const;

	// 아직 실행되지 않은 그래프 Wave만 취소하고 이미 완료된 노드는 유지합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle Cheat|Graph")
	void CancelGraphExecution();

	void ToggleCheatHUD();
	bool IsCheatHUDExpanded() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Runtime")
	FString LastStatusMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle Cheat|Graph")
	FString PuzzleGraphStatusMessage;

private:
	void CollectConditionDependencyActors(
		AUOUPuzzleConditionGroupActor& PuzzleGroup,
		TArray<AActor*>& OutDependencyActors,
		TMap<AActor*, TArray<UUOUPuzzleConditionSourceComponent*>>& OutConditionSourcesByActor) const;
	void BuildPuzzleGraphConnections();
	void AddPuzzleGraphEdge(int32 SourceNodeIndex, int32 TargetNodeIndex, AActor* RelationActor);
	bool ValidateAndAssignPuzzleGraphDepths();
	bool BuildGraphExecutionWaves(int32 TargetNodeIndex);
	void ExecuteNextGraphWave();
	void CheckCurrentGraphWaveCompletion();
	void ScheduleGraphCompletionCheck();
	bool ArePuzzleGroupResultsCompleted(const AUOUPuzzleConditionGroupActor* PuzzleGroup) const;
	float GetDelayBeforeNextGraphWave(const FUOUDevelopmentPuzzleCheatGraphNode& Node) const;
	void FinishGraphExecution();
	void EnsureCheatHUDCreated();
	void RemoveCheatHUD();

	UPROPERTY(Transient)
	TArray<FUOUDevelopmentPuzzleCheatGraphNode> PuzzleGraphNodes;

	UPROPERTY(Transient)
	TArray<FUOUDevelopmentPuzzleCheatGraphEdge> PuzzleGraphEdges;

	UPROPERTY(Transient)
	bool bPuzzleGraphValid = false;

	UPROPERTY(Transient)
	TArray<FUOUDevelopmentPuzzleCheatGraphExecutionWave> PendingGraphExecutionWaves;

	UPROPERTY(Transient)
	int32 PendingGraphWavePosition = 0;

	UPROPERTY(Transient)
	TArray<FUOUDevelopmentPuzzleCheatActiveGraphNode> ActiveGraphNodes;

	UPROPERTY(Transient)
	int32 ActivatedGraphNodeCount = 0;

	UPROPERTY(Transient)
	bool bGraphExecutionActive = false;

	FTimerHandle GraphCompletionTimerHandle;

	TSharedPtr<SUOUDevelopmentPuzzleCheatHUD> CheatHUDWidget;
	TWeakObjectPtr<UGameViewportClient> CheatHUDViewport;
};
