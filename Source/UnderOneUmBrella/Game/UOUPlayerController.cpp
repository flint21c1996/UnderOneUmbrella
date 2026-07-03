// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Game/UOUDevelopmentLevelTravelWidget.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UObject/SoftObjectPath.h"

namespace
{
// config가 비어 있어도 인게임 HUD를 바로 띄울 수 있도록 기본 경로를 둡니다.
constexpr TCHAR DefaultInGameHUDWidgetClassPath[] = TEXT("/Game/UOU/UI/WBP_InGameHUD.WBP_InGameHUD_C");
constexpr TCHAR DefaultTitleLevelPath[] = TEXT("/Game/UOU/Maps/TitleMap.TitleMap");
constexpr TCHAR DefaultTutorialLevelPath[] = TEXT("/Game/UOU/Maps/TutorialMap_1.TutorialMap_1");
constexpr TCHAR DefaultMouseStarLevelPath[] = TEXT("/Game/UOU/Maps/MouseStar.MouseStar");
constexpr TCHAR DefaultTempLevelPath[] = TEXT("/Game/UOU/Maps/TempMap.TempMap");
}

AUOUPlayerController::AUOUPlayerController()
{
	// 인게임 설정창에서는 타이틀 복귀 버튼을 보여줘야 합니다.
	SetCanReturnToTitle(true);
	SetCanRestartCurrentStage(true);
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
	CreateDevelopmentLevelTravelWidget();
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

void AUOUPlayerController::CreateDevelopmentLevelTravelWidget()
{
#if !UE_BUILD_SHIPPING
	if (!bShowDevelopmentLevelTravelWidget || DevelopmentLevelTravelWidget != nullptr)
	{
		return;
	}

	TArray<TSoftObjectPtr<UWorld>> QuickLevels = DevelopmentTravelLevels;
	if (QuickLevels.IsEmpty())
	{
		QuickLevels.Add(TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultTitleLevelPath)));
		QuickLevels.Add(TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultTutorialLevelPath)));
		QuickLevels.Add(TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultMouseStarLevelPath)));
		QuickLevels.Add(TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultTempLevelPath)));
	}

	TSoftObjectPtr<UWorld> TitleLevel = DevelopmentTitleLevel;
	if (TitleLevel.IsNull())
	{
		TitleLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultTitleLevelPath));
	}

	DevelopmentLevelTravelWidget = CreateWidget<UUOUDevelopmentLevelTravelWidget>(
		this,
		UUOUDevelopmentLevelTravelWidget::StaticClass());
	if (DevelopmentLevelTravelWidget == nullptr)
	{
		return;
	}

	DevelopmentLevelTravelWidget->SetQuickLevels(QuickLevels);
	DevelopmentLevelTravelWidget->SetTitleLevel(TitleLevel);
	DevelopmentLevelTravelWidget->AddToViewport(250);
#endif
}

void AUOUPlayerController::ToggleDevelopmentLevelTravelWidget()
{
#if !UE_BUILD_SHIPPING
	if (DevelopmentLevelTravelWidget == nullptr)
	{
		CreateDevelopmentLevelTravelWidget();
		return;
	}

	const ESlateVisibility CurrentVisibility = DevelopmentLevelTravelWidget->GetVisibility();
	const bool bShouldShow = CurrentVisibility == ESlateVisibility::Collapsed
		|| CurrentVisibility == ESlateVisibility::Hidden;
	DevelopmentLevelTravelWidget->SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
#endif
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

#if !UE_BUILD_SHIPPING
	InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AUOUPlayerController::ToggleDevelopmentLevelTravelWidget);
#endif
}
