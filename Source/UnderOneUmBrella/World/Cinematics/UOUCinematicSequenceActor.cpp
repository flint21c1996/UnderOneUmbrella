// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Cinematics/UOUCinematicSequenceActor.h"

#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlayer.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"

AUOUCinematicSequenceActor::AUOUCinematicSequenceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
}

void AUOUCinematicSequenceActor::BeginPlay()
{
	Super::BeginPlay();

	BindSequencePlayer();
}

void AUOUCinematicSequenceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSequencePlayer();

	if (bPlaybackActive)
	{
		FinishCinematic(false);
	}

	Super::EndPlay(EndPlayReason);
}

void AUOUCinematicSequenceActor::PlayCinematic()
{
	if (bPlayOnlyOnce && bHasPlayed)
	{
		return;
	}

	ULevelSequencePlayer* SequencePlayer = GetSequencePlayer();
	if (SequencePlayer == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UOU Cinematic Sequence Actor '%s' has no valid Level Sequence Player."), *GetNameSafe(this));
		return;
	}

	if (SequencePlayer->IsPlaying())
	{
		return;
	}

	BindSequencePlayer();

	APlayerController* PlayerController = ResolvePlayerController();
	CachedPlayerController = PlayerController;
	PreviousViewTarget = PlayerController != nullptr ? PlayerController->GetViewTarget() : nullptr;

	if (PlayerController != nullptr)
	{
		EnterCinematicState(PlayerController);
		RequestPlayerInputBlock(PlayerController);

		if (CinematicViewTarget != nullptr)
		{
			PlayerController->SetViewTargetWithBlend(CinematicViewTarget, FMath::Max(0.0f, BlendInTime));
		}
	}

	if (bRestartSequenceFromBeginning)
	{
		RewindSequencePlayer(SequencePlayer);
	}

	bPlaybackActive = true;
	bHasPlayed = true;

	SequencePlayer->Play();
	OnCinematicStarted.Broadcast();
}

void AUOUCinematicSequenceActor::StopCinematic()
{
	ULevelSequencePlayer* SequencePlayer = GetSequencePlayer();
	if (SequencePlayer != nullptr && (SequencePlayer->IsPlaying() || SequencePlayer->IsPaused()))
	{
		SequencePlayer->StopAtCurrentTime();
	}

	FinishCinematic(false);
}

void AUOUCinematicSequenceActor::PauseCinematic()
{
	ULevelSequencePlayer* SequencePlayer = GetSequencePlayer();
	if (SequencePlayer != nullptr && SequencePlayer->IsPlaying())
	{
		SequencePlayer->Pause();
	}
}

void AUOUCinematicSequenceActor::ResumeCinematic()
{
	ULevelSequencePlayer* SequencePlayer = GetSequencePlayer();
	if (SequencePlayer != nullptr && SequencePlayer->IsPaused())
	{
		SequencePlayer->Play();
	}
}

void AUOUCinematicSequenceActor::ResetPlaybackState()
{
	bHasPlayed = false;
}

bool AUOUCinematicSequenceActor::IsCinematicPlaying() const
{
	const ULevelSequencePlayer* SequencePlayer = GetSequencePlayer();
	return SequencePlayer != nullptr && (SequencePlayer->IsPlaying() || SequencePlayer->IsPaused());
}

void AUOUCinematicSequenceActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		PlayCinematic();
		break;
	case EOUUPuzzleResultAction::Deactivate:
		StopCinematic();
		break;
	case EOUUPuzzleResultAction::Pause:
		PauseCinematic();
		break;
	case EOUUPuzzleResultAction::Resume:
		ResumeCinematic();
		break;
	case EOUUPuzzleResultAction::Toggle:
		if (IsCinematicPlaying())
		{
			StopCinematic();
			return;
		}
		PlayCinematic();
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

void AUOUCinematicSequenceActor::HandleSequenceFinished()
{
	FinishCinematic(true);
}

ULevelSequencePlayer* AUOUCinematicSequenceActor::GetSequencePlayer() const
{
	return LevelSequenceActor != nullptr ? LevelSequenceActor->GetSequencePlayer() : nullptr;
}

void AUOUCinematicSequenceActor::BindSequencePlayer()
{
	if (ULevelSequencePlayer* SequencePlayer = GetSequencePlayer())
	{
		SequencePlayer->OnFinished.RemoveDynamic(this, &AUOUCinematicSequenceActor::HandleSequenceFinished);
		SequencePlayer->OnFinished.AddDynamic(this, &AUOUCinematicSequenceActor::HandleSequenceFinished);
	}
}

