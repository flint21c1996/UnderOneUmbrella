// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugController.h"

#include "Components/SceneComponent.h"
#include "Debug/UOUDebugControllerComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Debug/UOUDebugProviderComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "Debug/UOUDevelopmentToolsBuild.h"
#include "Debug/UOUPuzzleDebugProviderComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"

#if UOU_WITH_DEVELOPMENT_TOOLS

DEFINE_LOG_CATEGORY_STATIC(LogUOUDebugUtility, Log, All);

namespace UOUDebugControllerPrivate
{
	constexpr float DefaultWorldDebugVisibleDistance = 1500.0f;
	constexpr int32 DefaultMaxVisibleWorldDebugItems = 12;
	constexpr int32 DefaultMaxWorldDebugLabelLines = 8;

	UWorld* ResolveWorld(const UObject* WorldContextObject)
	{
		if (WorldContextObject != nullptr)
		{
			if (UWorld* World = WorldContextObject->GetWorld())
			{
				return World;
			}
		}

		if (GEngine == nullptr)
		{
			return nullptr;
		}

		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UWorld* World = WorldContext.World();
			if (World != nullptr && (World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Game))
			{
				return World;
			}
		}

		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UWorld* World = WorldContext.World();
			if (World != nullptr && World->WorldType == EWorldType::Editor)
			{
				return World;
			}
		}

		return nullptr;
	}

	void MarkControllerChanged(AUOUDebugController* DebugController)
	{
		if (DebugController == nullptr)
		{
			return;
		}

#if WITH_EDITOR
		DebugController->Modify();
#endif
	}

	void SetCategoryEnabled(AUOUDebugController* DebugController, EUOUDebugCategory Category, bool bEnabled)
	{
		if (DebugController == nullptr)
		{
			return;
		}

		switch (Category)
		{
		case EUOUDebugCategory::Player:
			DebugController->bEnablePlayerDebug = bEnabled;
			break;
		case EUOUDebugCategory::NPC:
			DebugController->bEnableNPCDebug = bEnabled;
			break;
		case EUOUDebugCategory::Puzzle:
			DebugController->bEnablePuzzleDebug = bEnabled;
			break;
		case EUOUDebugCategory::Interaction:
			DebugController->bEnableInteractionDebug = bEnabled;
			break;
		case EUOUDebugCategory::VFX:
			DebugController->bEnableVFXDebug = bEnabled;
			break;
		case EUOUDebugCategory::Performance:
			DebugController->bEnablePerformanceDebug = bEnabled;
			break;
		case EUOUDebugCategory::System:
		default:
			break;
		}

		if (UUOUDebugControllerComponentBase* ControllerComponent = DebugController->FindDebugControllerComponent(Category))
		{
			ControllerComponent->SetDebugEnabled(bEnabled);
		}
	}

	void SetAllCategoriesEnabled(AUOUDebugController* DebugController, bool bEnabled)
	{
		SetCategoryEnabled(DebugController, EUOUDebugCategory::Player, bEnabled);
		SetCategoryEnabled(DebugController, EUOUDebugCategory::NPC, bEnabled);
		SetCategoryEnabled(DebugController, EUOUDebugCategory::Puzzle, bEnabled);
		SetCategoryEnabled(DebugController, EUOUDebugCategory::Interaction, bEnabled);
		SetCategoryEnabled(DebugController, EUOUDebugCategory::VFX, bEnabled);
		SetCategoryEnabled(DebugController, EUOUDebugCategory::Performance, bEnabled);
	}

	void AddValidationLine(TArray<FString>& Lines, const FString& Line, ELogVerbosity::Type Verbosity = ELogVerbosity::Log)
	{
		Lines.Add(Line);

		if (Verbosity == ELogVerbosity::Warning)
		{
			UE_LOG(LogUOUDebugUtility, Warning, TEXT("%s"), *Line);
		}
		else
		{
			UE_LOG(LogUOUDebugUtility, Log, TEXT("%s"), *Line);
		}
	}
}

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

void AUOUDebugController::EnableAllDebug()
{
	UOUDebugControllerPrivate::MarkControllerChanged(this);

	bEnableDebugTools = true;
	RefreshDebugControllerComponents();
	UOUDebugControllerPrivate::SetAllCategoriesEnabled(this, true);
	LastValidationSummary = TEXT("All debug categories enabled.");
}

void AUOUDebugController::DisableAllDebug()
{
	UOUDebugControllerPrivate::MarkControllerChanged(this);

	bEnableDebugTools = false;
	RefreshDebugControllerComponents();
	UOUDebugControllerPrivate::SetAllCategoriesEnabled(this, false);
	LastValidationSummary = TEXT("All debug categories disabled.");
}

void AUOUDebugController::ShowPuzzleOnly()
{
	UOUDebugControllerPrivate::MarkControllerChanged(this);

	bEnableDebugTools = true;
	RefreshDebugControllerComponents();
	UOUDebugControllerPrivate::SetAllCategoriesEnabled(this, false);
	UOUDebugControllerPrivate::SetCategoryEnabled(this, EUOUDebugCategory::Puzzle, true);
	LastValidationSummary = TEXT("Puzzle debug preset applied.");
}

