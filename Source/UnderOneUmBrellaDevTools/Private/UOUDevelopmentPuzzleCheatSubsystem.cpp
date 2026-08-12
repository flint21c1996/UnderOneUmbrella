// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentPuzzleCheatSubsystem.h"

#include "Components/ActorComponent.h"
#include "Debug/UOUDevelopmentCheatBuild.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "Puzzle/Core/UOUPuzzleResultCompletionState.h"
#include "UOUDevelopmentDebugControlSubsystem.h"
#include "UOUDevelopmentDebugDrawSubsystem.h"
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
	constexpr float CompletionPollIntervalSeconds = 0.05f;

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
	PuzzleGraphNodes.Reset();
	PuzzleGraphEdges.Reset();
	PendingStepIndices.Reset();
	PendingQueuePosition = 0;
	ActiveStepIndex = INDEX_NONE;
	ActiveStepStartTimeSeconds = 0.0;
	ActiveStepMinimumWaitSeconds = 0.0f;
	bSequenceRunning = false;
	bPuzzleSequenceValid = false;
	bPuzzleGraphValid = false;
	PuzzleGraphStatusMessage.Reset();
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

	RefreshPuzzleGraph();

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
	if (!PuzzleSteps.IsEmpty() || !PuzzleGraphNodes.IsEmpty())
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
	ActiveStepIndex = INDEX_NONE;
	ActiveStepStartTimeSeconds = 0.0;
	ActiveStepMinimumWaitSeconds = 0.0f;
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

bool UUOUDevelopmentPuzzleCheatSubsystem::RefreshPuzzleGraph()
{
	PuzzleGraphNodes.Reset();
	PuzzleGraphEdges.Reset();
	bPuzzleGraphValid = false;

	UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld())
	{
		PuzzleGraphStatusMessage = TEXT("Puzzle graph refresh failed because no game world was available.");
		return false;
	}

	TArray<AUOUPuzzleConditionGroupActor*> PuzzleGroups;
	for (TActorIterator<AUOUPuzzleConditionGroupActor> It(World); It; ++It)
	{
		if (AUOUPuzzleConditionGroupActor* PuzzleGroup = *It; IsValid(PuzzleGroup))
		{
			PuzzleGroups.Add(PuzzleGroup);
		}
	}

	PuzzleGroups.Sort(
		[](const AUOUPuzzleConditionGroupActor& Left, const AUOUPuzzleConditionGroupActor& Right)
		{
			return Left.GetPathName() < Right.GetPathName();
		});

	PuzzleGraphNodes.Reserve(PuzzleGroups.Num());
	for (AUOUPuzzleConditionGroupActor* PuzzleGroup : PuzzleGroups)
	{
		FUOUDevelopmentPuzzleCheatGraphNode& Node = PuzzleGraphNodes.AddDefaulted_GetRef();
		Node.NodeIndex = PuzzleGraphNodes.Num() - 1;
		Node.DisplayName = UOUDevelopmentPuzzleCheatPrivate::ResolveDisplayName(*PuzzleGroup);
		Node.PuzzleGroup = PuzzleGroup;
	}

	BuildPuzzleGraphConnections();
	bPuzzleGraphValid = ValidateAndAssignPuzzleGraphDepths();
	return bPuzzleGraphValid;
}

TArray<FUOUDevelopmentPuzzleCheatGraphNode>
UUOUDevelopmentPuzzleCheatSubsystem::GetPuzzleGraphNodes() const
{
	return PuzzleGraphNodes;
}

TArray<FUOUDevelopmentPuzzleCheatGraphEdge>
UUOUDevelopmentPuzzleCheatSubsystem::GetPuzzleGraphEdges() const
{
	return PuzzleGraphEdges;
}

