// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDevelopmentCheatBuild.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"
#include "UOUDevelopmentPuzzleCheatSubsystem.h"

#if !UOU_WITH_PUZZLE_CHEATS
#error UnderOneUmBrellaDevTools must only be compiled when puzzle cheats are enabled.
#endif

DEFINE_LOG_CATEGORY_STATIC(LogUOUPuzzleCheatConsole, Log, All);

namespace UOUPuzzleCheatConsolePrivate
{
	UUOUDevelopmentPuzzleCheatSubsystem* ResolvePuzzleCheatSubsystem(UWorld* CommandWorld)
	{
		UWorld* GameWorld = CommandWorld != nullptr && CommandWorld->IsGameWorld()
			? CommandWorld
			: nullptr;

		if (GameWorld == nullptr && GEngine != nullptr)
		{
			for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
			{
				UWorld* CandidateWorld = WorldContext.World();
				if (CandidateWorld != nullptr && CandidateWorld->IsGameWorld())
				{
					GameWorld = CandidateWorld;
					break;
				}
			}
		}

		if (GameWorld == nullptr)
		{
			UE_LOG(LogUOUPuzzleCheatConsole, Error, TEXT("Puzzle cheat command requires an active PIE or game world."));
			return nullptr;
		}

		UUOUDevelopmentPuzzleCheatSubsystem* Subsystem =
			GameWorld->GetSubsystem<UUOUDevelopmentPuzzleCheatSubsystem>();
		if (Subsystem == nullptr)
		{
			UE_LOG(LogUOUPuzzleCheatConsole, Error, TEXT("Puzzle cheat subsystem was not available in %s."), *GameWorld->GetName());
		}
		return Subsystem;
	}

	void LogPuzzleCheatStatus(const UUOUDevelopmentPuzzleCheatSubsystem& Subsystem)
	{
		const TArray<FUOUDevelopmentPuzzleCheatGraphNode> GraphNodes = Subsystem.GetPuzzleGraphNodes();
		const TArray<FUOUDevelopmentPuzzleCheatGraphEdge> GraphEdges = Subsystem.GetPuzzleGraphEdges();

		UE_LOG(
			LogUOUPuzzleCheatConsole,
			Display,
			TEXT("Puzzle graph status: Running=%s, Valid=%s, Nodes=%d, Edges=%d, Message=%s, GraphMessage=%s"),
			Subsystem.IsGraphExecutionActive() ? TEXT("Yes") : TEXT("No"),
			Subsystem.IsPuzzleGraphValid() ? TEXT("Yes") : TEXT("No"),
			GraphNodes.Num(),
			GraphEdges.Num(),
			*Subsystem.LastStatusMessage,
			*Subsystem.PuzzleGraphStatusMessage);
		for (const FUOUDevelopmentPuzzleCheatGraphNode& Node : GraphNodes)
		{
			const AUOUPuzzleConditionGroupActor* PuzzleGroup = Node.PuzzleGroup.Get();
			UE_LOG(
				LogUOUPuzzleCheatConsole,
				Display,
				TEXT("  GraphNode=%d, Depth=%d, Label=%s, Actor=%s, Prerequisites=%d, Dependents=%d"),
				Node.NodeIndex,
				Node.ExecutionDepth,
				*Node.DisplayName.ToString(),
				PuzzleGroup != nullptr ? *PuzzleGroup->GetName() : TEXT("None"),
				Node.PrerequisiteNodeIndices.Num(),
				Node.DependentNodeIndices.Num());
		}
		for (const FUOUDevelopmentPuzzleCheatGraphEdge& Edge : GraphEdges)
		{
			const FString SourceName = GraphNodes.IsValidIndex(Edge.SourceNodeIndex)
				? GraphNodes[Edge.SourceNodeIndex].DisplayName.ToString()
				: TEXT("Invalid");
			const FString TargetName = GraphNodes.IsValidIndex(Edge.TargetNodeIndex)
				? GraphNodes[Edge.TargetNodeIndex].DisplayName.ToString()
				: TEXT("Invalid");
			UE_LOG(
				LogUOUPuzzleCheatConsole,
				Display,
				TEXT("  GraphEdge=%d->%d, Source=%s, Target=%s, RelationActor=%s"),
				Edge.SourceNodeIndex,
				Edge.TargetNodeIndex,
				*SourceName,
				*TargetName,
				*GetNameSafe(Edge.RelationActor.Get()));
		}
	}

