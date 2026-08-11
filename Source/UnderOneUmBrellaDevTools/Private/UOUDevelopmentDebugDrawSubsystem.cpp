// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentDebugDrawSubsystem.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Debug/UOUPuzzleDebugProviderComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Debug/UOUDevelopmentToolsBuild.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UOUDevelopmentDebugControlSubsystem.h"

#if !UOU_WITH_DEVELOPMENT_TOOLS
#error UOUDevelopmentDebugDrawSubsystem must only be compiled when development tools are enabled.
#endif

namespace UOUDevelopmentDebugDrawPrivate
{
	constexpr float PuzzleProviderRefreshIntervalSeconds = 1.0f;

	FString GetMovementModeName(const UCharacterMovementComponent* MovementComponent)
	{
		if (MovementComponent == nullptr)
		{
			return TEXT("None");
		}

		switch (MovementComponent->MovementMode)
		{
		case MOVE_None:
			return TEXT("None");
		case MOVE_Walking:
			return TEXT("Walking");
		case MOVE_NavWalking:
			return TEXT("NavWalking");
		case MOVE_Falling:
			return TEXT("Falling");
		case MOVE_Swimming:
			return TEXT("Swimming");
		case MOVE_Flying:
			return TEXT("Flying");
		case MOVE_Custom:
			return FString::Printf(TEXT("Custom(%d)"), MovementComponent->CustomMovementMode);
		default:
			return TEXT("Unknown");
		}
	}

	bool TryGetDebugObjectLocation(const UObject* Object, FVector& OutLocation)
	{
		if (const AActor* Actor = Cast<AActor>(Object))
		{
			OutLocation = Actor->GetActorLocation();
			return true;
		}

		if (const USceneComponent* SceneComponent = Cast<USceneComponent>(Object))
		{
			OutLocation = SceneComponent->GetComponentLocation();
			return true;
		}

		if (const UUOUPuzzleDebugProviderComponent* PuzzleDebugProvider =
			Cast<UUOUPuzzleDebugProviderComponent>(Object))
		{
			OutLocation = PuzzleDebugProvider->GetConditionGroupNodeWorldLocation();
			return true;
		}

		if (const UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
		{
			if (const AActor* Owner = ActorComponent->GetOwner())
			{
				OutLocation = Owner->GetActorLocation();
				return true;
			}
		}

		return false;
	}

	FString BuildPuzzleProviderLabelText(UObject* ProviderObject)
	{
		if (!IsValid(ProviderObject))
		{
			return FString();
		}

		const FText DisplayName = IUOUDebugProvider::Execute_GetDebugDisplayName(ProviderObject);
		const FText SummaryText = IUOUDebugProvider::Execute_GetDebugSummaryText(ProviderObject);
		FString LabelText = DisplayName.IsEmpty()
			? ProviderObject->GetName()
			: DisplayName.ToString();

		if (!SummaryText.IsEmpty())
		{
			LabelText += LINE_TERMINATOR;
			LabelText += SummaryText.ToString();
		}

		return LabelText;
	}
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
	PlayerDebugText.Reset();
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
		PlayerDebugText.Reset();
		return;
	}

	if (ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::Player))
	{
		RefreshPlayerDebugText();
	}
	else
	{
		PlayerDebugText.Reset();
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

	DrawPuzzleProviderConnections();
	DrawPuzzleProviderLabels();
}

void UUOUDevelopmentDebugDrawSubsystem::RefreshPlayerDebugText()
{
	UWorld* World = GetWorld();
	const APlayerController* PlayerController = World != nullptr
		? World->GetFirstPlayerController()
		: nullptr;
	const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	if (PlayerPawn == nullptr)
	{
		PlayerDebugText = TEXT("Player\nPawn: None");
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(PlayerPawn);
	const UCharacterMovementComponent* MovementComponent = Character != nullptr
		? Character->GetCharacterMovement()
		: nullptr;
	const FVector Velocity = PlayerPawn->GetVelocity();

	PlayerDebugText = FString::Printf(
		TEXT("Player: %s\nState: %s | Grounded: %s | Speed: %.1f\nLocation: %s | ZVel: %.1f"),
		*PlayerPawn->GetName(),
		*UOUDevelopmentDebugDrawPrivate::GetMovementModeName(MovementComponent),
		MovementComponent != nullptr && MovementComponent->IsMovingOnGround() ? TEXT("Yes") : TEXT("No"),
		Velocity.Size2D(),
		*PlayerPawn->GetActorLocation().ToCompactString(),
		Velocity.Z);
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

void UUOUDevelopmentDebugDrawSubsystem::DrawPuzzleProviderConnections() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (const TWeakObjectPtr<UObject>& WeakProviderObject : PuzzleDebugProviders)
	{
		UObject* ProviderObject = WeakProviderObject.Get();
		if (!IsValid(ProviderObject)
			|| !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
			|| !IUOUDebugProvider::Execute_IsDebugProviderEnabled(ProviderObject))
		{
			continue;
		}

		TArray<FUOUDebugConnection> Connections;
		IUOUDebugProvider::Execute_GetDebugConnections(ProviderObject, Connections);
		for (const FUOUDebugConnection& Connection : Connections)
		{
			FVector SourceLocation = FVector::ZeroVector;
			FVector TargetLocation = FVector::ZeroVector;
			if (!UOUDevelopmentDebugDrawPrivate::TryGetDebugObjectLocation(
					Connection.SourceObject.Get(),
					SourceLocation)
				|| !UOUDevelopmentDebugDrawPrivate::TryGetDebugObjectLocation(
					Connection.TargetObject.Get(),
					TargetLocation)
				|| SourceLocation.Equals(TargetLocation))
			{
				continue;
			}

			DrawDebugDirectionalArrow(
				World,
				SourceLocation,
				TargetLocation,
				80.0f,
				Connection.Color,
				false,
				0.0f,
				0,
				FMath::Max(0.0f, Connection.Thickness));
		}
	}
}

void UUOUDevelopmentDebugDrawSubsystem::DrawPuzzleProviderLabels() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (const TWeakObjectPtr<UObject>& WeakProviderObject : PuzzleDebugProviders)
	{
		UObject* ProviderObject = WeakProviderObject.Get();
		if (!IsValid(ProviderObject)
			|| !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
			|| !IUOUDebugProvider::Execute_IsDebugProviderEnabled(ProviderObject))
		{
			continue;
		}

		const FString LabelText =
			UOUDevelopmentDebugDrawPrivate::BuildPuzzleProviderLabelText(ProviderObject);
		if (LabelText.IsEmpty())
		{
			continue;
		}

		DrawDebugString(
			World,
			IUOUDebugProvider::Execute_GetDebugWorldLocation(ProviderObject),
			LabelText,
			nullptr,
			FColor::White,
			0.0f,
			true,
			1.0f);
	}
}
