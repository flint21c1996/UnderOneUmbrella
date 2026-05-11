// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"

#include "Components/SceneComponent.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"

AUOUPuzzleConditionGroupActor::AUOUPuzzleConditionGroupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	PuzzleConditionGroupComponent = CreateDefaultSubobject<UUOUPuzzleConditionGroupComponent>(
		TEXT("PuzzleConditionGroupComponent"));
	PuzzleConditionGroupComponent->bAutoCollectLocalConditionSources = false;
}

void AUOUPuzzleConditionGroupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshGroupSetup();
}

void AUOUPuzzleConditionGroupActor::BeginPlay()
{
	Super::BeginPlay();

	if (PuzzleConditionGroupComponent != nullptr)
	{
		PuzzleConditionGroupComponent->OnSatisfied.RemoveDynamic(this, &AUOUPuzzleConditionGroupActor::HandleGroupSatisfied);
		PuzzleConditionGroupComponent->OnSatisfied.AddDynamic(this, &AUOUPuzzleConditionGroupActor::HandleGroupSatisfied);

		PuzzleConditionGroupComponent->OnUnsatisfied.RemoveDynamic(
			this,
			&AUOUPuzzleConditionGroupActor::HandleGroupUnsatisfied);
		PuzzleConditionGroupComponent->OnUnsatisfied.AddDynamic(
			this,
			&AUOUPuzzleConditionGroupActor::HandleGroupUnsatisfied);

		PuzzleConditionGroupComponent->OnStateChanged.RemoveDynamic(
			this,
			&AUOUPuzzleConditionGroupActor::HandleGroupStateChanged);
		PuzzleConditionGroupComponent->OnStateChanged.AddDynamic(
			this,
			&AUOUPuzzleConditionGroupActor::HandleGroupStateChanged);
	}

	RefreshGroupSetup();
}

void AUOUPuzzleConditionGroupActor::RefreshGroupSetup()
{
	ResolveConditionSourcesFromActors();

	if (PuzzleConditionGroupComponent == nullptr)
	{
		return;
	}

	TArray<UUOUPuzzleConditionSourceComponent*> SourcePointers;
	SourcePointers.Reserve(ResolvedConditionSources.Num());
	for (UUOUPuzzleConditionSourceComponent* ConditionSource : ResolvedConditionSources)
	{
		if (ConditionSource != nullptr)
		{
			SourcePointers.Add(ConditionSource);
		}
	}

	PuzzleConditionGroupComponent->SetExternalConditionSources(SourcePointers);
	PuzzleConditionGroupComponent->RefreshNow();
}

bool AUOUPuzzleConditionGroupActor::IsSatisfied() const
{
	return PuzzleConditionGroupComponent != nullptr && PuzzleConditionGroupComponent->IsSatisfied();
}

void AUOUPuzzleConditionGroupActor::HandleGroupSatisfied()
{
	OnSatisfied.Broadcast();
	DispatchResultBindings(true);
	ReceiveGroupSatisfied();
}

void AUOUPuzzleConditionGroupActor::HandleGroupUnsatisfied()
{
	OnUnsatisfied.Broadcast();
	DispatchResultBindings(false);
	ReceiveGroupUnsatisfied();
}

void AUOUPuzzleConditionGroupActor::HandleGroupStateChanged(bool bNewSatisfied)
{
	OnStateChanged.Broadcast(bNewSatisfied);
	ReceiveGroupStateChanged(bNewSatisfied);
}

void AUOUPuzzleConditionGroupActor::ResolveConditionSourcesFromActors()
{
	ResolvedConditionSources.Reset();

	if (!bCollectConditionSourcesFromConditionActors)
	{
		return;
	}

	for (AActor* ConditionActor : ConditionActors)
	{
		if (ConditionActor == nullptr)
		{
			continue;
		}

		TInlineComponentArray<UUOUPuzzleConditionSourceComponent*> ConditionSourceComponents(ConditionActor);
		for (UUOUPuzzleConditionSourceComponent* ConditionSource : ConditionSourceComponents)
		{
			if (ConditionSource != nullptr)
			{
				ResolvedConditionSources.AddUnique(ConditionSource);
			}
		}
	}
}

void AUOUPuzzleConditionGroupActor::DispatchResultBindings(bool bSatisfied) const
{
	for (const FOUUPuzzleResultBinding& Binding : ResultBindings)
	{
		ExecuteResultAction(
			Binding.TargetActor.Get(),
			bSatisfied ? Binding.SatisfiedAction : Binding.UnsatisfiedAction);
	}
}

void AUOUPuzzleConditionGroupActor::ExecuteResultAction(AActor* TargetActor, EOUUPuzzleResultAction Action) const
{
	if (TargetActor == nullptr || Action == EOUUPuzzleResultAction::None)
	{
		return;
	}

	const FName FunctionName = GetActionFunctionName(Action);
	if (FunctionName.IsNone())
	{
		return;
	}

	if (UFunction* Function = TargetActor->FindFunction(FunctionName))
	{
		TargetActor->ProcessEvent(Function, nullptr);
		return;
	}

	TInlineComponentArray<UActorComponent*> ActorComponents(TargetActor);
	for (UActorComponent* Component : ActorComponents)
	{
		if (Component == nullptr)
		{
			continue;
		}

		if (UFunction* Function = Component->FindFunction(FunctionName))
		{
			Component->ProcessEvent(Function, nullptr);
			return;
		}
	}
}

FName AUOUPuzzleConditionGroupActor::GetActionFunctionName(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		return TEXT("Activate");
	case EOUUPuzzleResultAction::Deactivate:
		return TEXT("Deactivate");
	case EOUUPuzzleResultAction::Pause:
		return TEXT("Pause");
	case EOUUPuzzleResultAction::Resume:
		return TEXT("Resume");
	case EOUUPuzzleResultAction::Toggle:
		return TEXT("Toggle");
	case EOUUPuzzleResultAction::None:
	default:
		return NAME_None;
	}
}
