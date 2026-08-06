// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOULevelTransitionSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Game/UOULevelTransitionSettingsActor.h"
#include "UI/UOULevelTransitionOverlayWidget.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr float FadeOverlayTickInterval = 1.0f / 60.0f;
	constexpr TCHAR DefaultTransitionOverlayWidgetClassPath[] = TEXT("/Game/UOU/UI/WBP_LevelTransitionOverlay.WBP_LevelTransitionOverlay_C");
	constexpr TCHAR LevelTransitionDefaultTitleLevelPath[] = TEXT("/Game/UOU/Maps/TitleMap.TitleMap");

	FUOUTransitionMessageSettings SanitizeMessageSettings(FUOUTransitionMessageSettings Settings)
	{
		Settings.FontSize = FMath::Max(1, Settings.FontSize);
		Settings.WrapTextAt = FMath::Max(100.0f, Settings.WrapTextAt);
		Settings.ImageDesiredSize.X = FMath::Max(1.0f, Settings.ImageDesiredSize.X);
		Settings.ImageDesiredSize.Y = FMath::Max(1.0f, Settings.ImageDesiredSize.Y);
		Settings.ViewportZOrder = FMath::Max(0, Settings.ViewportZOrder);
		Settings.MessageFadeInDuration = FMath::Max(0.0f, Settings.MessageFadeInDuration);
		Settings.MessageHoldDuration = FMath::Max(0.0f, Settings.MessageHoldDuration);
		Settings.MessageFadeOutDuration = FMath::Max(0.0f, Settings.MessageFadeOutDuration);
		for (FUOUTransitionMessagePage& AdditionalMessagePage : Settings.AdditionalMessagePages)
		{
			AdditionalMessagePage.ImageDesiredSize.X = FMath::Max(1.0f, AdditionalMessagePage.ImageDesiredSize.X);
			AdditionalMessagePage.ImageDesiredSize.Y = FMath::Max(1.0f, AdditionalMessagePage.ImageDesiredSize.Y);
			AdditionalMessagePage.MessageHoldDuration = FMath::Max(0.0f, AdditionalMessagePage.MessageHoldDuration);
		}
		return Settings;
	}

	void AddMessagePage(
		TArray<FUOUTransitionMessageSettings>& MessagePages,
		const FUOUTransitionMessageSettings& BaseSettings,
		const FText& MessageText,
		UTexture2D* MessageImage,
		FVector2D ImageDesiredSize,
		float MessageHoldDuration)
	{
		if (MessageText.IsEmpty() && MessageImage == nullptr)
		{
			return;
		}

		FUOUTransitionMessageSettings PageSettings = BaseSettings;
		PageSettings.MessageText = MessageText;
		PageSettings.MessageImage = MessageImage;
		PageSettings.ImageDesiredSize = ImageDesiredSize;
		PageSettings.MessageHoldDuration = FMath::Max(0.0f, MessageHoldDuration);
		PageSettings.AdditionalMessageTexts.Reset();
		PageSettings.AdditionalMessagePages.Reset();
		MessagePages.Add(PageSettings);
	}

	TArray<FUOUTransitionMessageSettings> BuildMessagePages(const FUOUTransitionMessageSettings& MessageSettings)
	{
		const FUOUTransitionMessageSettings SanitizedSettings = SanitizeMessageSettings(MessageSettings);
		TArray<FUOUTransitionMessageSettings> MessagePages;

		AddMessagePage(
			MessagePages,
			SanitizedSettings,
			SanitizedSettings.MessageText,
			SanitizedSettings.MessageImage,
			SanitizedSettings.ImageDesiredSize,
			SanitizedSettings.MessageHoldDuration);
		for (const FText& AdditionalMessageText : SanitizedSettings.AdditionalMessageTexts)
		{
			AddMessagePage(
				MessagePages,
				SanitizedSettings,
				AdditionalMessageText,
				nullptr,
				SanitizedSettings.ImageDesiredSize,
				SanitizedSettings.MessageHoldDuration);
		}
		for (const FUOUTransitionMessagePage& AdditionalMessagePage : SanitizedSettings.AdditionalMessagePages)
		{
			AddMessagePage(
				MessagePages,
				SanitizedSettings,
				AdditionalMessagePage.MessageText,
				AdditionalMessagePage.MessageImage,
				AdditionalMessagePage.ImageDesiredSize,
				AdditionalMessagePage.MessageHoldDuration);
		}

		return MessagePages;
	}

	FUOULevelTransitionSettings SanitizeTransitionSettings(FUOULevelTransitionSettings Settings)
	{
		Settings.FadeOutDuration = FMath::Max(0.0f, Settings.FadeOutDuration);
		Settings.BlackHoldDuration = FMath::Max(0.0f, Settings.BlackHoldDuration);
		Settings.FadeInDuration = FMath::Max(0.0f, Settings.FadeInDuration);
		Settings.FadeOutMessageSettings = SanitizeMessageSettings(Settings.FadeOutMessageSettings);
		Settings.FadeInMessageSettings = SanitizeMessageSettings(Settings.FadeInMessageSettings);
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

	HideTransitionOverlay();

	Super::Deinitialize();
}

