// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOURotatableMirrorActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOULightInteractionSurfaceComponent;
class UUOURotatableMirrorComponent;

// 회전축, 거울 메시, 빛 반사면과 플레이어 밀기 영역을 한 번에 제공하는 기본 거울 액터입니다.
UCLASS(Blueprintable)
class AUOURotatableMirrorActor : public AActor
{
	GENERATED_BODY()

public:
	AUOURotatableMirrorActor();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Mirror|Reflection", meta = (DisplayName = "반사 시작 및 유지 각도 설정", ToolTip = "새 반사를 시작할 최대 입사각과 이미 반사 중일 때 유지할 최대 입사각을 설정합니다."))
	void SetReflectionIncidenceAngles(
		float StartMaximumAngleDegrees,
		float RetainedMaximumAngleDegrees);

	UFUNCTION(BlueprintCallable, Category = "Mirror|Reflection", meta = (DisplayName = "거울 밖 빛 허용 비율 설정", ToolTip = "빛 단면이 거울 밖으로 벗어나도 반사할 수 있는 비율을 0~100%로 설정합니다."))
	void SetBeamFootprintOverflowAllowance(float OverflowAllowancePercent);

	// 켜면 거울 메시의 로컬 바운드와 트랜스폼을 빛 반사 판정 박스에 자동 적용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Light Surface", meta = (DisplayName = "반사 판정 박스를 메시와 동기화", ToolTip = "거울 메시의 위치, 회전, 스케일 또는 메시 에셋이 바뀌면 반사 판정 박스도 같은 범위로 맞춥니다. 끄면 판정 박스를 직접 설정할 수 있습니다."))
	bool bSyncLightSurfaceToMirrorMesh = true;

	// 선 트레이스가 매우 얇은 메시를 안정적으로 맞히도록 메시 두께에 추가할 판정 여유입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Light Surface", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bSyncLightSurfaceToMirrorMesh", DisplayName = "반사 판정 두께 여유", ToolTip = "거울 앞뒤 방향의 판정 박스 두께에 추가할 여유입니다. 가로와 세로 크기에는 영향을 주지 않습니다."))
	float LightSurfaceThicknessPadding = 1.0f;

	// 거울 전체가 회전하는 중앙 회전축입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<USceneComponent> MirrorPivot = nullptr;

	// 화면 표시와 플레이어 차단을 담당하는 거울 메시입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<UStaticMeshComponent> MirrorMesh = nullptr;

	// 빛 차단 및 반사 판정을 담당하는 표면입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<UUOULightInteractionSurfaceComponent> LightInteractionSurface = nullptr;

	// 플레이어가 거울을 밀고 있는지 감지하는 Overlap 영역입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<UBoxComponent> PushVolume = nullptr;

	// 거울 왼쪽에서 플레이어가 잡을 기본 손잡이 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<USceneComponent> PushHandleLeft = nullptr;

	// 거울 오른쪽에서 플레이어가 잡을 기본 손잡이 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<USceneComponent> PushHandleRight = nullptr;

	// Push Volume 안의 플레이어 이동을 중앙 회전축 기준 회전으로 변환합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<UUOURotatableMirrorComponent> RotatableMirror = nullptr;

protected:
	void SyncLightInteractionSurfaceToMirrorMesh() const;
};
