// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOULevelTransitionSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FUOULevelTransitionSettings SanitizeTransitionSettings(FUOULevelTransitionSettings Settings)
	{
		Settings.FadeOutDuration = FMath::Max(0.0f, Settings.FadeOutDuration);
		Settings.BlackHoldDuration = FMath::Max(0.0f, Settings.BlackHoldDuration);
		Settings.FadeInDuration = FMath::Max(0.0f, Settings.FadeInDuration);
		return Settings;
	}
}

void UUOULevelTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UUOULevelTransitionSubsystem::HandlePostLoadMapWithWorld);
}

void UUOULevelTransitionSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	if (UWorld* World = GetSubsystemWorld())
	{
		ClearTransitionTimers(World);
	}

	Super::Deinitialize();
}

bool UUOULevelTransitionSubsystem::RequestLevelTransition(
	TSoftObjectPtr<UWorld> TargetLevel,
	FUOULevelTransitionSettings Settings)
{
	if (bIsTransitioning)
	{
		return false;
	}

	if (TargetLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("Level transition requested without a target level."));
		return false;
	}

	PendingTargetType = ETransitionTargetType::SoftLevel;
	PendingTargetLevel = TargetLevel;
	PendingLevelName = NAME_None;

	if (!BeginTransition(Settings))
	{
		ResetPendingTransition();
		return false;
	}

	return true;
}

bool UUOULevelTransitionSubsystem::RequestLevelTransitionByName(
	FName LevelName,
	FUOULevelTransitionSettings Settings)
{
	if (bIsTransitioning)
	{
		return false;
	}

	if (LevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Level transition requested without a level name."));
		return false;
	}

	PendingTargetType = ETransitionTargetType::LevelName;
	PendingTargetLevel.Reset();
	PendingLevelName = LevelName;

	if (!BeginTransition(Settings))
	{
		ResetPendingTransition();
		return false;
	}

	return true;
}

bool UUOULevelTransitionSubsystem::RestartCurrentLevel(FUOULevelTransitionSettings Settings)
{
	UWorld* World = GetSubsystemWorld();
	if (World == nullptr)
	{
		return false;
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	if (CurrentLevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Current level name could not be resolved for restart."));
		return false;
	}

	return RequestLevelTransitionByName(FName(*CurrentLevelName), Settings);
}

void UUOULevelTransitionSubsystem::CancelTransition()
{
	UWorld* World = GetSubsystemWorld();
	if (World != nullptr)
	{
		ClearTransitionTimers(World);

		if (APlayerController* PlayerController = ResolvePlayerController(World))
		{
			if (PlayerController->PlayerCameraManager != nullptr)
			{
				PlayerController->PlayerCameraManager->StopCameraFade();
			}
		}

		if (ActiveSettings.bLockPlayerInputDuringTransition)
		{
			SetPlayerInputLocked(World, false);
		}
	}

	ResetPendingTransition();
}

bool UUOULevelTransitionSubsystem::BeginTransition(FUOULevelTransitionSettings Settings)
{
	UWorld* World = GetSubsystemWorld();
	if (World == nullptr)
	{
		return false;
	}

	ActiveSettings = SanitizeTransitionSettings(Settings);
	bIsTransitioning = true;

	if (ActiveSettings.bLockPlayerInputDuringTransition)
	{
		SetPlayerInputLocked(World, true);
	}

	APlayerController* PlayerController = ResolvePlayerController(World);
	if (PlayerController == nullptr
		|| PlayerController->PlayerCameraManager == nullptr
		|| ActiveSettings.FadeOutDuration <= 0.0f)
	{
		FinishFadeOut();
		return true;
	}

	PlayerController->PlayerCameraManager->StartCameraFade(
		0.0f,
		1.0f,
		ActiveSettings.FadeOutDuration,
		ActiveSettings.FadeColor,
		ActiveSettings.bFadeAudio,
		true);

	World->GetTimerManager().SetTimer(
		FadeOutTimerHandle,
		this,
		&UUOULevelTransitionSubsystem::FinishFadeOut,
		ActiveSettings.FadeOutDuration,
		false);

	return true;
}

void UUOULevelTransitionSubsystem::FinishFadeOut()
{
	UWorld* World = GetSubsystemWorld();
	if (World == nullptr || ActiveSettings.BlackHoldDuration <= 0.0f)
	{
		OpenPendingLevel();
		return;
	}

	World->GetTimerManager().SetTimer(
		BlackHoldTimerHandle,
		this,
		&UUOULevelTransitionSubsystem::OpenPendingLevel,
		ActiveSettings.BlackHoldDuration,
		false);
}

void UUOULevelTransitionSubsystem::OpenPendingLevel()
{
	UWorld* World = GetSubsystemWorld();
	if (World == nullptr)
	{
		ResetPendingTransition();
		return;
	}

	ClearTransitionTimers(World);

	const ETransitionTargetType TargetType = PendingTargetType;
	const TSoftObjectPtr<UWorld> TargetLevel = PendingTargetLevel;
	const FName LevelName = PendingLevelName;
	bool bIssuedOpenLevel = false;
	bWaitingForPostLoadFadeIn = ActiveSettings.FadeInDuration > 0.0f;

	switch (TargetType)
	{
	case ETransitionTargetType::SoftLevel:
		if (!TargetLevel.IsNull())
		{
			UGameplayStatics::OpenLevelBySoftObjectPtr(World, TargetLevel);
			bIssuedOpenLevel = true;
		}
		break;
	case ETransitionTargetType::LevelName:
		if (!LevelName.IsNone())
		{
			UGameplayStatics::OpenLevel(World, LevelName);
			bIssuedOpenLevel = true;
		}
		break;
	case ETransitionTargetType::None:
	default:
		break;
	}

	if (!bIssuedOpenLevel || !bWaitingForPostLoadFadeIn)
	{
		FinishTransition();
	}
}

void UUOULevelTransitionSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!bWaitingForPostLoadFadeIn || LoadedWorld == nullptr || !LoadedWorld->IsGameWorld())
	{
		return;
	}

	bWaitingForPostLoadFadeIn = false;
	FadeInWorld = LoadedWorld;
	LoadedWorld->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &UUOULevelTransitionSubsystem::StartPostLoadFadeIn));
}