bool UUOULevelTransitionSubsystem::RequestLevelTransition(
	TSoftObjectPtr<UWorld> TargetLevel,
	FUOULevelTransitionSettings Settings)
{
	return RequestLevelTransitionFromWorld(GetSubsystemWorld(), TargetLevel, Settings);
}

bool UUOULevelTransitionSubsystem::RequestLevelTransitionFromWorld(
	UWorld* SourceWorld,
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

	if (!BeginTransition(SourceWorld, Settings))
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
	return RequestLevelTransitionByNameFromWorld(GetSubsystemWorld(), LevelName, Settings);
}

bool UUOULevelTransitionSubsystem::RequestLevelTransitionByNameFromWorld(
	UWorld* SourceWorld,
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

	if (!BeginTransition(SourceWorld, Settings))
	{
		ResetPendingTransition();
		return false;
	}

	return true;
}

bool UUOULevelTransitionSubsystem::RestartCurrentLevel(FUOULevelTransitionSettings Settings)
{
	return RestartCurrentLevelFromWorld(GetSubsystemWorld(), Settings);
}

bool UUOULevelTransitionSubsystem::RestartCurrentLevelFromWorld(
	UWorld* SourceWorld,
	FUOULevelTransitionSettings Settings)
{
	if (bIsTransitioning)
	{
		return false;
	}

	if (SourceWorld == nullptr)
	{
		return false;
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(SourceWorld, true);
	if (CurrentLevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Current level name could not be resolved for restart."));
		return false;
	}

	PendingTargetType = ETransitionTargetType::LevelName;
	PendingTargetLevel.Reset();
	PendingLevelName = FName(*CurrentLevelName);
	Settings.bSuppressFadeOutMessage = true;

	if (!BeginTransition(SourceWorld, Settings))
	{
		ResetPendingTransition();
		return false;
	}

	return true;
}

bool UUOULevelTransitionSubsystem::RequestNextLevel(FUOULevelTransitionSettings Settings)
{
	return RequestNextLevelFromWorld(GetSubsystemWorld(), Settings);
}

bool UUOULevelTransitionSubsystem::RequestNextLevelFromWorld(
	UWorld* SourceWorld,
	FUOULevelTransitionSettings Settings)
{
	if (bIsTransitioning)
	{
		return false;
	}

	if (SourceWorld == nullptr)
	{
		return false;
	}

	TSoftObjectPtr<UWorld> NextLevel;
	const AUOULevelTransitionSettingsActor* SettingsActor = FindLevelTransitionSettingsActor(SourceWorld);
	if (SettingsActor != nullptr)
	{
		NextLevel = SettingsActor->TargetLevel;
	}

	if (NextLevel.IsNull())
	{
		NextLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(LevelTransitionDefaultTitleLevelPath));
		UE_LOG(LogTemp, Warning, TEXT("Next level was not configured. Falling back to %s."), LevelTransitionDefaultTitleLevelPath);
	}

	PendingTargetType = ETransitionTargetType::SoftLevel;
	PendingTargetLevel = NextLevel;
	PendingLevelName = NAME_None;

	if (!BeginTransition(SourceWorld, Settings))
	{
		ResetPendingTransition();
		return false;
	}

	return true;
}

bool UUOULevelTransitionSubsystem::RequestPreviousLevel(FUOULevelTransitionSettings Settings)
{
	return RequestPreviousLevelFromWorld(GetSubsystemWorld(), Settings);
}

