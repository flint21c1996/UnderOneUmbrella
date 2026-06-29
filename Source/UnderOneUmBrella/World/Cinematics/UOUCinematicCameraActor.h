// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraTypes.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultCompletionState.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUCinematicCameraActor.generated.h"

class APlayerController;
class AUOUCinematicCameraTargetActor;
class UCameraComponent;
class UCurveFloat;
class UUOUPlayerInteractionExecutorComponent;
class USceneComponent;
class USplineComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUCinematicCameraMoveSignature, AUOUCinematicCameraActor*, CameraActor);

UENUM(BlueprintType)
enum class EUOUCinematicCameraEasingMode : uint8
{
	Linear,
	EaseIn,
	EaseOut,
	EaseInOut
};

UENUM(BlueprintType)
enum class EUOUCinematicCameraPathMode : uint8
{
	Linear,
	CubicBezier
};

struct FUOUCinematicCameraResolvedSettings
{
	ECameraProjectionMode::Type ProjectionMode = ECameraProjectionMode::Orthographic;
	float OrthographicWidth = 1800.0f;
	float FieldOfView = 60.0f;
	float AspectRatio = 16.0f / 9.0f;
	bool bConstrainAspectRatio = false;
};

// 배치한 카메라를 여러 목표 지점으로 이동시키며, 투영/오쏘 폭/FOV를 디테일 창에서 조정하는 연출용 카메라입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Cinematic Camera Actor"))
class UNDERONEUMBRELLA_API AUOUCinematicCameraActor : public AActor, public IUOUPuzzleResultReceiver, public IUOUPuzzleResultCompletionState
{
	GENERATED_BODY()

public:
	AUOUCinematicCameraActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera")
	TObjectPtr<UCameraComponent> CameraComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Preview")
	TObjectPtr<USplineComponent> MovePreviewPath = nullptr;

	UPROPERTY()
	TEnumAsByte<ECameraProjectionMode::Type> ProjectionMode = ECameraProjectionMode::Orthographic;

	UPROPERTY()
	bool bUseOrthographicProjection = true;

	UPROPERTY()
	float OrthographicWidth = 1800.0f;

	UPROPERTY()
	float FieldOfView = 60.0f;

	UPROPERTY()
	float AspectRatio = 16.0f / 9.0f;

