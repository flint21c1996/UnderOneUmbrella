// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugSubsystem.h"

#include "Debug/UOUDebugController.h"
#include "Debug/UOUDebugControllerComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Debug/UOUPuzzleDebugProviderComponent.h"
#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMemory.h"
#include "InputCoreTypes.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUInteractionComponent.h"
#include "Player/UOUPushPullInteractorComponent.h"
#include "Player/UOURainReceiverComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Player/UOUUmbrellaLightInteractionComponent.h"
#include "Player/UOUWaterContainerComponent.h"
#include "Puzzle/PushPull/UOUPushPullObjectComponent.h"
#include "RHICommandList.h"
#include "RHIStats.h"
#include "RenderCounters.h"
#include "RenderTimer.h"
#include "UnrealClient.h"
#include "World/Light/UOULightInteractionSurfaceComponent.h"

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

	FString ClampDebugTextLines(const FString& Text, int32 MaxLineCount)
	{
		if (MaxLineCount <= 0 || Text.IsEmpty())
		{
			return Text;
		}

		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, false);
		if (Lines.Num() <= MaxLineCount)
		{
			return Text;
		}

		const int32 ContentLineCount = FMath::Max(0, MaxLineCount - 1);
		Lines.SetNum(ContentLineCount);
		Lines.Add(TEXT("..."));
		return FString::Join(Lines, LINE_TERMINATOR);
	}

	FString FormatMemoryGB(uint64 Bytes)
	{
		constexpr double BytesPerGB = 1024.0 * 1024.0 * 1024.0;
		return FString::Printf(TEXT("%.2f GB"), static_cast<double>(Bytes) / BytesPerGB);
	}

	FString FormatCompactCount(int32 Count)
	{
		if (Count >= 1000000)
		{
			return FString::Printf(TEXT("%.1fM"), Count / 1000000.0f);
		}

		if (Count >= 10000)
		{
			return FString::Printf(TEXT("%.1fK"), Count / 1000.0f);
		}

		return FString::FromInt(Count);
	}

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
		return GetEnumValueName(StaticEnum<EUOUUmbrellaDirectionState>(), static_cast<int64>(UmbrellaState));
	}


	FString GetPourReceiverTypeName(EUOUUmbrellaPourReceiverType ReceiverType)
	{
		return GetEnumValueName(StaticEnum<EUOUUmbrellaPourReceiverType>(), static_cast<int64>(ReceiverType));
	}

	FString GetLightInteractionModeName(EUOULightInteractionMode LightInteractionMode)
	{
		return GetEnumValueName(StaticEnum<EUOULightInteractionMode>(), static_cast<int64>(LightInteractionMode));
	}

	FString FormatVectorCompact(const FVector& Vector)
	{
		return FString::Printf(TEXT("%.0f, %.0f, %.0f"), Vector.X, Vector.Y, Vector.Z);
	}

	FIntPoint GetFallbackViewportSize(const UWorld* World)
	{
		if (World == nullptr)
		{
			return FIntPoint::ZeroValue;
		}

		const UGameViewportClient* GameViewport = World->GetGameViewport();
		if (GameViewport == nullptr || GameViewport->Viewport == nullptr)
		{
			return FIntPoint::ZeroValue;
		}

		return GameViewport->Viewport->GetSizeXY();
	}

	FString BuildProviderLabelText(UObject* ProviderObject, int32 MaxLineCount)
	{
		const FText DisplayName = IUOUDebugProvider::Execute_GetDebugDisplayName(ProviderObject);
		const FText SummaryText = IUOUDebugProvider::Execute_GetDebugSummaryText(ProviderObject);

		FString LabelText = !DisplayName.IsEmpty()
			? DisplayName.ToString()
			: ProviderObject->GetName();

		if (!SummaryText.IsEmpty())
		{
			LabelText += LINE_TERMINATOR;
			LabelText += SummaryText.ToString();
		}

		return ClampDebugTextLines(LabelText, MaxLineCount);
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

		if (const UUOUPuzzleDebugProviderComponent* PuzzleDebugProvider = Cast<UUOUPuzzleDebugProviderComponent>(Object))
		{
			OutLocation = PuzzleDebugProvider->GetConditionGroupNodeWorldLocation();
			return true;
		}

		if (const UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
		{
			const AActor* Owner = ActorComponent->GetOwner();
			if (Owner != nullptr)
			{
				OutLocation = Owner->GetActorLocation();
				return true;
			}
		}

		return false;
	}

	bool ShouldDrawProvider(const UObject* ProviderObject, const AUOUDebugController* DebugController)
	{
		if (ProviderObject == nullptr
			|| DebugController == nullptr
			|| !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
			|| !IUOUDebugProvider::Execute_IsDebugProviderEnabled(ProviderObject))
		{
			return false;
		}

		const EUOUDebugCategory Category = IUOUDebugProvider::Execute_GetDebugCategory(ProviderObject);
		return Category == EUOUDebugCategory::Puzzle && DebugController->IsCategoryEnabled(Category);
	}

	bool ShouldDrawProviderLabel(const UObject* ProviderObject, const AUOUDebugController* DebugController)
	{
		if (!ShouldDrawProvider(ProviderObject, DebugController))
		{
			return false;
		}

		const EUOUDebugCategory Category = IUOUDebugProvider::Execute_GetDebugCategory(ProviderObject);
		if (Category == EUOUDebugCategory::Puzzle)
		{
			const UUOUPuzzleDebugControllerComponent* PuzzleController =
				Cast<UUOUPuzzleDebugControllerComponent>(DebugController->FindDebugControllerComponent(Category));
			return PuzzleController == nullptr || PuzzleController->bShowWorldLabels;
		}

		if (Category == EUOUDebugCategory::NPC)
		{
			const UUOUNPCDebugControllerComponent* NPCController =
				Cast<UUOUNPCDebugControllerComponent>(DebugController->FindDebugControllerComponent(Category));
			return NPCController == nullptr || NPCController->bShowWorldLabels;
		}

		return true;
	}

	bool ShouldDrawProviderConnections(const UObject* ProviderObject, const AUOUDebugController* DebugController)
	{
		if (!ShouldDrawProvider(ProviderObject, DebugController))
		{
			return false;
		}

		const EUOUDebugCategory Category = IUOUDebugProvider::Execute_GetDebugCategory(ProviderObject);
		if (Category == EUOUDebugCategory::Puzzle)
		{
			const UUOUPuzzleDebugControllerComponent* PuzzleController =
				Cast<UUOUPuzzleDebugControllerComponent>(DebugController->FindDebugControllerComponent(Category));
			return PuzzleController == nullptr || PuzzleController->bShowConnections;
		}

		return true;
	}

	void DrawPuzzleConditionGroupNode(UWorld* World, const UObject* ProviderObject, const AUOUDebugController* DebugController)
	{
		const UUOUPuzzleDebugProviderComponent* PuzzleDebugProvider = Cast<UUOUPuzzleDebugProviderComponent>(ProviderObject);
		if (World == nullptr || PuzzleDebugProvider == nullptr || !PuzzleDebugProvider->bShowConditionGroupNode)
		{
			return;
		}

		const float NodeSize = FMath::Max(1.0f, PuzzleDebugProvider->ConditionGroupNodeSize);
		const FColor NodeColor = DebugController != nullptr
			? DebugController->PuzzleConditionGroupNodeColor
			: FColor::White;
		DrawDebugPoint(World, PuzzleDebugProvider->GetConditionGroupNodeWorldLocation(), NodeSize, NodeColor, false, 0.0f, 0);
	}

	void DrawReadableDebugString(
		UWorld* World,
		const FVector& Location,
		const FString& Text,
		const AUOUDebugController* DebugController,
		const FColor& TextColor,
		float TextScale)
	{
		if (World == nullptr || Text.IsEmpty())
		{
			return;
		}

		const bool bDrawShadow = DebugController == nullptr || DebugController->bUseWorldTextShadow;
		DrawDebugString(World, Location, Text, nullptr, TextColor, 0.0f, bDrawShadow, TextScale);
	}

	bool TryGetViewPoint(UWorld* World, FVector& OutViewLocation, FRotator& OutViewRotation)
	{
		if (World == nullptr)
		{
			return false;
		}

		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController == nullptr)
		{
			return false;
		}

		PlayerController->GetPlayerViewPoint(OutViewLocation, OutViewRotation);
		return true;
	}

	bool TryGetViewLocation(UWorld* World, FVector& OutViewLocation)
	{
		FRotator ViewRotation = FRotator::ZeroRotator;
		return TryGetViewPoint(World, OutViewLocation, ViewRotation);
	}

	AActor* GetDebugObjectOwnerActor(UObject* Object)
	{
		if (AActor* Actor = Cast<AActor>(Object))
		{
			return Actor;
		}

		if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
		{
			return ActorComponent->GetOwner();
		}

		return nullptr;
	}

	bool IsFocusTraceHitObject(const FHitResult& FocusHit, UObject* Object)
	{
		if (Object == nullptr)
		{
			return false;
		}

		if (FocusHit.GetActor() != nullptr && FocusHit.GetActor() == GetDebugObjectOwnerActor(Object))
		{
			return true;
		}

		if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
		{
			return FocusHit.GetComponent() != nullptr && FocusHit.GetComponent() == ActorComponent;
		}

		return false;
	}

	bool TryGetFocusTraceHit(UWorld* World, const FVector& ViewLocation, const FRotator& ViewRotation, float TraceDistance, FHitResult& OutHit)
	{
		if (World == nullptr || TraceDistance <= 0.0f)
		{
			return false;
		}

		const FVector TraceStart = ViewLocation;
		const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * TraceDistance);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOUDebugFocusTrace), true);
		return World->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	}

	struct FProviderDrawCandidate
	{
		UObject* ProviderObject = nullptr;
		float DistanceSquared = 0.0f;
		float ViewDot = -1.0f;
		bool bHitByFocusTrace = false;
	};

	int32 FindFocusedProviderCandidateIndex(const TArray<FProviderDrawCandidate>& Candidates)
	{
		int32 BestHitIndex = INDEX_NONE;
		int32 BestViewIndex = INDEX_NONE;

		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			const FProviderDrawCandidate& Candidate = Candidates[Index];
			if (Candidate.bHitByFocusTrace
				&& (BestHitIndex == INDEX_NONE || Candidate.DistanceSquared < Candidates[BestHitIndex].DistanceSquared))
			{
				BestHitIndex = Index;
			}

			if (BestViewIndex == INDEX_NONE
				|| Candidate.ViewDot > Candidates[BestViewIndex].ViewDot
				|| (FMath::IsNearlyEqual(Candidate.ViewDot, Candidates[BestViewIndex].ViewDot)
					&& Candidate.DistanceSquared < Candidates[BestViewIndex].DistanceSquared))
			{
				BestViewIndex = Index;
			}
		}

		return BestHitIndex != INDEX_NONE ? BestHitIndex : BestViewIndex;
	}

	void GatherProviderDrawCandidates(
		const TArray<TWeakObjectPtr<UObject>>& RegisteredProviders,
		UWorld* World,
		const AUOUDebugController* DebugController,
		bool bLabelPass,
		TArray<UObject*>& OutProviderObjects)
	{
		OutProviderObjects.Reset();

		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		const bool bHasViewLocation = TryGetViewPoint(World, ViewLocation, ViewRotation);
		const FVector ViewDirection = ViewRotation.Vector();
		const float MaxVisibleDistance = DebugController != nullptr ? DebugController->WorldDebugVisibleDistance : 0.0f;
		const float MaxVisibleDistanceSquared = FMath::Square(MaxVisibleDistance);
		const bool bFocusMode = DebugController != nullptr && DebugController->bOnlyShowFocusedActor;

		FHitResult FocusHit;
		const bool bHasFocusHit = bFocusMode && bHasViewLocation && TryGetFocusTraceHit(
			World,
			ViewLocation,
			ViewRotation,
			MaxVisibleDistance > 0.0f ? MaxVisibleDistance : 10000.0f,
			FocusHit);

		TArray<FProviderDrawCandidate> Candidates;
		for (const TWeakObjectPtr<UObject>& RegisteredProvider : RegisteredProviders)
		{
			UObject* ProviderObject = RegisteredProvider.Get();
			const bool bShouldDraw = bLabelPass
				? ShouldDrawProviderLabel(ProviderObject, DebugController)
				: ShouldDrawProviderConnections(ProviderObject, DebugController);
			if (!bShouldDraw)
			{
				continue;
			}

			float DistanceSquared = 0.0f;
			float ViewDot = -1.0f;
			if (bHasViewLocation)
			{
				FVector ProviderLocation = FVector::ZeroVector;
				if (TryGetDebugObjectLocation(ProviderObject, ProviderLocation))
				{
					DistanceSquared = FVector::DistSquared(ViewLocation, ProviderLocation);
					if (MaxVisibleDistance > 0.0f && DistanceSquared > MaxVisibleDistanceSquared)
					{
						continue;
					}

					const FVector ToProvider = (ProviderLocation - ViewLocation).GetSafeNormal();
					ViewDot = FVector::DotProduct(ViewDirection, ToProvider);
				}
			}

			Candidates.Add({ ProviderObject, DistanceSquared, ViewDot, bHasFocusHit && IsFocusTraceHitObject(FocusHit, ProviderObject) });
		}

		if (bFocusMode)
		{
			const int32 FocusedIndex = FindFocusedProviderCandidateIndex(Candidates);
			if (Candidates.IsValidIndex(FocusedIndex))
			{
				OutProviderObjects.Add(Candidates[FocusedIndex].ProviderObject);
			}
			return;
		}

		if (bHasViewLocation)
		{
			Candidates.Sort(
				[](const FProviderDrawCandidate& Left, const FProviderDrawCandidate& Right)
				{
					return Left.DistanceSquared < Right.DistanceSquared;
				});
		}

		const int32 MaxVisibleItems = DebugController != nullptr
			? FMath::Max(1, DebugController->MaxVisibleWorldDebugItems)
			: Candidates.Num();
		const int32 VisibleCount = FMath::Min(MaxVisibleItems, Candidates.Num());
		OutProviderObjects.Reserve(VisibleCount);
		for (int32 Index = 0; Index < VisibleCount; ++Index)
		{
			OutProviderObjects.Add(Candidates[Index].ProviderObject);
		}
	}

}

void UUOUDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ControllerSearchTimeRemaining = 0.0f;
	ProviderCompactTimeRemaining = 0.0f;
	PerformanceStatsUpdateTimeRemaining = 0.0f;
	PerformanceStatsAccumulatedDeltaTime = 0.0f;
	PerformanceStatsSampleCount = 0;
	CachedPerformanceStatsText.Reset();
}

void UUOUDebugSubsystem::Deinitialize()
{
	ActiveDebugController.Reset();
	RegisteredProviders.Reset();
	CachedPerformanceStatsText.Reset();

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

	ProviderCompactTimeRemaining -= DeltaTime;
	if (ProviderCompactTimeRemaining <= 0.0f)
	{
		CompactRegisteredProviders();
		ProviderCompactTimeRemaining = 1.0f;
	}

	DrawControllerStatus();
	DrawPlayerDebug();
	DrawPerformanceStats(DeltaTime);
	DrawVFXDebug();
	DrawRegisteredProviderConnections();
	DrawRegisteredProviderLabelBoards();
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

void UUOUDebugSubsystem::RegisterDebugProvider(UObject* ProviderObject)
{
	if (!IsValid(ProviderObject) || !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass()))
	{
		return;
	}

	for (const TWeakObjectPtr<UObject>& RegisteredProvider : RegisteredProviders)
	{
		if (RegisteredProvider.Get() == ProviderObject)
		{
			return;
		}
	}

	RegisteredProviders.Add(ProviderObject);
}

