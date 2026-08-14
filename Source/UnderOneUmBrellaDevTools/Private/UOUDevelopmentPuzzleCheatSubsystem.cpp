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
	constexpr float DefaultDelayAfterActivationSeconds = 0.25f;
	constexpr float CompletionPollIntervalSeconds = 0.05f;

	FText ResolveDisplayName(const AActor& Actor)
	{
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
	RefreshPuzzleGraph();
	if (!PuzzleGraphNodes.IsEmpty())
	{
		EnsureCheatHUDCreated();
	}
}

void UUOUDevelopmentPuzzleCheatSubsystem::Deinitialize()
{
	RemoveCheatHUD();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GraphCompletionTimerHandle);
	}

	PuzzleGraphNodes.Reset();
	PuzzleGraphEdges.Reset();
	PendingGraphExecutionWaves.Reset();
	ActiveGraphNodes.Reset();
	PendingGraphWavePosition = 0;
	bPuzzleGraphValid = false;
	bGraphExecutionActive = false;
	ActivatedGraphNodeCount = 0;
	PuzzleGraphStatusMessage.Reset();
	Super::Deinitialize();
}

bool UUOUDevelopmentPuzzleCheatSubsystem::RefreshPuzzleGraph()
{
	if (bGraphExecutionActive)
	{
		LastStatusMessage = TEXT("Cannot refresh the puzzle graph while it is executing.");
		return false;
	}

	PuzzleGraphNodes.Reset();
	PuzzleGraphEdges.Reset();
	bPuzzleGraphValid = false;

	UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld())
	{
		PuzzleGraphStatusMessage = TEXT("Puzzle graph refresh failed because no game world was available.");
		LastStatusMessage = PuzzleGraphStatusMessage;
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
	LastStatusMessage = PuzzleGraphStatusMessage;
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
	TArray<AActor*>& OutDependencyActors,
	TMap<AActor*, TArray<UUOUPuzzleConditionSourceComponent*>>& OutConditionSourcesByActor) const
{
	OutDependencyActors.Reset();
	OutConditionSourcesByActor.Reset();

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

	for (UUOUPuzzleConditionSourceComponent* ResolvedConditionSource : PuzzleGroup.ResolvedConditionSources)
	{
		if (ResolvedConditionSource != nullptr)
		{
			if (AActor* ConditionOwner = ResolvedConditionSource->GetOwner(); IsValid(ConditionOwner))
			{
				OutDependencyActors.AddUnique(ConditionOwner);
				OutConditionSourcesByActor.FindOrAdd(ConditionOwner).AddUnique(ResolvedConditionSource);
			}
		}
	}
}