bool UUOULevelTransitionSubsystem::RequestPreviousLevelFromWorld(
	UWorld* SourceWorld,
	FUOULevelTransitionSettings Settings)
{
	if (bIsTransitioning)
	{
		return false;
	}

	if (SourceWorld == nullptr)
	{
		return false;
	}

	TSoftObjectPtr<UWorld> PreviousLevel;
	const AUOULevelTransitionSettingsActor* SettingsActor = FindLevelTransitionSettingsActor(SourceWorld);
	if (SettingsActor != nullptr)
	{
		PreviousLevel = SettingsActor->PreviousLevel;
	}

	if (PreviousLevel.IsNull())
	{
		PreviousLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(LevelTransitionDefaultTitleLevelPath));
		UE_LOG(LogTemp, Warning, TEXT("Previous level was not configured. Falling back to %s."), LevelTransitionDefaultTitleLevelPath);
	}

	PendingTargetType = ETransitionTargetType::SoftLevel;
	PendingTargetLevel = PreviousLevel;
	PendingLevelName = NAME_None;
	Settings.bSuppressFadeOutMessage = true;

	if (!BeginTransition(SourceWorld, Settings))
	{
		ResetPendingTransition();
		return false;
	}

	return true;
}

void UUOULevelTransitionSubsystem::CancelTransition()
{
	UWorld* World = GetActiveTransitionWorld();
	if (World != nullptr)
	{
		ClearTransitionTimers(World);

		if (APlayerController* PlayerController = ResolvePlayerController(World))
		{
			if (ActiveSettings.bFadeAudio && PlayerController->PlayerCameraManager != nullptr)
			{
				PlayerController->PlayerCameraManager->StopCameraFade();
			}
		}

		if (bInputLockedDuringTransition)
		{
			SetPlayerInputLocked(World, false);
		}
	}

	ResetPendingTransition();
}

bool UUOULevelTransitionSubsystem::BeginTransition(UWorld* TransitionWorld, FUOULevelTransitionSettings Settings)
{
	UWorld* World = TransitionWorld;
	if (World == nullptr)
	{
		return false;
	}

	ActiveTransitionWorld = World;
	ActiveSettings = SanitizeTransitionSettings(Settings);
	if (ActiveSettings.bUseCurrentMapExitSettings)
	{
		ApplyCurrentMapExitSettings(World, ActiveSettings);
	}
	ActiveSettings = SanitizeTransitionSettings(ActiveSettings);
	if (ActiveSettings.bSuppressFadeOutMessage)
	{
		ActiveSettings.FadeOutMessageSettings = FUOUTransitionMessageSettings();
	}
	bIsTransitioning = true;
	FadeOverlayElapsedTime = 0.0f;

	if (ActiveSettings.bLockPlayerInputDuringTransition)
	{
		SetPlayerInputLocked(World, true);
		bInputLockedDuringTransition = true;
	}

	ShowTransitionOverlay(World, ActiveSettings.FadeOutMessageSettings, 0.0f, 0.0f);

	APlayerController* PlayerController = ResolvePlayerController(World);
	if (ActiveSettings.bFadeAudio && PlayerController != nullptr && PlayerController->PlayerCameraManager != nullptr)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(
			0.0f,
			1.0f,
			ActiveSettings.FadeOutDuration,
			ActiveSettings.FadeColor,
			ActiveSettings.bFadeAudio,
			true);
	}

	if (ActiveSettings.FadeOutDuration <= 0.0f)
	{
		SetTransitionBackgroundOpacity(1.0f);
		FinishFadeOut();
		return true;
	}

	SetTransitionBackgroundOpacity(0.0f);
	SetTransitionMessageOpacity(0.0f);

	World->GetTimerManager().SetTimer(
		FadeOutTimerHandle,
		this,
		&UUOULevelTransitionSubsystem::UpdateFadeOutOverlay,
		FadeOverlayTickInterval,
		true);

	return true;
}

void UUOULevelTransitionSubsystem::UpdateFadeOutOverlay()
{
	UWorld* World = GetActiveTransitionWorld();
	if (World == nullptr)
	{
		FinishFadeOut();
		return;
	}

	FadeOverlayElapsedTime += FadeOverlayTickInterval;
	const float Alpha = ActiveSettings.FadeOutDuration > 0.0f
		? FMath::Clamp(FadeOverlayElapsedTime / ActiveSettings.FadeOutDuration, 0.0f, 1.0f)
		: 1.0f;
	SetTransitionBackgroundOpacity(Alpha);

	if (Alpha >= 1.0f)
	{
		World->GetTimerManager().ClearTimer(FadeOutTimerHandle);
		FinishFadeOut();
	}
}

