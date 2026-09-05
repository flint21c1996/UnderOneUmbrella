// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOUMapSelectPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Engine/GameInstance.h"
#include "Game/UOULevelTransitionSubsystem.h"
#include "Game/UOUPlayerProgressSubsystem.h"
#include "InputCoreTypes.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"
#include "World/Stage/UOUStageSelectNodeActor.h"

AUOUMapSelectPlayerController::AUOUMapSelectPlayerController()
{
	SetCanReturnToTitle(true);
	SetCanRestartCurrentStage(true);
}

void AUOUMapSelectPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyMapSelectInputMode();

	if (!IsLocalController())
	{
		return;
	}

	TSubclassOf<UUserWidget> HUDClass = MapSelectHUDWidgetClass.LoadSynchronous();
	if (HUDClass == nullptr)
	{
		HUDClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UOU/UI/WBP_InGameHUD.WBP_InGameHUD_C"));
	}
	if (HUDClass != nullptr)
	{
		MapSelectHUDWidget = CreateWidget<UUserWidget>(this, HUDClass);
		if (MapSelectHUDWidget != nullptr)
		{
			MapSelectHUDWidget->AddToViewport();

			// 공통 HUD의 다음/이전 레벨 영역은 스테이지 선택 화면에서만 숨깁니다.
			// WBP_InGameHUD의 버튼과 문구를 함께 감싸는 SizeBox 이름입니다.
			static const FName LevelNavigationWidgetNames[] =
			{
				TEXT("SB_Settings_1"),
				TEXT("SB_Settings_2")
			};
			for (const FName WidgetName : LevelNavigationWidgetNames)
			{
				if (UWidget* NavigationWidget = MapSelectHUDWidget->GetWidgetFromName(WidgetName))
				{
					NavigationWidget->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
}

void AUOUMapSelectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent != nullptr)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AUOUMapSelectPlayerController::ToggleSettingsMenu);

		FInputKeyBinding& ConfirmBinding = InputComponent->BindKey(
			EKeys::LeftMouseButton,
			IE_Pressed,
			this,
			&AUOUMapSelectPlayerController::HandleStageConfirmInput);
		// Keep the project's existing umbrella click input available when no stage transition occurs.
		ConfirmBinding.bConsumeInput = false;
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
	if (IsSettingsMenuOpen() || bIsOpeningStage || !Stage.bUnlocked || Stage.Level.IsNull())
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
	if (bIsOpeningStage)
	{
		if (UUOUPlayerProgressSubsystem* ProgressSubsystem =
			GameInstance->GetSubsystem<UUOUPlayerProgressSubsystem>())
		{
			ProgressSubsystem->BeginStageAttempt(Stage.StageId, Stage.Level, Stage.RewardIds);
		}
	}
	return bIsOpeningStage;
}

void AUOUMapSelectPlayerController::RegisterStageSelectNode(AUOUStageSelectNodeActor* StageNode)
{
	if (!IsValid(StageNode))
	{
		return;
	}

	OverlappingStageNodes.Remove(StageNode);
	OverlappingStageNodes.Add(StageNode);
}

void AUOUMapSelectPlayerController::UnregisterStageSelectNode(AUOUStageSelectNodeActor* StageNode)
{
	OverlappingStageNodes.Remove(StageNode);
}

AUOUStageSelectNodeActor* AUOUMapSelectPlayerController::GetFocusedStageSelectNode() const
{
	for (int32 Index = OverlappingStageNodes.Num() - 1; Index >= 0; --Index)
	{
		if (IsValid(OverlappingStageNodes[Index]))
		{
			return OverlappingStageNodes[Index];
		}
	}

	return nullptr;
}

bool AUOUMapSelectPlayerController::ConfirmFocusedStage()
{
	if (IsSettingsMenuOpen() || bIsOpeningStage)
	{
		return false;
	}

	AUOUStageSelectNodeActor* FocusedStageNode = GetFocusedStageSelectNode();
	return FocusedStageNode != nullptr && FocusedStageNode->ActivateStage();
}

void AUOUMapSelectPlayerController::HandleStageConfirmInput()
{
	ConfirmFocusedStage();
}

void AUOUMapSelectPlayerController::RestoreInputModeAfterSettingsMenu()
{
	ApplyMapSelectInputMode();
}

void AUOUMapSelectPlayerController::ApplyMapSelectInputMode()
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
