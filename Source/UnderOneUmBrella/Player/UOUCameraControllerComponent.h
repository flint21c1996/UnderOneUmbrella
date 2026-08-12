// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUCameraControllerComponent.generated.h"

class UCameraComponent;
class UMaterialInterface;
class UMeshComponent;
class USpringArmComponent;

// 가려진 메시가 원래 어떤 머티리얼을 쓰고 있었는지 복구하기 위한 저장 구조다.
USTRUCT()
struct FOccludedMeshState
{
	GENERATED_BODY()

	// 투명 처리 전 원래 머티리얼 배열을 그대로 기억해둔다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	// 투명 처리 전 표시 상태를 기억해둔다.
	UPROPERTY(Transient)
	bool bWasVisible = true;
};

// 8방향 스냅 카메라와 줌, 가림 처리를 한 번에 관리하는 카메라 전용 컴포넌트다.
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUCameraControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 카메라 기본 각도와 거리 제한값을 초기화한다.
	UUOUCameraControllerComponent();

	// 시작 시점에 스프링암과 카메라 참조를 잡는다.
	virtual void BeginPlay() override;

	// 종료 시 임시로 바꾼 가림 머티리얼을 원래대로 복구한다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 목표 회전과 거리로 부드럽게 보간하고 가림 처리를 갱신한다.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 카메라를 한 단계 왼쪽으로 돌린다.
	void RotateCameraLeft();

	// 카메라를 한 단계 오른쪽으로 돌린다.
	void RotateCameraRight();

	// 카메라를 더 가까이 당긴다.
	void ZoomCameraIn();

	// 카메라를 더 멀리 보낸다.
	void ZoomCameraOut();

	// 플레이어 이동 계산에 쓸 현재 카메라 yaw 값을 돌려준다.
	float GetMovementYaw() const;

	// 통합 플레이어 디버그 HUD에서 목표 yaw 값을 확인합니다.
	float GetTargetCameraYaw() const { return TargetCameraYaw; }

	// 스냅 카메라가 아직 목표 yaw까지 보간 중인지 확인합니다.
	bool IsSnapCameraRotationInProgress() const;

	// 통합 플레이어 디버그 HUD에서 현재 카메라 거리를 확인합니다.
	float GetCurrentCameraDistance() const;

	// 통합 플레이어 디버그 HUD에서 목표 카메라 거리를 확인합니다.
	float GetTargetCameraDistance() const { return TargetCameraDistance; }

	// 통합 플레이어 디버그 HUD에서 현재 가림 처리 중인 메시 수를 확인합니다.
	int32 GetOccludedMeshCount() const { return OccludedMeshStates.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Camera|Occlusion")
	void SetCameraOcclusionEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure, Category = "Camera|Occlusion")
	bool IsCameraOcclusionEnabled() const { return bCameraOcclusionEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Camera|Dialogue")
	void StartDialogueFocus(AActor* SpeakerActor);

	UFUNCTION(BlueprintCallable, Category = "Camera|Dialogue")
	void EndDialogueFocus();

	UFUNCTION(BlueprintPure, Category = "Camera|Dialogue")
	bool IsDialogueFocusActive() const { return bDialogueFocusActive; }

	// 임시 줌과 함께 평상시 주시점으로부터 포커스 위치를 이동해 대상이 화면에서 부각되도록 합니다.
	UFUNCTION(BlueprintCallable, Category = "Camera|Temporary Zoom")
	void RequestTemporaryFocusZoom(
		UObject* RequestSource,
		float TargetDistance,
		float TargetOrthoWidth,
		FVector FocusOffset,
		bool bFaceOwnerFromFront);

	// 자신이 등록한 임시 줌만 해제하여 다른 연출의 요청을 잘못 종료하지 않도록 합니다.
	UFUNCTION(BlueprintCallable, Category = "Camera|Temporary Zoom")
	void ReleaseTemporaryZoom(UObject* RequestSource);

	UFUNCTION(BlueprintPure, Category = "Camera|Temporary Zoom")
	bool IsTemporaryZoomRequestedBy(const UObject* RequestSource) const;

	// 특정 월드 구간에서 사용할 주시점 보정값을 설정합니다. 실제 이동은 평상시 카메라 보간 흐름을 그대로 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Camera|Area Framing")
	void SetAreaCameraOffset(FVector NewOffset);

	// 월드 구간용 주시점 보정을 해제해 평상시 구도로 돌아갑니다.
	UFUNCTION(BlueprintCallable, Category = "Camera|Area Framing")
	void ClearAreaCameraOffset();

	UFUNCTION(BlueprintPure, Category = "Camera|Area Framing")
	FVector GetAreaCameraOffset() const { return AreaCameraOffset; }

protected:
	// 참조를 수동으로 넣지 않아도 기본 카메라 구성을 자동으로 찾게 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|References")
	bool bAutoFindCameraComponents = true;

	// 실제 길이와 회전을 가지는 카메라 붐이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|References")
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;

	// 화면을 그리는 실제 카메라 컴포넌트다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|References")
	TObjectPtr<UCameraComponent> FollowCamera = nullptr;

	// 임시 테스트용 투영 모드다. 켜면 원근감 없이 오소그래픽으로 화면을 그린다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Projection")
	bool bUseOrthographicProjection = true;

	// 오소그래픽 카메라가 한 화면에 보여주는 월드 폭이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Projection", meta = (ClampMin = "1.0"))
	float OrthographicWidth = 1800.0f;

	// 카메라가 내려다보는 기본 피치 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Snap")
	float CameraPitchAngle = -45.0f;

	// 한 번에 회전할 카메라 스텝 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Snap", meta = (ClampMin = "1.0"))
	float CameraRotateStep = 45.0f;

	// 현재 카메라가 목표 각도로 따라가는 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Snap", meta = (ClampMin = "0.0"))
	float CameraRotationInterpSpeed = 12.0f;

	// 줌 입력 한 번에 더하거나 뺄 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float CameraZoomStep = 35.0f;

	// 카메라가 더 가까워질 수 있는 최소 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float MinCameraDistance = 250.0f;

	// 카메라가 더 멀어질 수 있는 최대 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float MaxCameraDistance = 600.0f;

	// 현재 카메라 거리가 목표 거리로 따라가는 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float CameraZoomInterpSpeed = 10.0f;

	// 가림 메시를 탐지할 때 쓰는 구체 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion", meta = (ClampMin = "0.0"))
	float OcclusionProbeRadius = 36.0f;

	// 화면 좌우 가장자리에 가까운 전경 벽까지 잡기 위해 중심 sweep에서 좌우로 벌리는 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion", meta = (ClampMin = "0.0"))
	float OcclusionProbeLateralExtent = 650.0f;

	// 화면 위아래 전경 가림까지 잡기 위해 중심 sweep에서 상하로 벌리는 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion", meta = (ClampMin = "0.0"))
	float OcclusionProbeVerticalExtent = 220.0f;

	// 목표 높이보다 충분히 낮게 끝나는 낮은 발판/바닥 메시를 가림 후보에서 제외하는 여유값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion", meta = (ClampMin = "0.0"))
	float OcclusionLowObjectIgnoreBelowTarget = 40.0f;

	// 카메라와 플레이어 사이 가림 처리를 사용할지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion")
	bool bCameraOcclusionEnabled = false;

	// 한 번에 투명 처리할 최대 메시 수입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion", meta = (ClampMin = "1"))
	int32 MaxOccludedMeshCount = 16;

	// 플레이어 중심보다 약간 위를 가림 탐지 기준으로 보정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion")
	FVector OcclusionTargetOffset = FVector(0.0f, 0.0f, 60.0f);

	// 가림 메시를 임시로 바꿔 끼울 반투명 머티리얼이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion")
	TObjectPtr<UMaterialInterface> OccluderFadeMaterial = nullptr;

	// 플레이어와 NPC 사이를 비추는 대화용 카메라 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue", meta = (ClampMin = "0.0"))
	float DialogueCameraDistance = 360.0f;

	// 대화 연출 중 카메라 거리를 바꿀지 정합니다. 끄면 현재 줌 거리를 유지합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue")
	bool bAdjustDistanceDuringDialogue = false;

	// 직교 카메라 대화 연출 중 화면 폭을 따로 줄일지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue")
	bool bAdjustOrthoWidthDuringDialogue = false;

	// 직교 카메라 대화 연출 중 사용할 화면 폭입니다. 값이 작을수록 더 가까이 보입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue", meta = (ClampMin = "1.0"))
	float DialogueOrthographicWidth = 1300.0f;

	// 대화 카메라가 바라보는 기준점 높이입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue")
	float DialogueLookAtHeight = 80.0f;

	// 기준점 옆으로 빠지는 거리입니다. 값이 클수록 숄더뷰 느낌이 강해집니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue")
	float DialogueShoulderOffset = 120.0f;

	// Player와 NPC 사이를 기준으로 카메라를 좌우로 공전시키는 각도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue", meta = (ClampMin = "-80.0", ClampMax = "80.0"))
	float DialogueOrbitAngleDegrees = 60.0f;

	// 대화 카메라가 바라보는 지점보다 살짝 위에서 내려다보도록 올리는 높이입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue")
	float DialogueCameraHeightOffset = 40.0f;

	// 0.5는 플레이어와 NPC 중앙, 1.0은 NPC 위치를 기준점으로 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DialogueFocusBiasToSpeaker = 0.55f;

	// 현재 카메라가 있던 쪽을 유지해서 대화 시작 시 카메라가 반대편으로 튀지 않게 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue")
	bool bKeepCurrentDialogueCameraSide = true;

	// 대화 카메라가 목표 구도로 따라가는 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Dialogue", meta = (ClampMin = "0.0"))
	float DialogueCameraInterpSpeed = 6.0f;

	// 카메라가 최종적으로 도달해야 하는 목표 yaw 값이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Runtime")
	float TargetCameraYaw = 0.0f;

	// 카메라가 최종적으로 도달해야 하는 목표 거리 값이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Runtime")
	float TargetCameraDistance = 0.0f;

	// 현재 투명 처리 중인 메시와 원래 머티리얼 정보를 보관한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Runtime")
	bool bDialogueFocusActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Camera|Runtime")
	float TemporaryZoomTargetDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Camera|Runtime")
	float TemporaryZoomTargetOrthoWidth = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Camera|Runtime")
	FVector TemporaryZoomFocusOffset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Camera|Runtime")
	bool bTemporaryZoomFaceOwnerFromFront = false;

	// 지역 카메라 볼륨이 평상시 주시점에 더하는 월드 공간 보정값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Camera|Runtime")
	FVector AreaCameraOffset = FVector::ZeroVector;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UMeshComponent>, FOccludedMeshState> OccludedMeshStates;

	// 소유 액터에서 카메라 관련 컴포넌트를 캐싱한다.
	void CacheCameraComponents();

	// 시작 시점에 기본 각도와 거리를 실제 카메라 릭에 반영한다.
	void InitializeCameraRig();

	// 디테일 창의 투영 설정을 실제 FollowCamera에 반영한다.
	void ApplyCameraProjection();

	// 목표 yaw와 거리로 현재 카메라를 부드럽게 갱신한다.
	void UpdateSnapCamera(float DeltaSeconds);
	void UpdateDialogueCamera(float DeltaSeconds);
	bool HasTemporaryZoomRequest() const;
	float GetEffectiveTargetCameraYaw() const;
	float GetEffectiveTargetCameraDistance() const;
	float GetEffectiveTargetOrthoWidth() const;
	FVector GetEffectiveTargetCameraOffset() const;

	// 플레이어와 카메라 사이를 가리는 메시를 찾아 임시로 투명 처리한다.
	void UpdateCameraOcclusion();
	FVector GetCameraOcclusionTraceEnd(const AActor* Owner) const;
	void BuildCameraOcclusionProbeOffsets(TArray<FVector2D>& OutProbeOffsets) const;
	bool IsCameraOcclusionCandidate(const UMeshComponent* MeshComponent, const AActor* Owner, const FVector& TraceEnd) const;
	// 플레이어가 현재 밟고 있는 바닥 컴포넌트는 가림 처리에서 제외한다.
	bool IsOwnerSupportMesh(const UMeshComponent* MeshComponent, const AActor* Owner) const;

	// 한 메시를 반투명 머티리얼로 바꿔 가림을 완화한다.
	void ApplyOcclusionToMesh(UMeshComponent* MeshComponent);

	// 한 메시의 원래 머티리얼을 복구한다.
	void RestoreOcclusionFromMesh(UMeshComponent* MeshComponent);

	// 현재 가림 처리 중인 모든 메시를 한 번에 복구한다.
	void RestoreAllOccludedMeshes();
	FVector GetOwnerDialogueLocation() const;
	FVector GetSpeakerDialogueLocation() const;

	UPROPERTY(Transient)
	TObjectPtr<AActor> DialogueSpeakerActor = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> TemporaryZoomRequestSource;

	FVector RegularCameraTargetOffset = FVector::ZeroVector;
	FVector TargetCameraOffset = FVector::ZeroVector;
	float SavedTargetCameraYaw = 0.0f;
	float SavedTargetCameraDistance = 0.0f;
	float DialogueSideSign = 1.0f;
};