void UUOULevelTransitionSubsystem::FinishFadeOut()
{
	SetTransitionBackgroundOpacity(1.0f);
	SetTransitionMessageOpacity(0.0f);

	UWorld* World = GetActiveTransitionWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(FadeOutTimerHandle);
	}

	StartFadeOutMessageSequence();
}

void UUOULevelTransitionSubsystem::StartFadeOutMessageSequence()
{
	StartMessageSequence(ETransitionMessageStage::FadeOut, ActiveSettings.FadeOutMessageSettings);
}

void UUOULevelTransitionSubsystem::StartFadeInMessageSequence()
{
	StartMessageSequence(ETransitionMessageStage::FadeIn, ActiveSettings.FadeInMessageSettings);
}

void UUOULevelTransitionSubsystem::StartMessageSequence(
	ETransitionMessageStage MessageStage,
	const FUOUTransitionMessageSettings& MessageSettings)
{
	ActiveMessageStage = MessageStage;
	ActiveMessagePages = BuildMessagePages(MessageSettings);
	ActiveMessagePageIndex = ActiveMessagePages.Num() > 0 ? 0 : INDEX_NONE;
	ActiveMessageSettings = ActiveMessagePageIndex != INDEX_NONE
		? ActiveMessagePages[ActiveMessagePageIndex]
		: FUOUTransitionMessageSettings();
	MessageElapsedTime = 0.0f;

	StartActiveMessagePage();
}

void UUOULevelTransitionSubsystem::StartActiveMessagePage()
{
	UWorld* World = ActiveMessageStage == ETransitionMessageStage::FadeIn
		? FadeInWorld.Get()
		: GetActiveTransitionWorld();
	if (World == nullptr)
	{
		World = GetActiveTransitionWorld();
	}

	MessageElapsedTime = 0.0f;

	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(MessageTimerHandle);
		ShowTransitionOverlay(World, ActiveMessageSettings, 1.0f, 0.0f);
	}

	SetTransitionBackgroundOpacity(1.0f);
	SetTransitionMessageOpacity(0.0f);

	if (ActiveMessagePageIndex == INDEX_NONE || !ActiveMessageSettings.ShouldDisplay())
	{
		FinishMessageSequence();
		return;
	}

	if (ActiveMessageSettings.MessageFadeInDuration <= 0.0f)
	{
		SetTransitionMessageOpacity(1.0f);
		FinishMessageFadeIn();
		return;
	}

	if (World == nullptr)
	{
		FinishMessageSequence();
		return;
	}

	World->GetTimerManager().SetTimer(
		MessageTimerHandle,
		this,
		&UUOULevelTransitionSubsystem::UpdateMessageFadeIn,
		FadeOverlayTickInterval,
		true);
}

void UUOULevelTransitionSubsystem::UpdateMessageFadeIn()
{
	UWorld* World = ActiveMessageStage == ETransitionMessageStage::FadeIn
		? FadeInWorld.Get()
		: GetActiveTransitionWorld();
	if (World == nullptr)
	{
		FinishMessageSequence();
		return;
	}

	MessageElapsedTime += FadeOverlayTickInterval;
	const float Alpha = ActiveMessageSettings.MessageFadeInDuration > 0.0f
		? FMath::Clamp(MessageElapsedTime / ActiveMessageSettings.MessageFadeInDuration, 0.0f, 1.0f)
		: 1.0f;
	SetTransitionMessageOpacity(Alpha);

	if (Alpha >= 1.0f)
	{
		World->GetTimerManager().ClearTimer(MessageTimerHandle);
		FinishMessageFadeIn();
	}
}

void UUOULevelTransitionSubsystem::FinishMessageFadeIn()
{
	SetTransitionMessageOpacity(1.0f);
	MessageElapsedTime = 0.0f;

	UWorld* World = ActiveMessageStage == ETransitionMessageStage::FadeIn
		? FadeInWorld.Get()
		: GetActiveTransitionWorld();
	if (World == nullptr || ActiveMessageSettings.MessageHoldDuration <= 0.0f)
	{
		FinishMessageHold();
		return;
	}

	World->GetTimerManager().SetTimer(
		MessageTimerHandle,
		this,
		&UUOULevelTransitionSubsystem::FinishMessageHold,
		ActiveMessageSettings.MessageHoldDuration,
		false);
}

