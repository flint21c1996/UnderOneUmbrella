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
class UUOUUmbrellaComponent;

UENUM(BlueprintType)
enum class EUOUWaterIcePhase : uint8
{
	Water UMETA(DisplayName = "물"),
	Ice UMETA(DisplayName = "얼음")
};

UENUM(BlueprintType)
enum class EUOUWaterIceControlMode : uint8
{
	Temperature UMETA(DisplayName = "온도 기반"),
	PlayerUmbrellaArea UMETA(DisplayName = "플레이어 우산 범위")
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Control", meta = (ToolTip = "온도로 상태를 바꿀지, 플레이어 위치와 우산 상태로 상태를 바꿀지 선택합니다."))
	EUOUWaterIceControlMode ControlMode = EUOUWaterIceControlMode::Temperature;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Umbrella Area", meta = (ClampMin = "1.0", Units = "cm", EditCondition = "ControlMode == EUOUWaterIceControlMode::PlayerUmbrellaArea", EditConditionHides, ToolTip = "격자 한 칸의 가로·세로 크기입니다."))
	float TileWorldSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Umbrella Area", meta = (ClampMin = "0", EditCondition = "ControlMode == EUOUWaterIceControlMode::PlayerUmbrellaArea", EditConditionHides, ToolTip = "우산을 펼쳤을 때 플레이어 타일에서 몇 칸까지 얼릴지 정합니다. 1이면 3x3입니다."))
	int32 OpenUmbrellaRadiusInTiles = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Umbrella Area", meta = (ClampMin = "0", EditCondition = "ControlMode == EUOUWaterIceControlMode::PlayerUmbrellaArea", EditConditionHides, ToolTip = "우산을 접었을 때 플레이어 타일에서 몇 칸까지 얼릴지 정합니다. 0이면 현재 칸만 얼립니다."))
	int32 ClosedUmbrellaRadiusInTiles = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Umbrella Area", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "ControlMode == EUOUWaterIceControlMode::PlayerUmbrellaArea", EditConditionHides, ToolTip = "위아래 층에 있는 다른 타일까지 함께 얼지 않도록 허용할 높이 차이를 제한합니다."))
	float PlayerVerticalTolerance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Umbrella Area", meta = (ClampMin = "0.02", Units = "s", EditCondition = "ControlMode == EUOUWaterIceControlMode::PlayerUmbrellaArea", EditConditionHides, ToolTip = "플레이어 위치와 우산 상태를 다시 확인하는 주기입니다."))
	float UmbrellaAreaRefreshInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Umbrella Area", meta = (ClampMin = "0.0", EditCondition = "ControlMode == EUOUWaterIceControlMode::PlayerUmbrellaArea", EditConditionHides, ToolTip = "우산 동결 범위 안에서 빛을 받지 않는 타일의 온도를 초당 낮추는 양입니다."))
	float UmbrellaCoolingRate = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Visual", meta = (ToolTip = "격자 위치에 따라 얼음 높이에 작고 고정된 차이를 적용합니다."))
	bool bUseIceHeightVariation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Visual", meta = (ClampMin = "0.5", ClampMax = "1.5", EditCondition = "bUseIceHeightVariation", ToolTip = "얼음 높이 배율의 최솟값입니다."))
	float MinimumIceHeightScale = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Visual", meta = (ClampMin = "0.5", ClampMax = "1.5", EditCondition = "bUseIceHeightVariation", ToolTip = "얼음 높이 배율의 최댓값입니다."))
	float MaximumIceHeightScale = 1.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice|Visual", meta = (EditCondition = "bUseIceHeightVariation", ToolTip = "같은 격자에서도 다른 높이 패턴이 필요할 때 변경하는 시드입니다."))
	int32 IceHeightPatternSeed = 1337;

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

	UFUNCTION(BlueprintCallable, Category = "Water Ice|State", meta = (ToolTip = "플레이어 위치와 우산 상태를 확인하고, 범위 안의 타일 온도를 한 주기만큼 낮춥니다."))
	void RefreshPhaseFromUmbrellaArea();

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
	void ResolvePlayerReferences();
	bool IsInsidePlayerTileRange(const FVector& PlayerLocation, int32 RadiusInTiles) const;
	void CacheIceMeshBaseTransform();
	void ApplyIceHeightVariation();
	float ResolveIceHeightScale() const;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CachedPlayerActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUUmbrellaComponent> CachedUmbrellaComponent = nullptr;

	FVector BaseIceMeshRelativeLocation = FVector::ZeroVector;
	FVector BaseIceMeshRelativeScale = FVector::OneVector;
	bool bHasCachedIceMeshBaseTransform = false;
	FTimerHandle UmbrellaAreaRefreshTimerHandle;
};
