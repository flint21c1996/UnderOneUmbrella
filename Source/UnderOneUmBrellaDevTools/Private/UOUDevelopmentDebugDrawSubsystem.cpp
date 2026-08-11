// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentDebugDrawSubsystem.h"

#include "Components/ActorComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Debug/UOUDevelopmentToolsBuild.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UOUDevelopmentDebugControlSubsystem.h"

#if !UOU_WITH_DEVELOPMENT_TOOLS
#error UOUDevelopmentDebugDrawSubsystem must only be compiled when development tools are enabled.
#endif

namespace UOUDevelopmentDebugDrawPrivate
{
	constexpr float PuzzleProviderRefreshIntervalSeconds = 1.0f;
}

bool UUOUDevelopmentDebugDrawSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr
		&& World->IsGameWorld()
		&& World->GetNetMode() != NM_DedicatedServer;
}

void UUOUDevelopmentDebugDrawSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UUOUDevelopmentDebugControlSubsystem>();
	if (UWorld* World = GetWorld())
	{
		DebugControlSubsystem = World->GetSubsystem<UUOUDevelopmentDebugControlSubsystem>();
	}
	PuzzleProviderRefreshTimeRemaining = 0.0f;
}

void UUOUDevelopmentDebugDrawSubsystem::Deinitialize()
{
	PuzzleDebugProviders.Reset();
	PuzzleProviderRefreshTimeRemaining = 0.0f;
	DebugControlSubsystem.Reset();
	Super::Deinitialize();
}

void UUOUDevelopmentDebugDrawSubsystem::Tick(float DeltaTime)
{
	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem = DebugControlSubsystem.Get();
	if (ControlSubsystem == nullptr || !ControlSubsystem->IsDebugToolsEnabled())
	{
		return;
	}

	if (!ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::Puzzle))
	{
		return;
	}

	PuzzleProviderRefreshTimeRemaining -= DeltaTime;
	if (PuzzleProviderRefreshTimeRemaining <= 0.0f)
	{
		RefreshPuzzleDebugProviders();
		PuzzleProviderRefreshTimeRemaining =
			UOUDevelopmentDebugDrawPrivate::PuzzleProviderRefreshIntervalSeconds;
	}
}

TStatId UUOUDevelopmentDebugDrawSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUOUDevelopmentDebugDrawSubsystem, STATGROUP_Tickables);
}

void UUOUDevelopmentDebugDrawSubsystem::RefreshPuzzleDebugProviders()
{
	PuzzleDebugProviders.Reset();

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		TryAddPuzzleDebugProvider(Actor);

		TInlineComponentArray<UActorComponent*> Components(Actor);
		for (UActorComponent* Component : Components)
		{
			TryAddPuzzleDebugProvider(Component);
		}
	}
}

int32 UUOUDevelopmentDebugDrawSubsystem::GetPuzzleDebugProviderCount() const
{
	int32 ValidProviderCount = 0;
	for (const TWeakObjectPtr<UObject>& ProviderObject : PuzzleDebugProviders)
	{
		if (ProviderObject.IsValid())
		{
			++ValidProviderCount;
		}
	}

	return ValidProviderCount;
}

void UUOUDevelopmentDebugDrawSubsystem::TryAddPuzzleDebugProvider(UObject* ProviderObject)
{
	if (!IsValid(ProviderObject)
		|| !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
		|| IUOUDebugProvider::Execute_GetDebugCategory(ProviderObject) != EUOUDebugCategory::Puzzle)
	{
		return;
	}

	PuzzleDebugProviders.AddUnique(TWeakObjectPtr<UObject>(ProviderObject));
}
