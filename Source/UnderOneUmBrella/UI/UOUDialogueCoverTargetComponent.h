// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UOUDialogueCoverTargetComponent.generated.h"

class UUOUUmbrellaCoverVolumeComponent;

// 우산으로 씌워줘야 하는 대화 대상 지점을 나타내는 기준점 컴포넌트입니다.
// 실제 충돌 범위가 아니라 위치와 판정 반경만 제공해서, 에디터에 불필요한 스피어를 하나 더 그리지 않습니다.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Cover Target"))
class UNDERONEUMBRELLA_API UUOUDialogueCoverTargetComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUOUDialogueCoverTargetComponent();

	// 대화 대상 지점의 판정 반경입니다.
	// 이 지점이 우산 커버 박스에 반경만큼 닿으면 우산을 씌워준 것으로 봅니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Cover", meta = (ClampMin = "0.0"))
	float CoverTargetRadius = 80.0f;

	// 화면상 거의 닿아 보이는 경계 상황을 성공으로 봐주기 위한 여유 거리입니다.
	// 디테일 창에서 키우면 기준점과 우산 커버 박스가 살짝 떨어져 있어도 대화 커버로 인정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Cover", meta = (ClampMin = "0.0"))
	float CoverTouchTolerance = 5.0f;

	// 컴포넌트 스케일까지 반영한 최종 판정 반경을 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Cover")
	float GetScaledCoverRadius() const;

	// 지정한 우산 커버 박스와 이 기준점 반경이 현재 닿아 있는지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Cover")
	bool IsCoveredByUmbrellaVolume(const UUOUUmbrellaCoverVolumeComponent* CoverVolume) const;
};
