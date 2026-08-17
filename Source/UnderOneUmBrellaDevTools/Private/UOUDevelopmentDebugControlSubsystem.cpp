// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentDebugControlSubsystem.h"

#include "Debug/UOUDevelopmentToolsBuild.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#if !UOU_WITH_DEVELOPMENT_TOOLS
#error UOUDevelopmentDebugControlSubsystem must only be compiled when development tools are enabled.
#endif

bool UUOUDevelopmentDebugControlSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr && World->IsGameWorld();
}

bool UUOUDevelopmentDebugControlSubsystem::IsDebugToolsEnabled() const
{
	return bDebugToolsEnabled;
}

void UUOUDevelopmentDebugControlSubsystem::SetDebugToolsEnabled(bool bNewEnabled)
{
	bDebugToolsEnabled = bNewEnabled;
}

bool UUOUDevelopmentDebugControlSubsystem::IsDebugCategoryEnabled(EUOUDebugCategory Category) const
{
	switch (Category)
	{
	case EUOUDebugCategory::Player:
	case EUOUDebugCategory::NPC:
	case EUOUDebugCategory::Puzzle:
	case EUOUDebugCategory::Interaction:
	case EUOUDebugCategory::VFX:
	case EUOUDebugCategory::Performance:
	case EUOUDebugCategory::System:
		return !DisabledDebugCategories.Contains(Category);
	default:
		return false;
	}
}

void UUOUDevelopmentDebugControlSubsystem::SetDebugCategoryEnabled(
	EUOUDebugCategory Category,
	bool bNewEnabled)
{
	switch (Category)
	{
	case EUOUDebugCategory::Player:
	case EUOUDebugCategory::NPC:
	case EUOUDebugCategory::Puzzle:
	case EUOUDebugCategory::Interaction:
	case EUOUDebugCategory::VFX:
	case EUOUDebugCategory::Performance:
	case EUOUDebugCategory::System:
		break;
	default:
		return;
	}

	if (bNewEnabled)
	{
		DisabledDebugCategories.Remove(Category);
	}
	else
	{
		DisabledDebugCategories.Add(Category);
	}

}

void UUOUDevelopmentDebugControlSubsystem::ToggleSelectedDebugActor(AActor* DebugActor)
{
	for (auto SelectedActorIterator = SelectedDebugActors.CreateIterator();
		SelectedActorIterator;
		++SelectedActorIterator)
	{
		if (!SelectedActorIterator->IsValid())
		{
			SelectedActorIterator.RemoveCurrent();
		}
	}

	if (!IsValid(DebugActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> WeakDebugActor(DebugActor);
	if (SelectedDebugActors.Contains(WeakDebugActor))
	{
		SelectedDebugActors.Remove(WeakDebugActor);
		return;
	}

	SelectedDebugActors.Add(WeakDebugActor);
}

void UUOUDevelopmentDebugControlSubsystem::ClearSelectedDebugActors()
{
	SelectedDebugActors.Reset();
}

bool UUOUDevelopmentDebugControlSubsystem::IsDebugActorSelected(const AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	for (const TWeakObjectPtr<AActor>& SelectedActor : SelectedDebugActors)
	{
		if (SelectedActor.Get() == Actor)
		{
			return true;
		}
	}

	return false;
}

TArray<AActor*> UUOUDevelopmentDebugControlSubsystem::GetSelectedDebugActors() const
{
	TArray<AActor*> ValidSelectedActors;
	for (const TWeakObjectPtr<AActor>& SelectedActor : SelectedDebugActors)
	{
		if (AActor* Actor = SelectedActor.Get(); IsValid(Actor))
		{
			ValidSelectedActors.Add(Actor);
		}
	}

	ValidSelectedActors.Sort([](const AActor& Left, const AActor& Right)
	{
		return Left.GetName() < Right.GetName();
	});
	return ValidSelectedActors;
}

bool UUOUDevelopmentDebugControlSubsystem::ShouldDrawDebugActor(const AActor* Actor) const
{
	return IsDebugActorSelected(Actor);
}