void UUOULevelTransitionSubsystem::FinishMessageHold()
{
	MessageElapsedTime = 0.0f;

	if (ActiveMessageSettings.MessageFadeOutDuration <= 0.0f)
	{
		SetTransitionMessageOpacity(0.0f);
		FinishMessageSequence();
		return;
	}

	UWorld* World = ActiveMessageStage == ETransitionMessageStage::FadeIn
		? FadeInWorld.Get()
		: GetActiveTransitionWorld();
	if (World == nullptr)
	{
		FinishMessageSequence();
		return;
	}

	World->GetTimerManager().SetTimer(
		MessageTimerHandle,
		this,
		&UUOULevelTransitionSubsystem::UpdateMessageFadeOut,
		FadeOverlayTickInterval,
		true);
}

void UUOULevelTransitionSubsystem::UpdateMessageFadeOut()
{
	UWorld* World = ActiveMessageStage == ETransitionMessageStage::FadeIn
		? FadeInWorld.Get()
		: GetActiveTransitionWorld();
	if (World == nullptr)
	{
		FinishMessageSequence();
		return;
	}

	MessageElapsedTime += FadeOverlayTickInterval;
	const float Alpha = ActiveMessageSettings.MessageFadeOutDuration > 0.0f
		? 1.0f - FMath::Clamp(MessageElapsedTime / ActiveMessageSettings.MessageFadeOutDuration, 0.0f, 1.0f)
		: 0.0f;
	SetTransitionMessageOpacity(Alpha);

	if (Alpha <= 0.0f)
	{
		World->GetTimerManager().ClearTimer(MessageTimerHandle);
		FinishMessageSequence();
	}
}

void UUOULevelTransitionSubsystem::FinishMessageSequence()
{
	SetTransitionMessageOpacity(0.0f);

	UWorld* World = ActiveMessageStage == ETransitionMessageStage::FadeIn
		? FadeInWorld.Get()
		: GetActiveTransitionWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(MessageTimerHandle);
	}

	const int32 NextMessagePageIndex = ActiveMessagePageIndex + 1;
	if (ActiveMessagePages.IsValidIndex(NextMessagePageIndex))
	{
		ActiveMessagePageIndex = NextMessagePageIndex;
		ActiveMessageSettings = ActiveMessagePages[ActiveMessagePageIndex];
		StartActiveMessagePage();
		return;
	}

	const ETransitionMessageStage FinishedStage = ActiveMessageStage;
	ActiveMessageStage = ETransitionMessageStage::None;
	ActiveMessageSettings = FUOUTransitionMessageSettings();
	ActiveMessagePages.Reset();
	ActiveMessagePageIndex = INDEX_NONE;
	MessageElapsedTime = 0.0f;

	switch (FinishedStage)
	{
	case ETransitionMessageStage::FadeOut:
		ContinueAfterFadeOutMessageSequence();
		break;
	case ETransitionMessageStage::FadeIn:
		ContinueAfterFadeInMessageSequence();
		break;
	case ETransitionMessageStage::None:
	default:
		break;
	}
}

void UUOULevelTransitionSubsystem::ContinueAfterFadeOutMessageSequence()
{
	UWorld* World = GetActiveTransitionWorld();
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

void UUOULevelTransitionSubsystem::ContinueAfterFadeInMessageSequence()
{
	UWorld* World = FadeInWorld.Get();
	if (World == nullptr)
	{
		World = GetActiveTransitionWorld();
	}

	if (World == nullptr || ActiveSettings.FadeInDuration <= 0.0f)
	{
		FinishTransition();
		return;
	}

	FadeOverlayElapsedTime = 0.0f;

	World->GetTimerManager().SetTimer(
		FadeInTimerHandle,
		this,
		&UUOULevelTransitionSubsystem::UpdateFadeInOverlay,
		FadeOverlayTickInterval,
		true);
}

void UUOULevelTransitionSubsystem::OpenPendingLevel()
{
	UWorld* World = GetActiveTransitionWorld();
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
	bWaitingForPostLoadFadeIn = true;

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

	if (!bIssuedOpenLevel)
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
	FadeOverlayElapsedTime = 0.0f;
	LoadedWorld->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &UUOULevelTransitionSubsystem::StartPostLoadFadeIn));
}

