// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "UOUUmbrellaLightShadeVolumeComponent.generated.h"

// 펼친 우산 아래에서 게임플레이용 빛 노출을 차단하는 전용 범위입니다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella Light Shade Volume", ToolTip="펼친 우산 아래에서 게임플레이용 빛과 온도 노출을 차단하는 범위입니다."))
class UNDERONEUMBRELLA_API UUOUUmbrellaLightShadeVolumeComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UUOUUmbrellaLightShadeVolumeComponent();

	virtual void OnRegister() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Light Shade", meta = (ToolTip = "현재 이 범위가 게임플레이용 빛을 차단하고 있는지 나타냅니다."))
	bool bShadeEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Light Shade|Direction", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg", DisplayName = "최대 빛 차단 입사각", ToolTip = "우산 차단면의 앞면 노멀과 입사광 사이에서 허용할 최대 각도입니다. 90도를 넘기면 평행한 입사광과 제한된 범위의 뒷면 입사광도 차단합니다."))
	float MaximumBlockingIncidenceAngle = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Light Shade|Direction", meta = (DisplayName = "앞면 기준으로 차단", ToolTip = "활성화하면 우산 차단면의 앞면을 기준으로 입사각을 판정합니다. 최대 입사각이 90도를 넘으면 그 초과분만큼 뒷면 방향도 허용합니다."))
	bool bBlockFrontFaceOnly = true;

	UFUNCTION(BlueprintCallable, Category = "Umbrella|Light Shade", meta = (ToolTip = "우산 그늘 범위의 빛 차단을 켜거나 끕니다."))
	void SetShadeEnabled(bool bNewShadeEnabled);

	UFUNCTION(BlueprintPure, Category = "Umbrella|Light Shade", meta = (ToolTip = "현재 이 범위가 게임플레이용 빛을 차단할 수 있으면 true를 반환합니다."))
	bool CanShadeLight() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella|Light Shade", meta = (ToolTip = "광선 진행 방향이 현재 우산 차단면의 허용 입사각 안에 있으면 true를 반환합니다."))
	bool CanShadeIncomingLight(const FVector& IncomingDirection) const;

	UFUNCTION(BlueprintPure, Category = "Umbrella|Light Shade", meta = (ToolTip = "월드 위치가 활성화된 우산 그늘 범위 안에 있으면 true를 반환합니다."))
	bool ContainsWorldPosition(const FVector& WorldPosition) const;

protected:
	void ApplyCollisionSettings();
};
