// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "UOUDialogueCoverTargetComponent.generated.h"

class UUOUUmbrellaCoverVolumeComponent;

// 우산으로 씌워줘야 하는 대화 대상 지점을 나타내는 스피어입니다.
// 에디터에서 반경을 눈으로 보며 조정하고, 플레이어의 대화용 우산 커버 박스와 닿으면 커버 성공으로 봅니다.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Cover Target"))
class UNDERONEUMBRELLA_API UUOUDialogueCoverTargetComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UUOUDialogueCoverTargetComponent();

	// 화면상 거의 닿아 보이는 경계 상황을 성공으로 봐주기 위한 여유 거리입니다.
	// 디테일 창에서 키우면 스피어와 우산 커버 박스가 살짝 떨어져 있어도 대화 커버로 인정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Cover", meta = (ClampMin = "0.0"))
	float CoverTouchTolerance = 5.0f;

	// 지정한 우산 커버 박스와 이 스피어가 현재 닿아 있는지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Cover")
	bool IsCoveredByUmbrellaVolume(const UUOUUmbrellaCoverVolumeComponent* CoverVolume) const;
};