void UUOUDevelopmentPuzzleCheatSubsystem::BuildPuzzleGraphConnections()
{
	TMap<AActor*, TArray<int32>> ConsumerNodeIndicesByActor;
	TMap<AUOUPuzzleConditionGroupActor*, int32> NodeIndexByGroup;
	TArray<TMap<AActor*, TArray<UUOUPuzzleConditionSourceComponent*>>> ConditionSourcesByInputActorByNode;
	ConditionSourcesByInputActorByNode.SetNum(PuzzleGraphNodes.Num());

	for (FUOUDevelopmentPuzzleCheatGraphNode& Node : PuzzleGraphNodes)
	{
		AUOUPuzzleConditionGroupActor* PuzzleGroup = Node.PuzzleGroup.Get();
		if (!IsValid(PuzzleGroup))
		{
			continue;
		}

		NodeIndexByGroup.Add(PuzzleGroup, Node.NodeIndex);

		TArray<AActor*> DependencyActors;
		TMap<AActor*, TArray<UUOUPuzzleConditionSourceComponent*>>& ConditionSourcesByInputActor =
			ConditionSourcesByInputActorByNode[Node.NodeIndex];
		CollectConditionDependencyActors(
			*PuzzleGroup,
			DependencyActors,
			ConditionSourcesByInputActor);
		for (AActor* DependencyActor : DependencyActors)
		{
			Node.InputActors.AddUnique(DependencyActor);
			ConsumerNodeIndicesByActor.FindOrAdd(DependencyActor).AddUnique(Node.NodeIndex);
		}

		for (const FOUUPuzzleResultBinding& ResultBinding : PuzzleGroup->ResultBindings)
		{
			AActor* ResultActor = ResultBinding.TargetActor.Get();
			if (IsValid(ResultActor)
				&& ResultBinding.SatisfiedAction != EOUUPuzzleResultAction::None)
			{
				Node.ResultActors.AddUnique(ResultActor);
			}
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

	for (FUOUDevelopmentPuzzleCheatGraphNode& Node : PuzzleGraphNodes)
	{
		TSet<AActor*> IncomingRelationActors;
		for (const FUOUDevelopmentPuzzleCheatGraphEdge& Edge : PuzzleGraphEdges)
		{
			if (Edge.TargetNodeIndex == Node.NodeIndex && IsValid(Edge.RelationActor.Get()))
			{
				IncomingRelationActors.Add(Edge.RelationActor.Get());
			}
		}

		Node.ExternalInputs.Reset();
		for (AActor* InputActor : Node.InputActors)
		{
			if (!IsValid(InputActor) || IncomingRelationActors.Contains(InputActor))
			{
				continue;
			}

			FUOUDevelopmentPuzzleCheatExternalInput& ExternalInput =
				Node.ExternalInputs.AddDefaulted_GetRef();
			ExternalInput.InputActor = InputActor;
			ExternalInput.DisplayName = UOUDevelopmentPuzzleCheatPrivate::ResolveDisplayName(*InputActor);

			if (!ConditionSourcesByInputActorByNode.IsValidIndex(Node.NodeIndex))
			{
				continue;
			}

			const TMap<AActor*, TArray<UUOUPuzzleConditionSourceComponent*>>& ConditionSourcesByInputActor =
				ConditionSourcesByInputActorByNode[Node.NodeIndex];
			if (const TArray<UUOUPuzzleConditionSourceComponent*>* ConditionSources =
				ConditionSourcesByInputActor.Find(InputActor))
			{
				for (UUOUPuzzleConditionSourceComponent* ConditionSource : *ConditionSources)
				{
					if (IsValid(ConditionSource))
					{
						ExternalInput.ConditionSources.AddUnique(
							TWeakObjectPtr<UUOUPuzzleConditionSourceComponent>(ConditionSource));
					}
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

bool UUOUDevelopmentPuzzleCheatSubsystem::AdvanceThroughGraphNode(int32 TargetNodeIndex)
{
	if (bGraphExecutionActive)
	{
		LastStatusMessage = TEXT("A puzzle graph execution is already running.");
		return false;
	}

	if (!bPuzzleGraphValid && !RefreshPuzzleGraph())
	{
		LastStatusMessage = FString::Printf(
			TEXT("Cannot start graph execution: %s"),
			*PuzzleGraphStatusMessage);
		return false;
	}

	if (!BuildGraphExecutionWaves(TargetNodeIndex))
	{
		return false;
	}

	if (PendingGraphExecutionWaves.IsEmpty())
	{
		LastStatusMessage = FString::Printf(
			TEXT("Puzzle graph node %d and all of its prerequisites are already satisfied."),
			TargetNodeIndex);
		return true;
	}

	PendingGraphWavePosition = 0;
	ActivatedGraphNodeCount = 0;
	bGraphExecutionActive = true;
	LastStatusMessage = FString::Printf(
		TEXT("Starting puzzle graph execution through node %d (%d wave(s))."),
		TargetNodeIndex,
		PendingGraphExecutionWaves.Num());
	ExecuteNextGraphWave();
	return true;
}

bool UUOUDevelopmentPuzzleCheatSubsystem::ResolveExternalInput(
	int32 NodeIndex,
	int32 ExternalInputIndex)
{
	if (bGraphExecutionActive)
	{
		LastStatusMessage = TEXT("그래프 실행 중에는 외부 입력을 개별 실행할 수 없습니다.");
		return false;
	}

	if (!PuzzleGraphNodes.IsValidIndex(NodeIndex))
	{
		LastStatusMessage = FString::Printf(
			TEXT("외부 입력 실행 실패: 그래프 노드 %d을(를) 찾지 못했습니다."),
			NodeIndex);
		return false;
	}

	const FUOUDevelopmentPuzzleCheatGraphNode& Node = PuzzleGraphNodes[NodeIndex];
	if (!Node.ExternalInputs.IsValidIndex(ExternalInputIndex))
	{
		LastStatusMessage = FString::Printf(
			TEXT("외부 입력 실행 실패: 노드 %d에 외부 입력 %d이(가) 없습니다."),
			NodeIndex,
			ExternalInputIndex);
		return false;
	}

	const FUOUDevelopmentPuzzleCheatExternalInput& ExternalInput =
		Node.ExternalInputs[ExternalInputIndex];
	AActor* InputActor = ExternalInput.InputActor.Get();
	if (!IsValid(InputActor))
	{
		LastStatusMessage = FString::Printf(
			TEXT("외부 입력 실행 실패: 노드 %d의 입력 액터가 유효하지 않습니다."),
			NodeIndex);
		return false;
	}

	if (ExternalInput.ConditionSources.IsEmpty())
	{
		LastStatusMessage = FString::Printf(
			TEXT("외부 입력 '%s'에 연결된 ConditionSource가 없습니다."),
			*ExternalInput.DisplayName.ToString());
		return false;
	}

	int32 ValidSourceCount = 0;
	int32 AcceptedSourceCount = 0;
	for (const TWeakObjectPtr<UUOUPuzzleConditionSourceComponent>& ConditionSourceReference :
		ExternalInput.ConditionSources)
	{
		UUOUPuzzleConditionSourceComponent* ConditionSource = ConditionSourceReference.Get();
		if (!IsValid(ConditionSource))
		{
			continue;
		}

		++ValidSourceCount;
		if (ConditionSource->IsSatisfied()
			|| ConditionSource->TryResolveInputForCheat(InputActor))
		{
			++AcceptedSourceCount;
		}
	}

	const bool bAllSourcesAccepted = ValidSourceCount == ExternalInput.ConditionSources.Num()
		&& AcceptedSourceCount == ValidSourceCount;
	if (!bAllSourcesAccepted)
	{
		LastStatusMessage = FString::Printf(
			TEXT("외부 입력 '%s' 해결 요청 일부 실패: %d/%d Source 처리."),
			*ExternalInput.DisplayName.ToString(),
			AcceptedSourceCount,
			ExternalInput.ConditionSources.Num());
		return false;
	}

	LastStatusMessage = FString::Printf(
		TEXT("외부 입력 '%s' 해결 요청 완료: %d개 Source 처리."),
		*ExternalInput.DisplayName.ToString(),
		AcceptedSourceCount);
	return true;
}

bool UUOUDevelopmentPuzzleCheatSubsystem::IsExternalInputSatisfied(
	int32 NodeIndex,
	int32 ExternalInputIndex) const
{
	if (!PuzzleGraphNodes.IsValidIndex(NodeIndex))
	{
		return false;
	}

	const FUOUDevelopmentPuzzleCheatGraphNode& Node = PuzzleGraphNodes[NodeIndex];
	if (!Node.ExternalInputs.IsValidIndex(ExternalInputIndex))
	{
		return false;
	}

	const FUOUDevelopmentPuzzleCheatExternalInput& ExternalInput =
		Node.ExternalInputs[ExternalInputIndex];
	if (!IsValid(ExternalInput.InputActor.Get()) || ExternalInput.ConditionSources.IsEmpty())
	{
		return false;
	}

	for (const TWeakObjectPtr<UUOUPuzzleConditionSourceComponent>& ConditionSourceReference :
		ExternalInput.ConditionSources)
	{
		const UUOUPuzzleConditionSourceComponent* ConditionSource = ConditionSourceReference.Get();
		if (!IsValid(ConditionSource) || !ConditionSource->IsSatisfied())
		{
			return false;
		}
	}

	return true;
}

bool UUOUDevelopmentPuzzleCheatSubsystem::IsGraphNodeActive(int32 NodeIndex) const
{
	return bGraphExecutionActive && ActiveGraphNodes.ContainsByPredicate(
		[NodeIndex](const FUOUDevelopmentPuzzleCheatActiveGraphNode& ActiveNode)
		{
			return ActiveNode.NodeIndex == NodeIndex;
		});
}

void UUOUDevelopmentPuzzleCheatSubsystem::CancelGraphExecution()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GraphCompletionTimerHandle);
	}

	const bool bHadPendingExecution = bGraphExecutionActive
		|| !PendingGraphExecutionWaves.IsEmpty();
	PendingGraphExecutionWaves.Reset();
	ActiveGraphNodes.Reset();
	PendingGraphWavePosition = 0;
	ActivatedGraphNodeCount = 0;
	bGraphExecutionActive = false;
	LastStatusMessage = bHadPendingExecution
		? TEXT("Pending puzzle graph execution cancelled. Already activated nodes were kept.")
		: TEXT("No puzzle graph execution was running.");
}

bool UUOUDevelopmentPuzzleCheatSubsystem::BuildGraphExecutionWaves(int32 TargetNodeIndex)
{
	PendingGraphExecutionWaves.Reset();
	ActiveGraphNodes.Reset();

	if (!PuzzleGraphNodes.IsValidIndex(TargetNodeIndex))
	{
		LastStatusMessage = FString::Printf(
			TEXT("Puzzle graph node index %d was not found."),
			TargetNodeIndex);
		return false;
	}

	TSet<int32> RequiredNodeIndices;
	TArray<int32> NodesToVisit;
	NodesToVisit.Add(TargetNodeIndex);
	while (!NodesToVisit.IsEmpty())
	{
		const int32 NodeIndex = NodesToVisit.Pop(EAllowShrinking::No);
		if (RequiredNodeIndices.Contains(NodeIndex))
		{
			continue;
		}

		if (!PuzzleGraphNodes.IsValidIndex(NodeIndex))
		{
			PendingGraphExecutionWaves.Reset();
			LastStatusMessage = FString::Printf(
				TEXT("Puzzle graph contains an invalid prerequisite node index %d."),
				NodeIndex);
			return false;
		}

		RequiredNodeIndices.Add(NodeIndex);
		NodesToVisit.Append(PuzzleGraphNodes[NodeIndex].PrerequisiteNodeIndices);
	}

	TArray<int32> OrderedNodeIndices = RequiredNodeIndices.Array();
	OrderedNodeIndices.Sort(
		[this](int32 LeftNodeIndex, int32 RightNodeIndex)
		{
			const FUOUDevelopmentPuzzleCheatGraphNode& LeftNode = PuzzleGraphNodes[LeftNodeIndex];
			const FUOUDevelopmentPuzzleCheatGraphNode& RightNode = PuzzleGraphNodes[RightNodeIndex];
			return LeftNode.ExecutionDepth == RightNode.ExecutionDepth
				? LeftNodeIndex < RightNodeIndex
				: LeftNode.ExecutionDepth < RightNode.ExecutionDepth;
		});

	for (const int32 NodeIndex : OrderedNodeIndices)
	{
		const FUOUDevelopmentPuzzleCheatGraphNode& Node = PuzzleGraphNodes[NodeIndex];
		const AUOUPuzzleConditionGroupActor* PuzzleGroup = Node.PuzzleGroup.Get();
		if (!IsValid(PuzzleGroup))
		{
			PendingGraphExecutionWaves.Reset();
			LastStatusMessage = FString::Printf(
				TEXT("Cannot build puzzle graph execution: node %d has no valid PuzzleGroup."),
				NodeIndex);
			return false;
		}

		if (PuzzleGroup->IsSatisfied())
		{
			continue;
		}

		if (PendingGraphExecutionWaves.IsEmpty()
			|| PendingGraphExecutionWaves.Last().ExecutionDepth != Node.ExecutionDepth)
		{
			FUOUDevelopmentPuzzleCheatGraphExecutionWave& Wave =
				PendingGraphExecutionWaves.AddDefaulted_GetRef();
			Wave.ExecutionDepth = Node.ExecutionDepth;
		}

		PendingGraphExecutionWaves.Last().NodeIndices.Add(NodeIndex);
	}

	return true;
}

void UUOUDevelopmentPuzzleCheatSubsystem::ExecuteNextGraphWave()
{
	if (!bGraphExecutionActive)
	{
		return;
	}

	if (!PendingGraphExecutionWaves.IsValidIndex(PendingGraphWavePosition))
	{
		FinishGraphExecution();
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		CancelGraphExecution();
		LastStatusMessage = TEXT("Puzzle graph execution stopped because its world was unavailable.");
		return;
	}

	ActiveGraphNodes.Reset();
	const FUOUDevelopmentPuzzleCheatGraphExecutionWave& Wave =
		PendingGraphExecutionWaves[PendingGraphWavePosition];
	const double WaveStartTimeSeconds = World->GetTimeSeconds();
	for (const int32 NodeIndex : Wave.NodeIndices)
	{
		if (!PuzzleGraphNodes.IsValidIndex(NodeIndex))
		{
			CancelGraphExecution();
			LastStatusMessage = FString::Printf(
				TEXT("Puzzle graph execution lost node index %d."),
				NodeIndex);
			return;
		}

		const FUOUDevelopmentPuzzleCheatGraphNode& Node = PuzzleGraphNodes[NodeIndex];
		AUOUPuzzleConditionGroupActor* PuzzleGroup = Node.PuzzleGroup.Get();
		if (!IsValid(PuzzleGroup))
		{
			CancelGraphExecution();
			LastStatusMessage = FString::Printf(
				TEXT("Puzzle graph node %d lost its PuzzleGroup."),
				NodeIndex);
			return;
		}

		if (PuzzleGroup->IsSatisfied())
		{
			continue;
		}

		if (!PuzzleGroup->ForceSatisfiedForCheat())
		{
			CancelGraphExecution();
			LastStatusMessage = FString::Printf(
				TEXT("Failed to satisfy puzzle graph node %d (%s)."),
				NodeIndex,
				*Node.DisplayName.ToString());
			return;
		}

		FUOUDevelopmentPuzzleCheatActiveGraphNode& ActiveNode =
			ActiveGraphNodes.AddDefaulted_GetRef();
		ActiveNode.NodeIndex = NodeIndex;
		ActiveNode.StartTimeSeconds = WaveStartTimeSeconds;
		ActiveNode.MinimumWaitSeconds = GetDelayBeforeNextGraphWave(Node)
			+ UOUDevelopmentPuzzleCheatPrivate::CompletionPollIntervalSeconds;
		++ActivatedGraphNodeCount;
	}

	if (ActiveGraphNodes.IsEmpty())
	{
		++PendingGraphWavePosition;
		ExecuteNextGraphWave();
		return;
	}

	LastStatusMessage = FString::Printf(
		TEXT("Activated puzzle graph wave %d/%d at depth %d (%d node(s)); waiting for result completion."),
		PendingGraphWavePosition + 1,
		PendingGraphExecutionWaves.Num(),
		Wave.ExecutionDepth,
		ActiveGraphNodes.Num());
	ScheduleGraphCompletionCheck();
}

void UUOUDevelopmentPuzzleCheatSubsystem::CheckCurrentGraphWaveCompletion()
{
	if (!bGraphExecutionActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		CancelGraphExecution();
		LastStatusMessage = TEXT("Puzzle graph execution stopped because its world was unavailable.");
		return;
	}

	for (const FUOUDevelopmentPuzzleCheatActiveGraphNode& ActiveNode : ActiveGraphNodes)
	{
		if (!PuzzleGraphNodes.IsValidIndex(ActiveNode.NodeIndex))
		{
			CancelGraphExecution();
			LastStatusMessage = TEXT("Puzzle graph execution lost an active node.");
			return;
		}

		const AUOUPuzzleConditionGroupActor* PuzzleGroup =
			PuzzleGraphNodes[ActiveNode.NodeIndex].PuzzleGroup.Get();
		if (!IsValid(PuzzleGroup))
		{
			CancelGraphExecution();
			LastStatusMessage = TEXT("Puzzle graph execution lost an active PuzzleGroup.");
			return;
		}

		const double ElapsedSeconds = World->GetTimeSeconds() - ActiveNode.StartTimeSeconds;
		if (ElapsedSeconds < ActiveNode.MinimumWaitSeconds
			|| !ArePuzzleGroupResultsCompleted(PuzzleGroup))
		{
			ScheduleGraphCompletionCheck();
			return;
		}
	}

	++PendingGraphWavePosition;
	ActiveGraphNodes.Reset();
	ExecuteNextGraphWave();
}

void UUOUDevelopmentPuzzleCheatSubsystem::ScheduleGraphCompletionCheck()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		CancelGraphExecution();
		LastStatusMessage = TEXT("Puzzle graph execution stopped because its world was unavailable.");
		return;
	}

	World->GetTimerManager().SetTimer(
		GraphCompletionTimerHandle,
		this,
		&UUOUDevelopmentPuzzleCheatSubsystem::CheckCurrentGraphWaveCompletion,
		UOUDevelopmentPuzzleCheatPrivate::CompletionPollIntervalSeconds,
		false);
}

float UUOUDevelopmentPuzzleCheatSubsystem::GetDelayBeforeNextGraphWave(
	const FUOUDevelopmentPuzzleCheatGraphNode& Node) const
{
	float RequiredDelaySeconds =
		UOUDevelopmentPuzzleCheatPrivate::DefaultDelayAfterActivationSeconds;
	if (const AUOUPuzzleConditionGroupActor* PuzzleGroup = Node.PuzzleGroup.Get())
	{
		float LongestResultDelay = 0.0f;
		for (const FOUUPuzzleResultBinding& Binding : PuzzleGroup->ResultBindings)
		{
			LongestResultDelay = FMath::Max(LongestResultDelay, Binding.SatisfiedDelaySeconds);
		}
		RequiredDelaySeconds = FMath::Max(RequiredDelaySeconds, LongestResultDelay);
	}

	return RequiredDelaySeconds;
}

void UUOUDevelopmentPuzzleCheatSubsystem::FinishGraphExecution()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GraphCompletionTimerHandle);
	}

	const int32 CompletedNodeCount = ActivatedGraphNodeCount;
	PendingGraphExecutionWaves.Reset();
	ActiveGraphNodes.Reset();
	PendingGraphWavePosition = 0;
	ActivatedGraphNodeCount = 0;
	bGraphExecutionActive = false;
	LastStatusMessage = FString::Printf(
		TEXT("Puzzle graph execution completed. Activated %d node(s)."),
		CompletedNodeCount);
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

bool UUOUDevelopmentPuzzleCheatSubsystem::ArePuzzleGroupResultsCompleted(
	const AUOUPuzzleConditionGroupActor* PuzzleGroup) const
{
	if (!IsValid(PuzzleGroup))
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