void UUOUDebugSubsystem::UnregisterDebugProvider(UObject* ProviderObject)
{
	RegisteredProviders.RemoveAll(
		[ProviderObject](const TWeakObjectPtr<UObject>& RegisteredProvider)
		{
			return !RegisteredProvider.IsValid() || RegisteredProvider.Get() == ProviderObject;
		});
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

bool UUOUDebugSubsystem::IsDebugCategoryEnabled(const UObject* WorldContextObject, EUOUDebugCategory Category)
{
	const UUOUDebugSubsystem* DebugSubsystem = UOUDebugSubsystemPrivate::GetDebugSubsystem(WorldContextObject);
	return DebugSubsystem != nullptr && DebugSubsystem->IsDebugEnabled(Category);
}

bool UUOUDebugSubsystem::IsDebugScreenMessageEnabled(const UObject* WorldContextObject, EUOUDebugCategory Category)
{
	const UUOUDebugSubsystem* DebugSubsystem = UOUDebugSubsystemPrivate::GetDebugSubsystem(WorldContextObject);
	return DebugSubsystem != nullptr && DebugSubsystem->IsScreenMessageEnabled(Category);
}

bool UUOUDebugSubsystem::IsDebugWorldDrawEnabled(const UObject* WorldContextObject, EUOUDebugCategory Category)
{
	const UUOUDebugSubsystem* DebugSubsystem = UOUDebugSubsystemPrivate::GetDebugSubsystem(WorldContextObject);
	return DebugSubsystem != nullptr && DebugSubsystem->IsWorldDrawEnabled(Category);
}

bool UUOUDebugSubsystem::IsDebugWorldLabelEnabled(const UObject* WorldContextObject, EUOUDebugCategory Category)
{
	const UUOUDebugSubsystem* DebugSubsystem = UOUDebugSubsystemPrivate::GetDebugSubsystem(WorldContextObject);
	return DebugSubsystem != nullptr && DebugSubsystem->IsWorldLabelEnabled(Category);
}

FColor UUOUDebugSubsystem::GetDebugCategoryColor(const UObject* WorldContextObject, EUOUDebugCategory Category, FColor FallbackColor)
{
	if (WorldContextObject == nullptr)
	{
		return FallbackColor;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (World == nullptr)
	{
		return FallbackColor;
	}

	const UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>();
	const AUOUDebugController* DebugController = DebugSubsystem != nullptr
		? DebugSubsystem->GetActiveDebugController()
		: nullptr;

	return DebugController != nullptr
		? DebugController->GetDebugCategoryColor(Category)
		: FallbackColor;
}

UUOUDebugControllerComponentBase* UUOUDebugSubsystem::FindDebugControllerComponent(EUOUDebugCategory Category) const
{
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	return DebugController != nullptr
		? DebugController->FindDebugControllerComponent(Category)
		: nullptr;
}

int32 UUOUDebugSubsystem::GetRegisteredProviderCount() const
{
	return RegisteredProviders.Num();
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

	TryAutoCreateRuntimeDebugController(World);
}

void UUOUDebugSubsystem::TryAutoCreateRuntimeDebugController(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	if (!ShouldAutoCreateRuntimeDebugController(World))
	{
		return;
	}

	if (AUOUDebugController* DebugController = AUOUDebugController::FindOrCreateDebugController(World))
	{
		RegisterDebugController(DebugController);
	}
#endif
}

bool UUOUDebugSubsystem::ShouldAutoCreateRuntimeDebugController(const UWorld* World) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return World != nullptr
		&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE)
		&& World->GetNetMode() != NM_DedicatedServer;
#endif
}

