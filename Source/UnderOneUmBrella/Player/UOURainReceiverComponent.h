// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOURainReceiverComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRainExposureChangedSignature, float, NewExposure, float, MaxExposure);

// 플레이어가 비를 얼마나 직접 맞고 있는지 저장하고 서서히 말리는 컴포넌트다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOURainReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 비 노출량 초기값과 건조 속도를 설정한다.
	UUOURainReceiverComponent();

	// 시작 시 현재 비 노출량을 초기값으로 맞춘다.
	virtual void BeginPlay() override;

	// 최근 비를 맞지 않았다면 자연 건조를 진행한다.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Rain")
	FOnRainExposureChangedSignature OnRainExposureChanged;

	// 비를 더 맞았을 때 누적할 수 있는 최대 노출량이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float MaxExposure = 5.0f;

	// 시작 시 노출량을 디버그나 테스트용으로 미리 채워두는 값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float InitialExposure = 0.0f;

	// 비를 맞지 않을 때 자연스럽게 줄어드는 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float NaturalDryRate = 0.35f;

	// 마지막으로 비를 맞은 뒤 바로 마르지 않도록 주는 유예 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float DryingGraceTime = 0.12f;

	// 현재 플레이어가 직접 비를 맞아 누적된 양이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	float CurrentExposure = 0.0f;

	// 비를 실제로 맞았을 때 노출량을 더한다.
	UFUNCTION(BlueprintCallable, Category = "Rain")
	void ApplyRainExposure(float ExposureAmount);

	// 현재 비 노출량을 즉시 0으로 초기화한다.
	UFUNCTION(BlueprintCallable, Category = "Rain")
	void ClearExposure();

	// 현재 노출량을 최대치 대비 비율로 돌려준다.
	UFUNCTION(BlueprintPure, Category = "Rain")
	float GetExposureRatio() const;

protected:
	// 마지막으로 비를 맞은 시점을 저장해 자연 건조 시작 타이밍에 쓴다.
	float LastExposureTimestamp = -BIG_NUMBER;

	// 내부 노출량을 안전한 범위로 제한하며 갱신한다.
	void SetExposure(float NewExposure);

	// 노출량 변경 이벤트를 필요한 곳으로 방송한다.
	void BroadcastExposureChanged();
};
