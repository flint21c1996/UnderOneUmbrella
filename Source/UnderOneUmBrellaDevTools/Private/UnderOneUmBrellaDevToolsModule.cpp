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
		const TArray<FUOUDevelopmentPuzzleCheatStep> Steps = Subsystem.GetPuzzleSteps();
		const TArray<FUOUDevelopmentPuzzleCheatGraphNode> GraphNodes = Subsystem.GetPuzzleGraphNodes();
		const TArray<FUOUDevelopmentPuzzleCheatGraphEdge> GraphEdges = Subsystem.GetPuzzleGraphEdges();
		UE_LOG(
			LogUOUPuzzleCheatConsole,
			Display,
			TEXT("Puzzle cheat status: Running=%s, FirstIncomplete=%d, Steps=%d, Message=%s"),
			Subsystem.IsSequenceRunning() ? TEXT("Yes") : TEXT("No"),
			Subsystem.GetFirstIncompleteStepOrder(),
			Steps.Num(),
			*Subsystem.LastStatusMessage);

		for (const FUOUDevelopmentPuzzleCheatStep& Step : Steps)
		{
			const AUOUPuzzleConditionGroupActor* PuzzleGroup = Step.PuzzleGroup.Get();
			UE_LOG(
				LogUOUPuzzleCheatConsole,
				Display,
				TEXT("  Step=%d, Label=%s, Actor=%s, Satisfied=%s, Delay=%.3fs"),
				Step.StepOrder,
				*Step.DisplayName.ToString(),
				PuzzleGroup != nullptr ? *PuzzleGroup->GetName() : TEXT("None"),
				PuzzleGroup != nullptr && PuzzleGroup->IsSatisfied() ? TEXT("Yes") : TEXT("No"),
				Step.DelayAfterActivationSeconds);
		}

		UE_LOG(
			LogUOUPuzzleCheatConsole,
			Display,
			TEXT("Puzzle graph status: Valid=%s, Nodes=%d, Edges=%d, Message=%s"),
			Subsystem.IsPuzzleGraphValid() ? TEXT("Yes") : TEXT("No"),
			GraphNodes.Num(),
			GraphEdges.Num(),
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

		const bool bSequenceSucceeded = Subsystem->RefreshPuzzleSequence();
		UE_LOG(
			LogUOUPuzzleCheatConsole,
			Display,
			TEXT("Puzzle cheat refresh: Sequence=%s, Graph=%s"),
			bSequenceSucceeded ? TEXT("Succeeded") : TEXT("Failed"),
			Subsystem->IsPuzzleGraphValid() ? TEXT("Succeeded") : TEXT("Failed"));
		LogPuzzleCheatStatus(*Subsystem);
	}

	void ExecuteNext(const TArray<FString>& Args, UWorld* CommandWorld)
	{
		UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = ResolvePuzzleCheatSubsystem(CommandWorld);
		if (Subsystem == nullptr)
		{
			return;
		}

		const bool bAccepted = Subsystem->AdvanceToNextPuzzle();
		UE_LOG(LogUOUPuzzleCheatConsole, Display, TEXT("Puzzle cheat next request: %s, Message=%s"), bAccepted ? TEXT("Accepted") : TEXT("Rejected"), *Subsystem->LastStatusMessage);
	}

	void ExecuteAdvance(const TArray<FString>& Args, UWorld* CommandWorld)
	{
		if (Args.Num() != 1 || !Args[0].IsNumeric())
		{
			UE_LOG(LogUOUPuzzleCheatConsole, Error, TEXT("Usage: uou.PuzzleCheat.Advance <StepOrder>"));
			return;
		}

		UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = ResolvePuzzleCheatSubsystem(CommandWorld);
		if (Subsystem == nullptr)
		{
			return;
		}

		const int32 TargetStepOrder = FCString::Atoi(*Args[0]);
		const bool bAccepted = Subsystem->AdvanceThroughStep(TargetStepOrder);
		UE_LOG(
			LogUOUPuzzleCheatConsole,
			Display,
			TEXT("Puzzle cheat advance request through StepOrder %d: %s, Message=%s"),
			TargetStepOrder,
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

		Subsystem->CancelPendingSequence();
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
		TEXT("현재 PIE 또는 게임 월드에서 태그가 설정된 퍼즐 조건 그룹을 다시 수집합니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteRefresh));

	FAutoConsoleCommandWithWorldAndArgs NextCommand(
		TEXT("uou.PuzzleCheat.Next"),
		TEXT("태그가 설정된 퍼즐 중 첫 번째 미완료 단계를 만족 상태로 진행합니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteNext));

	FAutoConsoleCommandWithWorldAndArgs AdvanceCommand(
		TEXT("uou.PuzzleCheat.Advance"),
		TEXT("지정한 StepOrder까지 태그가 설정된 미완료 퍼즐을 순서대로 진행합니다. 사용법: uou.PuzzleCheat.Advance <StepOrder>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteAdvance));

	FAutoConsoleCommandWithWorldAndArgs CancelCommand(
		TEXT("uou.PuzzleCheat.Cancel"),
		TEXT("이미 완료한 단계는 유지하고 아직 실행되지 않은 퍼즐 치트 예약을 취소합니다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteCancel));

	FAutoConsoleCommandWithWorldAndArgs StatusCommand(
		TEXT("uou.PuzzleCheat.Status"),
		TEXT("수집된 퍼즐 치트 단계와 각 단계의 현재 런타임 상태를 로그에 출력합니다."),
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
