// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugSubsystem.h"

#include "Debug/UOUDebugController.h"
#include "Debug/UOUDebugControllerComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UUOUDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ControllerSearchTimeRemaining = 0.0f;
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

	CompactRegisteredProviders();
	DrawControllerStatus();
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
		DebugController->bEnableDebugTools ? FColor::Cyan : FColor::Silver,
		BuildControllerStatusText());
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
		if (ControllerComponent != nullptr && ControllerComponent->IsDebugEnabled())
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