void UUOUDebugSubsystem::CompactRegisteredProviders()
{
	RegisteredProviders.RemoveAll(
		[](const TWeakObjectPtr<UObject>& RegisteredProvider)
		{
			const UObject* ProviderObject = RegisteredProvider.Get();
			return ProviderObject == nullptr
				|| !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass());
		});
}

void UUOUDebugSubsystem::DrawControllerStatus() const
{
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	if (DebugController == nullptr || !DebugController->bShowControllerStatus || GEngine == nullptr)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(
		0x5500D06,
		0.0f,
		FColor::White,
		BuildControllerStatusText());
}

void UUOUDebugSubsystem::DrawPlayerDebug() const
{
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	if (DebugController == nullptr || GEngine == nullptr || !IsScreenMessageEnabled(EUOUDebugCategory::Player))
	{
		return;
	}

	const UUOUPlayerDebugControllerComponent* PlayerController =
		Cast<UUOUPlayerDebugControllerComponent>(FindDebugControllerComponent(EUOUDebugCategory::Player));
	if (PlayerController == nullptr || !PlayerController->bShowViewportHUD)
	{
		return;
	}

	const FString PlayerDebugText = BuildPlayerDebugText(*PlayerController);
	if (PlayerDebugText.IsEmpty())
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(
		0x5500D09,
		0.0f,
		DebugController->GetDebugCategoryColor(EUOUDebugCategory::Player),
		PlayerDebugText,
		false,
		FVector2D(1.0f, 1.0f));
}

void UUOUDebugSubsystem::DrawPerformanceStats(float DeltaTime) const
{
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	if (DebugController == nullptr || GEngine == nullptr || !IsScreenMessageEnabled(EUOUDebugCategory::Performance))
	{
		CachedPerformanceStatsText.Reset();
		PerformanceStatsUpdateTimeRemaining = 0.0f;
		PerformanceStatsAccumulatedDeltaTime = 0.0f;
		PerformanceStatsSampleCount = 0;
		return;
	}

	const UUOUPerformanceDebugControllerComponent* PerformanceController =
		Cast<UUOUPerformanceDebugControllerComponent>(FindDebugControllerComponent(EUOUDebugCategory::Performance));
	if (PerformanceController == nullptr || !PerformanceController->bShowViewportStats)
	{
		CachedPerformanceStatsText.Reset();
		PerformanceStatsUpdateTimeRemaining = 0.0f;
		PerformanceStatsAccumulatedDeltaTime = 0.0f;
		PerformanceStatsSampleCount = 0;
		return;
	}

	PerformanceStatsAccumulatedDeltaTime += FMath::Max(0.0f, DeltaTime);
	++PerformanceStatsSampleCount;
	PerformanceStatsUpdateTimeRemaining -= DeltaTime;

	const float UpdateInterval = FMath::Max(0.05f, PerformanceController->ViewportStatsUpdateInterval);
	if (PerformanceStatsUpdateTimeRemaining <= 0.0f || CachedPerformanceStatsText.IsEmpty())
	{
		const float AverageDeltaTime = PerformanceStatsSampleCount > 0
			? PerformanceStatsAccumulatedDeltaTime / PerformanceStatsSampleCount
			: DeltaTime;

		CachedPerformanceStatsText = BuildPerformanceStatsText(AverageDeltaTime, *PerformanceController);
		PerformanceStatsAccumulatedDeltaTime = 0.0f;
		PerformanceStatsSampleCount = 0;
		PerformanceStatsUpdateTimeRemaining = UpdateInterval;
	}

	if (CachedPerformanceStatsText.IsEmpty())
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(
		0x5500D07,
		UpdateInterval + 0.1f,
		DebugController->GetDebugCategoryColor(EUOUDebugCategory::Performance),
		CachedPerformanceStatsText);
}