void AUOUDebugController::ShowGameplayDebug()
{
	UOUDebugControllerPrivate::MarkControllerChanged(this);

	bEnableDebugTools = true;
	RefreshDebugControllerComponents();
	UOUDebugControllerPrivate::SetCategoryEnabled(this, EUOUDebugCategory::Player, true);
	UOUDebugControllerPrivate::SetCategoryEnabled(this, EUOUDebugCategory::NPC, true);
	UOUDebugControllerPrivate::SetCategoryEnabled(this, EUOUDebugCategory::Puzzle, true);
	UOUDebugControllerPrivate::SetCategoryEnabled(this, EUOUDebugCategory::Interaction, true);
	UOUDebugControllerPrivate::SetCategoryEnabled(this, EUOUDebugCategory::VFX, false);
	UOUDebugControllerPrivate::SetCategoryEnabled(this, EUOUDebugCategory::Performance, false);
	LastValidationSummary = TEXT("Gameplay debug preset applied.");
}

void AUOUDebugController::ResetDisplayDefaults()
{
	UOUDebugControllerPrivate::MarkControllerChanged(this);

	WorldDebugVisibleDistance = UOUDebugControllerPrivate::DefaultWorldDebugVisibleDistance;
	MaxVisibleWorldDebugItems = UOUDebugControllerPrivate::DefaultMaxVisibleWorldDebugItems;
	MaxWorldDebugLabelLines = UOUDebugControllerPrivate::DefaultMaxWorldDebugLabelLines;
	bOnlyShowFocusedActor = false;
	bUseWorldTextShadow = true;
	LastValidationSummary = TEXT("Debug display defaults restored.");
}

void AUOUDebugController::ValidateDebugSetup()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		LastValidationSummary = TEXT("Debug setup validation failed: no world.");
		UE_LOG(LogUOUDebugUtility, Warning, TEXT("%s"), *LastValidationSummary);
		return;
	}

	RefreshDebugControllerComponents();

	TArray<FString> ValidationLines;
	int32 ControllerCount = 0;
	for (TActorIterator<AUOUDebugController> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			++ControllerCount;
		}
	}

	UOUDebugControllerPrivate::AddValidationLine(
		ValidationLines,
		FString::Printf(TEXT("Debug Controllers: %d"), ControllerCount),
		ControllerCount == 1 ? ELogVerbosity::Log : ELogVerbosity::Warning);

	int32 ProviderCount = 0;
	int32 DisabledCategoryProviderCount = 0;
	int32 DuplicatePuzzleProviderGroupCount = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
			&& IUOUDebugProvider::Execute_IsDebugProviderEnabled(Actor))
		{
			++ProviderCount;
			const EUOUDebugCategory Category = IUOUDebugProvider::Execute_GetDebugCategory(Actor);
			if (!IsCategoryEnabled(Category))
			{
				++DisabledCategoryProviderCount;
			}
		}

		TInlineComponentArray<UActorComponent*> Components(Actor);
		int32 PuzzleProviderCountOnActor = 0;
		for (UActorComponent* Component : Components)
		{
			if (!IsValid(Component) || !Component->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass()))
			{
				continue;
			}

			if (!IUOUDebugProvider::Execute_IsDebugProviderEnabled(Component))
			{
				continue;
			}

			++ProviderCount;
			const EUOUDebugCategory Category = IUOUDebugProvider::Execute_GetDebugCategory(Component);
			if (!IsCategoryEnabled(Category))
			{
				++DisabledCategoryProviderCount;
			}

			if (Component->IsA<UUOUPuzzleDebugProviderComponent>())
			{
				++PuzzleProviderCountOnActor;
			}
		}

		if (Actor->IsA<AUOUPuzzleConditionGroupActor>() && PuzzleProviderCountOnActor > 1)
		{
			++DuplicatePuzzleProviderGroupCount;
			UOUDebugControllerPrivate::AddValidationLine(
				ValidationLines,
				FString::Printf(TEXT("Duplicate puzzle debug providers: %s (%d)"), *Actor->GetName(), PuzzleProviderCountOnActor),
				ELogVerbosity::Warning);
		}
	}

	UOUDebugControllerPrivate::AddValidationLine(
		ValidationLines,
		FString::Printf(TEXT("Debug Providers: %d"), ProviderCount));

	if (DisabledCategoryProviderCount > 0)
	{
		UOUDebugControllerPrivate::AddValidationLine(
			ValidationLines,
			FString::Printf(TEXT("Providers hidden by disabled categories: %d"), DisabledCategoryProviderCount),
			ELogVerbosity::Warning);
	}

	if (DuplicatePuzzleProviderGroupCount == 0 && DisabledCategoryProviderCount == 0 && ControllerCount == 1)
	{
		UOUDebugControllerPrivate::AddValidationLine(ValidationLines, TEXT("Debug setup validation passed."));
	}

	LastValidationSummary = FString::Join(ValidationLines, LINE_TERMINATOR);
}

