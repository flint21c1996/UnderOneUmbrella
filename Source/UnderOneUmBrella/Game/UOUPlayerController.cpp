// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Game/UOULevelTransitionSubsystem.h"
#include "InputCoreTypes.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"

namespace
{
// config가 비어 있어도 인게임 HUD를 바로 띄울 수 있도록 기본 경로를 둡니다.
constexpr TCHAR DefaultInGameHUDWidgetClassPath[] = TEXT("/Game/UOU/UI/WBP_InGameHUD.WBP_InGameHUD_C");
constexpr TCHAR DefaultStageSelectLevelPath[] = TEXT("/Game/UOU/Maps/L_MapSelect.L_MapSelect");
}

AUOUPlayerController::AUOUPlayerController()
{
	// 인게임 설정창에서는 타이틀 복귀 버튼을 보여줘야 합니다.
	SetCanReturnToTitle(true);
	SetCanRestartCurrentStage(true);
}

void AUOUPlayerController::OpenStageSelect()
{
	if (bIsOpeningStageSelect)
	{
		return;
	}

	if (StageSelectLevel.IsNull())
	{
		StageSelectLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultStageSelectLevelPath));
	}

	const FString StageSelectPackageName = StageSelectLevel.ToSoftObjectPath().GetLongPackageName();
	if (!FPackageName::DoesPackageExist(StageSelectPackageName))
	{
		UE_LOG(LogTemp, Error, TEXT("Stage select level does not exist: %s"), *StageSelectPackageName);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UUOULevelTransitionSubsystem* TransitionSubsystem = GameInstance != nullptr
		? GameInstance->GetSubsystem<UUOULevelTransitionSubsystem>()
		: nullptr;
	if (TransitionSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("OpenStageSelect failed because the level transition subsystem was not available."));
		return;
	}

	FUOULevelTransitionSettings Settings;
	Settings.FadeOutDuration = 0.35f;
	Settings.BlackHoldDuration = 0.05f;
	Settings.FadeInDuration = 0.35f;
	Settings.bUseCurrentMapExitSettings = false;
	Settings.bUseLoadedMapEnterSettings = false;
	bIsOpeningStageSelect = TransitionSubsystem->RequestLevelTransition(StageSelectLevel, Settings);
}

void AUOUPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyInGameInputMode();

	// 인게임 HUD는 실제 플레이 화면 위에 얹는 최소 UI입니다.
	if (InGameHUDWidgetClass.IsNull())
	{
		InGameHUDWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultInGameHUDWidgetClassPath));
		UE_LOG(LogTemp, Warning, TEXT("In-game HUD widget was not configured. Falling back to %s."), DefaultInGameHUDWidgetClassPath);
	}

	TSubclassOf<UUserWidget> LoadedInGameHUDWidgetClass = InGameHUDWidgetClass.LoadSynchronous();
	if (LoadedInGameHUDWidgetClass == nullptr)
	{
		InGameHUDWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultInGameHUDWidgetClassPath));
		LoadedInGameHUDWidgetClass = InGameHUDWidgetClass.LoadSynchronous();
	}

	if (LoadedInGameHUDWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load in-game HUD widget class."));
		return;
	}

	InGameHUDWidget = CreateWidget<UUserWidget>(this, LoadedInGameHUDWidgetClass);
	if (InGameHUDWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create in-game HUD widget."));
		return;
	}

	InGameHUDWidget->AddToViewport();
}

void AUOUPlayerController::RestoreInputModeAfterSettingsMenu()
{
	ApplyInGameInputMode();
}

void AUOUPlayerController::ApplyInGameInputMode()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
}

void AUOUPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
	{
		return;
	}

	// ESC는 타이틀과 인게임 공통으로 설정창 토글에 사용합니다.
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AUOUPlayerController::ToggleSettingsMenu);
}
