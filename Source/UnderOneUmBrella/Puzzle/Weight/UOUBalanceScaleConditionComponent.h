// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUBalanceScaleConditionComponent.generated.h"

class UActorComponent;

// 좌우 무게 차이를 비교해서 저울이 균형을 이루었는지 판단하는 조건 컴포넌트입니다.
// 두 쪽 무게를 읽어 허용 오차 안에 들어오면 만족 상태로 바뀝니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUBalanceScaleConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUBalanceScaleConditionComponent();

	// 시작 시 좌우 무게 소스 참조를 해석합니다.
	virtual void BeginPlay() override;

	// 매 틱마다 무게를 다시 읽어서 균형 상태를 갱신합니다.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FText GetDebugSummaryText_Implementation() const override;

	// 왼쪽 무게 소스 참조입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	FComponentReference LeftWeightSource;

	// 오른쪽 무게 소스 참조입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	FComponentReference RightWeightSource;

	// 허용할 수 있는 최대 무게 차이입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float AllowedDifference = 0.25f;

	// 판정을 시작하기 위한 최소 총 무게입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float MinimumTotalWeight = 0.1f;

	// 현재 왼쪽에서 읽힌 무게입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float LeftWeight = 0.0f;

	// 현재 오른쪽에서 읽힌 무게입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float RightWeight = 0.0f;

	// 좌우 무게 차이를 절대값으로 저장한 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float WeightDifference = 0.0f;

protected:
	// 해석된 왼쪽 무게 소스 컴포넌트입니다.
	UPROPERTY(Transient)
	TObjectPtr<UActorComponent> ResolvedLeftWeightSource = nullptr;

	// 해석된 오른쪽 무게 소스 컴포넌트입니다.
	UPROPERTY(Transient)
	TObjectPtr<UActorComponent> ResolvedRightWeightSource = nullptr;

	// 참조 설정을 바탕으로 좌우 무게 소스를 찾습니다.
	void ResolveWeightSources();

	// 현재 무게값을 바탕으로 균형 만족 상태를 갱신합니다.
	void RefreshBalanceState();

	// 지정한 컴포넌트에서 퍼즐용 무게 값을 읽습니다.
	float GetWeightFromComponent(const UActorComponent* Component) const;
};