void UUOUDevelopmentPuzzleCheatSubsystem::CollectConditionDependencyActors(
	AUOUPuzzleConditionGroupActor& PuzzleGroup,
	TArray<AActor*>& OutDependencyActors) const
{
	OutDependencyActors.Reset();

	for (AActor* ConditionActor : PuzzleGroup.ConditionActors)
	{
		if (IsValid(ConditionActor))
		{
			OutDependencyActors.AddUnique(ConditionActor);
		}
	}

	for (const FComponentReference& ConditionSourceReference : PuzzleGroup.ConditionSourceReferences)
	{
		if (const UActorComponent* ConditionSourceComponent = ConditionSourceReference.GetComponent(&PuzzleGroup))
		{
			if (AActor* ConditionOwner = ConditionSourceComponent->GetOwner(); IsValid(ConditionOwner))
			{
				OutDependencyActors.AddUnique(ConditionOwner);
			}
		}
	}

	for (const UUOUPuzzleConditionSourceComponent* ResolvedConditionSource : PuzzleGroup.ResolvedConditionSources)
	{
		if (ResolvedConditionSource != nullptr)
		{
			if (AActor* ConditionOwner = ResolvedConditionSource->GetOwner(); IsValid(ConditionOwner))
			{
				OutDependencyActors.AddUnique(ConditionOwner);
			}
		}
	}
}

void UUOUDevelopmentPuzzleCheatSubsystem::BuildPuzzleGraphConnections()
{
	TMap<AActor*, TArray<int32>> ConsumerNodeIndicesByActor;
	TMap<AUOUPuzzleConditionGroupActor*, int32> NodeIndexByGroup;

	for (const FUOUDevelopmentPuzzleCheatGraphNode& Node : PuzzleGraphNodes)
	{
		AUOUPuzzleConditionGroupActor* PuzzleGroup = Node.PuzzleGroup.Get();
		if (!IsValid(PuzzleGroup))
		{
			continue;
		}

		NodeIndexByGroup.Add(PuzzleGroup, Node.NodeIndex);

		TArray<AActor*> DependencyActors;
		CollectConditionDependencyActors(*PuzzleGroup, DependencyActors);
		for (AActor* DependencyActor : DependencyActors)
		{
			ConsumerNodeIndicesByActor.FindOrAdd(DependencyActor).AddUnique(Node.NodeIndex);
		}
	}

	for (const FUOUDevelopmentPuzzleCheatGraphNode& SourceNode : PuzzleGraphNodes)
	{
		const AUOUPuzzleConditionGroupActor* SourceGroup = SourceNode.PuzzleGroup.Get();
		if (!IsValid(SourceGroup))
		{
			continue;
		}

		for (const FOUUPuzzleResultBinding& ResultBinding : SourceGroup->ResultBindings)
		{
			AActor* RelationActor = ResultBinding.TargetActor.Get();
			if (!IsValid(RelationActor)
				|| ResultBinding.SatisfiedAction == EOUUPuzzleResultAction::None)
			{
				continue;
			}

			if (AUOUPuzzleConditionGroupActor* TargetGroup = Cast<AUOUPuzzleConditionGroupActor>(RelationActor))
			{
				if (const int32* TargetNodeIndex = NodeIndexByGroup.Find(TargetGroup))
				{
					AddPuzzleGraphEdge(SourceNode.NodeIndex, *TargetNodeIndex, RelationActor);
				}
			}

			if (const TArray<int32>* ConsumerNodeIndices = ConsumerNodeIndicesByActor.Find(RelationActor))
			{
				for (const int32 TargetNodeIndex : *ConsumerNodeIndices)
				{
					AddPuzzleGraphEdge(SourceNode.NodeIndex, TargetNodeIndex, RelationActor);
				}
			}
		}
	}
}

void UUOUDevelopmentPuzzleCheatSubsystem::AddPuzzleGraphEdge(
	int32 SourceNodeIndex,
	int32 TargetNodeIndex,
	AActor* RelationActor)
{
	if (!PuzzleGraphNodes.IsValidIndex(SourceNodeIndex)
		|| !PuzzleGraphNodes.IsValidIndex(TargetNodeIndex)
		|| SourceNodeIndex == TargetNodeIndex)
	{
		return;
	}

	const bool bAlreadyExists = PuzzleGraphEdges.ContainsByPredicate(
		[SourceNodeIndex, TargetNodeIndex](const FUOUDevelopmentPuzzleCheatGraphEdge& Edge)
		{
			return Edge.SourceNodeIndex == SourceNodeIndex && Edge.TargetNodeIndex == TargetNodeIndex;
		});
	if (bAlreadyExists)
	{
		return;
	}

	FUOUDevelopmentPuzzleCheatGraphEdge& Edge = PuzzleGraphEdges.AddDefaulted_GetRef();
	Edge.SourceNodeIndex = SourceNodeIndex;
	Edge.TargetNodeIndex = TargetNodeIndex;
	Edge.RelationActor = RelationActor;

	PuzzleGraphNodes[SourceNodeIndex].DependentNodeIndices.AddUnique(TargetNodeIndex);
	PuzzleGraphNodes[TargetNodeIndex].PrerequisiteNodeIndices.AddUnique(SourceNodeIndex);
}

