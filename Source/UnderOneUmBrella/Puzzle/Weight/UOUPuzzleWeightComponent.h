// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Puzzle/Core/UOUPuzzleWeightSource.h"
#include "UOUPuzzleWeightComponent.generated.h"

class UUOUWaterContainerComponent;

// ???대옒?ㅻ뒗 ?뚮젅?댁뼱???곸옄泥섎읆 ?쇱꽌媛 ?쎌쓣 湲곗? 臾닿쾶瑜??쒓납?먯꽌 ?뺤쓽?쒕떎.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUPuzzleWeightComponent : public UActorComponent, public IUOUPuzzleWeightSource
{
	GENERATED_BODY()

public:
	UUOUPuzzleWeightComponent();

	virtual void BeginPlay() override;
	virtual float GetPuzzleWeight() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float BaseWeight = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	bool bIncludeStoredWaterWeight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	TObjectPtr<UUOUWaterContainerComponent> WaterContainer = nullptr;

protected:
	float GetWaterWeightContribution() const;
};
