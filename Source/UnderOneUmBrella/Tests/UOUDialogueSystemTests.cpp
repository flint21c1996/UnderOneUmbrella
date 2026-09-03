// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Puzzle/Dialogue/UOUDialoguePuzzleStateComponent.h"
#include "UI/UOUBubbleConversationComponent.h"
#include "UI/UOUBubbleConversationData.h"
#include "UI/UOUDialogueSourceComponent.h"
#include "UI/UOUDialogueTriggerComponent.h"
#include "UI/UOUUISubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUDialoguePuzzleStateComponentTest,
	"UnderOneUmbrella.UI.Dialogue.PuzzleStateAdapter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUDialoguePuzzleStateComponentTest::RunTest(const FString& Parameters)
{
	UUOUDialogueSourceComponent* DialogueSource = NewObject<UUOUDialogueSourceComponent>();
	UUOUDialogueTriggerComponent* DialogueTrigger = NewObject<UUOUDialogueTriggerComponent>();
	UUOUDialoguePuzzleStateComponent* StateAdapter = NewObject<UUOUDialoguePuzzleStateComponent>();
	TestFalse(
		TEXT("Solved bubble-only behavior is opt-in for existing puzzle state components."),
		StateAdapter->bUseBubbleOnlyWhenSolved);
	TestFalse(
		TEXT("Immediate solved bubble playback is opt-in for existing puzzle state components."),
		StateAdapter->bShowSolvedBubbleImmediately);
	StateAdapter->TargetDialogueSource = DialogueSource;
	StateAdapter->TargetDialogueTrigger = DialogueTrigger;
	StateAdapter->UnsolvedDialogueState = TEXT("BeforePuzzle");
	StateAdapter->SolvedDialogueState = TEXT("AfterPuzzle");
	StateAdapter->bUseBubbleOnlyWhenSolved = true;
	StateAdapter->bShowSolvedBubbleImmediately = true;
	DialogueTrigger->DialogueSource = DialogueSource;

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
	TestFalse(
		TEXT("Solved bubble-only state blocks full dialogue interaction."),
		DialogueTrigger->bDialogueInteractionEnabled);
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
	TestTrue(
		TEXT("Returning to the unsolved phase restores dialogue interaction."),
		DialogueTrigger->bDialogueInteractionEnabled);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUDialoguePuzzleStatePreservesDisabledTriggerTest,
	"UnderOneUmbrella.UI.Dialogue.PuzzleStatePreservesDisabledTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUDialoguePuzzleStatePreservesDisabledTriggerTest::RunTest(const FString& Parameters)
{
	UUOUDialogueSourceComponent* DialogueSource = NewObject<UUOUDialogueSourceComponent>();
	UUOUDialogueTriggerComponent* DialogueTrigger = NewObject<UUOUDialogueTriggerComponent>();
	DialogueTrigger->DialogueSource = DialogueSource;
	DialogueTrigger->bDialogueInteractionEnabled = false;

	UUOUDialoguePuzzleStateComponent* StateAdapter = NewObject<UUOUDialoguePuzzleStateComponent>();
	StateAdapter->TargetDialogueSource = DialogueSource;
	StateAdapter->TargetDialogueTrigger = DialogueTrigger;
	StateAdapter->bUseBubbleOnlyWhenSolved = true;
	StateAdapter->bShowSolvedBubbleImmediately = true;

	IUOUPuzzleResultReceiver::Execute_ApplyPuzzleResult(
		StateAdapter,
		EOUUPuzzleResultAction::Activate);
	TestFalse(
		TEXT("Solving keeps an already-disabled dialogue trigger disabled."),
		DialogueTrigger->bDialogueInteractionEnabled);

	IUOUPuzzleResultReceiver::Execute_ApplyPuzzleResult(
		StateAdapter,
		EOUUPuzzleResultAction::Deactivate);
	TestFalse(
		TEXT("Unsolving restores the trigger's original disabled state."),
		DialogueTrigger->bDialogueInteractionEnabled);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUBubbleOnlyDialoguePlaybackTest,
	"UnderOneUmbrella.UI.Dialogue.BubbleOnlyPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUBubbleOnlyDialoguePlaybackTest::RunTest(const FString& Parameters)
{
	UUOUDialogueSourceComponent* FirstDialogueSource = NewObject<UUOUDialogueSourceComponent>();
	FUOUDialogueLine FirstBubbleLine;
	FirstBubbleLine.BubbleText = FText::FromString(TEXT("First NPC line."));
	FirstBubbleLine.BubbleDuration = 1.5f;
	FirstDialogueSource->InlineLines.Add(FirstBubbleLine);

	UUOUDialogueSourceComponent* SecondDialogueSource = NewObject<UUOUDialogueSourceComponent>();
	FUOUDialogueLine SecondBubbleLine;
	SecondBubbleLine.BubbleText = FText::FromString(TEXT("Second NPC line."));
	SecondBubbleLine.BubbleDuration = 2.0f;
	SecondDialogueSource->InlineLines.Add(SecondBubbleLine);

	UUOUUISubsystem* UISubsystem = NewObject<UUOUUISubsystem>();
	TestTrue(
		TEXT("Bubble-only playback accepts the first source."),
		UISubsystem->StartBubbleOnlyDialogue(FirstDialogueSource));
	TestTrue(
		TEXT("Bubble-only playback accepts a second source without replacing the first."),
		UISubsystem->StartBubbleOnlyDialogue(SecondDialogueSource));
	TestTrue(
		TEXT("The first source remains active after the second source starts."),
		UISubsystem->IsBubbleOnlyDialoguePlayingForSource(FirstDialogueSource));
	TestTrue(
		TEXT("The second source has an independent playback state."),
		UISubsystem->IsBubbleOnlyDialoguePlayingForSource(SecondDialogueSource));
	TestFalse(
		TEXT("Bubble-only playback does not start the full dialogue state."),
		UISubsystem->IsDialoguePlaying());
	TestFalse(
		TEXT("The first bubble-only source does not consume its normal playback record."),
		FirstDialogueSource->bHasPlayed);
	TestFalse(
		TEXT("The second bubble-only source does not consume its normal playback record."),
		SecondDialogueSource->bHasPlayed);

	UISubsystem->StopBubbleOnlyDialogueForSource(FirstDialogueSource);
	TestFalse(
		TEXT("Stopping one source clears only that source."),
		UISubsystem->IsBubbleOnlyDialoguePlayingForSource(FirstDialogueSource));
	TestTrue(
		TEXT("Stopping the first source leaves the second source active."),
		UISubsystem->IsBubbleOnlyDialoguePlayingForSource(SecondDialogueSource));

	UISubsystem->StopBubbleOnlyDialogue();
	TestFalse(
		TEXT("Stopping all bubble-only playback clears every source."),
		UISubsystem->IsBubbleOnlyDialoguePlaying());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUBubbleConversationConfigurationTest,
	"UnderOneUmbrella.UI.Dialogue.BubbleConversationConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUBubbleConversationConfigurationTest::RunTest(const FString& Parameters)
{
	UUOUBubbleConversationComponent* Conversation = NewObject<UUOUBubbleConversationComponent>();
	FUOUBubbleConversationLine InlineLine;
	InlineLine.SpeakerId = TEXT("NPC_A");
	InlineLine.BubbleText = FText::FromString(TEXT("Inline line."));
	Conversation->InlineLines.Add(InlineLine);

	TestEqual(
		TEXT("A conversation uses inline lines when no reusable data asset is assigned."),
		Conversation->GetConversationLineCount(),
		1);

	UUOUBubbleConversationData* ConversationData = NewObject<UUOUBubbleConversationData>();
	FUOUBubbleConversationLine FirstDataLine;
	FirstDataLine.SpeakerId = TEXT("NPC_A");
	FirstDataLine.BubbleText = FText::FromString(TEXT("First data line."));
	ConversationData->Lines.Add(FirstDataLine);
	FUOUBubbleConversationLine SecondDataLine;
	SecondDataLine.SpeakerId = TEXT("NPC_B");
	SecondDataLine.BubbleText = FText::FromString(TEXT("Second data line."));
	ConversationData->Lines.Add(SecondDataLine);
	Conversation->ConversationData = ConversationData;

	TestTrue(
		TEXT("A reusable conversation data asset recognizes configured bubble lines."),
		ConversationData->HasValidLines());
	TestEqual(
		TEXT("A valid conversation data asset takes precedence over inline lines."),
		Conversation->GetConversationLineCount(),
		2);

	IUOUPuzzleResultReceiver::Execute_ApplyPuzzleResult(
		Conversation,
		EOUUPuzzleResultAction::Deactivate);
	TestTrue(
		TEXT("Deactivate immediately reports its conversation result as completed."),
		IUOUPuzzleResultCompletionState::Execute_IsPuzzleResultCompleted(
			Conversation,
			EOUUPuzzleResultAction::Deactivate));
	TestFalse(
		TEXT("Deactivate clears the previous Activate completion state."),
		IUOUPuzzleResultCompletionState::Execute_IsPuzzleResultCompleted(
			Conversation,
			EOUUPuzzleResultAction::Activate));

	return true;
}

#endif
