// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUWaterAmountConditionComponent.generated.h"

class UUOUWaterContainerComponent;

// ???대옒?ㅻ뒗 臾????而댄룷?뚰듃???꾩옱 ?묒쓣 ?쇱쫹 議곌굔?쇰줈 諛붽퓭二쇰뒗 ?대뙌?곕떎.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUWaterAmountConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUWaterAmountConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Water")
	bool bAutoFindWaterContainer = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Water")
	FComponentReference WaterContainerReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Water")
	bool bUseContainerMaxAmountAsRequirement = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Water", meta = (ClampMin = "0.0"))
	float RequiredAmount = 1.0f;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	TObjectPtr<UUOUWaterContainerComponent> WaterContainer = nullptr;

protected:
	UFUNCTION()
	void HandleWaterAmountChanged(float NewAmount, float MaxAmount);

	void ResolveWaterContainer();
	void SubscribeWaterContainer();
	void UnsubscribeWaterContainer();
	void RefreshSatisfiedState();
	float GetRequiredAmount() const;
};
