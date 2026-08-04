// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOUMapSelectPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Game/UOULevelTransitionSubsystem.h"
#include "InputCoreTypes.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"

namespace
{
constexpr TCHAR DefaultStageSelectWidgetClassPath[] = TEXT("/Game/UOU/UI/WBP_MapSelect.WBP_MapSelect_C");

FUOUStageDefinition MakeStage(const FName StageId, const TCHAR* DisplayName, const TCHAR* LevelPath)
{
	FUOUStageDefinition Stage;
	Stage.StageId = StageId;
	Stage.DisplayName = FText::FromString(DisplayName);
	Stage.Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(LevelPath));
	return Stage;
}
}

AUOUMapSelectPlayerController::AUOUMapSelectPlayerController()
{
	SetCanReturnToTitle(true);
	SetCanRestartCurrentStage(false);

	Stages.Reserve(6);
	Stages.Add(MakeStage(TEXT("Tutorial"), TEXT("Tutorial"), TEXT("/Game/UOU/Maps/L_Tutorial.L_Tutorial")));
	Stages.Add(MakeStage(TEXT("MS_1"), TEXT("Stage 1"), TEXT("/Game/UOU/Maps/L_MS_1.L_MS_1")));
	Stages.Add(MakeStage(TEXT("MS_2"), TEXT("Stage 2"), TEXT("/Game/UOU/Maps/L_MS_2.L_MS_2")));
	Stages.Add(MakeStage(TEXT("MS_3"), TEXT("Stage 3"), TEXT("/Game/UOU/Maps/L_MS_3.L_MS_3")));
	Stages.Add(MakeStage(TEXT("MS_4"), TEXT("Stage 4"), TEXT("/Game/UOU/Maps/L_MS_4.L_MS_4")));
	Stages.Add(MakeStage(TEXT("MS_5"), TEXT("Stage 5"), TEXT("/Game/UOU/Maps/L_MS_5.L_MS_5")));
}

void AUOUMapSelectPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (StageSelectWidgetClass.IsNull())
	{
		StageSelectWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(DefaultStageSelectWidgetClassPath));
	}

	if (TSubclassOf<UUserWidget> LoadedWidgetClass = StageSelectWidgetClass.LoadSynchronous())
	{
		StageSelectWidget = CreateWidget<UUserWidget>(this, LoadedWidgetClass);
		if (StageSelectWidget != nullptr)
		{
			StageSelectWidget->AddToViewport();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Map select widget was not found. Create %s or configure StageSelectWidgetClass."), DefaultStageSelectWidgetClassPath);
	}

	ApplyMapSelectInputMode();
}

void AUOUMapSelectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent != nullptr)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AUOUMapSelectPlayerController::ToggleSettingsMenu);
	}
}

bool AUOUMapSelectPlayerController::EnterStageByIndex(const int32 StageIndex)
{
	if (!Stages.IsValidIndex(StageIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid stage index: %d"), StageIndex);
		return false;
	}

	return EnterStage(Stages[StageIndex]);
}

bool AUOUMapSelectPlayerController::EnterStage(const FUOUStageDefinition& Stage)
{
	if (bIsOpeningStage || !Stage.bUnlocked || Stage.Level.IsNull())
	{
		return false;
	}

	const FString StagePackageName = Stage.Level.ToSoftObjectPath().GetLongPackageName();
	if (!FPackageName::DoesPackageExist(StagePackageName))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot enter stage because the level does not exist: %s"), *StagePackageName);
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UUOULevelTransitionSubsystem* TransitionSubsystem = GameInstance != nullptr
		? GameInstance->GetSubsystem<UUOULevelTransitionSubsystem>()
		: nullptr;
	if (TransitionSubsystem == nullptr || TransitionSubsystem->IsTransitioning())
	{
		return false;
	}

	bIsOpeningStage = TransitionSubsystem->RequestLevelTransition(Stage.Level, FUOULevelTransitionSettings());
	return bIsOpeningStage;
}

void AUOUMapSelectPlayerController::RestoreInputModeAfterSettingsMenu()
{
	ApplyMapSelectInputMode();
}

void AUOUMapSelectPlayerController::ApplyMapSelectInputMode()
{
	FInputModeGameAndUI InputMode;
	if (StageSelectWidget != nullptr)
	{
		InputMode.SetWidgetToFocus(StageSelectWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
}
