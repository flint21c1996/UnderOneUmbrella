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
	}

	void ExecuteRefresh(const TArray<FString>& Args, UWorld* CommandWorld)
	{
		UUOUDevelopmentPuzzleCheatSubsystem* Subsystem = ResolvePuzzleCheatSubsystem(CommandWorld);
		if (Subsystem == nullptr)
		{
			return;
		}

		const bool bSucceeded = Subsystem->RefreshPuzzleSequence();
		UE_LOG(LogUOUPuzzleCheatConsole, Display, TEXT("Puzzle cheat refresh: %s"), bSucceeded ? TEXT("Succeeded") : TEXT("Failed"));
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

	FAutoConsoleCommandWithWorldAndArgs RefreshCommand(
		TEXT("uou.PuzzleCheat.Refresh"),
		TEXT("Rescans tagged puzzle condition groups in the active PIE or game world."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteRefresh));

	FAutoConsoleCommandWithWorldAndArgs NextCommand(
		TEXT("uou.PuzzleCheat.Next"),
		TEXT("Satisfies the first incomplete tagged puzzle step."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteNext));

	FAutoConsoleCommandWithWorldAndArgs AdvanceCommand(
		TEXT("uou.PuzzleCheat.Advance"),
		TEXT("Satisfies incomplete tagged puzzle steps through the supplied StepOrder."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteAdvance));

	FAutoConsoleCommandWithWorldAndArgs CancelCommand(
		TEXT("uou.PuzzleCheat.Cancel"),
		TEXT("Cancels pending puzzle cheat steps without reverting completed steps."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteCancel));

	FAutoConsoleCommandWithWorldAndArgs StatusCommand(
		TEXT("uou.PuzzleCheat.Status"),
		TEXT("Logs the collected puzzle cheat steps and their runtime state."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteStatus));
}

class FUnderOneUmBrellaDevToolsModule final : public IModuleInterface
{
};

IMPLEMENT_MODULE(FUnderOneUmBrellaDevToolsModule, UnderOneUmBrellaDevTools)