bool UUOUDevelopmentPuzzleCheatSubsystem::ValidateAndAssignPuzzleGraphDepths()
{
	if (PuzzleGraphNodes.IsEmpty())
	{
		PuzzleGraphStatusMessage = TEXT("No PuzzleConditionGroup actors were found for the puzzle graph.");
		return false;
	}

	TArray<int32> RemainingPrerequisiteCounts;
	RemainingPrerequisiteCounts.SetNumZeroed(PuzzleGraphNodes.Num());
	TArray<int32> ReadyNodeIndices;

	for (FUOUDevelopmentPuzzleCheatGraphNode& Node : PuzzleGraphNodes)
	{
		Node.PrerequisiteNodeIndices.Sort();
		Node.DependentNodeIndices.Sort();
		Node.ExecutionDepth = INDEX_NONE;
		RemainingPrerequisiteCounts[Node.NodeIndex] = Node.PrerequisiteNodeIndices.Num();
		if (Node.PrerequisiteNodeIndices.IsEmpty())
		{
			Node.ExecutionDepth = 0;
			ReadyNodeIndices.Add(Node.NodeIndex);
		}
	}
	ReadyNodeIndices.Sort();

	int32 ProcessedNodeCount = 0;
	int32 MaximumExecutionDepth = 0;
	while (!ReadyNodeIndices.IsEmpty())
	{
		const int32 NodeIndex = ReadyNodeIndices[0];
		ReadyNodeIndices.RemoveAt(0, 1, EAllowShrinking::No);
		if (!PuzzleGraphNodes.IsValidIndex(NodeIndex))
		{
			continue;
		}

		++ProcessedNodeCount;
		FUOUDevelopmentPuzzleCheatGraphNode& Node = PuzzleGraphNodes[NodeIndex];
		MaximumExecutionDepth = FMath::Max(MaximumExecutionDepth, Node.ExecutionDepth);

		for (const int32 DependentNodeIndex : Node.DependentNodeIndices)
		{
			if (!PuzzleGraphNodes.IsValidIndex(DependentNodeIndex))
			{
				continue;
			}

			FUOUDevelopmentPuzzleCheatGraphNode& DependentNode = PuzzleGraphNodes[DependentNodeIndex];
			DependentNode.ExecutionDepth = FMath::Max(
				DependentNode.ExecutionDepth,
				Node.ExecutionDepth + 1);
			--RemainingPrerequisiteCounts[DependentNodeIndex];
			if (RemainingPrerequisiteCounts[DependentNodeIndex] == 0)
			{
				ReadyNodeIndices.Add(DependentNodeIndex);
				ReadyNodeIndices.Sort();
			}
		}
	}

	const int32 CyclicNodeCount = PuzzleGraphNodes.Num() - ProcessedNodeCount;
	if (CyclicNodeCount > 0)
	{
		PuzzleGraphStatusMessage = FString::Printf(
			TEXT("Puzzle graph is invalid: %d of %d node(s) could not be resolved because of a cyclic dependency."),
			CyclicNodeCount,
			PuzzleGraphNodes.Num());
		return false;
	}

	PuzzleGraphStatusMessage = FString::Printf(
		TEXT("Collected %d puzzle graph node(s), %d edge(s), and %d execution depth level(s)."),
		PuzzleGraphNodes.Num(),
		PuzzleGraphEdges.Num(),
		MaximumExecutionDepth + 1);
	return true;
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
		TEXT("Activated puzzle cheat step %d and waiting for result completion: %s"),
		Step.StepOrder,
		*Step.DisplayName.ToString());

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		CancelPendingSequence();
		LastStatusMessage = TEXT("Puzzle cheat sequence stopped because its world was unavailable.");
		return;
	}

	ActiveStepIndex = StepIndex;
	ActiveStepStartTimeSeconds = World->GetTimeSeconds();
	ActiveStepMinimumWaitSeconds = GetDelayBeforeNextStep(Step)
		+ UOUDevelopmentPuzzleCheatPrivate::CompletionPollIntervalSeconds;
	ScheduleCompletionCheck();
}