void UUOUDebugSubsystem::DrawVFXDebug() const
{
	UWorld* World = GetWorld();
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	if (World == nullptr || DebugController == nullptr || !DebugController->IsCategoryEnabled(EUOUDebugCategory::VFX))
	{
		return;
	}

	const UUOUVFXDebugControllerComponent* VFXController =
		Cast<UUOUVFXDebugControllerComponent>(FindDebugControllerComponent(EUOUDebugCategory::VFX));
	if (VFXController == nullptr)
	{
		return;
	}

	struct FVFXOwnerStats
	{
		int32 ActiveNiagaraCount = 0;
		int32 ActiveCascadeCount = 0;
		int32 LocationSampleCount = 0;
		FVector AccumulatedLocation = FVector::ZeroVector;
	};

	int32 TotalNiagaraCount = 0;
	int32 ActiveNiagaraCount = 0;
	int32 TotalCascadeCount = 0;
	int32 ActiveCascadeCount = 0;
	TMap<AActor*, FVFXOwnerStats> OwnerStats;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		TArray<UNiagaraComponent*> NiagaraComponents;
		Actor->GetComponents<UNiagaraComponent>(NiagaraComponents);
		TotalNiagaraCount += NiagaraComponents.Num();
		for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
		{
			if (!IsValid(NiagaraComponent) || !NiagaraComponent->IsRegistered() || !NiagaraComponent->IsActive())
			{
				continue;
			}

			++ActiveNiagaraCount;
			FVFXOwnerStats& Stats = OwnerStats.FindOrAdd(Actor);
			++Stats.ActiveNiagaraCount;
			++Stats.LocationSampleCount;
			Stats.AccumulatedLocation += NiagaraComponent->GetComponentLocation();
		}

		TArray<UParticleSystemComponent*> CascadeComponents;
		Actor->GetComponents<UParticleSystemComponent>(CascadeComponents);
		TotalCascadeCount += CascadeComponents.Num();
		for (UParticleSystemComponent* CascadeComponent : CascadeComponents)
		{
			if (!IsValid(CascadeComponent) || !CascadeComponent->IsRegistered() || !CascadeComponent->IsActive())
			{
				continue;
			}

			++ActiveCascadeCount;
			FVFXOwnerStats& Stats = OwnerStats.FindOrAdd(Actor);
			++Stats.ActiveCascadeCount;
			++Stats.LocationSampleCount;
			Stats.AccumulatedLocation += CascadeComponent->GetComponentLocation();
		}
	}

	if (VFXController->bShowParticleCount && GEngine != nullptr)
	{
		const FString VFXSummary = FString::Printf(
			TEXT("VFX\nNiagara Components: %d / %d active\nCascade Components: %d / %d active\nOwners: %d"),
			ActiveNiagaraCount,
			TotalNiagaraCount,
			ActiveCascadeCount,
			TotalCascadeCount,
			OwnerStats.Num());

		GEngine->AddOnScreenDebugMessage(
			0x5500D08,
			0.0f,
			DebugController->GetDebugCategoryColor(EUOUDebugCategory::VFX),
			VFXSummary);
	}

	if (!VFXController->bShowWorldDebug || !VFXController->bShowNiagaraOwners)
	{
		return;
	}

	struct FVFXOwnerDrawCandidate
	{
		FString Text;
		FVector Location = FVector::ZeroVector;
		float DistanceSquared = 0.0f;
		float ViewDot = -1.0f;
	};

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	const bool bHasViewLocation = UOUDebugSubsystemPrivate::TryGetViewPoint(World, ViewLocation, ViewRotation);
	const FVector ViewDirection = ViewRotation.Vector();
	const float MaxVisibleDistance = DebugController->WorldDebugVisibleDistance;
	const float MaxVisibleDistanceSquared = FMath::Square(MaxVisibleDistance);

	TArray<FVFXOwnerDrawCandidate> Candidates;
	for (const TPair<AActor*, FVFXOwnerStats>& Pair : OwnerStats)
	{
		const AActor* Owner = Pair.Key;
		const FVFXOwnerStats& Stats = Pair.Value;
		if (!IsValid(Owner) || Stats.LocationSampleCount <= 0)
		{
			continue;
		}

		FVector LabelLocation = Stats.AccumulatedLocation / static_cast<float>(Stats.LocationSampleCount);
		LabelLocation.Z += 120.0f;

		float DistanceSquared = 0.0f;
		float ViewDot = -1.0f;
		if (bHasViewLocation)
		{
			DistanceSquared = FVector::DistSquared(ViewLocation, LabelLocation);
			if (MaxVisibleDistance > 0.0f && DistanceSquared > MaxVisibleDistanceSquared)
			{
				continue;
			}

			ViewDot = FVector::DotProduct(ViewDirection, (LabelLocation - ViewLocation).GetSafeNormal());
		}

		FVFXOwnerDrawCandidate Candidate;
		Candidate.Text = FString::Printf(
			TEXT("VFX: %s\nNiagara: %d\nCascade: %d"),
			*Owner->GetName(),
			Stats.ActiveNiagaraCount,
			Stats.ActiveCascadeCount);
		Candidate.Location = LabelLocation;
		Candidate.DistanceSquared = DistanceSquared;
		Candidate.ViewDot = ViewDot;
		Candidates.Add(MoveTemp(Candidate));
	}

	if (DebugController->bOnlyShowFocusedActor)
	{
		Candidates.Sort(
			[](const FVFXOwnerDrawCandidate& Left, const FVFXOwnerDrawCandidate& Right)
			{
				if (!FMath::IsNearlyEqual(Left.ViewDot, Right.ViewDot))
				{
					return Left.ViewDot > Right.ViewDot;
				}

				return Left.DistanceSquared < Right.DistanceSquared;
			});
	}
	else if (bHasViewLocation)
	{
		Candidates.Sort(
			[](const FVFXOwnerDrawCandidate& Left, const FVFXOwnerDrawCandidate& Right)
			{
				return Left.DistanceSquared < Right.DistanceSquared;
			});
	}

	const int32 MaxVisibleItems = DebugController->bOnlyShowFocusedActor
		? 1
		: FMath::Max(1, DebugController->MaxVisibleWorldDebugItems);
	const int32 VisibleCount = FMath::Min(MaxVisibleItems, Candidates.Num());
	const FColor LabelColor = DebugController->GetDebugCategoryColor(EUOUDebugCategory::VFX);
	for (int32 Index = 0; Index < VisibleCount; ++Index)
	{
		UOUDebugSubsystemPrivate::DrawReadableDebugString(World, Candidates[Index].Location, Candidates[Index].Text, DebugController, LabelColor, 0.9f);
	}
}

