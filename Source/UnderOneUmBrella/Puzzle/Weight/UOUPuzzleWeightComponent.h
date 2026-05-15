// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Puzzle/Core/UOUPuzzleWeightSource.h"
#include "UOUPuzzleWeightComponent.generated.h"

class UUOUWaterContainerComponent;

// 퍼즐에서 사용할 기본 무게와 저장된 물 무게를 합산해 주는 무게 컴포넌트입니다.
// 상자나 오브젝트가 버튼과 저울에 올라갔을 때 공통 방식으로 무게를 제공합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUPuzzleWeightComponent : public UActorComponent, public IUOUPuzzleWeightSource
{
	GENERATED_BODY()

public:
	UUOUPuzzleWeightComponent();

	// 시작 시 물 컨테이너 참조 상태를 확인합니다.
	virtual void BeginPlay() override;

	// 현재 퍼즐 판정에 사용할 총 무게를 계산해서 반환합니다.
	virtual float GetPuzzleWeight() const override;

	// 이 오브젝트의 기본 무게입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float BaseWeight = 5.0f;

	// 저장된 물 무게를 총 무게에 합산할지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	bool bIncludeStoredWaterWeight = true;

	// 물 무게를 읽어올 물 컨테이너 컴포넌트입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	TObjectPtr<UUOUWaterContainerComponent> WaterContainer = nullptr;

protected:
	// 연결된 물 컨테이너에서 추가 무게 기여분을 계산합니다.
	float GetWaterWeightContribution() const;
};