void AUOUDebugController::FindOrCreateDebugControllerInLevel()
{
	AUOUDebugController* DebugController = FindOrCreateDebugController(this);
	LastValidationSummary = DebugController != nullptr
		? FString::Printf(TEXT("Debug controller found or created: %s"), *DebugController->GetName())
		: TEXT("Debug controller could not be found or created.");
}

AUOUDebugController* AUOUDebugController::FindOrCreateDebugController(const UObject* WorldContextObject)
{
	UWorld* World = UOUDebugControllerPrivate::ResolveWorld(WorldContextObject);
	if (World == nullptr)
	{
		UE_LOG(LogUOUDebugUtility, Warning, TEXT("FindOrCreateDebugController failed: no world."));
		return nullptr;
	}

	for (TActorIterator<AUOUDebugController> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = MakeUniqueObjectName(World, AUOUDebugController::StaticClass(), TEXT("UOUDebugController"));
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags = RF_Transactional;

	AUOUDebugController* DebugController = World->SpawnActor<AUOUDebugController>(
		AUOUDebugController::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);

	if (DebugController == nullptr)
	{
		UE_LOG(LogUOUDebugUtility, Warning, TEXT("FindOrCreateDebugController failed: spawn failed."));
		return nullptr;
	}

#if WITH_EDITOR
	DebugController->SetActorLabel(TEXT("UOUDebugController"));
	DebugController->Modify();
	DebugController->MarkPackageDirty();
#endif

	UE_LOG(LogUOUDebugUtility, Log, TEXT("Created UOU Debug Controller: %s"), *DebugController->GetName());
	return DebugController;
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

#else

AUOUDebugController::AUOUDebugController()
{
	PrimaryActorTick.bCanEverTick = false;
	bEnableDebugTools = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootSceneComponent);
	PlayerDebugController =
		CreateDefaultSubobject<UUOUPlayerDebugControllerComponent>(TEXT("PlayerDebugController"));
	NPCDebugController =
		CreateDefaultSubobject<UUOUNPCDebugControllerComponent>(TEXT("NPCDebugController"));
	PuzzleDebugController =
		CreateDefaultSubobject<UUOUPuzzleDebugControllerComponent>(TEXT("PuzzleDebugController"));
	InteractionDebugController =
		CreateDefaultSubobject<UUOUInteractionDebugControllerComponent>(TEXT("InteractionDebugController"));
	VFXDebugController =
		CreateDefaultSubobject<UUOUVFXDebugControllerComponent>(TEXT("VFXDebugController"));
	PerformanceDebugController =
		CreateDefaultSubobject<UUOUPerformanceDebugControllerComponent>(TEXT("PerformanceDebugController"));
}

void AUOUDebugController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AUOUDebugController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AUOUDebugController::BeginPlay()
{
	Super::BeginPlay();
}

void AUOUDebugController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AUOUDebugController::RefreshDebugControllerComponents()
{
	DebugControllerComponents.Reset();
}

void AUOUDebugController::EnableAllDebug()
{
	bEnableDebugTools = false;
}

void AUOUDebugController::DisableAllDebug()
{
	bEnableDebugTools = false;
}

void AUOUDebugController::ShowPuzzleOnly()
{
	bEnableDebugTools = false;
}

void AUOUDebugController::ShowGameplayDebug()
{
	bEnableDebugTools = false;
}

void AUOUDebugController::ResetDisplayDefaults()
{
}

void AUOUDebugController::ValidateDebugSetup()
{
	LastValidationSummary = TEXT("Development debug tools are excluded from this build.");
}

void AUOUDebugController::FindOrCreateDebugControllerInLevel()
{
	LastValidationSummary = TEXT("Development debug tools are excluded from this build.");
}

AUOUDebugController* AUOUDebugController::FindOrCreateDebugController(const UObject*)
{
	return nullptr;
}

bool AUOUDebugController::IsCategoryEnabled(EUOUDebugCategory) const
{
	return false;
}

FColor AUOUDebugController::GetDebugCategoryColor(EUOUDebugCategory) const
{
	return FColor::White;
}

FColor AUOUDebugController::GetPuzzleConnectionColor(EUOUDebugConnectionType) const
{
	return FColor::White;
}

FColor AUOUDebugController::GetDebugConnectionColor(const FUOUDebugConnection& Connection) const
{
	return Connection.Color;
}

UUOUDebugControllerComponentBase* AUOUDebugController::FindDebugControllerComponent(
	EUOUDebugCategory) const
{
	return nullptr;
}

const TArray<TObjectPtr<UUOUDebugControllerComponentBase>>&
AUOUDebugController::GetDebugControllerComponents() const
{
	return DebugControllerComponents;
}

#endif