void UUOUDebugSubsystem::DrawRegisteredProviderLabelBoards() const
{
	UWorld* World = GetWorld();
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	if (World == nullptr || DebugController == nullptr || !DebugController->bEnableDebugTools)
	{
		return;
	}

	TArray<UObject*> ProviderObjects;
	UOUDebugSubsystemPrivate::GatherProviderDrawCandidates(RegisteredProviders, World, DebugController, true, ProviderObjects);

	for (UObject* ProviderObject : ProviderObjects)
	{
		const FVector LabelLocation = IUOUDebugProvider::Execute_GetDebugWorldLocation(ProviderObject);
		const FString LabelText = UOUDebugSubsystemPrivate::BuildProviderLabelText(ProviderObject, DebugController->MaxWorldDebugLabelLines);
		const EUOUDebugCategory Category = IUOUDebugProvider::Execute_GetDebugCategory(ProviderObject);
		const FColor LabelColor = DebugController->GetDebugCategoryColor(Category);
		UOUDebugSubsystemPrivate::DrawReadableDebugString(World, LabelLocation, LabelText, DebugController, LabelColor, 1.0f);
	}
}

void UUOUDebugSubsystem::DrawRegisteredProviderConnections() const
{
	UWorld* World = GetWorld();
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	if (World == nullptr || DebugController == nullptr || !DebugController->bEnableDebugTools)
	{
		return;
	}

	TArray<UObject*> ProviderObjects;
	UOUDebugSubsystemPrivate::GatherProviderDrawCandidates(RegisteredProviders, World, DebugController, false, ProviderObjects);

	for (UObject* ProviderObject : ProviderObjects)
	{
		TArray<FUOUDebugConnection> Connections;
		IUOUDebugProvider::Execute_GetDebugConnections(ProviderObject, Connections);

		UOUDebugSubsystemPrivate::DrawPuzzleConditionGroupNode(World, ProviderObject, DebugController);

		for (const FUOUDebugConnection& Connection : Connections)
		{
			FVector SourceLocation;
			FVector TargetLocation;
			if (!UOUDebugSubsystemPrivate::TryGetDebugObjectLocation(Connection.SourceObject, SourceLocation)
				|| !UOUDebugSubsystemPrivate::TryGetDebugObjectLocation(Connection.TargetObject, TargetLocation)
				|| SourceLocation.Equals(TargetLocation))
			{
				continue;
			}

			const FColor ConnectionColor = DebugController->GetDebugConnectionColor(Connection);
			const float Thickness = FMath::Max(0.0f, Connection.Thickness);
			DrawDebugDirectionalArrow(World, SourceLocation, TargetLocation, 80.0f, ConnectionColor, false, 0.0f, 0, Thickness);

			if (!Connection.Label.IsEmpty())
			{
				const FVector LabelLocation = FMath::Lerp(SourceLocation, TargetLocation, 0.5f) + FVector(0.0f, 0.0f, 40.0f);
				UOUDebugSubsystemPrivate::DrawReadableDebugString(World, LabelLocation, Connection.Label.ToString(), DebugController, ConnectionColor, 0.8f);
			}
		}
	}
}

