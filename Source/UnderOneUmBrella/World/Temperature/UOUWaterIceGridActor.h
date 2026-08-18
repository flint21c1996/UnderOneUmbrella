// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Temperature/UOUWaterIcePhaseActor.h"
#include "UOUWaterIceGridActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UUOULightExposureSourceComponent;
class UUOUUmbrellaComponent;

// 많은 물·얼음 타일을 두 개의 HISM 컴포넌트로 묶어 관리하는 격자 액터입니다.
UCLASS(meta = (DisplayName = "UOU Water Ice Grid Actor"))
class UNDERONEUMBRELLA_API AUOUWaterIceGridActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUWaterIceGridActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Layout", meta = (ClampMin = "1", ToolTip = "격자의 X축 타일 개수입니다."))
	int32 GridSizeX = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Layout", meta = (ClampMin = "1", ToolTip = "격자의 Y축 타일 개수입니다."))
	int32 GridSizeY = 25;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Layout", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "타일 중심 사이의 간격입니다."))
	float TileSpacing = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Layout", meta = (ClampMin = "0.1", ClampMax = "1.0", ToolTip = "격자 한 칸에서 메시가 차지하는 가로·세로 비율입니다."))
	float TileFillRatio = 0.98f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Layout", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "물·얼음 타일의 기본 높이입니다."))
	float TileHeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature", meta = (Units = "Celsius"))
	float InitialTemperature = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature", meta = (Units = "Celsius"))
	float AmbientTemperature = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature", meta = (Units = "Celsius"))
	float MinimumTemperature = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature", meta = (Units = "Celsius"))
	float MaximumTemperature = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature", meta = (Units = "Celsius", ToolTip = "물 타일이 이 온도 이하가 되면 얼음으로 바뀝니다."))
	float FreezeTemperature = -5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature", meta = (Units = "Celsius", ToolTip = "얼음 타일이 이 온도 이상이 되면 물로 바뀝니다."))
	float MeltTemperature = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature", meta = (ClampMin = "0.0", ToolTip = "빛 세기 1당 초당 상승하는 온도입니다."))
	float LightHeatingRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature", meta = (ClampMin = "0.0", ToolTip = "빛과 우산 냉각이 없을 때 주변 온도로 돌아가는 초당 속도입니다."))
	float TemperatureRecoveryRate = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Umbrella", meta = (ClampMin = "0.0", ToolTip = "우산 동결 범위에서 초당 낮아지는 온도입니다."))
	float UmbrellaCoolingRate = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Umbrella", meta = (ClampMin = "0", ToolTip = "우산을 펼쳤을 때 동결할 반경입니다. 1이면 3x3입니다."))
	int32 OpenUmbrellaRadiusInTiles = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Umbrella", meta = (ClampMin = "0", ToolTip = "우산을 접었을 때 동결할 반경입니다. 0이면 현재 칸만 해당합니다."))
	int32 ClosedUmbrellaRadiusInTiles = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Visual", meta = (ClampMin = "0.5", ClampMax = "1.5"))
	float MinimumIceHeightScale = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Visual", meta = (ClampMin = "0.5", ClampMax = "1.5"))
	float MaximumIceHeightScale = 1.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Visual")
	int32 IceHeightPatternSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Visual")
	TObjectPtr<UStaticMesh> WaterMeshAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Visual")
	TObjectPtr<UStaticMesh> IceMeshAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Visual")
	TObjectPtr<UMaterialInterface> WaterMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Visual")
	TObjectPtr<UMaterialInterface> IceMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Runtime", meta = (ClampMin = "0.02", Units = "s", ToolTip = "격자 전체의 온도와 상태를 갱신하는 주기입니다."))
	float UpdateInterval = 0.1f;

	UFUNCTION(BlueprintCallable, Category = "Water Ice Grid", meta = (ToolTip = "현재 설정으로 격자 인스턴스를 다시 만듭니다."))
	void RebuildGrid();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WaterInstances = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> IceInstances = nullptr;

private:
	struct FTileRuntime
	{
		float Temperature = 0.0f;
		float IceHeightScale = 1.0f;
		EUOUWaterIcePhase Phase = EUOUWaterIcePhase::Water;
	};

	void ValidateSettings();
	void UpdateGrid();
	void RefreshLightSources();
	void ResolvePlayerReferences();
	float CalculateTileLightIntensity(const FVector& TileWorldLocation) const;
	bool IsPlayerBlockingLight(const FVector& RayStart, const FVector& TileWorldLocation) const;
	bool IsTileInsideUmbrellaArea(int32 TileX, int32 TileY) const;
	void ApplyTilePhase(int32 TileIndex, bool bMarkRenderStateDirty);
	FTransform MakeTileTransform(int32 TileIndex, EUOUWaterIcePhase Phase) const;
	FVector GetTileLocalLocation(int32 TileIndex) const;

	TArray<FTileRuntime> Tiles;
	TArray<TWeakObjectPtr<UUOULightExposureSourceComponent>> CachedLightSources;
	TWeakObjectPtr<AActor> CachedPlayerActor;
	TWeakObjectPtr<UUOUUmbrellaComponent> CachedUmbrellaComponent;
	float NextLightSourceRefreshTime = 0.0f;
	FTimerHandle UpdateTimerHandle;
};
