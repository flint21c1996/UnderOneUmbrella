// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UOUUmbrellaLightInteractionComponent.generated.h"

class UUOULightInteractionSurfaceComponent;
class UUOUUmbrellaLightShadeVolumeComponent;
class USceneComponent;

UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella Light Interaction"))
class UNDERONEUMBRELLA_API UUOUUmbrellaLightInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUUmbrellaLightInteractionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bAutoFindUmbrellaComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bAutoFindLightSurfaceComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bAutoFindLightShadeVolumeComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bSpreadUmbrellaBlocksLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bLightReflectingStateReflectsLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light", meta = (ToolTip = "빛 반사 상태에서 우산 반사판과 그늘 범위가 입사광을 차단하도록 합니다."))
	bool bLightReflectingStateBlocksLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light", meta = (ToolTip = "빛 반사 상태에서도 반사 허용 각도를 벗어난 입사광은 우산과 그늘 범위를 통과시킵니다."))
	bool bPassLightThroughOutsideReflectionAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light", meta = (ClampMin = "0.0", ClampMax = "89.9", Units = "deg", ToolTip = "우산 정면과 입사광 사이의 최대 반사 허용각입니다. 이 각도를 벗어난 빛은 반사 상태에서도 우산을 통과합니다."))
	float MaximumUmbrellaReflectionIncidenceAngle = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light", meta = (ClampMin = "0.0", ClampMax = "89.9", Units = "deg", DisplayName = "최대 우산 빛 차단 입사각", ToolTip = "펼친 우산이 차단할 수 있는 최대 입사각입니다. 기본 75도에서는 위와 대각선 상단광을 막고 수평 측면광은 통과시킵니다."))
	float MaximumUmbrellaBlockingIncidenceAngle = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bCreateRuntimeLightSurfaceWhenMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bCreateRuntimeLightShadeVolumeWhenMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light|Placement", meta = (DisplayName = "비 차단 박스와 빛 판정 동기화", ToolTip = "일반 펼침 상태에서는 빛 차단/반사 박스를 비 차단 박스와 같은 위치, 회전, 크기로 배치합니다."))
	bool bAlignLightInteractionToRainBlocker = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light|Placement", meta = (EditCondition = "bAlignLightInteractionToRainBlocker", DisplayName = "빛 반사 상태 회전 오프셋", ToolTip = "빛 반사 상태에서 비 차단 박스를 플레이어 중심으로 회전시킬 로컬 회전입니다. 기본 -90도 Pitch는 머리 위 박스를 플레이어 앞으로 이동시키고 세웁니다."))
	FRotator LightReflectingBlockerRotationOffset = FRotator(-90.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light|Placement", meta = (EditCondition = "bAlignLightInteractionToRainBlocker", DisplayName = "빛 반사 상태 추가 위치 오프셋", ToolTip = "회전된 빛 판정 박스에 추가할 플레이어 로컬 위치 오프셋입니다."))
	FVector LightReflectingBlockerAdditionalLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light", meta = (ClampMin = "0.0"))
	FVector RuntimeSurfaceBoxExtent = FVector(70.0f, 70.0f, 6.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	FVector RuntimeSurfaceRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	FRotator RuntimeSurfaceRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light Shade", meta = (ClampMin = "0.0", DisplayName = "Shade Volume Box Extent", ToolTip = "펼친 우산 아래에서 게임플레이용 빛과 온도를 차단할 박스의 절반 크기입니다."))
	FVector RuntimeShadeVolumeBoxExtent = FVector(90.0f, 90.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light Shade", meta = (DisplayName = "Shade Volume Relative Location", ToolTip = "우산 기준점에서 그늘 박스 중심까지의 로컬 위치입니다."))
	FVector RuntimeShadeVolumeRelativeLocation = FVector(0.0f, 0.0f, -100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light Shade", meta = (DisplayName = "Shade Volume Relative Rotation", ToolTip = "우산 기준점에 대한 그늘 박스의 로컬 회전입니다."))
	FRotator RuntimeShadeVolumeRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UUOUUmbrellaComponent> UmbrellaComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UUOULightInteractionSurfaceComponent> LightSurfaceComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UUOUUmbrellaLightShadeVolumeComponent> LightShadeVolumeComponent = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Umbrella|Light")
	void RefreshLightInteractionMode();

protected:
	void ResolveReferences();
	void EnsureRuntimeLightSurface();
	void EnsureRuntimeLightShadeVolume();
	USceneComponent* GetLightInteractionAttachParent() const;
	void ApplyRuntimeLightSurfacePlacement() const;
	void ApplyRuntimeLightShadeVolumePlacement() const;
	bool TryResolveRainBlockerAlignedTransform(
		FVector& OutWorldCenter,
		FRotator& OutWorldRotation,
		FVector& OutHalfExtent) const;
	UFUNCTION()
	void HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella);
};
