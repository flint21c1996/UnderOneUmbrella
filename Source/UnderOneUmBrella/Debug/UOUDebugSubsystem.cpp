// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugSubsystem.h"

#include "Debug/UOUDebugController.h"
#include "Debug/UOUDebugControllerComponent.h"
#include "Debug/UOUDevelopmentToolsBuild.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#if UOU_WITH_DEVELOPMENT_TOOLS

namespace UOUDebugSubsystemPrivate
{
	const UUOUDebugSubsystem* GetDebugSubsystem(const UObject* WorldContextObject)
	{
		if (WorldContextObject == nullptr)
		{
			return nullptr;
		}

		UWorld* World = WorldContextObject->GetWorld();
		return World != nullptr ? World->GetSubsystem<UUOUDebugSubsystem>() : nullptr;
	}
}

bool UUOUDebugSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr && World->IsGameWorld();
}

void UUOUDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ControllerSearchTimeRemaining = 0.0f;
}

void UUOUDebugSubsystem::Deinitialize()
{
	ActiveDebugController.Reset();

	Super::Deinitialize();
}

void UUOUDebugSubsystem::Tick(float DeltaTime)
{
	ControllerSearchTimeRemaining -= DeltaTime;
	if (!ActiveDebugController.IsValid() || ControllerSearchTimeRemaining <= 0.0f)
	{
		ResolveDebugController();
		ControllerSearchTimeRemaining = 1.0f;
	}
}

TStatId UUOUDebugSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUOUDebugSubsystem, STATGROUP_Tickables);
}

void UUOUDebugSubsystem::RegisterDebugController(AUOUDebugController* DebugController)
{
	if (IsValid(DebugController))
	{
		DebugController->RefreshDebugControllerComponents();
		ActiveDebugController = DebugController;
	}
}

void UUOUDebugSubsystem::UnregisterDebugController(AUOUDebugController* DebugController)
{
	if (ActiveDebugController.Get() == DebugController)
	{
		ActiveDebugController.Reset();
		ControllerSearchTimeRemaining = 0.0f;
	}
}

AUOUDebugController* UUOUDebugSubsystem::GetActiveDebugController() const
{
	return ActiveDebugController.Get();
}

bool UUOUDebugSubsystem::IsDebugEnabled(EUOUDebugCategory Category) const
{
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	return DebugController != nullptr && DebugController->IsCategoryEnabled(Category);
}

bool UUOUDebugSubsystem::IsScreenMessageEnabled(EUOUDebugCategory Category) const
{
	if (!IsDebugEnabled(Category))
	{
		return false;
	}

	switch (Category)
	{
	case EUOUDebugCategory::Player:
		if (const UUOUPlayerDebugControllerComponent* PlayerController =
			Cast<UUOUPlayerDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return PlayerController->bShowViewportHUD;
		}
		return true;
	case EUOUDebugCategory::Interaction:
		if (const UUOUInteractionDebugControllerComponent* InteractionController =
			Cast<UUOUInteractionDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return InteractionController->bShowScreenDebug;
		}
		return true;
	case EUOUDebugCategory::Puzzle:
		if (const UUOUPuzzleDebugControllerComponent* PuzzleController =
			Cast<UUOUPuzzleDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return PuzzleController->bShowSummaryText;
		}
		return true;
	case EUOUDebugCategory::Performance:
		if (const UUOUPerformanceDebugControllerComponent* PerformanceController =
			Cast<UUOUPerformanceDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return PerformanceController->bShowViewportStats;
		}
		return true;
	default:
		return true;
	}
}

bool UUOUDebugSubsystem::IsWorldDrawEnabled(EUOUDebugCategory Category) const
{
	if (!IsDebugEnabled(Category))
	{
		return false;
	}

	switch (Category)
	{
	case EUOUDebugCategory::Player:
		if (const UUOUPlayerDebugControllerComponent* PlayerController =
			Cast<UUOUPlayerDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return PlayerController->bShowWorldDebug;
		}
		return true;
	case EUOUDebugCategory::NPC:
		if (const UUOUNPCDebugControllerComponent* NPCController =
			Cast<UUOUNPCDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return NPCController->bShowMoveTarget || NPCController->bShowPath;
		}
		return true;
	case EUOUDebugCategory::Puzzle:
		if (const UUOUPuzzleDebugControllerComponent* PuzzleController =
			Cast<UUOUPuzzleDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return PuzzleController->bShowConnections || PuzzleController->bShowHeatWirePathDebug;
		}
		return true;
	case EUOUDebugCategory::Interaction:
		if (const UUOUInteractionDebugControllerComponent* InteractionController =
			Cast<UUOUInteractionDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return InteractionController->bShowTrace || InteractionController->bShowCandidate;
		}
		return true;
	case EUOUDebugCategory::VFX:
		if (const UUOUVFXDebugControllerComponent* VFXController =
			Cast<UUOUVFXDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return VFXController->bShowWorldDebug;
		}
		return true;
	default:
		return true;
	}
}

bool UUOUDebugSubsystem::IsWorldLabelEnabled(EUOUDebugCategory Category) const
{
	if (!IsDebugEnabled(Category))
	{
		return false;
	}

	switch (Category)
	{
	case EUOUDebugCategory::Player:
		if (const UUOUPlayerDebugControllerComponent* PlayerController =
			Cast<UUOUPlayerDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return PlayerController->bShowWorldDebug;
		}
		return true;
	case EUOUDebugCategory::NPC:
		if (const UUOUNPCDebugControllerComponent* NPCController =
			Cast<UUOUNPCDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return NPCController->bShowWorldLabels;
		}
		return true;
	case EUOUDebugCategory::Puzzle:
		if (const UUOUPuzzleDebugControllerComponent* PuzzleController =
			Cast<UUOUPuzzleDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return PuzzleController->bShowWorldLabels;
		}
		return true;
	case EUOUDebugCategory::Interaction:
		if (const UUOUInteractionDebugControllerComponent* InteractionController =
			Cast<UUOUInteractionDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return InteractionController->bShowCandidate;
		}
		return true;
	case EUOUDebugCategory::VFX:
		if (const UUOUVFXDebugControllerComponent* VFXController =
			Cast<UUOUVFXDebugControllerComponent>(FindDebugControllerComponent(Category)))
		{
			return VFXController->bShowWorldDebug;
		}
		return true;
	default:
		return true;
	}
}

