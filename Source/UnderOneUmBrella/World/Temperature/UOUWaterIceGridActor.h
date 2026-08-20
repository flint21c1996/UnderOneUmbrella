// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightExposureTypes.h"
#include "World/Temperature/UOUWaterIcePhaseActor.h"
#include "UOUWaterIceGridActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UCapsuleComponent;
class UBoxComponent;
class USceneComponent;
class UStaticMesh;
class UUOULightExposureSourceComponent;
class UUOUUmbrellaComponent;

// 레벨에서는 하나의 물 필드로 배치하고 내부에서는 셀 데이터와 HISM으로 물·얼음을 관리합니다.
UCLASS(meta = (DisplayName = "UOU Water Ice Field"))
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
	float LightHeatingRate = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature", meta = (ClampMin = "0.0", ToolTip = "빛과 우산 냉각이 없을 때 주변 온도로 돌아가는 초당 속도입니다."))
	float TemperatureRecoveryRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature|Variation", meta = (ToolTip = "타일마다 온도 상승·하강 속도에 작은 차이를 적용합니다."))
	bool bEnableTemperatureRateVariation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature|Variation", meta = (ClampMin = "0.1", ClampMax = "2.0", ToolTip = "타일별 온도 상승 속도의 최소 배율입니다."))
	float MinimumHeatingRateMultiplier = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature|Variation", meta = (ClampMin = "0.1", ClampMax = "2.0", ToolTip = "타일별 온도 상승 속도의 최대 배율입니다."))
	float MaximumHeatingRateMultiplier = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature|Variation", meta = (ClampMin = "0.1", ClampMax = "2.0", ToolTip = "타일별 온도 하강 속도의 최소 배율입니다."))
	float MinimumCoolingRateMultiplier = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature|Variation", meta = (ClampMin = "0.1", ClampMax = "2.0", ToolTip = "타일별 온도 하강 속도의 최대 배율입니다."))
	float MaximumCoolingRateMultiplier = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Temperature|Variation", meta = (ToolTip = "타일별 온도 속도 패턴을 고정하는 시드입니다."))
	int32 TemperatureRatePatternSeed = 2718;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Umbrella", meta = (ClampMin = "0.0", ToolTip = "우산 동결 범위에서 초당 낮아지는 온도입니다."))
	float UmbrellaCoolingRate = 24.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Collision", meta = (ToolTip = "얼음 상태 타일에 플레이어와 물리 오브젝트 충돌을 적용합니다."))
	bool bEnableIceCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Collision", meta = (ClampMin = "100.0", Units = "cm", ToolTip = "물 타일에 진입하지 못하도록 만드는 투명 차단 충돌의 높이입니다."))
	float WaterBlockingHeight = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Collision", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "물과 얼음의 경계에 생성되는 투명 차단 벽의 두께입니다."))
	float WaterBoundaryThickness = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Collision", meta = (ToolTip = "플레이어가 차지한 물 타일이 얼 때 새 얼음 충돌 위로 발 위치를 보정해 끼임을 방지합니다."))
	bool bResolvePlayerOverlapOnFreeze = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Collision", meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "cm", ToolTip = "동결 시 플레이어 발과 얼음 상면 사이에 둘 여유 높이입니다."))
	float PlayerFreezeSurfaceClearance = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Runtime", meta = (ClampMin = "0.02", Units = "s", ToolTip = "격자 전체의 온도와 상태를 갱신하는 주기입니다."))
	float UpdateInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Runtime", meta = (ClampMin = "1", ToolTip = "충돌을 부분 갱신할 청크 한 변의 타일 수입니다."))
	int32 CollisionChunkSize = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Runtime|Light Cache", meta = (ClampMin = "0.02", Units = "s", ToolTip = "움직이는 빛 경로 캐시를 다시 계산할 수 있는 최소 간격입니다. 0.1초면 최대 10Hz입니다."))
	float LightCacheMinimumRefreshInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Runtime|Light Cache", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "이 거리 이상 빛 경로가 이동했을 때 캐시를 다시 계산합니다."))
	float LightCachePositionThreshold = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Runtime|Light Cache", meta = (ClampMin = "0.0", Units = "deg", ToolTip = "이 각도 이상 빛 방향이 바뀌었을 때 캐시를 다시 계산합니다."))
	float LightCacheDirectionThreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Runtime|Light Cache", meta = (ClampMin = "0.0", Units = "s", ToolTip = "빛 이동이 멈춘 뒤 마지막 정확한 캐시 갱신을 보장하기 위한 대기 시간입니다."))
	float LightCacheSettleDelay = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Debug", meta = (ToolTip = "활성 타일, 빛 검사, 충돌 청크 통계를 로컬 Output Log에만 출력합니다."))
	bool bEnableLocalPerformanceLogging = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Ice Grid|Debug", meta = (ClampMin = "0.2", Units = "s"))
	float PerformanceLogInterval = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Water Ice Grid", meta = (ToolTip = "현재 설정으로 격자 인스턴스를 다시 만듭니다."))
	void RebuildGrid();

	UFUNCTION(BlueprintPure, Category = "Water Ice Grid", meta = (ToolTip = "월드 위치가 필드 안에 있으면 해당 셀 좌표를 반환합니다."))
	bool WorldLocationToTileCoordinates(const FVector& WorldLocation, int32& OutTileX, int32& OutTileY) const;

	UFUNCTION(BlueprintPure, Category = "Water Ice Grid", meta = (ToolTip = "지정한 셀의 현재 온도를 반환합니다. 범위 밖이면 주변 온도를 반환합니다."))
	float GetTileTemperature(int32 TileX, int32 TileY) const;

	UFUNCTION(BlueprintPure, Category = "Water Ice Grid", meta = (ToolTip = "지정한 셀이 얼음 상태인지 반환합니다."))
	bool IsTileFrozen(int32 TileX, int32 TileY) const;

	UFUNCTION(BlueprintCallable, Category = "Water Ice Grid", meta = (ToolTip = "지정한 셀의 온도를 설정하고 물·얼음 상태를 즉시 다시 판정합니다."))
	bool SetTileTemperature(int32 TileX, int32 TileY, float NewTemperature);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (ToolTip = "에디터에서 전체 물 필드 범위를 표시하는 충돌 없는 박스입니다."))
	TObjectPtr<UBoxComponent> FieldBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WaterInstances = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> IceInstances = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (ToolTip = "서로 붙은 얼음 타일을 직사각형으로 병합한 보이지 않는 충돌입니다."))
	TObjectPtr<UInstancedStaticMeshComponent> MergedIceCollisionInstances = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> WaterBlockingInstances = nullptr;

