// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "UOUUmbrellaLightShadeVolumeComponent.generated.h"

// 펼친 우산 아래에서 게임플레이용 빛 노출을 차단하는 전용 범위입니다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella Light Shade Volume", ToolTip="펼친 우산 아래에서 게임플레이용 빛과 온도 노출을 차단하는 범위입니다."))
class UUOUUmbrellaLightShadeVolumeComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UUOUUmbrellaLightShadeVolumeComponent();

	virtual void OnRegister() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Light Shade", meta = (ToolTip = "현재 이 범위가 게임플레이용 빛을 차단하고 있는지 나타냅니다."))
	bool bShadeEnabled = false;

	UFUNCTION(BlueprintCallable, Category = "Umbrella|Light Shade", meta = (ToolTip = "우산 그늘 범위의 빛 차단을 켜거나 끕니다."))
	void SetShadeEnabled(bool bNewShadeEnabled);

	UFUNCTION(BlueprintPure, Category = "Umbrella|Light Shade", meta = (ToolTip = "현재 이 범위가 게임플레이용 빛을 차단할 수 있으면 true를 반환합니다."))
	bool CanShadeLight() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella|Light Shade", meta = (ToolTip = "월드 위치가 활성화된 우산 그늘 범위 안에 있으면 true를 반환합니다."))
	bool ContainsWorldPosition(const FVector& WorldPosition) const;

protected:
	void ApplyCollisionSettings();
};
