// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentPuzzleCheatSubsystem.h"

#include "Debug/UOUDevelopmentCheatBuild.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"
#include "Widgets/SUOUDevelopmentPuzzleCheatHUD.h"

#if !UOU_WITH_PUZZLE_CHEATS
#error UOUDevelopmentPuzzleCheatSubsystem must only be compiled when puzzle cheats are enabled.
#endif

namespace UOUDevelopmentPuzzleCheatPrivate
{
	constexpr TCHAR StepTagPrefix[] = TEXT("UOU.PuzzleCheat.Step.");
	constexpr TCHAR DelayTagPrefix[] = TEXT("UOU.PuzzleCheat.DelayMs.");
	constexpr TCHAR LabelTagPrefix[] = TEXT("UOU.PuzzleCheat.Label.");
	constexpr float DefaultDelayAfterActivationSeconds = 0.25f;

	bool TryParseIntegerTag(const AActor& Actor, const TCHAR* Prefix, int32& OutValue)
	{
		const FString PrefixString(Prefix);
		for (const FName Tag : Actor.Tags)
		{
			const FString TagString = Tag.ToString();
			if (!TagString.StartsWith(PrefixString, ESearchCase::CaseSensitive))
			{
				continue;
			}

			const FString ValueString = TagString.RightChop(PrefixString.Len());
			if (ValueString.IsNumeric())
			{
				OutValue = FCString::Atoi(*ValueString);
				return true;
			}
		}

		return false;
	}

	FText ResolveDisplayName(const AActor& Actor)
	{
		const FString PrefixString(LabelTagPrefix);
		for (const FName Tag : Actor.Tags)
		{
			const FString TagString = Tag.ToString();
			if (!TagString.StartsWith(PrefixString, ESearchCase::CaseSensitive))
			{
				continue;
			}

			FString Label = TagString.RightChop(PrefixString.Len());
			Label.ReplaceInline(TEXT("_"), TEXT(" "));
			if (!Label.IsEmpty())
			{
				return FText::FromString(Label);
			}
		}

#if WITH_EDITOR
		return FText::FromString(Actor.GetActorLabel());
#else
		return FText::FromString(Actor.GetName());
#endif
	}
}

bool UUOUDevelopmentPuzzleCheatSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr && World->IsGameWorld();
}

void UUOUDevelopmentPuzzleCheatSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	RefreshPuzzleSequence();
}

void UUOUDevelopmentPuzzleCheatSubsystem::Deinitialize()
{
	RemoveCheatHUD();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceTimerHandle);
	}

	PuzzleSteps.Reset();
	PendingStepIndices.Reset();
	PendingQueuePosition = 0;
	bSequenceRunning = false;
	bPuzzleSequenceValid = false;
	Super::Deinitialize();
}

bool UUOUDevelopmentPuzzleCheatSubsystem::RefreshPuzzleSequence()
{
	if (bSequenceRunning)
	{
		LastStatusMessage = TEXT("Cannot refresh the puzzle cheat sequence while it is running.");
		return false;
	}

	PuzzleSteps.Reset();
	bPuzzleSequenceValid = false;

	UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld())
	{
		LastStatusMessage = TEXT("Puzzle cheat sequence refresh failed because no game world was available.");
		return false;
	}

	for (TActorIterator<AUOUPuzzleConditionGroupActor> It(World); It; ++It)
	{
		AUOUPuzzleConditionGroupActor* PuzzleGroup = *It;
		if (!IsValid(PuzzleGroup))
		{
			continue;
		}

		int32 StepOrder = INDEX_NONE;
		if (!UOUDevelopmentPuzzleCheatPrivate::TryParseIntegerTag(
				*PuzzleGroup,
				UOUDevelopmentPuzzleCheatPrivate::StepTagPrefix,
				StepOrder))
		{
			continue;
		}

		int32 DelayMilliseconds = FMath::RoundToInt(
			UOUDevelopmentPuzzleCheatPrivate::DefaultDelayAfterActivationSeconds * 1000.0f);
		UOUDevelopmentPuzzleCheatPrivate::TryParseIntegerTag(
			*PuzzleGroup,
			UOUDevelopmentPuzzleCheatPrivate::DelayTagPrefix,
			DelayMilliseconds);

		FUOUDevelopmentPuzzleCheatStep& Step = PuzzleSteps.AddDefaulted_GetRef();
		Step.StepOrder = StepOrder;
		Step.DisplayName = UOUDevelopmentPuzzleCheatPrivate::ResolveDisplayName(*PuzzleGroup);
		Step.PuzzleGroup = PuzzleGroup;
		Step.DelayAfterActivationSeconds = FMath::Max(0, DelayMilliseconds) / 1000.0f;
	}

	PuzzleSteps.Sort(
		[](const FUOUDevelopmentPuzzleCheatStep& Left, const FUOUDevelopmentPuzzleCheatStep& Right)
		{
			if (Left.StepOrder != Right.StepOrder)
			{
				return Left.StepOrder < Right.StepOrder;
			}

			const FString LeftName = Left.PuzzleGroup != nullptr ? Left.PuzzleGroup->GetName() : FString();
			const FString RightName = Right.PuzzleGroup != nullptr ? Right.PuzzleGroup->GetName() : FString();
			return LeftName < RightName;
		});

	int32 DuplicateOrderCount = 0;
	for (int32 StepIndex = 1; StepIndex < PuzzleSteps.Num(); ++StepIndex)
	{
		if (PuzzleSteps[StepIndex - 1].StepOrder == PuzzleSteps[StepIndex].StepOrder)
		{
			++DuplicateOrderCount;
		}
	}

	bPuzzleSequenceValid = PuzzleSteps.Num() > 0 && DuplicateOrderCount == 0;
	LastStatusMessage = FString::Printf(
		TEXT("Collected %d puzzle cheat step(s); duplicate step orders: %d."),
		PuzzleSteps.Num(),
		DuplicateOrderCount);
	if (!PuzzleSteps.IsEmpty())
	{
		EnsureCheatHUDCreated();
	}
	return bPuzzleSequenceValid;
}