FString UUOUDebugSubsystem::BuildPlayerDebugText(const UUOUPlayerDebugControllerComponent& PlayerController) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return FString();
	}

	const APlayerController* LocalPlayerController = World->GetFirstPlayerController();
	APawn* PlayerPawn = LocalPlayerController != nullptr ? LocalPlayerController->GetPawn() : nullptr;
	if (PlayerPawn == nullptr)
	{
		return TEXT("Player\nPawn: None");
	}

	AUOUCharacter* UOUCharacter = Cast<AUOUCharacter>(PlayerPawn);
	ACharacter* Character = Cast<ACharacter>(PlayerPawn);
	const UCharacterMovementComponent* MovementComponent = Character != nullptr ? Character->GetCharacterMovement() : nullptr;
	const UUOUUmbrellaComponent* UmbrellaComponent = PlayerPawn->FindComponentByClass<UUOUUmbrellaComponent>();
	const UUOUPushPullInteractorComponent* PushPullComponent = UOUCharacter != nullptr
		? UOUCharacter->GetPushPullInteractorComponent()
		: PlayerPawn->FindComponentByClass<UUOUPushPullInteractorComponent>();
	const UUOUInteractionComponent* InteractionComponent = PlayerPawn->FindComponentByClass<UUOUInteractionComponent>();
	const UUOUCameraControllerComponent* CameraControllerComponent = UOUCharacter != nullptr
		? UOUCharacter->GetCameraControllerComponent()
		: PlayerPawn->FindComponentByClass<UUOUCameraControllerComponent>();
	const UUOUUmbrellaLightInteractionComponent* UmbrellaLightInteractionComponent =
		PlayerPawn->FindComponentByClass<UUOUUmbrellaLightInteractionComponent>();

	TArray<FString> Lines;
	Lines.Add(TEXT("Player"));

	if (PlayerController.bShowMovementState)
	{
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
			*UOUDebugSubsystemPrivate::GetMovementModeName(MovementComponent),
			UOUDebugSubsystemPrivate::GetYesNo(MovementComponent != nullptr && MovementComponent->IsMovingOnGround()),
			Velocity.Size2D()));
		Lines.Add(FString::Printf(
			TEXT("Location: %s | ZVel: %.1f"),
			*UOUDebugSubsystemPrivate::FormatVectorCompact(PlayerPawn->GetActorLocation()),
			Velocity.Z));
		Lines.Add(FString::Printf(
			TEXT("Jump: %s | Blocked: %s"),
			UOUDebugSubsystemPrivate::GetYesNo(Character != nullptr && Character->CanJump()),
			JumpBlockedReasons.Num() > 0 ? *FString::Join(JumpBlockedReasons, TEXT(", ")) : TEXT("No")));
	}

	if (PlayerController.bShowInputState)
	{
		const bool bPhysicalContextKeyDown = LocalPlayerController != nullptr
			&& LocalPlayerController->IsInputKeyDown(EKeys::RightMouseButton);
		const bool bRoutesToPour = UmbrellaComponent != nullptr
			&& UmbrellaComponent->HasUmbrella()
			&& UmbrellaComponent->IsUpsideDown()
			&& UmbrellaComponent->GetCurrentStoredWater() > 0.0f;

		Lines.Add(FString::Printf(
			TEXT("Input: RMB %s | Context Route: %s"),
			UOUDebugSubsystemPrivate::GetYesNo(bPhysicalContextKeyDown),
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
	}

	if (PlayerController.bShowUmbrellaState)
	{
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
				UOUDebugSubsystemPrivate::GetYesNo(UmbrellaComponent->HasUmbrella()),
				*UOUDebugSubsystemPrivate::GetUmbrellaStateName(UmbrellaComponent->CurrentState),
				*UOUDebugSubsystemPrivate::GetUmbrellaDirectionStateName(UmbrellaComponent->CurrentDirectionState)));
			Lines.Add(FString::Printf(
				TEXT("Water: %.2f / %.2f | Rain: %.2f / %.2f"),
				StoredWater,
				MaxWater,
				RainExposure,
				MaxRainExposure));
			Lines.Add(FString::Printf(
				TEXT("Collect: %s | BlockRain: %s | BlocksJump: %s"),
				UOUDebugSubsystemPrivate::GetYesNo(UmbrellaComponent->CanCollectWater()),
				UOUDebugSubsystemPrivate::GetYesNo(UmbrellaComponent->IsBlockingRain()),
				UOUDebugSubsystemPrivate::GetYesNo(UmbrellaComponent->BlocksJumping())));

			if (UmbrellaLightInteractionComponent != nullptr)
			{
				const UUOULightInteractionSurfaceComponent* LightSurfaceComponent =
					UmbrellaLightInteractionComponent->LightSurfaceComponent;
				Lines.Add(FString::Printf(
					TEXT("Light Surface: %s | Mode: %s"),
					LightSurfaceComponent != nullptr ? *LightSurfaceComponent->GetName() : TEXT("None"),
					LightSurfaceComponent != nullptr
						? *UOUDebugSubsystemPrivate::GetLightInteractionModeName(LightSurfaceComponent->LightInteractionMode)
						: TEXT("None")));
			}
		}
	}

	if (PlayerController.bShowPourState && UmbrellaComponent != nullptr)
	{
		Lines.Add(FString::Printf(
			TEXT("Pour: %s | Hit: %s | Delivered: %s"),
			UOUDebugSubsystemPrivate::GetYesNo(UmbrellaComponent->IsPouring()),
			UOUDebugSubsystemPrivate::GetYesNo(UmbrellaComponent->bLastPourTraceHit),
			UOUDebugSubsystemPrivate::GetYesNo(UmbrellaComponent->bLastPourDeliveredWater)));
		Lines.Add(FString::Printf(
			TEXT("Pour Target: %s | Receiver: %s"),
			*UmbrellaComponent->LastPourTargetName,
			*UOUDebugSubsystemPrivate::GetPourReceiverTypeName(UmbrellaComponent->LastPourReceiverType)));
		Lines.Add(FString::Printf(
			TEXT("Pour Amount: %.2f | Stored: %.2f -> %.2f"),
			UmbrellaComponent->LastPourAmount,
			UmbrellaComponent->LastPourStoredWaterBefore,
			UmbrellaComponent->LastPourStoredWaterAfter));
	}

	if (PlayerController.bShowInteractionTarget)
	{
		if (InteractionComponent == nullptr)
		{
			Lines.Add(TEXT("Interaction: None"));
		}
		else
		{
			Lines.Add(FString::Printf(
				TEXT("Interaction: Enabled %s | Candidate: %s"),
				UOUDebugSubsystemPrivate::GetYesNo(InteractionComponent->bInteractionEnabled),
				*UOUDebugSubsystemPrivate::GetComponentDebugName(InteractionComponent->CurrentCandidateComponent)));
			Lines.Add(FString::Printf(
				TEXT("Interaction Range: %.1f | Radius: %.1f"),
				InteractionComponent->InteractionRange,
				InteractionComponent->InteractionProbeRadius));
		}
	}

	if (PlayerController.bShowPushPullState)
	{
		if (PushPullComponent == nullptr)
		{
			Lines.Add(TEXT("PushPull: None"));
		}
		else
		{
			const UUOUPushPullObjectComponent* CandidateObject = PushPullComponent->GetCurrentCandidateObject();
			const UUOUPushPullObjectComponent* GrabbedObject = PushPullComponent->GetGrabbedObject();
			Lines.Add(FString::Printf(
				TEXT("PushPull: Candidate %s | Grabbed %s"),
				CandidateObject != nullptr ? *UOUDebugSubsystemPrivate::GetActorDebugName(CandidateObject->GetOwner()) : TEXT("None"),
				GrabbedObject != nullptr ? *UOUDebugSubsystemPrivate::GetActorDebugName(GrabbedObject->GetOwner()) : TEXT("None")));
			Lines.Add(FString::Printf(
				TEXT("Hands: %s | Held: %s | TooFar: %s"),
				UOUDebugSubsystemPrivate::GetYesNo(PushPullComponent->CanUseHandsForDebug()),
				UOUDebugSubsystemPrivate::GetYesNo(PushPullComponent->IsGrabInputHeld()),
				UOUDebugSubsystemPrivate::GetYesNo(PushPullComponent->IsGrabbedObjectTooFarForDebug())));
			Lines.Add(FString::Printf(
				TEXT("Axis: %s | Input: %.2f | Last: %s"),
				*UOUDebugSubsystemPrivate::FormatVectorCompact(PushPullComponent->GetGrabbedMoveAxis()),
				PushPullComponent->GetCurrentAxisInput(),
				*PushPullComponent->GetLastFailureReason()));
		}
	}

	if (PlayerController.bShowCameraState && CameraControllerComponent != nullptr)
	{
		Lines.Add(FString::Printf(
			TEXT("Camera: Yaw %.1f -> %.1f | Dist %.1f -> %.1f | Occluded %d"),
			CameraControllerComponent->GetMovementYaw(),
			CameraControllerComponent->GetTargetCameraYaw(),
			CameraControllerComponent->GetCurrentCameraDistance(),
			CameraControllerComponent->GetTargetCameraDistance(),
			CameraControllerComponent->GetOccludedMeshCount()));
	}

	return FString::Join(Lines, LINE_TERMINATOR);
}

