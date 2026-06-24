// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Results/UOUObjectiveVisualResultComponent.h"

#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"

UUOUObjectiveVisualResultComponent::UUOUObjectiveVisualResultComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUObjectiveVisualResultComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveTargetNiagaraComponent();
	SetObjectiveVisualActive(bStartActive);
}

void UUOUObjectiveVisualResultComponent::ShowObjectiveVisual()
{
	SetObjectiveVisualActive(true);
}

void UUOUObjectiveVisualResultComponent::HideObjectiveVisual()
{
	SetObjectiveVisualActive(false);
}

void UUOUObjectiveVisualResultComponent::ToggleObjectiveVisual()
{
	SetObjectiveVisualActive(!bObjectiveVisualActive);
}

void UUOUObjectiveVisualResultComponent::SetObjectiveVisualActive(bool bNewActive)
{
	bObjectiveVisualActive = bNewActive;
	ApplyObjectiveVisualState();
}

bool UUOUObjectiveVisualResultComponent::IsObjectiveVisualActive() const
{
	return bObjectiveVisualActive;
}

UNiagaraComponent* UUOUObjectiveVisualResultComponent::ResolveTargetNiagaraComponent()
{
	if (IsValid(TargetNiagaraComponent))
	{
		return TargetNiagaraComponent.Get();
	}

	if (UActorComponent* ReferencedComponent = TargetNiagaraComponentReference.GetComponent(GetOwner()))
	{
		TargetNiagaraComponent = Cast<UNiagaraComponent>(ReferencedComponent);
		if (IsValid(TargetNiagaraComponent))
		{
			return TargetNiagaraComponent.Get();
		}
	}

	TargetNiagaraComponent = FindTargetNiagaraComponent();
	return TargetNiagaraComponent.Get();
}

void UUOUObjectiveVisualResultComponent::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		ShowObjectiveVisual();
		break;
	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		HideObjectiveVisual();
		break;
	case EOUUPuzzleResultAction::Toggle:
		ToggleObjectiveVisual();
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

void UUOUObjectiveVisualResultComponent::ApplyObjectiveVisualState()
{
	UNiagaraComponent* NiagaraComponent = ResolveTargetNiagaraComponent();
	if (NiagaraComponent == nullptr)
	{
		return;
	}

	NiagaraComponent->SetAutoActivate(false);

	if (bControlVisibility)
	{
		NiagaraComponent->SetVisibility(bObjectiveVisualActive, true);
		NiagaraComponent->SetHiddenInGame(!bObjectiveVisualActive, true);
	}

	if (bObjectiveVisualActive)
	{
		if (!NiagaraComponent->IsActive() || bResetOnActivate)
		{
			NiagaraComponent->Activate(bResetOnActivate);
		}
		return;
	}

	if (NiagaraComponent->IsActive())
	{
		NiagaraComponent->Deactivate();
	}
}

UNiagaraComponent* UUOUObjectiveVisualResultComponent::FindTargetNiagaraComponent() const
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return nullptr;
	}

	TArray<UNiagaraComponent*> NiagaraComponents;
	OwnerActor->GetComponents<UNiagaraComponent>(NiagaraComponents);
	if (NiagaraComponents.Num() == 0)
	{
		return nullptr;
	}

	if (bAutoFindNiagaraComponentByNameOrTag && !TargetNiagaraComponentName.IsNone())
	{
		const FString TargetName = TargetNiagaraComponentName.ToString();
		for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
		{
			if (NiagaraComponent == nullptr)
			{
				continue;
			}

			if (NiagaraComponent->GetFName() == TargetNiagaraComponentName
				|| NiagaraComponent->ComponentTags.Contains(TargetNiagaraComponentName)
				|| NiagaraComponent->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
			{
				return NiagaraComponent;
			}
		}
	}

	return bAutoFindFirstNiagaraComponent ? NiagaraComponents[0] : nullptr;
}
