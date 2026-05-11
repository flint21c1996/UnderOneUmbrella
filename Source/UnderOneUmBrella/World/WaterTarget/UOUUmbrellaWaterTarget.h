// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUUmbrellaWaterTarget.generated.h"

class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOUWaterContainerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUOUUmbrellaWaterTargetChangedSignature, float, CurrentWater, float, RequiredWater, bool, bIsActivated);

// ???대옒?ㅻ뒗 ?곗궛?먯꽌 遺볥뒗 臾쇱쓣 諛쏆븘 ??ν븯怨??쇱쫹 議곌굔?대굹 臾닿쾶 蹂?붿뿉 ?섍꺼二쇰뒗 ????≫꽣瑜??대떦?쒕떎.
UCLASS(meta=(DisplayName="UOU Umbrella Water Target"))
class AUOUUmbrellaWaterTarget : public AActor
{
	GENERATED_BODY()

public:
	AUOUUmbrellaWaterTarget();

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "Water Target")
	FOnUOUUmbrellaWaterTargetChangedSignature OnWaterChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Target")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Target")
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Target")
	TObjectPtr<UUOUWaterContainerComponent> TargetWaterContainer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target", meta = (ClampMin = "0.0"))
	float RequiredWater = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	bool bAddWaterToPhysicsMass = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	TObjectPtr<UPrimitiveComponent> WeightedPrimitive = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	FLinearColor IdleColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	FLinearColor ActivatedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	FName PrimaryColorParameterName = TEXT("BaseColor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	FName SecondaryColorParameterName = TEXT("Color");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Target|Runtime")
	bool bIsActivated = false;

	UFUNCTION(BlueprintCallable, Category = "Water Target")
	void ReceiveWater(float WaterAmount);

	UFUNCTION(BlueprintPure, Category = "Water Target")
	float GetReceivedWater() const;

	UFUNCTION(BlueprintPure, Category = "Water Target")
	float GetAddedWeight() const;

protected:
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuntimeMaterialInstances;
	float BaseMassKg = 0.0f;

	void CacheWeightedPrimitive();
	void RefreshActivatedState();
	void RefreshMass();
	void RefreshVisual();
	void EnsureRuntimeMaterials();
	void BroadcastChanged();
};
