// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentDebugDrawSubsystem.h"

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
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
#include "InputCoreTypes.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUInteractionComponent.h"
#include "Player/UOUPushPullInteractorComponent.h"
#include "Player/UOURainReceiverComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Player/UOUUmbrellaLightInteractionComponent.h"
#include "Player/UOUWaterContainerComponent.h"
#include "Puzzle/PushPull/UOUPushPullObjectComponent.h"
#include "UOUDevelopmentDebugControlSubsystem.h"
#include "World/Light/UOULightInteractionSurfaceComponent.h"

#if !UOU_WITH_DEVELOPMENT_TOOLS
#error UOUDevelopmentDebugDrawSubsystem must only be compiled when development tools are enabled.
#endif

namespace UOUDevelopmentDebugDrawPrivate
{
	constexpr float PuzzleProviderRefreshIntervalSeconds = 1.0f;

	const TCHAR* GetYesNo(bool bValue)
	{
		return bValue ? TEXT("Yes") : TEXT("No");
	}

	FString GetActorDebugName(const AActor* Actor)
	{
		return Actor != nullptr ? Actor->GetName() : TEXT("None");
	}

	FString GetComponentDebugName(const UActorComponent* Component)
	{
		if (Component == nullptr)
		{
			return TEXT("None");
		}

		const AActor* Owner = Component->GetOwner();
		return Owner != nullptr
			? FString::Printf(TEXT("%s.%s"), *Owner->GetName(), *Component->GetName())
			: Component->GetName();
	}

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

	FString GetEnumValueName(const UEnum* Enum, int64 Value)
	{
		return Enum != nullptr ? Enum->GetNameStringByValue(Value) : TEXT("Unknown");
	}

	FString GetUmbrellaStateName(EUOUUmbrellaState UmbrellaState)
	{
		return GetEnumValueName(StaticEnum<EUOUUmbrellaState>(), static_cast<int64>(UmbrellaState));
	}

	FString GetUmbrellaDirectionStateName(EUOUUmbrellaDirectionState UmbrellaState)
	{
		return GetEnumValueName(
			StaticEnum<EUOUUmbrellaDirectionState>(),
			static_cast<int64>(UmbrellaState));
	}

	FString GetPourReceiverTypeName(EUOUUmbrellaPourReceiverType ReceiverType)
	{
		return GetEnumValueName(
			StaticEnum<EUOUUmbrellaPourReceiverType>(),
			static_cast<int64>(ReceiverType));
	}

	FString GetLightInteractionModeName(EUOULightInteractionMode LightInteractionMode)
	{
		return GetEnumValueName(
			StaticEnum<EUOULightInteractionMode>(),
			static_cast<int64>(LightInteractionMode));
	}

