// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugSubsystem.h"

#include "Debug/UOUDebugController.h"
#include "Debug/UOUDebugControllerComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Debug/UOUPuzzleDebugProviderComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

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

	bool TryGetViewLocation(UWorld* World, FVector& OutViewLocation)
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

		FRotator ViewRotation = FRotator::ZeroRotator;
		PlayerController->GetPlayerViewPoint(OutViewLocation, ViewRotation);
		return true;
	}

	struct FProviderDrawCandidate
	{
		UObject* ProviderObject = nullptr;
		float DistanceSquared = 0.0f;
	};

	void GatherProviderDrawCandidates(
		const TArray<TWeakObjectPtr<UObject>>& RegisteredProviders,
		UWorld* World,
		const AUOUDebugController* DebugController,
		bool bLabelPass,
		TArray<UObject*>& OutProviderObjects)
	{
		OutProviderObjects.Reset();

		FVector ViewLocation = FVector::ZeroVector;
		const bool bHasViewLocation = TryGetViewLocation(World, ViewLocation);
		const float MaxVisibleDistance = DebugController != nullptr ? DebugController->WorldDebugVisibleDistance : 0.0f;
		const float MaxVisibleDistanceSquared = FMath::Square(MaxVisibleDistance);

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
				}
			}

			Candidates.Add({ ProviderObject, DistanceSquared });
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
}

void UUOUDebugSubsystem::Deinitialize()
{
	ActiveDebugController.Reset();
	RegisteredProviders.Reset();

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
			return PuzzleController->bShowConnections;
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