void UUOULevelTransitionSubsystem::StartPostLoadFadeIn()
{
	UWorld* World = FadeInWorld.Get();
	if (World == nullptr)
	{
		World = GetSubsystemWorld();
	}

	APlayerController* PlayerController = ResolvePlayerController(World);
	if (World == nullptr || PlayerController == nullptr || PlayerController->PlayerCameraManager == nullptr)
	{
		FinishTransition();
		return;
	}

	PlayerController->PlayerCameraManager->StartCameraFade(
		1.0f,
		1.0f,
		0.0f,
		ActiveSettings.FadeColor,
		false,
		true);

	if (ActiveSettings.FadeInDuration <= 0.0f)
	{
		PlayerController->PlayerCameraManager->StopCameraFade();
		FinishTransition();
		return;
	}

	PlayerController->PlayerCameraManager->StartCameraFade(
		1.0f,
		0.0f,
		ActiveSettings.FadeInDuration,
		ActiveSettings.FadeColor,
		ActiveSettings.bFadeAudio,
		false);

	World->GetTimerManager().SetTimer(
		FadeInTimerHandle,
		this,
		&UUOULevelTransitionSubsystem::FinishTransition,
		ActiveSettings.FadeInDuration,
		false);
}

void UUOULevelTransitionSubsystem::FinishTransition()
{
	UWorld* World = FadeInWorld.Get();
	if (World == nullptr)
	{
		World = GetSubsystemWorld();
	}

	if (World != nullptr)
	{
		ClearTransitionTimers(World);

		if (ActiveSettings.bLockPlayerInputDuringTransition)
		{
			SetPlayerInputLocked(World, false);
		}
	}

	ResetPendingTransition();
}

void UUOULevelTransitionSubsystem::ClearTransitionTimers(UWorld* World)
{
	if (World == nullptr)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(FadeOutTimerHandle);
	World->GetTimerManager().ClearTimer(BlackHoldTimerHandle);
	World->GetTimerManager().ClearTimer(FadeInTimerHandle);
}

void UUOULevelTransitionSubsystem::ResetPendingTransition()
{
	PendingTargetType = ETransitionTargetType::None;
	PendingTargetLevel.Reset();
	PendingLevelName = NAME_None;
	ActiveSettings = FUOULevelTransitionSettings();
	FadeInWorld.Reset();
	bIsTransitioning = false;
	bWaitingForPostLoadFadeIn = false;
}

void UUOULevelTransitionSubsystem::SetPlayerInputLocked(UWorld* World, bool bLocked) const
{
	APlayerController* PlayerController = ResolvePlayerController(World);
	if (PlayerController == nullptr)
	{
		return;
	}

	PlayerController->SetIgnoreMoveInput(bLocked);
	PlayerController->SetIgnoreLookInput(bLocked);
}

APlayerController* UUOULevelTransitionSubsystem::ResolvePlayerController(UWorld* World) const
{
	return World != nullptr ? World->GetFirstPlayerController() : nullptr;
}

UWorld* UUOULevelTransitionSubsystem::GetSubsystemWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance != nullptr ? GameInstance->GetWorld() : nullptr;
}
