// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUCinematicSequenceActor.generated.h"

class ALevelSequenceActor;
class APlayerController;
class UUOUPlayerInteractionExecutorComponent;
class ULevelSequencePlayer;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOUCinematicSequenceEvent);

// 퍼즐 결과 또는 블루프린트 호출로 레벨 시퀀스를 재생하고 플레이 카메라로 복귀시키는 액터입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Cinematic Sequence Actor"))
class UNDERONEUMBRELLA_API AUOUCinematicSequenceActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUCinematicSequenceActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Cinematic|Sequence", meta = (ToolTip = "재생할 Level Sequence Actor입니다. 실제 카메라 키프레임은 이 액터가 참조하는 Level Sequence Asset에 들어 있습니다."))
	TObjectPtr<ALevelSequenceActor> LevelSequenceActor = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Cinematic|Camera", meta = (AllowedClasses = "/Script/CinematicCamera.CineCameraActor", ToolTip = "시네마틱 시작 시 부드럽게 전환할 카메라 액터입니다. 비워두면 시퀀서의 Camera Cuts 트랙만 사용합니다."))
	TObjectPtr<AActor> CinematicViewTarget = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Cinematic|Camera", meta = (ToolTip = "시네마틱 종료 후 복귀할 View Target입니다. 비워두면 시네마틱 시작 전 View Target으로 돌아갑니다."))
	TObjectPtr<AActor> ReturnViewTargetOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Camera", meta = (ClampMin = "0.0", ToolTip = "플레이 카메라에서 시네마틱 카메라로 전환되는 시간입니다."))
	float BlendInTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Camera", meta = (ClampMin = "0.0", ToolTip = "시네마틱 종료 후 플레이 카메라로 복귀하는 시간입니다."))
	float BlendOutTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Playback", meta = (ToolTip = "켜져 있으면 같은 플레이 세션에서 한 번만 재생합니다."))
	bool bPlayOnlyOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Playback", meta = (ToolTip = "재생 요청마다 시퀀스를 처음 프레임으로 되돌린 뒤 시작합니다."))
	bool bRestartSequenceFromBeginning = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Playback", meta = (ToolTip = "시퀀스가 끝났을 때 카메라를 복귀시킵니다."))
	bool bRestoreViewTargetOnFinish = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Playback", meta = (ToolTip = "시네마틱 재생 중 PlayerController의 Cinematic Mode를 사용합니다."))
	bool bUseCinematicMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Playback", meta = (EditCondition = "bUseCinematicMode", ToolTip = "시네마틱 재생 중 플레이어 이동 입력을 막습니다."))
	bool bDisableMovementDuringPlayback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Playback", meta = (EditCondition = "bUseCinematicMode", ToolTip = "시네마틱 재생 중 시점 전환 입력을 막습니다."))
	bool bDisableTurningDuringPlayback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Playback", meta = (EditCondition = "bUseCinematicMode", ToolTip = "시네마틱 재생 중 HUD를 숨깁니다."))
	bool bHideHUDDuringPlayback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Playback", meta = (EditCondition = "bUseCinematicMode", ToolTip = "시네마틱 재생 중 플레이어 Pawn을 숨깁니다."))
	bool bHidePlayerDuringPlayback = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic|Playback", meta = (ToolTip = "시네마틱 재생 중 UOU 캐릭터 입력 게이트를 통해 플레이어 조작을 막습니다."))
	bool bBlockPlayerInputDuringPlayback = true;

	UPROPERTY(BlueprintAssignable, Category = "Cinematic|Events")
	FUOUCinematicSequenceEvent OnCinematicStarted;

	UPROPERTY(BlueprintAssignable, Category = "Cinematic|Events")
	FUOUCinematicSequenceEvent OnCinematicFinished;

	UPROPERTY(BlueprintAssignable, Category = "Cinematic|Events")
	FUOUCinematicSequenceEvent OnCinematicStopped;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cinematic|Runtime")
	bool bHasPlayed = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cinematic|Runtime")
	bool bPlaybackActive = false;

	UFUNCTION(BlueprintCallable, Category = "Cinematic|Actions")
	void PlayCinematic();

	UFUNCTION(BlueprintCallable, Category = "Cinematic|Actions")
	void StopCinematic();

	UFUNCTION(BlueprintCallable, Category = "Cinematic|Actions")
	void PauseCinematic();

	UFUNCTION(BlueprintCallable, Category = "Cinematic|Actions")
	void ResumeCinematic();

	UFUNCTION(BlueprintCallable, Category = "Cinematic|Actions")
	void ResetPlaybackState();

	UFUNCTION(BlueprintPure, Category = "Cinematic|Runtime")
	bool IsCinematicPlaying() const;

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<APlayerController> CachedPlayerController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviousViewTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUPlayerInteractionExecutorComponent> LockedInputExecutorComponent = nullptr;

	UFUNCTION()
	void HandleSequenceFinished();

	ULevelSequencePlayer* GetSequencePlayer() const;
	void BindSequencePlayer();
	void UnbindSequencePlayer();
	APlayerController* ResolvePlayerController() const;
	AActor* ResolveReturnViewTarget(APlayerController* PlayerController) const;
	void EnterCinematicState(APlayerController* PlayerController);
	void ExitCinematicState(APlayerController* PlayerController);
	void RestoreViewTarget(APlayerController* PlayerController);
	void RequestPlayerInputBlock(APlayerController* PlayerController);
	void ReleasePlayerInputBlock();
	void FinishCinematic(bool bNaturalFinish);
	void RewindSequencePlayer(ULevelSequencePlayer* SequencePlayer) const;
};
