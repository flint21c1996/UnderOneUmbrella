// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUMenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/UOULevelTransitionSubsystem.h"
#include "UObject/SoftObjectPath.h"

namespace
{
// config가 비어 있어도 프로토타입을 바로 실행할 수 있도록 기본 경로를 둡니다.
constexpr TCHAR DefaultSettingsMenuWidgetClassPath[] = TEXT("/Game/UOU/UI/WBP_SettingsMenu.WBP_SettingsMenu_C");
constexpr TCHAR DefaultTitleLevelPath[] = TEXT("/Game/UOU/Maps/TitleMap.TitleMap");
}

AUOUMenuPlayerController::AUOUMenuPlayerController()
{
}

void AUOUMenuPlayerController::OpenSettingsMenu()
{
	// 이미 열린 상태에서 중복 생성하면 입력 모드가 꼬일 수 있으므로 한 번만 생성합니다.
	if (bSettingsMenuOpen)
	{
		return;
	}

	if (SettingsMenuWidgetClass.IsNull())
	{
		SettingsMenuWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultSettingsMenuWidgetClassPath));
		UE_LOG(LogTemp, Warning, TEXT("Settings menu widget was not configured. Falling back to %s."), DefaultSettingsMenuWidgetClassPath);
	}

	TSubclassOf<UUserWidget> LoadedSettingsMenuWidgetClass = SettingsMenuWidgetClass.LoadSynchronous();
	if (LoadedSettingsMenuWidgetClass == nullptr)
	{
		SettingsMenuWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultSettingsMenuWidgetClassPath));
		LoadedSettingsMenuWidgetClass = SettingsMenuWidgetClass.LoadSynchronous();
	}

	if (LoadedSettingsMenuWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load settings menu widget class."));
		return;
	}

	SettingsMenuWidget = CreateWidget<UUserWidget>(this, LoadedSettingsMenuWidgetClass);
	if (SettingsMenuWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create settings menu widget."));
		return;
	}

	// 설정창은 현재 화면 위에 떠야 하므로 높은 ZOrder로 올립니다.
	SettingsMenuWidget->AddToViewport(100);
	bSettingsMenuOpen = true;
	ApplySettingsMenuInputMode(SettingsMenuWidget);
}

void AUOUMenuPlayerController::CloseSettingsMenu()
{
	if (!bSettingsMenuOpen)
	{
		return;
	}

	if (SettingsMenuWidget != nullptr)
	{
		SettingsMenuWidget->RemoveFromParent();
		SettingsMenuWidget = nullptr;
	}

	bSettingsMenuOpen = false;
	RestoreInputModeAfterSettingsMenu();
}

void AUOUMenuPlayerController::ToggleSettingsMenu()
{
	if (bSettingsMenuOpen)
	{
		CloseSettingsMenu();
	}
	else
	{
		OpenSettingsMenu();
	}
}

void AUOUMenuPlayerController::ReturnToTitle()
{
	// 타이틀 화면 자체에서는 '타이틀로 돌아가기' 버튼이 동작하지 않도록 권한 플래그를 확인합니다.
	if (!bCanReturnToTitle)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnToTitle was requested, but this controller does not allow it."));
		return;
	}

	if (TitleLevel.IsNull())
	{
		TitleLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultTitleLevelPath));
		UE_LOG(LogTemp, Warning, TEXT("Title level was not configured. Falling back to %s."), DefaultTitleLevelPath);
	}

	UGameInstance* GameInstance = GetGameInstance();
	UUOULevelTransitionSubsystem* TransitionSubsystem = GameInstance != nullptr
		? GameInstance->GetSubsystem<UUOULevelTransitionSubsystem>()
		: nullptr;
	if (TransitionSubsystem != nullptr)
	{
		FUOULevelTransitionSettings ReturnToTitleSettings;
		ReturnToTitleSettings.FadeOutDuration = 0.35f;
		ReturnToTitleSettings.BlackHoldDuration = 0.05f;
		ReturnToTitleSettings.FadeInDuration = 0.35f;
		ReturnToTitleSettings.bUseCurrentMapExitSettings = false;
		ReturnToTitleSettings.bUseLoadedMapEnterSettings = false;
		TransitionSubsystem->RequestLevelTransition(TitleLevel, ReturnToTitleSettings);
	}
}

void AUOUMenuPlayerController::RestartCurrentStage()
{
	if (!bCanRestartCurrentStage)
	{
		UE_LOG(LogTemp, Warning, TEXT("RestartCurrentStage was requested, but this controller does not allow it."));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UUOULevelTransitionSubsystem* TransitionSubsystem = GameInstance != nullptr
		? GameInstance->GetSubsystem<UUOULevelTransitionSubsystem>()
		: nullptr;
	if (TransitionSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("RestartCurrentStage failed because the level transition subsystem was not available."));
		return;
	}

	FUOULevelTransitionSettings RestartSettings;
	if (!TransitionSubsystem->RestartCurrentLevelFromWorld(GetWorld(), RestartSettings))
	{
		UE_LOG(LogTemp, Warning, TEXT("RestartCurrentStage failed to start a current level restart transition."));
	}
}

void AUOUMenuPlayerController::GoToNextLevel()
{
	UGameInstance* GameInstance = GetGameInstance();
	UUOULevelTransitionSubsystem* TransitionSubsystem = GameInstance != nullptr
		? GameInstance->GetSubsystem<UUOULevelTransitionSubsystem>()
		: nullptr;
	if (TransitionSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("GoToNextLevel failed because the level transition subsystem was not available."));
		return;
	}

	FUOULevelTransitionSettings NextLevelSettings;
	if (!TransitionSubsystem->RequestNextLevelFromWorld(GetWorld(), NextLevelSettings))
	{
		UE_LOG(LogTemp, Warning, TEXT("GoToNextLevel failed to start a next level transition."));
	}
}

void AUOUMenuPlayerController::ToggleTestSetting()
{
	RestartCurrentStage();
}

void AUOUMenuPlayerController::ApplySettingsMenuInputMode(UUserWidget* InSettingsMenuWidget)
{
	// 설정창이 열린 동안에는 게임 조작을 잠그고 UI 입력을 우선합니다.
	if (InSettingsMenuWidget != nullptr)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InSettingsMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}

	bShowMouseCursor = true;

	// Ignore 입력은 누적 카운터라 Reset 후 다시 잠가야 닫을 때 예상대로 풀립니다.
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
}

void AUOUMenuPlayerController::RestoreInputModeAfterSettingsMenu()
{
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
}