	FString FormatVectorCompact(const FVector& Vector)
	{
		return FString::Printf(TEXT("%.0f, %.0f, %.0f"), Vector.X, Vector.Y, Vector.Z);
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
	const APlayerController* LocalPlayerController = World != nullptr
		? World->GetFirstPlayerController()
		: nullptr;
	APawn* PlayerPawn = LocalPlayerController != nullptr ? LocalPlayerController->GetPawn() : nullptr;
	if (PlayerPawn == nullptr)
	{
		PlayerDebugText = TEXT("Player\nPawn: None");
		return;
	}

	AUOUCharacter* UOUCharacter = Cast<AUOUCharacter>(PlayerPawn);
	ACharacter* Character = Cast<ACharacter>(PlayerPawn);
	const UCharacterMovementComponent* MovementComponent = Character != nullptr
		? Character->GetCharacterMovement()
		: nullptr;
	const UUOUUmbrellaComponent* UmbrellaComponent =
		PlayerPawn->FindComponentByClass<UUOUUmbrellaComponent>();
	const UUOUPushPullInteractorComponent* PushPullComponent = UOUCharacter != nullptr
		? UOUCharacter->GetPushPullInteractorComponent()
		: PlayerPawn->FindComponentByClass<UUOUPushPullInteractorComponent>();
	const UUOUInteractionComponent* InteractionComponent =
		PlayerPawn->FindComponentByClass<UUOUInteractionComponent>();
	const UUOUCameraControllerComponent* CameraControllerComponent = UOUCharacter != nullptr
		? UOUCharacter->GetCameraControllerComponent()
		: PlayerPawn->FindComponentByClass<UUOUCameraControllerComponent>();
	const UUOUUmbrellaLightInteractionComponent* UmbrellaLightInteractionComponent =
		PlayerPawn->FindComponentByClass<UUOUUmbrellaLightInteractionComponent>();

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Player: %s"), *PlayerPawn->GetName()));

	const FVector Velocity = PlayerPawn->GetVelocity();
	TArray<FString> JumpBlockedReasons;
	if (PushPullComponent != nullptr && PushPullComponent->BlocksJumping())
	{
		JumpBlockedReasons.Add(TEXT("PushPull"));
	}
	if (UmbrellaComponent != nullptr && UmbrellaComponent->BlocksJumping())
	{
		JumpBlockedReasons.Add(TEXT("Umbrella"));
	}

	Lines.Add(FString::Printf(
		TEXT("State: %s | Grounded: %s | Speed: %.1f"),
		*UOUDevelopmentDebugDrawPrivate::GetMovementModeName(MovementComponent),
		UOUDevelopmentDebugDrawPrivate::GetYesNo(
			MovementComponent != nullptr && MovementComponent->IsMovingOnGround()),
		Velocity.Size2D()));
	Lines.Add(FString::Printf(
		TEXT("Location: %s | ZVel: %.1f"),
		*UOUDevelopmentDebugDrawPrivate::FormatVectorCompact(PlayerPawn->GetActorLocation()),
		Velocity.Z));
	Lines.Add(FString::Printf(
		TEXT("Jump: %s | Blocked: %s"),
		UOUDevelopmentDebugDrawPrivate::GetYesNo(Character != nullptr && Character->CanJump()),
		JumpBlockedReasons.Num() > 0 ? *FString::Join(JumpBlockedReasons, TEXT(", ")) : TEXT("No")));

	const bool bPhysicalContextKeyDown = LocalPlayerController != nullptr
		&& LocalPlayerController->IsInputKeyDown(EKeys::RightMouseButton);
	const bool bRoutesToPour = UmbrellaComponent != nullptr
		&& UmbrellaComponent->HasUmbrella()
		&& UmbrellaComponent->IsUpsideDown()
		&& UmbrellaComponent->GetCurrentStoredWater() > 0.0f;
	Lines.Add(FString::Printf(
		TEXT("Input: RMB %s | Context Route: %s"),
		UOUDevelopmentDebugDrawPrivate::GetYesNo(bPhysicalContextKeyDown),
		bRoutesToPour ? TEXT("Pour") : TEXT("PushPull")));
	if (UOUCharacter != nullptr)
	{
		Lines.Add(FString::Printf(
			TEXT("Counts: Context %d/%d | PushPull %d/%d"),
			UOUCharacter->GetContextInteractPressedCount(),
			UOUCharacter->GetContextInteractReleasedCount(),
			UOUCharacter->GetPushPullPressedCount(),
			UOUCharacter->GetPushPullReleasedCount()));
	}

	if (UmbrellaComponent == nullptr)
	{
		Lines.Add(TEXT("Umbrella: None"));
	}
	else
	{
		const float StoredWater = UmbrellaComponent->GetCurrentStoredWater();
		const float MaxWater = UmbrellaComponent->StoredWaterContainer != nullptr
			? UmbrellaComponent->StoredWaterContainer->MaxAmount
			: 0.0f;
		const float RainExposure = UmbrellaComponent->GetCurrentPlayerRainAmount();
		const float MaxRainExposure = UmbrellaComponent->RainReceiver != nullptr
			? UmbrellaComponent->RainReceiver->MaxExposure
			: 0.0f;

		Lines.Add(FString::Printf(
			TEXT("Umbrella: Owned %s | State: %s | Direction: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->HasUmbrella()),
			*UOUDevelopmentDebugDrawPrivate::GetUmbrellaStateName(UmbrellaComponent->CurrentState),
			*UOUDevelopmentDebugDrawPrivate::GetUmbrellaDirectionStateName(
				UmbrellaComponent->CurrentDirectionState)));
		Lines.Add(FString::Printf(
			TEXT("Water: %.2f / %.2f | Rain: %.2f / %.2f"),
			StoredWater,
			MaxWater,
			RainExposure,
			MaxRainExposure));
		Lines.Add(FString::Printf(
			TEXT("Collect: %s | BlockRain: %s | BlocksJump: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->CanCollectWater()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->IsBlockingRain()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->BlocksJumping())));

		if (UmbrellaLightInteractionComponent != nullptr)
		{
			const UUOULightInteractionSurfaceComponent* LightSurfaceComponent =
				UmbrellaLightInteractionComponent->LightSurfaceComponent;
			Lines.Add(FString::Printf(
				TEXT("Light Surface: %s | Mode: %s"),
				LightSurfaceComponent != nullptr ? *LightSurfaceComponent->GetName() : TEXT("None"),
				LightSurfaceComponent != nullptr
					? *UOUDevelopmentDebugDrawPrivate::GetLightInteractionModeName(
						LightSurfaceComponent->LightInteractionMode)
					: TEXT("None")));
		}

		Lines.Add(FString::Printf(
			TEXT("Pour: %s | Hit: %s | Delivered: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->IsPouring()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->bLastPourTraceHit),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->bLastPourDeliveredWater)));
		Lines.Add(FString::Printf(
			TEXT("Pour Target: %s | Receiver: %s"),
			*UmbrellaComponent->LastPourTargetName,
			*UOUDevelopmentDebugDrawPrivate::GetPourReceiverTypeName(
				UmbrellaComponent->LastPourReceiverType)));
		Lines.Add(FString::Printf(
			TEXT("Pour Amount: %.2f | Stored: %.2f -> %.2f"),
			UmbrellaComponent->LastPourAmount,
			UmbrellaComponent->LastPourStoredWaterBefore,
			UmbrellaComponent->LastPourStoredWaterAfter));
	}

	if (InteractionComponent == nullptr)
	{
		Lines.Add(TEXT("Interaction: None"));
	}
	else
	{
		Lines.Add(FString::Printf(
			TEXT("Interaction: Enabled %s | Candidate: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(InteractionComponent->bInteractionEnabled),
			*UOUDevelopmentDebugDrawPrivate::GetComponentDebugName(
				InteractionComponent->CurrentCandidateComponent)));
		Lines.Add(FString::Printf(
			TEXT("Interaction Range: %.1f | Radius: %.1f"),
			InteractionComponent->InteractionRange,
			InteractionComponent->InteractionProbeRadius));
	}

	if (PushPullComponent == nullptr)
	{
		Lines.Add(TEXT("PushPull: None"));
	}
	else
	{
		const UUOUPushPullObjectComponent* CandidateObject =
			PushPullComponent->GetCurrentCandidateObject();
		const UUOUPushPullObjectComponent* GrabbedObject = PushPullComponent->GetGrabbedObject();
		Lines.Add(FString::Printf(
			TEXT("PushPull: Candidate %s | Grabbed %s"),
			CandidateObject != nullptr
				? *UOUDevelopmentDebugDrawPrivate::GetActorDebugName(CandidateObject->GetOwner())
				: TEXT("None"),
			GrabbedObject != nullptr
				? *UOUDevelopmentDebugDrawPrivate::GetActorDebugName(GrabbedObject->GetOwner())
				: TEXT("None")));
		Lines.Add(FString::Printf(
			TEXT("Hands: %s | Held: %s | TooFar: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(PushPullComponent->CanUseHandsForDebug()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(PushPullComponent->IsGrabInputHeld()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(
				PushPullComponent->IsGrabbedObjectTooFarForDebug())));
		Lines.Add(FString::Printf(
			TEXT("Axis: %s | Input: %.2f | Last: %s"),
			*UOUDevelopmentDebugDrawPrivate::FormatVectorCompact(
				PushPullComponent->GetGrabbedMoveAxis()),
			PushPullComponent->GetCurrentAxisInput(),
			*PushPullComponent->GetLastFailureReason()));
	}

	if (CameraControllerComponent != nullptr)
	{
		Lines.Add(FString::Printf(
			TEXT("Camera: Yaw %.1f -> %.1f | Dist %.1f -> %.1f | Occluded %d"),
			CameraControllerComponent->GetMovementYaw(),
			CameraControllerComponent->GetTargetCameraYaw(),
			CameraControllerComponent->GetCurrentCameraDistance(),
			CameraControllerComponent->GetTargetCameraDistance(),
			CameraControllerComponent->GetOccludedMeshCount()));
	}

	PlayerDebugText = FString::Join(Lines, LINE_TERMINATOR);
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
