// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraTypes.h"
#include "GameFramework/Actor.h"
#include "UOUCinematicCameraTargetActor.generated.h"

class UArrowComponent;
class UCameraComponent;
class USceneComponent;

// 연출 카메라가 도착할 위치와 해당 구간의 카메라 설정을 배치형 마커로 관리합니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Cinematic Camera Target Actor"))
class UNDERONEUMBRELLA_API AUOUCinematicCameraTargetActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUCinematicCameraTargetActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Target")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Target")
	TObjectPtr<UArrowComponent> TargetDirectionMarker = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Target")
	TObjectPtr<UCameraComponent> TargetCameraPreview = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (DisplayName = "Override Projection Mode", ToolTip = "켜면 이 목표 지점에 도착할 때 사용할 카메라 투영 방식을 별도로 지정합니다."))
	bool bOverrideProjectionMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (EditCondition = "bOverrideProjectionMode", DisplayName = "Projection Mode", ToolTip = "이 목표 지점에 도착할 때 사용할 투영 방식입니다."))
	TEnumAsByte<ECameraProjectionMode::Type> ProjectionMode = ECameraProjectionMode::Orthographic;

	UPROPERTY()
	bool bUseOrthographicProjection = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (DisplayName = "Override Orthographic Width", ToolTip = "켜면 이 목표 지점의 직교 카메라 화면 폭을 별도로 지정합니다."))
	bool bOverrideOrthographicWidth = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (EditCondition = "bOverrideOrthographicWidth", ClampMin = "1.0", DisplayName = "Orthographic Width", ToolTip = "직교 투영에서 화면에 담을 월드 폭입니다. 값이 작을수록 더 가까이 보입니다."))
	float OrthographicWidth = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (DisplayName = "Override Field Of View", ToolTip = "켜면 이 목표 지점의 원근 카메라 FOV를 별도로 지정합니다."))
	bool bOverrideFieldOfView = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (EditCondition = "bOverrideFieldOfView", ClampMin = "5.0", ClampMax = "170.0", DisplayName = "Field Of View", ToolTip = "원근 투영에서 사용할 시야각입니다."))
	float FieldOfView = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (DisplayName = "Override Aspect Ratio", ToolTip = "켜면 이 목표 지점의 화면 비율을 별도로 지정합니다."))
	bool bOverrideAspectRatio = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (EditCondition = "bOverrideAspectRatio", ClampMin = "0.1", DisplayName = "Aspect Ratio", ToolTip = "카메라 화면 비율입니다. 1.777은 16:9에 해당합니다."))
	float AspectRatio = 16.0f / 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (DisplayName = "Override Constrain Aspect Ratio", ToolTip = "켜면 이 목표 지점에서 화면 비율 고정 여부를 별도로 지정합니다."))
	bool bOverrideConstrainAspectRatio = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Projection", meta = (EditCondition = "bOverrideConstrainAspectRatio", DisplayName = "Constrain Aspect Ratio", ToolTip = "카메라가 지정한 화면 비율을 강제로 유지할지 정합니다."))
	bool bConstrainAspectRatio = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Move Step", meta = (DisplayName = "Override Move Duration", ToolTip = "켜면 이 목표 지점으로 이동하는 시간만 별도로 지정합니다."))
	bool bOverrideMoveDuration = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Move Step", meta = (EditCondition = "bOverrideMoveDuration", ClampMin = "0.0", DisplayName = "Move Duration", ToolTip = "이 목표 지점까지 이동하는 데 걸리는 시간입니다."))
	float MoveDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic Camera|Move Step", meta = (DisplayName = "Continue To Next Step On Arrival", ToolTip = "켜면 이 목표 지점에 도착한 직후 다음 카메라 스텝을 자동으로 이어서 재생합니다."))
	bool bContinueToNextStepOnArrival = false;

	ECameraProjectionMode::Type ResolveProjectionMode(ECameraProjectionMode::Type CameraDefault) const;
	float ResolveOrthographicWidth(float CameraDefault) const;
	float ResolveFieldOfView(float CameraDefault) const;
	float ResolveAspectRatio(float CameraDefault) const;
	bool ResolveConstrainAspectRatio(bool bCameraDefault) const;
	float ResolveMoveDuration(float CameraDefault) const;

	void SyncPreviewCamera(
		ECameraProjectionMode::Type InProjectionMode,
		float InOrthographicWidth,
		float InFieldOfView,
		float InAspectRatio,
		bool bInConstrainAspectRatio);

	void SetTargetCameraPreviewVisible(bool bVisible);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