void AUOUCinematicSequenceActor::UnbindSequencePlayer()
{
	if (ULevelSequencePlayer* SequencePlayer = GetSequencePlayer())
	{
		SequencePlayer->OnFinished.RemoveDynamic(this, &AUOUCinematicSequenceActor::HandleSequenceFinished);
	}
}

APlayerController* AUOUCinematicSequenceActor::ResolvePlayerController() const
{
	UWorld* World = GetWorld();
	return World != nullptr ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
}

AActor* AUOUCinematicSequenceActor::ResolveReturnViewTarget(APlayerController* PlayerController) const
{
	if (ReturnViewTargetOverride != nullptr)
	{
		return ReturnViewTargetOverride;
	}

	if (PreviousViewTarget != nullptr)
	{
		return PreviousViewTarget;
	}

	return PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
}

void AUOUCinematicSequenceActor::EnterCinematicState(APlayerController* PlayerController)
{
	if (PlayerController == nullptr || !bUseCinematicMode)
	{
		return;
	}

	PlayerController->SetCinematicMode(
		true,
		bHidePlayerDuringPlayback,
		bHideHUDDuringPlayback,
		bDisableMovementDuringPlayback,
		bDisableTurningDuringPlayback);
}

void AUOUCinematicSequenceActor::ExitCinematicState(APlayerController* PlayerController)
{
	if (PlayerController == nullptr || !bUseCinematicMode)
	{
		return;
	}

	PlayerController->SetCinematicMode(
		false,
		bHidePlayerDuringPlayback,
		bHideHUDDuringPlayback,
		bDisableMovementDuringPlayback,
		bDisableTurningDuringPlayback);
}

void AUOUCinematicSequenceActor::RestoreViewTarget(APlayerController* PlayerController)
{
	if (PlayerController == nullptr || !bRestoreViewTargetOnFinish)
	{
		return;
	}

	if (AActor* ReturnViewTarget = ResolveReturnViewTarget(PlayerController))
	{
		PlayerController->SetViewTargetWithBlend(ReturnViewTarget, FMath::Max(0.0f, BlendOutTime));
	}
}

void AUOUCinematicSequenceActor::RequestPlayerInputBlock(APlayerController* PlayerController)
{
	if (!bBlockPlayerInputDuringPlayback || LockedInputExecutorComponent != nullptr || PlayerController == nullptr)
	{
		return;
	}

	UUOUPlayerInteractionExecutorComponent* InputExecutor =
		UUOUPlayerInteractionExecutorComponent::FindLocalPlayerExecutor(this);
	if (InputExecutor == nullptr)
	{
		return;
	}

	InputExecutor->RequestPlayerInputBlock(this, true);
	LockedInputExecutorComponent = InputExecutor;
}

void AUOUCinematicSequenceActor::ReleasePlayerInputBlock()
{
	if (LockedInputExecutorComponent == nullptr)
	{
		return;
	}

	LockedInputExecutorComponent->ReleasePlayerInputBlock(this);
	LockedInputExecutorComponent = nullptr;
}

void AUOUCinematicSequenceActor::FinishCinematic(bool bNaturalFinish)
{
	const bool bWasPlaybackActive = bPlaybackActive;
	if (!bWasPlaybackActive)
	{
		return;
	}

	APlayerController* PlayerController = CachedPlayerController != nullptr ? CachedPlayerController.Get() : ResolvePlayerController();
	if (PlayerController != nullptr)
	{
		RestoreViewTarget(PlayerController);
		ExitCinematicState(PlayerController);
	}
	ReleasePlayerInputBlock();

	bPlaybackActive = false;
	CachedPlayerController = nullptr;
	PreviousViewTarget = nullptr;

	if (bNaturalFinish)
	{
		OnCinematicFinished.Broadcast();
		return;
	}

	OnCinematicStopped.Broadcast();
}

void AUOUCinematicSequenceActor::RewindSequencePlayer(ULevelSequencePlayer* SequencePlayer) const
{
	if (SequencePlayer == nullptr)
	{
		return;
	}

	const FMovieSceneSequencePlaybackParams PlaybackParams(FFrameTime(0), EUpdatePositionMethod::Jump);
	SequencePlayer->SetPlaybackPosition(PlaybackParams);
}
