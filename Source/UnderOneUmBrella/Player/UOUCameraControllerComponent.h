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
struct FOccludedMeshState
{
	// 투명 처리 전 원래 머티리얼 배열을 그대로 기억해둔다.
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
};

// 8방향 스냅 카메라와 줌, 가림 처리를 한 번에 관리하는 카메라 전용 컴포넌트다.
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class UUOUCameraControllerComponent : public UActorComponent
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

	// 통합 플레이어 디버그 HUD에서 현재 카메라 거리를 확인합니다.
	float GetCurrentCameraDistance() const;

	// 통합 플레이어 디버그 HUD에서 목표 카메라 거리를 확인합니다.
	float GetTargetCameraDistance() const { return TargetCameraDistance; }

	// 통합 플레이어 디버그 HUD에서 현재 가림 처리 중인 메시 수를 확인합니다.
	int32 GetOccludedMeshCount() const { return OccludedMeshStates.Num(); }

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
	float OcclusionProbeRadius = 12.0f;

	// 플레이어 중심보다 약간 위를 가림 탐지 기준으로 보정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion")
	FVector OcclusionTargetOffset = FVector(0.0f, 0.0f, 60.0f);

	// 가림 메시를 임시로 바꿔 끼울 반투명 머티리얼이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion")
	TObjectPtr<UMaterialInterface> OccluderFadeMaterial = nullptr;

	// 카메라가 최종적으로 도달해야 하는 목표 yaw 값이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Runtime")
	float TargetCameraYaw = 0.0f;

	// 카메라가 최종적으로 도달해야 하는 목표 거리 값이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Runtime")
	float TargetCameraDistance = 0.0f;

	// 현재 투명 처리 중인 메시와 원래 머티리얼 정보를 보관한다.
	TMap<TObjectPtr<UMeshComponent>, FOccludedMeshState> OccludedMeshStates;

	// 소유 액터에서 카메라 관련 컴포넌트를 캐싱한다.
	void CacheCameraComponents();

	// 시작 시점에 기본 각도와 거리를 실제 카메라 릭에 반영한다.
	void InitializeCameraRig();

	// 목표 yaw와 거리로 현재 카메라를 부드럽게 갱신한다.
	void UpdateSnapCamera(float DeltaSeconds);

	// 플레이어와 카메라 사이를 가리는 메시를 찾아 임시로 투명 처리한다.
	void UpdateCameraOcclusion();

	// 한 메시를 반투명 머티리얼로 바꿔 가림을 완화한다.
	void ApplyOcclusionToMesh(UMeshComponent* MeshComponent);

	// 한 메시의 원래 머티리얼을 복구한다.
	void RestoreOcclusionFromMesh(UMeshComponent* MeshComponent);

	// 현재 가림 처리 중인 모든 메시를 한 번에 복구한다.
	void RestoreAllOccludedMeshes();
};