	void ExecuteRefresh(const TArray<FString>& Args, UWorld* CommandWorld)
	{
		UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = ResolvePuzzleCheatSubsystem(CommandWorld);
		if (Subsystem == nullptr)
		{
			return;
		}

		const bool bGraphSucceeded = Subsystem->RefreshPuzzleGraph();
		UE_LOG(
			LogUOUPuzzleCheatConsole,
			Display,
			TEXT("Puzzle graph refresh: %s"),
			bGraphSucceeded ? TEXT("Succeeded") : TEXT("Failed"));
		LogPuzzleCheatStatus(*Subsystem);
	}

	void ExecuteGraphNode(const TArray<FString>& Args, UWorld* CommandWorld)
	{
		if (Args.Num() != 1 || !Args[0].IsNumeric())
		{
			UE_LOG(LogUOUPuzzleCheatConsole, Error, TEXT("Usage: uou.PuzzleCheat.GraphNode <NodeIndex>"));
			return;
		}

		UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = ResolvePuzzleCheatSubsystem(CommandWorld);
		if (Subsystem == nullptr)
		{
			return;
		}

		const int32 TargetNodeIndex = FCString::Atoi(*Args[0]);
		const bool bAccepted = Subsystem->AdvanceThroughGraphNode(TargetNodeIndex);
		UE_LOG(
			LogUOUPuzzleCheatConsole,
			Display,
			TEXT("Puzzle cheat graph request through node %d: %s, Message=%s"),
			TargetNodeIndex,
			bAccepted ? TEXT("Accepted") : TEXT("Rejected"),
			*Subsystem->LastStatusMessage);
	}

	void ExecuteCancel(const TArray<FString>& Args, UWorld* CommandWorld)
	{
		UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = ResolvePuzzleCheatSubsystem(CommandWorld);
		if (Subsystem == nullptr)
		{
			return;
		}

		Subsystem->CancelGraphExecution();
		UE_LOG(LogUOUPuzzleCheatConsole, Display, TEXT("%s"), *Subsystem->LastStatusMessage);
	}

	void ExecuteStatus(const TArray<FString>& Args, UWorld* CommandWorld)
	{
		const UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = ResolvePuzzleCheatSubsystem(CommandWorld);
		if (Subsystem != nullptr)
		{
			LogPuzzleCheatStatus(*Subsystem);
		}
	}

	void ExecuteToggleHUD(const TArray<FString>& Args, UWorld* CommandWorld)
	{
		UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = ResolvePuzzleCheatSubsystem(CommandWorld);
		if (Subsystem == nullptr)
		{
			return;
		}

		Subsystem->ToggleCheatHUD();
		UE_LOG(
			LogUOUPuzzleCheatConsole,
			Display,
			TEXT("퍼즐 치트 HUD: %s"),
			Subsystem->IsCheatHUDExpanded() ? TEXT("펼침") : TEXT("접힘"));
	}

	FAutoConsoleCommandWithWorldAndArgs RefreshCommand(
		TEXT("uou.PuzzleCheat.Refresh"),
		TEXT("현재 PIE 또는 게임 월드의 Condition/Result 관계 그래프를 다시 수집합니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteRefresh));

	FAutoConsoleCommandWithWorldAndArgs GraphNodeCommand(
		TEXT("uou.PuzzleCheat.GraphNode"),
		TEXT("지정한 그래프 노드의 미완료 선행 노드를 병렬 묶음으로 처리한 뒤 대상 노드까지 진행합니다. 사용법: uou.PuzzleCheat.GraphNode <NodeIndex>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteGraphNode));

	FAutoConsoleCommandWithWorldAndArgs CancelCommand(
		TEXT("uou.PuzzleCheat.Cancel"),
		TEXT("이미 완료된 노드는 유지하고 아직 실행되지 않은 그래프 Wave를 취소합니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteCancel));

	FAutoConsoleCommandWithWorldAndArgs StatusCommand(
		TEXT("uou.PuzzleCheat.Status"),
		TEXT("수집된 퍼즐 관계 그래프와 현재 실행 상태를 로그에 출력합니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteStatus));

	FAutoConsoleCommandWithWorldAndArgs ToggleHUDCommand(
		TEXT("uou.PuzzleCheat.HUD.Toggle"),
		TEXT("퍼즐 치트 HUD 패널을 펼치거나 접습니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteToggleHUD));
}

class FUnderOneUmBrellaDevToolsModule final : public IModuleInterface
{
};

IMPLEMENT_MODULE(FUnderOneUmBrellaDevToolsModule, UnderOneUmBrellaDevTools)