private:
	struct FTileRuntime
	{
		float Temperature = 0.0f;
		float IceHeightScale = 1.0f;
		float HeatingRateMultiplier = 1.0f;
		float CoolingRateMultiplier = 1.0f;
		float CachedLightIntensity = 0.0f;
		EUOUWaterIcePhase Phase = EUOUWaterIcePhase::Water;
	};

	struct FLightSegmentSnapshot
	{
		FVector Start = FVector::ZeroVector;
		FVector Direction = FVector::ForwardVector;
		float Length = 0.0f;
		float StartRadius = 0.0f;
		float EndRadius = 0.0f;
		float Intensity = 0.0f;
	};

	void ValidateSettings();
	void ConfigureInstanceCollision();
	void UpdateFieldBounds();
	void UpdateGrid();
	void RefreshLightSources();
	void UnbindLightSources();
	void ResolvePlayerReferences();
	void RefreshTileLightIntensityCache();
	void CaptureLightPathSnapshot();
	bool HaveLightPathsChangedBeyondThreshold() const;
	void AddUmbrellaAffectedTiles(TSet<int32>& OutTileIndices) const;
	void ResolvePlayerOverlapWithNewIce(int32 TileIndex);
	float CalculateTileLightIntensity(const FVector& TileWorldLocation, bool bCheckPlayerOcclusion) const;
	bool IsPlayerBlockingLight(const FVector& RayStart, const FVector& TileWorldLocation) const;
	void ApplyTilePhase(int32 TileIndex, bool bMarkRenderStateDirty);
	void EvaluateTilePhase(int32 TileIndex, bool bMarkRenderStateDirty);
	void EnsureCollisionChunkComponents();
	void DestroyDynamicCollisionChunkComponents();
	void RebuildAllCollisionChunks();
	void RebuildCollisionChunks(const TSet<int32>& ChunkIndices);
	void RebuildWaterBoundaryCollisionChunk(int32 ChunkIndex);
	void RebuildMergedIceCollisionChunk(int32 ChunkIndex);
	void AddAffectedCollisionChunks(int32 TileIndex, TSet<int32>& OutChunkIndices) const;
	int32 GetCollisionChunkIndex(int32 TileX, int32 TileY) const;
	int32 GetCollisionChunkCountX() const;
	int32 GetCollisionChunkCountY() const;
	void LogLocalPerformanceStats();
	FTransform MakeTileTransform(int32 TileIndex, EUOUWaterIcePhase Phase) const;
	FTransform MakeMergedIceCollisionTransform(int32 StartX, int32 StartY, int32 Width, int32 Height) const;
	FTransform MakeWaterBoundaryTransform(int32 TileIndex, int32 OffsetX, int32 OffsetY) const;
	FVector GetTileLocalLocation(int32 TileIndex) const;
	int32 GetTileIndex(int32 TileX, int32 TileY) const;

	UFUNCTION()
	void HandleLightPathsUpdated(const TArray<FUOULightPathData>& LightPaths);

	TArray<FTileRuntime> Tiles;
	TSet<int32> ActiveTileIndices;
	TArray<TWeakObjectPtr<UUOULightExposureSourceComponent>> CachedLightSources;
	TArray<FLightSegmentSnapshot> CachedLightPathSnapshot;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> IceCollisionChunkComponents;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> WaterCollisionChunkComponents;
	TWeakObjectPtr<AActor> CachedPlayerActor;
	TWeakObjectPtr<UUOUUmbrellaComponent> CachedUmbrellaComponent;
	bool bLightIntensityCacheDirty = true;
	bool bLightCacheRefreshRequested = false;
	float NextLightSourceRefreshTime = 0.0f;
	float NextAllowedLightCacheRefreshTime = 0.0f;
	float LastLightPathUpdateTime = 0.0f;
	float NextPerformanceLogTime = 0.0f;
	int32 LightCacheRefreshesSinceLog = 0;
	int32 CollisionChunksRebuiltSinceLog = 0;
	mutable int64 LightSegmentTestsSinceLog = 0;
	FTimerHandle UpdateTimerHandle;
};