bool UUOUDebugSubsystem::IsDebugCategoryEnabled(
	const UObject* WorldContextObject,
	EUOUDebugCategory Category)
{
	const UUOUDebugSubsystem* DebugSubsystem =
		UOUDebugSubsystemPrivate::GetDebugSubsystem(WorldContextObject);
	return DebugSubsystem != nullptr && DebugSubsystem->IsDebugEnabled(Category);
}

bool UUOUDebugSubsystem::IsDebugScreenMessageEnabled(
	const UObject* WorldContextObject,
	EUOUDebugCategory Category)
{
	const UUOUDebugSubsystem* DebugSubsystem =
		UOUDebugSubsystemPrivate::GetDebugSubsystem(WorldContextObject);
	return DebugSubsystem != nullptr && DebugSubsystem->IsScreenMessageEnabled(Category);
}

bool UUOUDebugSubsystem::IsDebugWorldDrawEnabled(
	const UObject* WorldContextObject,
	EUOUDebugCategory Category)
{
	const UUOUDebugSubsystem* DebugSubsystem =
		UOUDebugSubsystemPrivate::GetDebugSubsystem(WorldContextObject);
	return DebugSubsystem != nullptr && DebugSubsystem->IsWorldDrawEnabled(Category);
}

bool UUOUDebugSubsystem::IsDebugWorldLabelEnabled(
	const UObject* WorldContextObject,
	EUOUDebugCategory Category)
{
	const UUOUDebugSubsystem* DebugSubsystem =
		UOUDebugSubsystemPrivate::GetDebugSubsystem(WorldContextObject);
	return DebugSubsystem != nullptr && DebugSubsystem->IsWorldLabelEnabled(Category);
}

FColor UUOUDebugSubsystem::GetDebugCategoryColor(
	const UObject* WorldContextObject,
	EUOUDebugCategory Category,
	FColor FallbackColor)
{
	const UUOUDebugSubsystem* DebugSubsystem =
		UOUDebugSubsystemPrivate::GetDebugSubsystem(WorldContextObject);
	const AUOUDebugController* DebugController = DebugSubsystem != nullptr
		? DebugSubsystem->GetActiveDebugController()
		: nullptr;
	return DebugController != nullptr
		? DebugController->GetDebugCategoryColor(Category)
		: FallbackColor;
}

UUOUDebugControllerComponentBase* UUOUDebugSubsystem::FindDebugControllerComponent(
	EUOUDebugCategory Category) const
{
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	return DebugController != nullptr
		? DebugController->FindDebugControllerComponent(Category)
		: nullptr;
}

void UUOUDebugSubsystem::ResolveDebugController()
{
	if (ActiveDebugController.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (TActorIterator<AUOUDebugController> It(World); It; ++It)
	{
		if (AUOUDebugController* DebugController = *It)
		{
			RegisterDebugController(DebugController);
			return;
	}
}

#else

bool UUOUDebugSubsystem::ShouldCreateSubsystem(UObject*) const
{
	return false;
}

void UUOUDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UUOUDebugSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UUOUDebugSubsystem::Tick(float)
{
}

TStatId UUOUDebugSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUOUDebugSubsystem, STATGROUP_Tickables);
}

void UUOUDebugSubsystem::RegisterDebugController(AUOUDebugController*)
{
}

void UUOUDebugSubsystem::UnregisterDebugController(AUOUDebugController*)
{
}

AUOUDebugController* UUOUDebugSubsystem::GetActiveDebugController() const
{
	return nullptr;
}

bool UUOUDebugSubsystem::IsDebugEnabled(EUOUDebugCategory) const
{
	return false;
}

bool UUOUDebugSubsystem::IsScreenMessageEnabled(EUOUDebugCategory) const
{
	return false;
}

bool UUOUDebugSubsystem::IsWorldDrawEnabled(EUOUDebugCategory) const
{
	return false;
}

bool UUOUDebugSubsystem::IsWorldLabelEnabled(EUOUDebugCategory) const
{
	return false;
}

bool UUOUDebugSubsystem::IsDebugCategoryEnabled(
	const UObject*,
	EUOUDebugCategory)
{
	return false;
}

bool UUOUDebugSubsystem::IsDebugScreenMessageEnabled(
	const UObject*,
	EUOUDebugCategory)
{
	return false;
}

bool UUOUDebugSubsystem::IsDebugWorldDrawEnabled(
	const UObject*,
	EUOUDebugCategory)
{
	return false;
}

bool UUOUDebugSubsystem::IsDebugWorldLabelEnabled(
	const UObject*,
	EUOUDebugCategory)
{
	return false;
}

FColor UUOUDebugSubsystem::GetDebugCategoryColor(
	const UObject*,
	EUOUDebugCategory,
	FColor FallbackColor)
{
	return FallbackColor;
}

UUOUDebugControllerComponentBase* UUOUDebugSubsystem::FindDebugControllerComponent(
	EUOUDebugCategory) const
{
	return nullptr;
}

void UUOUDebugSubsystem::ResolveDebugController()
{
}

#endif

}
