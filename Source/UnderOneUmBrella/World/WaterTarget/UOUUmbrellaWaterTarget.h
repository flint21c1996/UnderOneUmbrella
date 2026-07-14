// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Pour/UOUPourReceiverInterface.h"
#include "UOUUmbrellaWaterTarget.generated.h"

class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOUWaterContainerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUOUUmbrellaWaterTargetChangedSignature, float, CurrentWater, float, RequiredWater, bool, bIsActivated);

// 우산에서 부은 물을 저장하고 목표 수치 도달 여부를 관리하는 물 타겟 액터입니다.
// 물 저장 퍼즐이나 물을 담아 활성화하는 장치의 기본 베이스로 사용합니다.
UCLASS(meta=(DisplayName="UOU Umbrella Water Target"))
class AUOUUmbrellaWaterTarget : public AActor, public IUOUPourReceiver
{
	GENERATED_BODY()

public:
	AUOUUmbrellaWaterTarget();

	// 시작 시 물 컨테이너와 비주얼 상태를 초기화합니다.
	virtual void BeginPlay() override;

	// 물 양이 바뀔 때 외부에 현재 상태를 알리는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Water Target")
	FOnUOUUmbrellaWaterTargetChangedSignature OnWaterChanged;

	// 물 타겟 액터 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Target")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 물 타겟의 외형을 보여 주는 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Target")
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	// 실제로 저장된 물 양을 관리하는 컨테이너 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Target")
	TObjectPtr<UUOUWaterContainerComponent> TargetWaterContainer = nullptr;

	// 활성 상태가 되기 위해 필요한 최소 물 양입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target", meta = (ClampMin = "0.0"))
	float RequiredWater = 0.5f;

	// 저장된 물 양을 물리 질량에도 반영할지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	bool bAddWaterToPhysicsMass = true;

	// 질량을 실제로 변경할 프리미티브입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	TObjectPtr<UPrimitiveComponent> WeightedPrimitive = nullptr;

	// 비활성 상태일 때 사용할 기본 색입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	FLinearColor IdleColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

	// 활성 상태일 때 사용할 강조 색입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	FLinearColor ActivatedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);

	// 주 색상을 반영할 머티리얼 파라미터 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	FName PrimaryColorParameterName = TEXT("BaseColor");

	// 대체 색상 파라미터 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Target")
	FName SecondaryColorParameterName = TEXT("Color");

	// 현재 목표 물 양을 달성했는지 저장하는 런타임 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Target|Runtime")
	bool bIsActivated = false;

	// 외부에서 물을 받을 때 호출하는 진입점입니다.
	UFUNCTION(BlueprintCallable, Category = "Water Target")
	void ReceiveWater(float WaterAmount);

	virtual bool CanAcceptPour_Implementation(const FUOUPourInputContext& Context) const override;
	virtual FUOUPourReceiveResult TryReceivePour_Implementation(const FUOUPourInputContext& Context) override;
	virtual int32 GetPourReceivePriority_Implementation() const override;
	virtual bool CanAcceptPourAtLocation_Implementation(const FUOUPourInputContext& Context) const override;

	// 현재 저장된 물 양을 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Water Target")
	float GetReceivedWater() const;

	// 저장된 물 때문에 추가된 퍼즐용 무게를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Water Target")
	float GetAddedWeight() const;

protected:
	// 색상 파라미터를 갱신하기 위해 만든 런타임 머티리얼 인스턴스 목록입니다.
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuntimeMaterialInstances;

	// 물 추가 전 기본 질량을 저장해 두는 값입니다.
	float BaseMassKg = 0.0f;

	// 질량을 적용할 프리미티브를 다시 찾아 캐시합니다.
	void CacheWeightedPrimitive();

	// 현재 물 양과 요구량을 비교해 활성 상태를 갱신합니다.
	void RefreshActivatedState();

	// 저장된 물 양에 맞춰 실제 물리 질량을 갱신합니다.
	void RefreshMass();

	// 현재 활성 상태에 맞춰 비주얼 색을 갱신합니다.
	void RefreshVisual();

	// 색상 변경에 사용할 런타임 머티리얼 인스턴스를 준비합니다.
	void EnsureRuntimeMaterials();

	// 현재 물 양과 활성 상태를 외부에 방송합니다.
	void BroadcastChanged();
};
