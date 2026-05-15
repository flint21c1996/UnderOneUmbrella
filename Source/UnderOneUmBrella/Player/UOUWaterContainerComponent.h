// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUWaterContainerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaterAmountChangedSignature, float, NewAmount, float, MaxAmount);

// 물 양을 저장하고 퍼즐이나 무게 계산에 넘길 수 있게 관리하는 범용 물 컨테이너다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOUWaterContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 저장량 최대치와 초기값을 설정한다.
	UUOUWaterContainerComponent();

	// 시작 시 현재 물 양을 초기값으로 맞춘다.
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "Water")
	FOnWaterAmountChangedSignature OnWaterAmountChanged;

	// 이 컨테이너가 가질 수 있는 최대 물 양이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float MaxAmount = 5.0f;

	// 시작 시 디버그나 테스트용으로 미리 채워둘 물 양이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float InitialAmount = 0.0f;

	// 현재 물 양이 퍼즐 무게에 얼마나 기여할지 정하는 배수다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float WeightMultiplier = 1.0f;

	// 현재 실제로 저장된 물 양이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	float CurrentAmount = 0.0f;

	// 현재 물 양에 값을 더하고 실제 더해진 양을 돌려준다.
	UFUNCTION(BlueprintCallable, Category = "Water")
	float AddAmount(float AmountToAdd);

	// 현재 물 양에서 값을 빼고 실제 빠진 양을 돌려준다.
	UFUNCTION(BlueprintCallable, Category = "Water")
	float RemoveAmount(float AmountToRemove);

	// 물 양을 직접 설정하고 범위를 안전하게 제한한다.
	UFUNCTION(BlueprintCallable, Category = "Water")
	void SetAmount(float NewAmount);

	// 현재 물 양을 최대치 대비 비율로 돌려준다.
	UFUNCTION(BlueprintPure, Category = "Water")
	float GetFillRatio() const;

	// 현재 물 양이 퍼즐 무게에 더해질 값을 계산한다.
	UFUNCTION(BlueprintPure, Category = "Water")
	float GetWeightContribution() const;

protected:
	// 내부 물 양이 바뀐 뒤 변경 이벤트를 한 번에 방송한다.
	void BroadcastAmountChanged();
};