bool UUOUDevelopmentPuzzleCheatSubsystem::AdvanceToNextPuzzle()
{
	if (!bPuzzleSequenceValid && !RefreshPuzzleSequence())
	{
		return false;
	}

	const int32 FirstIncompleteStepOrder = GetFirstIncompleteStepOrder();
	if (FirstIncompleteStepOrder == INDEX_NONE)
	{
		LastStatusMessage = TEXT("All tagged puzzle cheat steps are already satisfied.");
		return false;
	}

	return AdvanceThroughStep(FirstIncompleteStepOrder);
}

bool UUOUDevelopmentPuzzleCheatSubsystem::AdvanceThroughStep(int32 TargetStepOrder)
{
	if (bSequenceRunning)
	{
		LastStatusMessage = TEXT("A puzzle cheat sequence is already running.");
		return false;
	}

	if (!bPuzzleSequenceValid && !RefreshPuzzleSequence())
	{
		return false;
	}

	if (!BuildActivationQueue(TargetStepOrder))
	{
		return false;
	}

	if (PendingStepIndices.IsEmpty())
	{
		LastStatusMessage = FString::Printf(
			TEXT("Puzzle steps through StepOrder %d are already satisfied."),
			TargetStepOrder);
		return true;
	}

	PendingQueuePosition = 0;
	bSequenceRunning = true;
	LastStatusMessage = FString::Printf(
		TEXT("Starting puzzle cheat sequence through StepOrder %d (%d pending step(s))."),
		TargetStepOrder,
		PendingStepIndices.Num());
	ExecuteNextQueuedStep();
	return true;
}

void UUOUDevelopmentPuzzleCheatSubsystem::CancelPendingSequence()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceTimerHandle);
	}

	const bool bHadPendingSequence = bSequenceRunning || !PendingStepIndices.IsEmpty();
	PendingStepIndices.Reset();
	PendingQueuePosition = 0;
	bSequenceRunning = false;
	LastStatusMessage = bHadPendingSequence
		? TEXT("Pending puzzle cheat sequence cancelled. Already activated steps were kept.")
		: TEXT("No puzzle cheat sequence was running.");
}

int32 UUOUDevelopmentPuzzleCheatSubsystem::GetFirstIncompleteStepOrder() const
{
	if (!bPuzzleSequenceValid)
	{
		return INDEX_NONE;
	}

	for (const FUOUDevelopmentPuzzleCheatStep& Step : PuzzleSteps)
	{
		const AUOUPuzzleConditionGroupActor* PuzzleGroup = Step.PuzzleGroup.Get();
		if (PuzzleGroup == nullptr || !PuzzleGroup->IsSatisfied())
		{
			return Step.StepOrder;
		}
	}

	return INDEX_NONE;
}

TArray<FUOUDevelopmentPuzzleCheatStep> UUOUDevelopmentPuzzleCheatSubsystem::GetPuzzleSteps() const
{
	return PuzzleSteps;
}

void UUOUDevelopmentPuzzleCheatSubsystem::ToggleCheatHUD()
{
	EnsureCheatHUDCreated();
	if (CheatHUDWidget.IsValid())
	{
		CheatHUDWidget->TogglePanel();
	}
}

bool UUOUDevelopmentPuzzleCheatSubsystem::IsCheatHUDExpanded() const
{
	return CheatHUDWidget.IsValid() && CheatHUDWidget->IsPanelExpanded();
}

