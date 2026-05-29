// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Stage/UOUFloorPlatformActor.h"
#include "UOUFloorPlatformTargetActor.generated.h"

class UMaterialInterface;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

// 목표 마커가 이동 구간별 회전 방식을 직접 덮어쓸지 정합니다.
// 기본값을 사용하면 플랫폼 액터에 있는 Movement 설정을 그대로 따릅니다.
UENUM(BlueprintType)
enum class EUOUFloorPlatformStepRotationMode : uint8
{
	UsePlatformDefault,
	TransformLerp,
	Hinge
};

// 층 플랫폼의 목표 위치와 목표 회전을 월드에 직접 배치해서 정하는 마커 액터입니다.
// 디자이너가 이 액터를 움직이고 돌리면 플랫폼은 해당 트랜스폼을 최종 상태로 사용합니다.
UCLASS(meta=(DisplayName="UOU Floor Platform Target Actor"))
class AUOUFloorPlatformTargetActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUFloorPlatformTargetActor();

	// 목표 마커 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Target")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 목표 위치의 기준점을 작은 구체로 보여주는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Target")
	TObjectPtr<USphereComponent> TargetOriginMarker = nullptr;

	// 목표 위치에 놓일 플랫폼 모습을 보여주는 확인용 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Target")
	TObjectPtr<UStaticMeshComponent> TargetPreviewMesh = nullptr;

	// 목표 발판을 실제 발판과 헷갈리지 않게 보여주기 위한 반투명 확인용 머티리얼입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Target")
	TObjectPtr<UMaterialInterface> TargetPreviewMaterial = nullptr;

	// 켜져 있으면 Target Preview Material을 미리보기 메쉬에 계속 적용합니다.
	// 끄면 TargetPreviewMesh 컴포넌트의 머티리얼 슬롯을 직접 수정할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Target")
	bool bOverrideTargetPreviewMeshMaterial = true;

	// 이 마커로 이동하는 구간에서 사용할 회전 방식을 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Step", meta = (DisplayName = "Step Rotation Mode"))
	EUOUFloorPlatformStepRotationMode StepRotationMode = EUOUFloorPlatformStepRotationMode::UsePlatformDefault;

	// Step Rotation Mode가 Hinge일 때 이 구간에서만 사용할 고정 변입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Step", meta = (EditCondition = "StepRotationMode == EUOUFloorPlatformStepRotationMode::Hinge", EditConditionHides, DisplayName = "Step Hinge Edge"))
	EUOUFloorPlatformHingeEdge StepHingeEdge = EUOUFloorPlatformHingeEdge::NegativeY;

	// Step Hinge Edge가 Custom일 때 이 구간에서만 사용할 로컬 힌지 위치입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Step", meta = (EditCondition = "StepRotationMode == EUOUFloorPlatformStepRotationMode::Hinge && StepHingeEdge == EUOUFloorPlatformHingeEdge::Custom", EditConditionHides, DisplayName = "Step Custom Hinge Local Offset"))
	FVector StepCustomHingeLocalOffset = FVector::ZeroVector;

	// 이 마커에 도착한 직후 다음 이동 스텝을 자동으로 한 번 더 실행할지 정합니다.
	// 특정 구간만 1->2->3처럼 이어붙이고, 이후 구간은 버튼 입력마다 한 칸씩 움직이게 할 때 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Step", meta = (DisplayName = "Continue To Next Step On Arrival"))
	bool bContinueToNextStepOnArrival = false;

	// 플랫폼 메쉬와 같은 리소스를 목표 미리보기 메쉬에 복사합니다.
	void SyncPreviewFromMesh(const UStaticMeshComponent* SourceMesh);

	// 목표 위치에 놓인 결과 플랫폼 미리보기 메쉬만 켜고 끕니다.
	void SetTargetPreviewMeshVisible(bool bVisible);

	// 플랫폼 기본 설정과 마커의 덮어쓰기 값을 합쳐 실제 이동 회전 방식을 반환합니다.
	EUOUFloorPlatformRotationMode ResolveRotationMode(EUOUFloorPlatformRotationMode PlatformDefault) const;

	// 플랫폼 기본 설정과 마커의 덮어쓰기 값을 합쳐 실제 힌지 변을 반환합니다.
	EUOUFloorPlatformHingeEdge ResolveHingeEdge(EUOUFloorPlatformHingeEdge PlatformDefault) const;

	// 플랫폼 기본 설정과 마커의 덮어쓰기 값을 합쳐 실제 커스텀 힌지 위치를 반환합니다.
	FVector ResolveCustomHingeLocalOffset(const FVector& PlatformDefault) const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// 미리보기 머티리얼 덮어쓰기 설정을 실제 PreviewMesh에 반영합니다.
	void ApplyPreviewMaterialSettings(const UStaticMeshComponent* SourceMesh);
};