void UUOULevelTransitionSubsystem::StartPostLoadFadeIn()
{
	UWorld* World = FadeInWorld.Get();
	if (World == nullptr)
	{
		World = GetActiveTransitionWorld();
	}

	APlayerController* PlayerController = ResolvePlayerController(World);
	if (World == nullptr)
	{
		FinishTransition();
		return;
	}

	if (ActiveSettings.bUseLoadedMapEnterSettings)
	{
		// 도착 맵에 설정 액터가 없으면 전환 요청에 담긴 Fade In 설정을 fallback으로 유지합니다.
		// 기존에는 이 경우 메시지 설정을 비워 문구와 이미지가 Fade In 단계에서 사라졌습니다.
		ApplyLoadedMapEnterSettings(World, ActiveSettings);
	}
	ActiveSettings = SanitizeTransitionSettings(ActiveSettings);

	if (ActiveSettings.bLockPlayerInputDuringTransition)
	{
		SetPlayerInputLocked(World, true);
		bInputLockedDuringTransition = true;
	}

	if (PlayerController != nullptr && PlayerController->PlayerCameraManager != nullptr)
	{
		PlayerController->PlayerCameraManager->StopCameraFade();
	}

	ShowTransitionOverlay(World, ActiveSettings.FadeInMessageSettings, 1.0f, 0.0f);
	StartFadeInMessageSequence();
}

void UUOULevelTransitionSubsystem::UpdateFadeInOverlay()
{
	UWorld* World = FadeInWorld.Get();
	if (World == nullptr)
	{
		World = GetActiveTransitionWorld();
	}

	if (World == nullptr)
	{
		FinishTransition();
		return;
	}

	FadeOverlayElapsedTime += FadeOverlayTickInterval;
	const float Alpha = ActiveSettings.FadeInDuration > 0.0f
		? 1.0f - FMath::Clamp(FadeOverlayElapsedTime / ActiveSettings.FadeInDuration, 0.0f, 1.0f)
		: 0.0f;
	SetTransitionBackgroundOpacity(Alpha);
	SetTransitionMessageOpacity(0.0f);

	if (Alpha <= 0.0f)
	{
		World->GetTimerManager().ClearTimer(FadeInTimerHandle);
		FinishTransition();
	}
}

