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

void UUOUDevelopmentDebugControlSubsystem::SetSelectedDebugActor(AActor* NewSelectedActor)
{
	if (IsValid(NewSelectedActor))
	{
		SelectedDebugActor = NewSelectedActor;
		return;
	}

	SelectedDebugActor.Reset();
}

AActor* UUOUDevelopmentDebugControlSubsystem::GetSelectedDebugActor() const
{
	return SelectedDebugActor.Get();
}

bool UUOUDevelopmentDebugControlSubsystem::ShouldDrawDebugActor(const AActor* Actor) const
{
	return IsValid(Actor) && SelectedDebugActor.Get() == Actor;
}
