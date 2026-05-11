// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUWaterContainerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaterAmountChangedSignature, float, NewAmount, float, MaxAmount);

// ???대옒?ㅻ뒗 ?꾩옱 ??λ맂 臾쇱쓽 ?묎낵 理쒕??? 臾닿쾶 湲곗뿬媛믪쓣 愿由ы븳??
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOUWaterContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUWaterContainerComponent();

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "Water")
	FOnWaterAmountChangedSignature OnWaterAmountChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float MaxAmount = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float InitialAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float WeightMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	float CurrentAmount = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Water")
	float AddAmount(float AmountToAdd);

	UFUNCTION(BlueprintCallable, Category = "Water")
	float RemoveAmount(float AmountToRemove);

	UFUNCTION(BlueprintCallable, Category = "Water")
	void SetAmount(float NewAmount);

	UFUNCTION(BlueprintPure, Category = "Water")
	float GetFillRatio() const;

	UFUNCTION(BlueprintPure, Category = "Water")
	float GetWeightContribution() const;

protected:
	void BroadcastAmountChanged();
};