void UUOUDevelopmentPuzzleCheatSubsystem::CheckCurrentStepCompletion()
{
	if (!bSequenceRunning)
	{
		return;
	}

	if (!PuzzleSteps.IsValidIndex(ActiveStepIndex))
	{
		CancelPendingSequence();
		LastStatusMessage = TEXT("Puzzle cheat sequence lost the step waiting for result completion.");
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		CancelPendingSequence();
		LastStatusMessage = TEXT("Puzzle cheat sequence stopped because its world was unavailable.");
		return;
	}

	const FUOUDevelopmentPuzzleCheatStep& ActiveStep = PuzzleSteps[ActiveStepIndex];
	const double ElapsedSeconds = World->GetTimeSeconds() - ActiveStepStartTimeSeconds;
	const bool bMinimumWaitCompleted = ElapsedSeconds >= ActiveStepMinimumWaitSeconds;
	const bool bResultsCompleted = bMinimumWaitCompleted && AreStepResultsCompleted(ActiveStep);
	if (!bResultsCompleted)
	{
		LastStatusMessage = FString::Printf(
			TEXT("Waiting for puzzle cheat step %d result completion: %s"),
			ActiveStep.StepOrder,
			*ActiveStep.DisplayName.ToString());
		ScheduleCompletionCheck();
		return;
	}

	++PendingQueuePosition;
	ActiveStepIndex = INDEX_NONE;
	ActiveStepStartTimeSeconds = 0.0;
	ActiveStepMinimumWaitSeconds = 0.0f;
	ExecuteNextQueuedStep();
}

void UUOUDevelopmentPuzzleCheatSubsystem::ScheduleCompletionCheck()
{
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
		&UUOUDevelopmentPuzzleCheatSubsystem::CheckCurrentStepCompletion,
		UOUDevelopmentPuzzleCheatPrivate::CompletionPollIntervalSeconds,
		false);
}

bool UUOUDevelopmentPuzzleCheatSubsystem::AreStepResultsCompleted(
	const FUOUDevelopmentPuzzleCheatStep& Step) const
{
	const AUOUPuzzleConditionGroupActor* PuzzleGroup = Step.PuzzleGroup.Get();
	if (PuzzleGroup == nullptr)
	{
		return false;
	}

	for (const FOUUPuzzleResultBinding& Binding : PuzzleGroup->ResultBindings)
	{
		AActor* TargetActor = Binding.TargetActor.Get();
		if (TargetActor == nullptr || Binding.SatisfiedAction == EOUUPuzzleResultAction::None)
		{
			continue;
		}

		if (!TargetActor->GetClass()->ImplementsInterface(UUOUPuzzleResultCompletionState::StaticClass()))
		{
			continue;
		}

		if (!IUOUPuzzleResultCompletionState::Execute_IsPuzzleResultCompleted(
				TargetActor,
				Binding.SatisfiedAction))
		{
			return false;
		}
	}

	return true;
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
	ActiveStepIndex = INDEX_NONE;
	ActiveStepStartTimeSeconds = 0.0;
	ActiveStepMinimumWaitSeconds = 0.0f;
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

	TWeakObjectPtr<UUOUDevelopmentDebugControlSubsystem> DebugControlSubsystem;
	TWeakObjectPtr<UUOUDevelopmentDebugDrawSubsystem> DebugDrawSubsystem;
	if (World != nullptr)
	{
		DebugControlSubsystem = World->GetSubsystem<UUOUDevelopmentDebugControlSubsystem>();
		DebugDrawSubsystem = World->GetSubsystem<UUOUDevelopmentDebugDrawSubsystem>();
	}

	SAssignNew(CheatHUDWidget, SUOUDevelopmentPuzzleCheatHUD)
		.DebugControlSubsystem(DebugControlSubsystem)
		.DebugDrawSubsystem(DebugDrawSubsystem)
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