	UPROPERTY()
	bool bConstrainAspectRatio = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (DisplayName = "Interpolate Camera Settings", ToolTip = "켜면 스텝별 직교 폭, FOV, 화면비를 이동 알파에 맞춰 보간합니다. 투영 방식 변경은 구간 시작 시 적용됩니다."))
	bool bInterpolateCameraSettings = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Start", meta = (DisplayName = "Saved Start Transform", ToolTip = "카메라가 리셋될 시작 위치입니다."))
	FTransform SavedStartTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Start", meta = (DisplayName = "Use Saved Start Transform", ToolTip = "켜면 저장된 시작 위치를 사용합니다. 꺼져 있으면 현재 배치 위치를 시작 위치로 사용합니다."))
	bool bUseSavedStartTransform = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Start", meta = (DisplayName = "Keep Actor At Saved Start In Editor", ToolTip = "에디터에서 맵을 다시 열 때 카메라를 저장된 시작 위치로 되돌립니다."))
	bool bKeepActorAtSavedStartInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Movement", meta = (ClampMin = "0.0", DisplayName = "Move Duration", ToolTip = "기본 카메라 이동 시간입니다. 스텝 마커에서 별도로 덮어쓸 수 있습니다."))
	float MoveDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Movement", meta = (ToolTip = "이동 알파 값을 보정하는 선택 곡선입니다."))
	TObjectPtr<UCurveFloat> MoveCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Movement", meta = (ToolTip = "MoveCurve가 없을 때 사용할 기본 가감속 방식입니다."))
	EUOUCinematicCameraEasingMode EasingMode = EUOUCinematicCameraEasingMode::EaseInOut;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Movement", meta = (ClampMin = "0.1", ToolTip = "EaseIn, EaseOut, EaseInOut 계산 강도입니다."))
	float EaseExponent = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Movement", meta = (ToolTip = "목표 지점까지 직선으로 갈지, 베지어 곡선을 따라갈지 정합니다."))
	EUOUCinematicCameraPathMode PathMode = EUOUCinematicCameraPathMode::Linear;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Movement", meta = (ToolTip = "CubicBezier 경로에서 시작점 쪽 제어점을 시작점 기준 로컬 오프셋으로 정합니다."))
	FVector BezierStartControlOffset = FVector(400.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Movement", meta = (ToolTip = "CubicBezier 경로에서 목표점 쪽 제어점을 목표점 기준 로컬 오프셋으로 정합니다."))
	FVector BezierEndControlOffset = FVector(-400.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Move Steps", meta = (DisplayName = "Use Camera Steps", ToolTip = "여러 카메라 목표 마커를 순서대로 사용합니다."))
	bool bUseCameraSteps = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Move Steps", meta = (DisplayName = "Camera Step Markers", ToolTip = "카메라가 순서대로 이동할 목표 마커 목록입니다."))
	TArray<TObjectPtr<AUOUCinematicCameraTargetActor>> CameraStepMarkers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Move Steps", meta = (ClampMin = "0", DisplayName = "Initial Step Index", ToolTip = "처음 Activate할 때 사용할 스텝 인덱스입니다."))
	int32 InitialCameraStepIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Move Steps", meta = (DisplayName = "Loop Camera Steps", ToolTip = "마지막 스텝 이후 다시 처음 스텝으로 돌아갈지 정합니다."))
	bool bLoopCameraSteps = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Move Steps", meta = (DisplayName = "Loop Through Start", ToolTip = "반복 이동 시 마지막 스텝 이후 시작 위치를 한 번 거쳐 처음 스텝으로 돌아갑니다."))
	bool bLoopCameraStepsThroughStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Move Steps", meta = (ClampMin = "1", UIMin = "1", DisplayName = "Move Step Count Per Activate", ToolTip = "Activate 한 번에 몇 개의 카메라 스텝을 진행할지 정합니다."))
	int32 MoveStepCountPerActivate = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (DisplayName = "Set View Target On Move", ToolTip = "게임 중 이동을 시작할 때 플레이어 시점을 이 카메라로 전환합니다."))
	bool bSetViewTargetOnMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (DisplayName = "Block Player Input During Playback", ToolTip = "카메라 연출 중 플레이어 입력을 프로젝트 입력 차단 시스템으로 막습니다."))
	bool bBlockPlayerInputDuringPlayback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (ClampMin = "0.0", DisplayName = "Blend In Time", ToolTip = "플레이어 카메라에서 연출 카메라로 전환하는 시간입니다."))
	float BlendInTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (DisplayName = "Restore View Target On Finish", ToolTip = "이동이 끝나거나 리셋될 때 이전 플레이어 시점으로 복구합니다."))
	bool bRestoreViewTargetOnFinish = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (ClampMin = "0.0", DisplayName = "Blend Out Time", ToolTip = "연출 카메라에서 이전 플레이어 시점으로 복구하는 시간입니다."))
	float BlendOutTime = 0.5f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (ToolTip = "연출 종료 시 돌아갈 View Target입니다. 비워두면 연출 시작 전 View Target으로 돌아갑니다."))
	TObjectPtr<AActor> ReturnViewTargetOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (DisplayName = "Use Cinematic Mode", ToolTip = "연출 카메라 이동 중 PlayerController의 Cinematic Mode를 사용합니다."))
	bool bUseCinematicMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (EditCondition = "bUseCinematicMode", DisplayName = "Disable Movement During Playback", ToolTip = "연출 중 플레이어 이동 입력을 막습니다."))
	bool bDisableMovementDuringPlayback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (EditCondition = "bUseCinematicMode", DisplayName = "Disable Turning During Playback", ToolTip = "연출 중 시점 전환 입력을 막습니다."))
	bool bDisableTurningDuringPlayback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (EditCondition = "bUseCinematicMode", DisplayName = "Hide HUD During Playback", ToolTip = "연출 중 HUD를 숨깁니다."))
	bool bHideHUDDuringPlayback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Playback", meta = (EditCondition = "bUseCinematicMode", DisplayName = "Hide Player During Playback", ToolTip = "연출 중 플레이어 Pawn을 숨깁니다."))
	bool bHidePlayerDuringPlayback = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Preview", meta = (DisplayName = "Show Target Camera Previews", ToolTip = "스텝 마커의 카메라 프리뷰 컴포넌트를 표시합니다."))
	bool bShowTargetCameraPreviews = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Preview", meta = (DisplayName = "Show Move Path Preview", ToolTip = "에디터에서 카메라 이동 경로를 스플라인으로 표시합니다."))
	bool bShowMovePreviewPath = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Runtime")
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Runtime")
	bool bIsPaused = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Runtime")
	bool bIsAtTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Runtime")
	int32 LastArrivedCameraStepIndex = INDEX_NONE;

	UPROPERTY(BlueprintAssignable, Category = "Cinematic Camera|Events")
	FUOUCinematicCameraMoveSignature OnCameraMoveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Cinematic Camera|Events")
	FUOUCinematicCameraMoveSignature OnCameraMoveFinished;

	UPROPERTY(BlueprintAssignable, Category = "Cinematic Camera|Events")
	FUOUCinematicCameraMoveSignature OnCameraMoveStopped;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cinematic Camera|Editor")
	void CaptureCurrentAsStart();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cinematic Camera|Editor")
	void ApplyDefaultCameraSettings();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cinematic Camera|Actions")
	void PlayCameraMove();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cinematic Camera|Actions", meta = (DisplayName = "Move To Next Camera Step"))
	void MoveToNextCameraStep();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cinematic Camera|Actions")
	void ResetCameraMove();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cinematic Camera|Actions")
	void SnapToTarget();

	UFUNCTION(BlueprintCallable, Category = "Cinematic Camera|Actions")
	void PauseCameraMove();

	UFUNCTION(BlueprintCallable, Category = "Cinematic Camera|Actions")
	void ResumeCameraMove();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cinematic Camera|Move Steps", meta = (DisplayName = "Add Camera Step"))
	void AddCameraStepMarker();

	UFUNCTION(BlueprintPure, Category = "Cinematic Camera|Runtime")
	bool IsMoving() const { return bIsMoving; }

	UFUNCTION(BlueprintPure, Category = "Cinematic Camera|Runtime")
	bool IsAtTarget() const { return bIsAtTarget; }

	UFUNCTION(BlueprintPure, Category = "Cinematic Camera|Runtime")
	int32 GetLastArrivedCameraStepIndex() const { return LastArrivedCameraStepIndex; }

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual bool IsPuzzleResultCompleted_Implementation(EOUUPuzzleResultAction Action) const override;
	virtual FOnUOUPuzzleResultCompletionStateChangedNativeSignature* GetPuzzleResultCompletionStateChangedEvent() override;

