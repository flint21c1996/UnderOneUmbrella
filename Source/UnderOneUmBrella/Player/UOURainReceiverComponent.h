// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOURainReceiverComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRainExposureChangedSignature, float, NewExposure, float, MaxExposure);

// ???대옒?ㅻ뒗 鍮꾩뿉 ?몄텧???묎낵 ?쒓컙 寃쎄낵???곕Ⅸ ?먯뿰 嫄댁“瑜?愿由ы븳??
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOURainReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOURainReceiverComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Rain")
	FOnRainExposureChangedSignature OnRainExposureChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float MaxExposure = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float InitialExposure = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float NaturalDryRate = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rain", meta = (ClampMin = "0.0"))
	float DryingGraceTime = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rain")
	float CurrentExposure = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Rain")
	void ApplyRainExposure(float ExposureAmount);

	UFUNCTION(BlueprintCallable, Category = "Rain")
	void ClearExposure();

	UFUNCTION(BlueprintPure, Category = "Rain")
	float GetExposureRatio() const;

protected:
	float LastExposureTimestamp = -BIG_NUMBER;

	void SetExposure(float NewExposure);
	void BroadcastExposureChanged();
};
