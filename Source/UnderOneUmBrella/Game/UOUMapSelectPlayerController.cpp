// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/UOUMapSelectPlayerController.h"

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
	SetCanRestartCurrentStage(false);
}

void AUOUMapSelectPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyMapSelectInputMode();
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
	if (bIsOpeningStage)
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