protected:
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	void EnsureStartTransform();
	void RefreshTargetTransforms();
	void ResetRuntimeStepIndex();
	void FinishMoveToTarget();
	void StopActiveMove(bool bBroadcastStopped);
	bool ShouldUseCameraSteps() const;
	AUOUCinematicCameraTargetActor* GetCameraStepMarkerAt(int32 StepIndex) const;
	AUOUCinematicCameraTargetActor* GetCurrentCameraStepMarker() const;
	bool ResolveNextCameraTargetTransform(FTransform& OutTargetTransform, int32& OutTargetIndex) const;
	void AdvanceCameraStepIndex();
	void RequestCameraMoveSteps(int32 StepCount);
	bool TryStartQueuedCameraMove();
	bool BeginMoveToTransform(
		const FTransform& InTargetTransform,
		AUOUCinematicCameraTargetActor* TargetMarker,
		int32 TargetStepIndex);

	FTransform BuildActiveCameraTransformAtAlpha(float Alpha) const;
	FTransform BuildTransformBetween(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha) const;
	FVector EvaluatePathLocation(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha) const;
	float ResolveMoveAlpha(float RawAlpha) const;
	float ResolveMoveDuration(const AUOUCinematicCameraTargetActor* TargetMarker) const;

	FUOUCinematicCameraResolvedSettings BuildDefaultCameraSettings() const;
	FUOUCinematicCameraResolvedSettings ReadCurrentCameraSettings() const;
	FUOUCinematicCameraResolvedSettings ResolveCameraSettingsFromTarget(const AUOUCinematicCameraTargetActor* TargetMarker) const;
	void ApplyCameraSettings(const FUOUCinematicCameraResolvedSettings& CameraSettings) const;
	void ApplyCameraSettingsAtAlpha(float Alpha);
	void SyncStepMarkerPreviews();
	void UpdateEditorPreviewVisuals();

	APlayerController* ResolvePlayerController() const;
	AActor* ResolveReturnViewTarget(APlayerController* PlayerController) const;
	void BeginCameraViewTarget();
	void FinishCameraViewTarget();
	void RequestPlayerInputBlockForPlayback();
	void ReleasePlayerInputBlockForPlayback();
	void EnterCinematicState(APlayerController* PlayerController);
	void ExitCinematicState(APlayerController* PlayerController);

	void SetPuzzleResultCompletionState(EOUUPuzzleResultAction Action, bool bNewCompleted);

private:
	UPROPERTY(Transient)
	float MoveElapsedTime = 0.0f;

	UPROPERTY(Transient)
	float ActiveMoveDuration = 0.0f;

	UPROPERTY(Transient)
	FTransform StartTransform = FTransform::Identity;

	UPROPERTY(Transient)
	FTransform TargetTransform = FTransform::Identity;

	UPROPERTY(Transient)
	FTransform MoveStartTransform = FTransform::Identity;

	UPROPERTY(Transient)
	FTransform MoveTargetTransform = FTransform::Identity;

	FUOUCinematicCameraResolvedSettings MoveStartCameraSettings;
	FUOUCinematicCameraResolvedSettings MoveTargetCameraSettings;

	UPROPERTY(Transient)
	TObjectPtr<AUOUCinematicCameraTargetActor> ActiveMoveTargetMarker = nullptr;

	UPROPERTY(Transient)
	int32 ActiveMoveTargetIndex = INDEX_NONE;

	UPROPERTY(Transient)
	int32 RuntimeCameraStepIndex = 0;

	UPROPERTY(Transient)
	int32 PendingCameraMoveCount = 0;

	UPROPERTY(Transient)
	bool bNextMoveReturnsToStart = false;

	UPROPERTY(Transient)
	bool bHasCapturedStartTransform = false;

	bool bActivateResultCompleted = false;
	bool bDeactivateResultCompleted = false;
	FOnUOUPuzzleResultCompletionStateChangedNativeSignature OnPuzzleResultCompletionStateChanged;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> CachedPlayerController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviousViewTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUPlayerInteractionExecutorComponent> LockedInputExecutorComponent = nullptr;

	UPROPERTY(Transient)
	bool bViewTargetActive = false;

	UPROPERTY(Transient)
	bool bIsApplyingEditorTransform = false;
};
