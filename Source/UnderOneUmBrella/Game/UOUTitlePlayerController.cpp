// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUTitlePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/SoftObjectPath.h"

namespace
{
// config가 비어 있어도 타이틀 메뉴를 바로 테스트할 수 있도록 기본 경로를 둡니다.
constexpr TCHAR DefaultTestLevelPath[] = TEXT("/Game/UOU/Maps/TempMap.TempMap");
constexpr TCHAR DefaultTitleMenuWidgetClassPath[] = TEXT("/Game/UOU/UI/WBP_TitleMenu.WBP_TitleMenu_C");
}

AUOUTitlePlayerController::AUOUTitlePlayerController()
{
	bShowMouseCursor = false;
}

void AUOUTitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 타이틀 메뉴는 PlayerController가 직접 생성해서 맵 자체는 비어 있는 상태로 유지합니다.
	if (TestLevel.IsNull())
	{
		TestLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(DefaultTestLevelPath));
		UE_LOG(LogTemp, Warning, TEXT("Title test level was not configured. Falling back to %s."), DefaultTestLevelPath);
	}

	if (TitleMenuWidgetClass.IsNull())
	{
		TitleMenuWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultTitleMenuWidgetClassPath));
		UE_LOG(LogTemp, Warning, TEXT("Title menu widget was not configured. Falling back to %s."), DefaultTitleMenuWidgetClassPath);
	}

	TSubclassOf<UUserWidget> LoadedTitleMenuWidgetClass = TitleMenuWidgetClass.LoadSynchronous();
	if (LoadedTitleMenuWidgetClass == nullptr)
	{
		TitleMenuWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultTitleMenuWidgetClassPath));
		LoadedTitleMenuWidgetClass = TitleMenuWidgetClass.LoadSynchronous();
	}

	if (LoadedTitleMenuWidgetClass != nullptr)
	{
		TitleMenuWidget = CreateWidget<UUserWidget>(this, LoadedTitleMenuWidgetClass);
		if (TitleMenuWidget != nullptr)
		{
			TitleMenuWidget->AddToViewport();
			ApplyTitleMenuInputMode();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load title menu widget class."));
		SetInputMode(FInputModeGameOnly());
	}
}

void AUOUTitlePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
	{
		return;
	}

	// 버튼 클릭 외에도 키보드로 기본 메뉴 동작을 빠르게 테스트할 수 있게 둡니다.
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AUOUTitlePlayerController::StartGame);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AUOUTitlePlayerController::StartGame);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AUOUTitlePlayerController::ToggleSettingsMenu);
	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AUOUTitlePlayerController::QuitGame);
}

void AUOUTitlePlayerController::StartGame()
{
	// 맵 전환 중에 버튼이 다시 눌려도 OpenLevel을 한 번만 요청합니다.
	if (bIsOpeningLevel || TestLevel.IsNull())
	{
		return;
	}

	bIsOpeningLevel = true;
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TestLevel);
}

void AUOUTitlePlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AUOUTitlePlayerController::RestoreInputModeAfterSettingsMenu()
{
	// 설정창을 닫으면 다시 타이틀 메뉴가 포커스를 가져야 합니다.
	ApplyTitleMenuInputMode();
}

void AUOUTitlePlayerController::ApplyTitleMenuInputMode()
{
	// 타이틀은 항상 UI 조작 화면이라 마우스 커서를 켜고 이동/시점 입력을 막습니다.
	if (TitleMenuWidget != nullptr)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TitleMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}

	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
}
