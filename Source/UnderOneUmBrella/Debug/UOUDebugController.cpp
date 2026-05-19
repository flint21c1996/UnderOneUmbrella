// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugController.h"

#include "Components/SceneComponent.h"
#include "Debug/UOUDebugControllerComponent.h"
#include "Debug/UOUDebugSubsystem.h"

AUOUDebugController::AUOUDebugController()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootSceneComponent);

	PlayerDebugController = CreateDefaultSubobject<UUOUPlayerDebugControllerComponent>(TEXT("PlayerDebugController"));
	NPCDebugController = CreateDefaultSubobject<UUOUNPCDebugControllerComponent>(TEXT("NPCDebugController"));
	PuzzleDebugController = CreateDefaultSubobject<UUOUPuzzleDebugControllerComponent>(TEXT("PuzzleDebugController"));
	InteractionDebugController = CreateDefaultSubobject<UUOUInteractionDebugControllerComponent>(TEXT("InteractionDebugController"));
	VFXDebugController = CreateDefaultSubobject<UUOUVFXDebugControllerComponent>(TEXT("VFXDebugController"));
	PerformanceDebugController = CreateDefaultSubobject<UUOUPerformanceDebugControllerComponent>(TEXT("PerformanceDebugController"));
}

void AUOUDebugController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshDebugControllerComponents();
}

void AUOUDebugController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	RefreshDebugControllerComponents();
}

void AUOUDebugController::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>())
		{
			DebugSubsystem->RegisterDebugController(this);
		}
	}
}

void AUOUDebugController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>())
		{
			DebugSubsystem->UnregisterDebugController(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AUOUDebugController::RefreshDebugControllerComponents()
{
	DebugControllerComponents.Reset();

	TArray<UUOUDebugControllerComponentBase*> Components;
	GetComponents<UUOUDebugControllerComponentBase>(Components);

	Components.Sort(
		[](const UUOUDebugControllerComponentBase& Left, const UUOUDebugControllerComponentBase& Right)
		{
			return Left.Priority > Right.Priority;
		});

	for (UUOUDebugControllerComponentBase* Component : Components)
	{
		if (IsValid(Component))
		{
			DebugControllerComponents.Add(Component);
		}
	}
}

bool AUOUDebugController::IsCategoryEnabled(EUOUDebugCategory Category) const
{
	if (!bEnableDebugTools)
	{
		return false;
	}

	bool bCategoryEnabled = false;
	switch (Category)
	{
	case EUOUDebugCategory::Player:
		bCategoryEnabled = bEnablePlayerDebug;
		break;
	case EUOUDebugCategory::NPC:
		bCategoryEnabled = bEnableNPCDebug;
		break;
	case EUOUDebugCategory::Puzzle:
		bCategoryEnabled = bEnablePuzzleDebug;
		break;
	case EUOUDebugCategory::Interaction:
		bCategoryEnabled = bEnableInteractionDebug;
		break;
	case EUOUDebugCategory::VFX:
		bCategoryEnabled = bEnableVFXDebug;
		break;
	case EUOUDebugCategory::Performance:
		bCategoryEnabled = bEnablePerformanceDebug;
		break;
	case EUOUDebugCategory::System:
		bCategoryEnabled = true;
		break;
	default:
		bCategoryEnabled = false;
		break;
	}

	if (!bCategoryEnabled)
	{
		return false;
	}

	const UUOUDebugControllerComponentBase* ControllerComponent = FindDebugControllerComponent(Category);
	return ControllerComponent == nullptr || ControllerComponent->IsDebugEnabled();
}

FColor AUOUDebugController::GetDebugCategoryColor(EUOUDebugCategory Category) const
{
	switch (Category)
	{
	case EUOUDebugCategory::Player:
		return PlayerDebugColor;
	case EUOUDebugCategory::NPC:
		return NPCDebugColor;
	case EUOUDebugCategory::Puzzle:
		return PuzzleDebugColor;
	case EUOUDebugCategory::Interaction:
		return InteractionDebugColor;
	case EUOUDebugCategory::VFX:
		return VFXDebugColor;
	case EUOUDebugCategory::Performance:
		return PerformanceDebugColor;
	case EUOUDebugCategory::System:
		return SystemDebugColor;
	default:
		return FColor::White;
	}
}

FColor AUOUDebugController::GetPuzzleConnectionColor(EUOUDebugConnectionType ConnectionType) const
{
	switch (ConnectionType)
	{
	case EUOUDebugConnectionType::PuzzleInput:
		return PuzzleInputConnectionColor;
	case EUOUDebugConnectionType::PuzzleCondition:
		return PuzzleConditionConnectionColor;
	case EUOUDebugConnectionType::PuzzleResult:
		return PuzzleResultConnectionColor;
	default:
		return PuzzleDebugColor;
	}
}

FColor AUOUDebugController::GetDebugConnectionColor(const FUOUDebugConnection& Connection) const
{
	const bool bPuzzleConnection =
		Connection.ConnectionType == EUOUDebugConnectionType::PuzzleInput
		|| Connection.ConnectionType == EUOUDebugConnectionType::PuzzleCondition
		|| Connection.ConnectionType == EUOUDebugConnectionType::PuzzleResult;

	if (bPuzzleConnection && bOverrideProviderPuzzleConnectionColors)
	{
		return GetPuzzleConnectionColor(Connection.ConnectionType);
	}

	return Connection.Color;
}

UUOUDebugControllerComponentBase* AUOUDebugController::FindDebugControllerComponent(EUOUDebugCategory Category) const
{
	for (UUOUDebugControllerComponentBase* Component : DebugControllerComponents)
	{
		if (IsValid(Component) && Component->DebugCategory == Category)
		{
			return Component;
		}
	}

	return nullptr;
}

const TArray<TObjectPtr<UUOUDebugControllerComponentBase>>& AUOUDebugController::GetDebugControllerComponents() const
{
	return DebugControllerComponents;
}