bool UUOUDevelopmentPuzzleCheatSubsystem::BuildActivationQueue(int32 TargetStepOrder)
{
	PendingStepIndices.Reset();

	const int32 TargetStepIndex = PuzzleSteps.IndexOfByPredicate(
		[TargetStepOrder](const FUOUDevelopmentPuzzleCheatStep& Step)
		{
			return Step.StepOrder == TargetStepOrder;
		});
	if (TargetStepIndex == INDEX_NONE)
	{
		LastStatusMessage = FString::Printf(
			TEXT("Puzzle cheat StepOrder %d was not found."),
			TargetStepOrder);
		return false;
	}

	for (int32 StepIndex = 0; StepIndex <= TargetStepIndex; ++StepIndex)
	{
		AUOUPuzzleConditionGroupActor* PuzzleGroup = PuzzleSteps[StepIndex].PuzzleGroup.Get();
		if (PuzzleGroup == nullptr)
		{
			PendingStepIndices.Reset();
			LastStatusMessage = FString::Printf(
				TEXT("Cannot build puzzle cheat sequence: StepOrder %d has no PuzzleGroup."),
				PuzzleSteps[StepIndex].StepOrder);
			return false;
		}

		if (!PuzzleGroup->IsSatisfied())
		{
			PendingStepIndices.Add(StepIndex);
		}
	}

	return true;
}

void UUOUDevelopmentPuzzleCheatSubsystem::ExecuteNextQueuedStep()
{
	if (!bSequenceRunning || !PendingStepIndices.IsValidIndex(PendingQueuePosition))
	{
		FinishSequence();
		return;
	}

	const int32 StepIndex = PendingStepIndices[PendingQueuePosition];
	if (!PuzzleSteps.IsValidIndex(StepIndex))
	{
		CancelPendingSequence();
		LastStatusMessage = FString::Printf(TEXT("Puzzle cheat sequence lost array index %d."), StepIndex);
		return;
	}

	const FUOUDevelopmentPuzzleCheatStep& Step = PuzzleSteps[StepIndex];
	AUOUPuzzleConditionGroupActor* PuzzleGroup = Step.PuzzleGroup.Get();
	if (PuzzleGroup == nullptr || !PuzzleGroup->ForceSatisfiedForCheat())
	{
		CancelPendingSequence();
		LastStatusMessage = FString::Printf(
			TEXT("Failed to satisfy puzzle cheat step %d (%s)."),
			Step.StepOrder,
			*Step.DisplayName.ToString());
		return;
	}

	LastStatusMessage = FString::Printf(
		TEXT("Activated puzzle cheat step %d: %s"),
		Step.StepOrder,
		*Step.DisplayName.ToString());
	++PendingQueuePosition;

	if (PendingQueuePosition >= PendingStepIndices.Num())
	{
		FinishSequence();
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		CancelPendingSequence();
		LastStatusMessage = TEXT("Puzzle cheat sequence stopped because its world was unavailable.");
		return;
	}

	World->GetTimerManager().SetTimer(
		SequenceTimerHandle,
		this,
		&UUOUDevelopmentPuzzleCheatSubsystem::ExecuteNextQueuedStep,
		FMath::Max(GetDelayBeforeNextStep(Step), KINDA_SMALL_NUMBER),
		false);
}

float UUOUDevelopmentPuzzleCheatSubsystem::GetDelayBeforeNextStep(
	const FUOUDevelopmentPuzzleCheatStep& Step) const
{
	float LongestResultDelay = 0.0f;
	if (const AUOUPuzzleConditionGroupActor* PuzzleGroup = Step.PuzzleGroup.Get())
	{
		for (const FOUUPuzzleResultBinding& Binding : PuzzleGroup->ResultBindings)
		{
			LongestResultDelay = FMath::Max(LongestResultDelay, Binding.SatisfiedDelaySeconds);
		}
	}

	return FMath::Max(Step.DelayAfterActivationSeconds, LongestResultDelay);
}

void UUOUDevelopmentPuzzleCheatSubsystem::FinishSequence()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceTimerHandle);
	}

	const int32 ActivatedStepCount = PendingQueuePosition;
	PendingStepIndices.Reset();
	PendingQueuePosition = 0;
	bSequenceRunning = false;
	LastStatusMessage = FString::Printf(
		TEXT("Puzzle cheat sequence completed. Activated %d step(s)."),
		ActivatedStepCount);
}

void UUOUDevelopmentPuzzleCheatSubsystem::EnsureCheatHUDCreated()
{
	if (CheatHUDWidget.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	UGameViewportClient* GameViewport = GameInstance != nullptr
		? GameInstance->GetGameViewportClient()
		: nullptr;
	if (GameViewport == nullptr)
	{
		return;
	}

	SAssignNew(CheatHUDWidget, SUOUDevelopmentPuzzleCheatHUD)
		.PuzzleCheatSubsystem(this);
	CheatHUDViewport = GameViewport;
	GameViewport->AddViewportWidgetContent(CheatHUDWidget.ToSharedRef(), 10000);
}

void UUOUDevelopmentPuzzleCheatSubsystem::RemoveCheatHUD()
{
	if (CheatHUDViewport.IsValid() && CheatHUDWidget.IsValid())
	{
		CheatHUDViewport->RemoveViewportWidgetContent(CheatHUDWidget.ToSharedRef());
	}

	CheatHUDWidget.Reset();
	CheatHUDViewport.Reset();
}
