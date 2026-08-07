// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "World/Light/UOULightBeamVisualTypes.h"
#include "UOULightBeamVisualInterface.generated.h"

UINTERFACE(BlueprintType, meta = (DisplayName = "UOU Light Beam Visual"))
class UNDERONEUMBRELLA_API UUOULightBeamVisualInterface : public UInterface
{
	GENERATED_BODY()
};

// LazyGodray나 Niagara처럼 실제 빛줄기를 표시하는 BP가 구현할 인터페이스입니다.
class UNDERONEUMBRELLA_API IUOULightBeamVisualInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Light|Visual", meta = (DisplayName = "빛 구간 적용", ToolTip = "계산된 빛 구간의 위치, 길이, 폭, 색상과 광량을 VFX에 적용합니다."))
	void ApplyLightBeamSegment(const FUOULightBeamVisualSegmentData& SegmentData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Light|Visual", meta = (DisplayName = "빛줄기 표시 설정", ToolTip = "풀링된 빛줄기 VFX의 표시 여부를 설정합니다."))
	void SetLightBeamVisualActive(bool bActive);
};