FString UUOUDebugSubsystem::BuildPerformanceStatsText(float DeltaTime, const UUOUPerformanceDebugControllerComponent& PerformanceController) const
{
	TArray<FString> Lines;

	if (PerformanceController.bShowFPS)
	{
		const float FPS = DeltaTime > KINDA_SMALL_NUMBER ? 1.0f / DeltaTime : 0.0f;
		Lines.Add(FString::Printf(TEXT("FPS: %.1f"), FPS));
	}

	if (PerformanceController.bShowFrameTime)
	{
		Lines.Add(FString::Printf(TEXT("Frame: %.2f ms"), DeltaTime * 1000.0f));
		Lines.Add(FString::Printf(TEXT("Game: %.2f ms"), FPlatformTime::ToMilliseconds(GGameThreadTime)));
		Lines.Add(FString::Printf(TEXT("Draw: %.2f ms"), FPlatformTime::ToMilliseconds(GRenderThreadTime)));
		Lines.Add(FString::Printf(TEXT("RHIT: %.2f ms"), FPlatformTime::ToMilliseconds(GRHIThreadTime)));
		Lines.Add(FString::Printf(TEXT("GPU Time: %.2f ms"), FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles())));
		Lines.Add(FString::Printf(TEXT("Input: %.2f ms"), FPlatformTime::ToMilliseconds64(GInputLatencyTime)));
	}

	if (PerformanceController.bShowMemory)
	{
		const FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
		Lines.Add(FString::Printf(TEXT("Mem: %s"), *UOUDebugSubsystemPrivate::FormatMemoryGB(MemoryStats.UsedPhysical)));
	}

	if (PerformanceController.bShowRenderStats)
	{
		float ScreenPercentage = GPixelRenderCounters.GetResolutionFraction() * 100.0f;
		FIntPoint RenderResolution = GPixelRenderCounters.GetRenderResolution();
		if (ScreenPercentage <= 0.0f || RenderResolution.X <= 0 || RenderResolution.Y <= 0)
		{
			ScreenPercentage = 100.0f;
			RenderResolution = UOUDebugSubsystemPrivate::GetFallbackViewportSize(GetWorld());
		}

		Lines.Add(FString::Printf(TEXT("RenderRes: %.1f%% (%dx%d)"), ScreenPercentage, RenderResolution.X, RenderResolution.Y));
		Lines.Add(FString::Printf(TEXT("Draws: %d"), GNumDrawCallsRHI[0]));
		Lines.Add(FString::Printf(TEXT("Prims: %s"), *UOUDebugSubsystemPrivate::FormatCompactCount(GNumPrimitivesDrawnRHI[0])));
	}

	if (PerformanceController.bShowWorldCounts)
	{
		UWorld* World = GetWorld();
		int32 ActorCount = 0;
		int32 ComponentCount = 0;
		if (World != nullptr)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (!IsValid(Actor))
				{
					continue;
				}

				++ActorCount;
				TInlineComponentArray<UActorComponent*> Components(Actor);
				ComponentCount += Components.Num();
			}
		}

		int32 WorldCount = 0;
		if (GEngine != nullptr)
		{
			for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
			{
				if (WorldContext.World() != nullptr)
				{
					++WorldCount;
				}
			}
		}

		Lines.Add(FString::Printf(TEXT("Actors: %d | Components: %d | Worlds: %d"), ActorCount, ComponentCount, WorldCount));
	}

	if (Lines.Num() == 0)
	{
		return FString();
	}

	return FString::Join(Lines, LINE_TERMINATOR);
}

FString UUOUDebugSubsystem::BuildControllerStatusText() const
{
	const AUOUDebugController* DebugController = ActiveDebugController.Get();
	if (DebugController == nullptr)
	{
		return TEXT("UOU Debug: No Controller");
	}

	TArray<FString> EnabledCategories;
	for (const TObjectPtr<UUOUDebugControllerComponentBase>& ControllerComponent : DebugController->GetDebugControllerComponents())
	{
		if (ControllerComponent != nullptr && DebugController->IsCategoryEnabled(ControllerComponent->DebugCategory))
		{
			EnabledCategories.Add(ControllerComponent->GetDebugCategoryName().ToString());
		}
	}

	return FString::Printf(
		TEXT("UOU Debug: %s | Providers: %d | Enabled: %s"),
		DebugController->bEnableDebugTools ? TEXT("On") : TEXT("Off"),
		RegisteredProviders.Num(),
		EnabledCategories.Num() > 0 ? *FString::Join(EnabledCategories, TEXT(", ")) : TEXT("None"));
}
