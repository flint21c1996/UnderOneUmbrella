// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUBalanceScaleConditionComponent.generated.h"

class UActorComponent;

// ???대옒?ㅻ뒗 ??臾닿쾶 ?뚯뒪瑜?鍮꾧탳???덉슜 ?ㅼ감 ?덉뿉??洹좏삎??留욌뒗吏 ?먮떒?쒕떎.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUBalanceScaleConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUBalanceScaleConditionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	FComponentReference LeftWeightSource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	FComponentReference RightWeightSource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float AllowedDifference = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float MinimumTotalWeight = 0.1f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float LeftWeight = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float RightWeight = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float WeightDifference = 0.0f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UActorComponent> ResolvedLeftWeightSource = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UActorComponent> ResolvedRightWeightSource = nullptr;

	void ResolveWeightSources();
	void RefreshBalanceState();
	float GetWeightFromComponent(const UActorComponent* Component) const;
};
