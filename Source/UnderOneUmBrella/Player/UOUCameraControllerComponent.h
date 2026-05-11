// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUCameraControllerComponent.generated.h"

class UCameraComponent;
class UMaterialInterface;
class UMeshComponent;
class USpringArmComponent;

// ??援ъ“泥대뒗 移대찓??媛由?泥섎━ 以??먮옒 癒명떚由ъ뼹 ?곹깭瑜??섎룎由ш린 ?꾪빐 蹂닿??쒕떎.
struct FOccludedMeshState
{
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
};

// ???대옒?ㅻ뒗 8諛⑺뼢 ?뚯쟾 移대찓?쇱? 以? 踰?媛由?泥섎━瑜??대떦?쒕떎.
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class UUOUCameraControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUCameraControllerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void RotateCameraLeft();
	void RotateCameraRight();
	void ZoomCameraIn();
	void ZoomCameraOut();

	float GetMovementYaw() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|References")
	bool bAutoFindCameraComponents = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|References")
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|References")
	TObjectPtr<UCameraComponent> FollowCamera = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Snap")
	float CameraPitchAngle = -45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Snap", meta = (ClampMin = "1.0"))
	float CameraRotateStep = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Snap", meta = (ClampMin = "0.0"))
	float CameraRotationInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float CameraZoomStep = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float MinCameraDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float MaxCameraDistance = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float CameraZoomInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion", meta = (ClampMin = "0.0"))
	float OcclusionProbeRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion")
	FVector OcclusionTargetOffset = FVector(0.0f, 0.0f, 60.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Occlusion")
	TObjectPtr<UMaterialInterface> OccluderFadeMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Runtime")
	float TargetCameraYaw = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Runtime")
	float TargetCameraDistance = 0.0f;

	TMap<TObjectPtr<UMeshComponent>, FOccludedMeshState> OccludedMeshStates;

	void CacheCameraComponents();
	void InitializeCameraRig();
	void UpdateSnapCamera(float DeltaSeconds);
	void UpdateCameraOcclusion();
	void ApplyOcclusionToMesh(UMeshComponent* MeshComponent);
	void RestoreOcclusionFromMesh(UMeshComponent* MeshComponent);
	void RestoreAllOccludedMeshes();
};
