// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "UOUWaterIcePhaseActor.generated.h"

class USceneComponent;
class UBoxComponent;
class USphereComponent;
class UStaticMeshComponent;
class UUOULightExposureReceiverComponent;

UENUM(BlueprintType)
enum class EUOUWaterIcePhase : uint8
{
	Water UMETA(DisplayName = "물"),
	Ice UMETA(DisplayName = "얼음")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnUOUWaterIcePhaseChangedSignature,
	EUOUWaterIcePhase,
	NewPhase,
	EUOUWaterIcePhase,
	PreviousPhase);

// 온도에 따라 물과 얼음의 외형 및 충돌 상태를 전환하는 액터입니다.
UCLASS(meta = (DisplayName = "UOU Water Ice Phase Actor"))
class UNDERONEUMBRELLA_API AUOUWaterIcePhaseActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUWaterIcePhaseActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintAssignable, Category = "Water Ice|State", meta = (ToolTip = "물과 얼음 상태가 실제로 변경됐을 때 호출됩니다."))
	FOnUOUWaterIcePhaseChangedSignature OnPhaseChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Temperature", meta = (Units = "Celsius", ToolTip = "현재 상태가 물일 때, 이 온도 이하가 되면 얼음으로 변합니다."))
	float FreezeTemperature = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Temperature", meta = (Units = "Celsius", ToolTip = "현재 상태가 얼음일 때, 이 온도 이상이 되면 물로 변합니다. 상태 떨림을 막기 위해 동결 온도보다 높게 설정합니다."))
	float MeltTemperature = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|State", meta = (ToolTip = "시작 온도가 동결점과 융해점 사이일 때 사용할 초기 상태입니다."))
	EUOUWaterIcePhase InitialPhase = EUOUWaterIcePhase::Water;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Collision", meta = (ToolTip = "물 상태에서 Water Mesh에 적용할 충돌 모드입니다."))
	TEnumAsByte<ECollisionEnabled::Type> WaterCollisionEnabled = ECollisionEnabled::NoCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Collision", meta = (ToolTip = "얼음 상태에서 Ice Mesh에 적용할 충돌 모드입니다."))
	TEnumAsByte<ECollisionEnabled::Type> IceCollisionEnabled = ECollisionEnabled::QueryAndPhysics;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Ice|Runtime", meta = (ToolTip = "현재 물 또는 얼음 상태입니다."))
	EUOUWaterIcePhase CurrentPhase = EUOUWaterIcePhase::Water;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Ice|Runtime", meta = (ToolTip = "마지막 상태 판정에 사용한 온도입니다."))
	float LastEvaluatedTemperature = 20.0f;

	UFUNCTION(BlueprintCallable, Category = "Water Ice|State", meta = (ToolTip = "연결된 온도 수신 컴포넌트의 현재 온도로 상태를 즉시 다시 판정합니다."))
	void RefreshPhaseFromTemperature();

	UFUNCTION(BlueprintCallable, Category = "Water Ice|State", meta = (ToolTip = "온도 판정과 별개로 물 또는 얼음 상태를 직접 설정합니다."))
	void SetPhase(EUOUWaterIcePhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Water Ice|State")
	bool IsFrozen() const
	{
		return CurrentPhase == EUOUWaterIcePhase::Ice;
	}

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WaterMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> IceMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (ToolTip = "물 상태에서 플레이어의 진입을 막는 범위입니다. 얼음 상태에서는 자동으로 비활성화됩니다."))
	TObjectPtr<UBoxComponent> WaterBlockingVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (ToolTip = "물/얼음 메시의 충돌 상태와 무관하게 광원이 이 액터를 탐지하는 온도 수신 범위입니다."))
	TObjectPtr<USphereComponent> LightReceiverVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUOULightExposureReceiverComponent> TemperatureReceiver = nullptr;

	UFUNCTION()
	void HandleTemperatureChanged(float NewTemperature, float PreviousTemperature);

	void ValidateSettings();
	void ApplyPhaseState(bool bBroadcastChange, EUOUWaterIcePhase PreviousPhase);
	EUOUWaterIcePhase ResolveInitialPhase(float Temperature) const;
};
