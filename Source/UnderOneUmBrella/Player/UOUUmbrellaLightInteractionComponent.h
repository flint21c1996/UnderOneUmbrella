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
class UUOUUmbrellaLightInteractionComponent : public UActorComponent
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bCreateRuntimeLightSurfaceWhenMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bCreateRuntimeLightShadeVolumeWhenMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light|Debug", meta = (DisplayName = "Draw Reflector Debug", ToolTip = "Puzzle 월드 디버그가 켜져 있을 때 우산 반사판 박스와 반사 방향을 표시합니다."))
	bool bDrawReflectorDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light|Debug", meta = (ClampMin = "0.0", DisplayName = "Reflector Debug Arrow Length", ToolTip = "우산 반사판에서 표시하는 반사 방향 화살표의 길이입니다."))
	float ReflectorDebugArrowLength = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light|Debug", meta = (ClampMin = "0.0", DisplayName = "Reflector Debug Thickness", ToolTip = "우산 반사판 디버그 박스와 화살표의 선 두께입니다."))
	float ReflectorDebugThickness = 3.0f;

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
	void DrawReflectorDebug() const;

	UFUNCTION()
	void HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella);
};
