// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentDebugControlSubsystem.h"

#include "Debug/UOUDebugController.h"
#include "Debug/UOUDebugControllerComponent.h"
#include "Debug/UOUDebugSubsystem.h"
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

void UUOUDevelopmentDebugControlSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (const AUOUDebugController* DebugController = ResolveDebugController())
	{
		bDebugToolsEnabled = DebugController->bEnableDebugTools;
		ImportCategoryStatesFromLegacyController(*DebugController);
	}
}

bool UUOUDevelopmentDebugControlSubsystem::IsDebugToolsEnabled() const
{
	return bDebugToolsEnabled;
}

void UUOUDevelopmentDebugControlSubsystem::SetDebugToolsEnabled(bool bNewEnabled)
{
	bDebugToolsEnabled = bNewEnabled;
	ApplyMasterStateToLegacyController();
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

	ApplyCategoryStateToLegacyController(Category);
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

void UUOUDevelopmentDebugControlSubsystem::ApplyMasterStateToLegacyController() const
{
	if (AUOUDebugController* DebugController = ResolveDebugController())
	{
		DebugController->bEnableDebugTools = bDebugToolsEnabled;
	}
}

void UUOUDevelopmentDebugControlSubsystem::ImportCategoryStatesFromLegacyController(
	const AUOUDebugController& DebugController)
{
	DisabledDebugCategories.Reset();

	const auto ImportCategory = [this, &DebugController](EUOUDebugCategory Category, bool bActorCategoryEnabled)
	{
		const UUOUDebugControllerComponentBase* ControllerComponent =
			DebugController.FindDebugControllerComponent(Category);
		const bool bComponentEnabled =
			ControllerComponent == nullptr || ControllerComponent->IsDebugEnabled();
		if (!bActorCategoryEnabled || !bComponentEnabled)
		{
			DisabledDebugCategories.Add(Category);
		}
	};

	ImportCategory(EUOUDebugCategory::Player, DebugController.bEnablePlayerDebug);
	ImportCategory(EUOUDebugCategory::NPC, DebugController.bEnableNPCDebug);
	ImportCategory(EUOUDebugCategory::Puzzle, DebugController.bEnablePuzzleDebug);
	ImportCategory(EUOUDebugCategory::Interaction, DebugController.bEnableInteractionDebug);
	ImportCategory(EUOUDebugCategory::VFX, DebugController.bEnableVFXDebug);
	ImportCategory(EUOUDebugCategory::Performance, DebugController.bEnablePerformanceDebug);
}

void UUOUDevelopmentDebugControlSubsystem::ApplyCategoryStateToLegacyController(
	EUOUDebugCategory Category) const
{
	AUOUDebugController* DebugController = ResolveDebugController();
	if (DebugController == nullptr)
	{
		return;
	}

	const bool bCategoryEnabled = IsDebugCategoryEnabled(Category);
	switch (Category)
	{
	case EUOUDebugCategory::Player:
		DebugController->bEnablePlayerDebug = bCategoryEnabled;
		break;
	case EUOUDebugCategory::NPC:
		DebugController->bEnableNPCDebug = bCategoryEnabled;
		break;
	case EUOUDebugCategory::Puzzle:
		DebugController->bEnablePuzzleDebug = bCategoryEnabled;
		break;
	case EUOUDebugCategory::Interaction:
		DebugController->bEnableInteractionDebug = bCategoryEnabled;
		break;
	case EUOUDebugCategory::VFX:
		DebugController->bEnableVFXDebug = bCategoryEnabled;
		break;
	case EUOUDebugCategory::Performance:
		DebugController->bEnablePerformanceDebug = bCategoryEnabled;
		break;
	case EUOUDebugCategory::System:
	default:
		break;
	}

	if (UUOUDebugControllerComponentBase* ControllerComponent =
		DebugController->FindDebugControllerComponent(Category))
	{
		ControllerComponent->SetDebugEnabled(bCategoryEnabled);
	}
}

AUOUDebugController* UUOUDevelopmentDebugControlSubsystem::ResolveDebugController() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>();
	return DebugSubsystem != nullptr ? DebugSubsystem->GetActiveDebugController() : nullptr;
}
