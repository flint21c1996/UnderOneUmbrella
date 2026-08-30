// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Puzzle/Dialogue/UOUDialoguePuzzleStateComponent.h"
#include "UI/UOUDialogueSourceComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUDialoguePuzzleStateComponentTest,
	"UnderOneUmbrella.UI.Dialogue.PuzzleStateAdapter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUDialoguePuzzleStateComponentTest::RunTest(const FString& Parameters)
{
	UUOUDialogueSourceComponent* DialogueSource = NewObject<UUOUDialogueSourceComponent>();
	UUOUDialoguePuzzleStateComponent* StateAdapter = NewObject<UUOUDialoguePuzzleStateComponent>();
	StateAdapter->TargetDialogueSource = DialogueSource;
	StateAdapter->UnsolvedDialogueState = TEXT("BeforePuzzle");
	StateAdapter->SolvedDialogueState = TEXT("AfterPuzzle");

	DialogueSource->bHasPlayed = true;
	IUOUPuzzleResultReceiver::Execute_ApplyPuzzleResult(
		StateAdapter,
		EOUUPuzzleResultAction::Activate);

	TestTrue(TEXT("Activate marks the dialogue puzzle as solved."), StateAdapter->IsPuzzleSolved());
	TestEqual(
		TEXT("Activate selects the solved dialogue state."),
		DialogueSource->GetDialogueState(),
		FName(TEXT("AfterPuzzle")));
	TestFalse(
		TEXT("Changing puzzle phase resets one-shot dialogue playback."),
		DialogueSource->bHasPlayed);
	TestEqual(
		TEXT("A unified-table proximity bubble can use the dialogue state as its style fallback."),
		DialogueSource->GetProximityBubbleStyle(),
		FName(TEXT("AfterPuzzle")));

	DialogueSource->bHasPlayed = true;
	IUOUPuzzleResultReceiver::Execute_ApplyPuzzleResult(
		StateAdapter,
		EOUUPuzzleResultAction::Deactivate);

	TestFalse(TEXT("Deactivate marks the dialogue puzzle as unsolved."), StateAdapter->IsPuzzleSolved());
	TestEqual(
		TEXT("Deactivate restores the unsolved dialogue state."),
		DialogueSource->GetDialogueState(),
		FName(TEXT("BeforePuzzle")));
	TestFalse(
		TEXT("Returning to the unsolved phase also resets playback."),
		DialogueSource->bHasPlayed);

	return true;
}

#endif