void UUOULevelTransitionSubsystem::FinishTransition()
{
	UWorld* World = FadeInWorld.Get();
	if (World == nullptr)
	{
		World = GetActiveTransitionWorld();
	}

	if (World != nullptr)
	{
		ClearTransitionTimers(World);

		if (bInputLockedDuringTransition)
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
	World->GetTimerManager().ClearTimer(MessageTimerHandle);
}

void UUOULevelTransitionSubsystem::ResetPendingTransition()
{
	HideTransitionOverlay();

	PendingTargetType = ETransitionTargetType::None;
	PendingTargetLevel.Reset();
	PendingLevelName = NAME_None;
	ActiveSettings = FUOULevelTransitionSettings();
	ActiveMessageSettings = FUOUTransitionMessageSettings();
	ActiveMessagePages.Reset();
	ActiveTransitionWorld.Reset();
	FadeInWorld.Reset();
	FadeOverlayElapsedTime = 0.0f;
	MessageElapsedTime = 0.0f;
	ActiveMessagePageIndex = INDEX_NONE;
	ActiveMessageStage = ETransitionMessageStage::None;
	bIsTransitioning = false;
	bWaitingForPostLoadFadeIn = false;
	bInputLockedDuringTransition = false;
}

void UUOULevelTransitionSubsystem::ApplyCurrentMapExitSettings(UWorld* World, FUOULevelTransitionSettings& Settings) const
{
	const AUOULevelTransitionSettingsActor* SettingsActor = FindLevelTransitionSettingsActor(World);
	if (SettingsActor == nullptr)
	{
		return;
	}

	Settings.FadeOutDuration = SettingsActor->ExitFadeOutDuration;
	Settings.BlackHoldDuration = SettingsActor->ExitBlackHoldDuration;
	Settings.FadeColor = SettingsActor->FadeColor;
	Settings.bFadeAudio = SettingsActor->bFadeAudio;
	Settings.bLockPlayerInputDuringTransition = SettingsActor->bLockPlayerInputDuringTransition;
	Settings.FadeOutMessageSettings = SettingsActor->ExitMessageSettings;
}

bool UUOULevelTransitionSubsystem::ApplyLoadedMapEnterSettings(UWorld* World, FUOULevelTransitionSettings& Settings) const
{
	const AUOULevelTransitionSettingsActor* SettingsActor = FindLevelTransitionSettingsActor(World);
	if (SettingsActor == nullptr)
	{
		return false;
	}

	Settings.FadeInDuration = SettingsActor->EnterFadeInDuration;
	Settings.FadeColor = SettingsActor->FadeColor;
	Settings.bFadeAudio = SettingsActor->bFadeAudio;
	Settings.bLockPlayerInputDuringTransition = SettingsActor->bLockPlayerInputDuringTransition;
	Settings.FadeInMessageSettings = SettingsActor->EnterMessageSettings;
	return true;
}

const AUOULevelTransitionSettingsActor* UUOULevelTransitionSubsystem::FindLevelTransitionSettingsActor(UWorld* World) const
{
	if (World == nullptr)
	{
		return nullptr;
	}

	for (TActorIterator<AUOULevelTransitionSettingsActor> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

UUOULevelTransitionOverlayWidget* UUOULevelTransitionSubsystem::GetOrCreateTransitionOverlay(UWorld* World, const FUOUTransitionMessageSettings& MessageSettings)
{
	if (TransitionOverlayWidget != nullptr)
	{
		if (!TransitionOverlayWidget->IsInViewport())
		{
			TransitionOverlayWidget->AddToViewport(MessageSettings.ViewportZOrder);
		}

		return TransitionOverlayWidget;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return nullptr;
	}

	TSoftClassPtr<UUOULevelTransitionOverlayWidget> OverlayWidgetClass{ FSoftClassPath(DefaultTransitionOverlayWidgetClassPath) };
	TSubclassOf<UUOULevelTransitionOverlayWidget> LoadedOverlayWidgetClass = OverlayWidgetClass.LoadSynchronous();
	if (LoadedOverlayWidgetClass == nullptr)
	{
		LoadedOverlayWidgetClass = UUOULevelTransitionOverlayWidget::StaticClass();
	}

	TransitionOverlayWidget = CreateWidget<UUOULevelTransitionOverlayWidget>(GameInstance, LoadedOverlayWidgetClass);
	if (TransitionOverlayWidget != nullptr)
	{
		TransitionOverlayWidget->AddToViewport(MessageSettings.ViewportZOrder);
	}

	return TransitionOverlayWidget;
}

void UUOULevelTransitionSubsystem::ShowTransitionOverlay(
	UWorld* World,
	const FUOUTransitionMessageSettings& MessageSettings,
	float InitialBackgroundOpacity,
	float InitialMessageOpacity)
{
	UUOULevelTransitionOverlayWidget* OverlayWidget = GetOrCreateTransitionOverlay(World, MessageSettings);
	if (OverlayWidget == nullptr)
	{
		return;
	}

	OverlayWidget->SetTransitionBackgroundColor(ActiveSettings.FadeColor);
	OverlayWidget->ApplyTransitionMessage(MessageSettings);
	OverlayWidget->SetRenderOpacity(1.0f);
	OverlayWidget->SetTransitionBackgroundOpacity(InitialBackgroundOpacity);
	OverlayWidget->SetTransitionMessageOpacity(InitialMessageOpacity);
}

void UUOULevelTransitionSubsystem::SetTransitionBackgroundOpacity(float NewOpacity)
{
	if (TransitionOverlayWidget != nullptr)
	{
		TransitionOverlayWidget->SetTransitionBackgroundOpacity(NewOpacity);
	}
}

void UUOULevelTransitionSubsystem::SetTransitionMessageOpacity(float NewOpacity)
{
	if (TransitionOverlayWidget != nullptr)
	{
		TransitionOverlayWidget->SetTransitionMessageOpacity(NewOpacity);
	}
}

void UUOULevelTransitionSubsystem::HideTransitionOverlay()
{
	if (TransitionOverlayWidget != nullptr)
	{
		TransitionOverlayWidget->RemoveFromParent();
		TransitionOverlayWidget = nullptr;
	}
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

UWorld* UUOULevelTransitionSubsystem::GetActiveTransitionWorld() const
{
	if (UWorld* World = ActiveTransitionWorld.Get())
	{
		return World;
	}

	return GetSubsystemWorld();
}

UWorld* UUOULevelTransitionSubsystem::GetSubsystemWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance != nullptr ? GameInstance->GetWorld() : nullptr;
}
