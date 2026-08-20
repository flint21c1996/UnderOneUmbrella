// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Temperature/UOUWaterIceGridActor.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Player/UOUUmbrellaComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"
#include "World/Light/UOULightExposureSourceComponent.h"

AUOUWaterIceGridActor::AUOUWaterIceGridActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.AddUnique(TEXT("UOUWaterIceLightOccluder"));

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	FieldBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("FieldBounds"));
	FieldBounds->SetupAttachment(RootScene);
	FieldBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FieldBounds->SetGenerateOverlapEvents(false);
	FieldBounds->SetHiddenInGame(true);

	WaterInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WaterInstances"));
	WaterInstances->SetupAttachment(RootScene);
	WaterInstances->SetGenerateOverlapEvents(false);
	WaterInstances->SetMobility(EComponentMobility::Movable);

	IceInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("IceInstances"));
	IceInstances->SetupAttachment(RootScene);
	IceInstances->SetGenerateOverlapEvents(false);
	IceInstances->SetMobility(EComponentMobility::Movable);

	MergedIceCollisionInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MergedIceCollisionInstances"));
	MergedIceCollisionInstances->SetupAttachment(RootScene);
	MergedIceCollisionInstances->SetGenerateOverlapEvents(false);
	MergedIceCollisionInstances->SetMobility(EComponentMobility::Movable);
	MergedIceCollisionInstances->SetHiddenInGame(true);
	MergedIceCollisionInstances->SetVisibility(false, true);
	MergedIceCollisionInstances->SetCanEverAffectNavigation(false);

	WaterBlockingInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WaterBlockingInstances"));
	WaterBlockingInstances->SetupAttachment(RootScene);
	WaterBlockingInstances->SetGenerateOverlapEvents(false);
	WaterBlockingInstances->SetMobility(EComponentMobility::Movable);
	WaterBlockingInstances->SetHiddenInGame(true);
	WaterBlockingInstances->SetVisibility(false, true);
	WaterBlockingInstances->SetCanEverAffectNavigation(false);
	ConfigureInstanceCollision();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		WaterMeshAsset = CubeMeshFinder.Object;
		IceMeshAsset = CubeMeshFinder.Object;
		MergedIceCollisionInstances->SetStaticMesh(CubeMeshFinder.Object);
		WaterBlockingInstances->SetStaticMesh(CubeMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaterMaterialFinder(
		TEXT("/Game/UOU/BluePrint/World/Lights/MI_UOU_Water_Blue.MI_UOU_Water_Blue"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> IceMaterialFinder(
		TEXT("/Game/UOU/BluePrint/World/Lights/MI_UOU_Ice_White.MI_UOU_Ice_White"));
	WaterMaterial = WaterMaterialFinder.Object;
	IceMaterial = IceMaterialFinder.Object;
}

void AUOUWaterIceGridActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ValidateSettings();
	ConfigureInstanceCollision();
	UpdateFieldBounds();
	RebuildGrid();
}

void AUOUWaterIceGridActor::BeginPlay()
{
	Super::BeginPlay();
	ValidateSettings();
	ConfigureInstanceCollision();
	UpdateFieldBounds();
	RebuildGrid();
	ResolvePlayerReferences();
	RefreshLightSources();
	GetWorldTimerManager().SetTimer(UpdateTimerHandle, this, &AUOUWaterIceGridActor::UpdateGrid, UpdateInterval, true);
}

void AUOUWaterIceGridActor::ConfigureInstanceCollision()
{
	if (WaterInstances != nullptr)
	{
		// 물은 시각 표현만 담당하며 발판 충돌을 만들지 않습니다.
		WaterInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WaterInstances->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (IceInstances != nullptr)
	{
		// 화면에 보이는 개별 얼음 인스턴스는 렌더링만 담당합니다.
		IceInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		IceInstances->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (MergedIceCollisionInstances != nullptr)
	{
		MergedIceCollisionInstances->SetCollisionEnabled(
			bEnableIceCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		MergedIceCollisionInstances->SetCollisionResponseToAllChannels(ECR_Block);
		MergedIceCollisionInstances->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	for (UInstancedStaticMeshComponent* Component : IceCollisionChunkComponents)
	{
		if (Component != nullptr && Component != MergedIceCollisionInstances)
		{
			Component->SetCollisionEnabled(bEnableIceCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
			Component->SetCollisionResponseToAllChannels(ECR_Block);
			Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
	}

	if (WaterBlockingInstances != nullptr)
	{
		WaterBlockingInstances->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		WaterBlockingInstances->SetCollisionResponseToAllChannels(ECR_Ignore);
		WaterBlockingInstances->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
	for (UInstancedStaticMeshComponent* Component : WaterCollisionChunkComponents)
	{
		if (Component != nullptr && Component != WaterBlockingInstances)
		{
			Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Component->SetCollisionResponseToAllChannels(ECR_Ignore);
			Component->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
	}
}

void AUOUWaterIceGridActor::UpdateFieldBounds()
{
	if (FieldBounds == nullptr)
	{
		return;
	}

	const FVector Extent(
		GridSizeX * TileSpacing * 0.5f,
		GridSizeY * TileSpacing * 0.5f,
		TileHeight * MaximumIceHeightScale * 0.5f);
	FieldBounds->SetBoxExtent(Extent);
	FieldBounds->SetRelativeLocation(FVector(0.0f, 0.0f, Extent.Z));
}

void AUOUWaterIceGridActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(UpdateTimerHandle);
	UnbindLightSources();
	DestroyDynamicCollisionChunkComponents();
	Super::EndPlay(EndPlayReason);
}

void AUOUWaterIceGridActor::ValidateSettings()
{
	GridSizeX = FMath::Max(1, GridSizeX);
	GridSizeY = FMath::Max(1, GridSizeY);
	TileSpacing = FMath::Max(1.0f, TileSpacing);
	TileFillRatio = FMath::Clamp(TileFillRatio, 0.1f, 1.0f);
	TileHeight = FMath::Max(1.0f, TileHeight);
	WaterBlockingHeight = FMath::Max(100.0f, WaterBlockingHeight);
	WaterBoundaryThickness = FMath::Clamp(WaterBoundaryThickness, 1.0f, TileSpacing * 0.5f);
	UpdateInterval = FMath::Max(0.02f, UpdateInterval);
	CollisionChunkSize = FMath::Max(1, CollisionChunkSize);
	LightCacheMinimumRefreshInterval = FMath::Max(0.02f, LightCacheMinimumRefreshInterval);
	LightCachePositionThreshold = FMath::Max(0.0f, LightCachePositionThreshold);
	LightCacheDirectionThreshold = FMath::Clamp(LightCacheDirectionThreshold, 0.0f, 180.0f);
	LightCacheSettleDelay = FMath::Max(0.0f, LightCacheSettleDelay);
	PerformanceLogInterval = FMath::Max(0.2f, PerformanceLogInterval);
	MinimumHeatingRateMultiplier = FMath::Clamp(MinimumHeatingRateMultiplier, 0.1f, 2.0f);
	MaximumHeatingRateMultiplier = FMath::Clamp(MaximumHeatingRateMultiplier, 0.1f, 2.0f);
	MinimumCoolingRateMultiplier = FMath::Clamp(MinimumCoolingRateMultiplier, 0.1f, 2.0f);
	MaximumCoolingRateMultiplier = FMath::Clamp(MaximumCoolingRateMultiplier, 0.1f, 2.0f);
	PlayerFreezeSurfaceClearance = FMath::Clamp(PlayerFreezeSurfaceClearance, 0.0f, 10.0f);
	if (MinimumHeatingRateMultiplier > MaximumHeatingRateMultiplier)
	{
		Swap(MinimumHeatingRateMultiplier, MaximumHeatingRateMultiplier);
	}
	if (MinimumCoolingRateMultiplier > MaximumCoolingRateMultiplier)
	{
		Swap(MinimumCoolingRateMultiplier, MaximumCoolingRateMultiplier);
	}
	if (MinimumTemperature > MaximumTemperature)
	{
		Swap(MinimumTemperature, MaximumTemperature);
	}
	FreezeTemperature = FMath::Clamp(FreezeTemperature, MinimumTemperature, MaximumTemperature);
	MeltTemperature = FMath::Clamp(MeltTemperature, FreezeTemperature, MaximumTemperature);
	InitialTemperature = FMath::Clamp(InitialTemperature, MinimumTemperature, MaximumTemperature);
	AmbientTemperature = FMath::Clamp(AmbientTemperature, MinimumTemperature, MaximumTemperature);
	if (MinimumIceHeightScale > MaximumIceHeightScale)
	{
		Swap(MinimumIceHeightScale, MaximumIceHeightScale);
	}
}

void AUOUWaterIceGridActor::RebuildGrid()
{
	if (WaterInstances == nullptr || IceInstances == nullptr ||
		MergedIceCollisionInstances == nullptr || WaterBlockingInstances == nullptr)
	{
		return;
	}
	EnsureCollisionChunkComponents();

	WaterInstances->SetStaticMesh(WaterMeshAsset);
	IceInstances->SetStaticMesh(IceMeshAsset);
	if (WaterMaterial != nullptr)
	{
		WaterInstances->SetMaterial(0, WaterMaterial);
	}
	if (IceMaterial != nullptr)
	{
		IceInstances->SetMaterial(0, IceMaterial);
	}

	WaterInstances->ClearInstances();
	IceInstances->ClearInstances();
	MergedIceCollisionInstances->ClearInstances();
	WaterBlockingInstances->ClearInstances();
	Tiles.SetNum(GridSizeX * GridSizeY);
	ActiveTileIndices.Reset();
	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		FTileRuntime& Tile = Tiles[TileIndex];
		Tile.Temperature = InitialTemperature;
		Tile.CachedLightIntensity = 0.0f;
		Tile.Phase = InitialTemperature <= FreezeTemperature
			? EUOUWaterIcePhase::Ice
			: EUOUWaterIcePhase::Water;
		const FVector LocalLocation = GetTileLocalLocation(TileIndex);
		const int32 TileX = TileIndex % GridSizeX;
		const int32 TileY = TileIndex / GridSizeX;
		FRandomStream HeightRandom(IceHeightPatternSeed + TileX * 73856093 + TileY * 19349663);
		Tile.IceHeightScale = HeightRandom.FRandRange(MinimumIceHeightScale, MaximumIceHeightScale);
		FRandomStream TemperatureRateRandom(
			TemperatureRatePatternSeed + TileX * 83492791 + TileY * 297657976);
		Tile.HeatingRateMultiplier = bEnableTemperatureRateVariation
			? TemperatureRateRandom.FRandRange(MinimumHeatingRateMultiplier, MaximumHeatingRateMultiplier)
			: 1.0f;
		Tile.CoolingRateMultiplier = bEnableTemperatureRateVariation
			? TemperatureRateRandom.FRandRange(MinimumCoolingRateMultiplier, MaximumCoolingRateMultiplier)
			: 1.0f;

		const FTransform WaterTransform = MakeTileTransform(TileIndex, EUOUWaterIcePhase::Water);
		const FTransform IceTransform = MakeTileTransform(TileIndex, EUOUWaterIcePhase::Ice);
		WaterInstances->AddInstance(Tile.Phase == EUOUWaterIcePhase::Water ? WaterTransform : FTransform(FQuat::Identity, LocalLocation, FVector::ZeroVector));
		IceInstances->AddInstance(Tile.Phase == EUOUWaterIcePhase::Ice ? IceTransform : FTransform(FQuat::Identity, LocalLocation, FVector::ZeroVector));
		if (!FMath::IsNearlyEqual(Tile.Temperature, AmbientTemperature, 0.01f))
		{
			ActiveTileIndices.Add(TileIndex);
		}
	}
	RebuildAllCollisionChunks();
	bLightIntensityCacheDirty = true;
	bLightCacheRefreshRequested = false;
	CachedLightPathSnapshot.Reset();
}

int32 AUOUWaterIceGridActor::GetTileIndex(int32 TileX, int32 TileY) const
{
	return TileX >= 0 && TileX < GridSizeX && TileY >= 0 && TileY < GridSizeY
		? TileY * GridSizeX + TileX
		: INDEX_NONE;
}

bool AUOUWaterIceGridActor::WorldLocationToTileCoordinates(
	const FVector& WorldLocation,
	int32& OutTileX,
	int32& OutTileY) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	OutTileX = FMath::RoundToInt(LocalLocation.X / TileSpacing + (GridSizeX - 1) * 0.5f);
	OutTileY = FMath::RoundToInt(LocalLocation.Y / TileSpacing + (GridSizeY - 1) * 0.5f);
	return GetTileIndex(OutTileX, OutTileY) != INDEX_NONE;
}

float AUOUWaterIceGridActor::GetTileTemperature(int32 TileX, int32 TileY) const
{
	const int32 TileIndex = GetTileIndex(TileX, TileY);
	return Tiles.IsValidIndex(TileIndex) ? Tiles[TileIndex].Temperature : AmbientTemperature;
}

bool AUOUWaterIceGridActor::IsTileFrozen(int32 TileX, int32 TileY) const
{
	const int32 TileIndex = GetTileIndex(TileX, TileY);
	return Tiles.IsValidIndex(TileIndex) && Tiles[TileIndex].Phase == EUOUWaterIcePhase::Ice;
}

bool AUOUWaterIceGridActor::SetTileTemperature(int32 TileX, int32 TileY, float NewTemperature)
{
	const int32 TileIndex = GetTileIndex(TileX, TileY);
	if (!Tiles.IsValidIndex(TileIndex))
	{
		return false;
	}

	Tiles[TileIndex].Temperature = FMath::Clamp(NewTemperature, MinimumTemperature, MaximumTemperature);
	ActiveTileIndices.Add(TileIndex);
	EvaluateTilePhase(TileIndex, true);
	return true;
}

FVector AUOUWaterIceGridActor::GetTileLocalLocation(int32 TileIndex) const
{
	const int32 TileX = TileIndex % GridSizeX;
	const int32 TileY = TileIndex / GridSizeX;
	return FVector(
		(static_cast<float>(TileX) - (GridSizeX - 1) * 0.5f) * TileSpacing,
		(static_cast<float>(TileY) - (GridSizeY - 1) * 0.5f) * TileSpacing,
		0.0f);
}

FTransform AUOUWaterIceGridActor::MakeTileTransform(int32 TileIndex, EUOUWaterIcePhase Phase) const
{
	const UStaticMesh* Mesh = Phase == EUOUWaterIcePhase::Ice ? IceMeshAsset : WaterMeshAsset;
	const FVector MeshSize = Mesh != nullptr ? Mesh->GetBoundingBox().GetSize() : FVector(100.0f);
	const float HeightScale = Phase == EUOUWaterIcePhase::Ice ? Tiles[TileIndex].IceHeightScale : 1.0f;
	const FVector DesiredSize(TileSpacing * TileFillRatio, TileSpacing * TileFillRatio, TileHeight * HeightScale);
	const FVector Scale(
		DesiredSize.X / FMath::Max(1.0f, MeshSize.X),
		DesiredSize.Y / FMath::Max(1.0f, MeshSize.Y),
		DesiredSize.Z / FMath::Max(1.0f, MeshSize.Z));
	FVector Location = GetTileLocalLocation(TileIndex);
	Location.Z = DesiredSize.Z * 0.5f;
	return FTransform(FQuat::Identity, Location, Scale);
}

FTransform AUOUWaterIceGridActor::MakeWaterBoundaryTransform(
	int32 TileIndex,
	int32 OffsetX,
	int32 OffsetY) const
{
	const UStaticMesh* CollisionMesh = WaterBlockingInstances != nullptr
		? WaterBlockingInstances->GetStaticMesh()
		: nullptr;
	const FVector MeshSize = CollisionMesh != nullptr ? CollisionMesh->GetBoundingBox().GetSize() : FVector(100.0f);
	const bool bVerticalBoundary = OffsetX != 0;
	const FVector DesiredSize(
		bVerticalBoundary ? WaterBoundaryThickness : TileSpacing,
		bVerticalBoundary ? TileSpacing : WaterBoundaryThickness,
		WaterBlockingHeight);
	const FVector Scale(
		DesiredSize.X / FMath::Max(1.0f, MeshSize.X),
		DesiredSize.Y / FMath::Max(1.0f, MeshSize.Y),
		DesiredSize.Z / FMath::Max(1.0f, MeshSize.Z));
	FVector Location = GetTileLocalLocation(TileIndex);
	Location.X += static_cast<float>(OffsetX) * TileSpacing * 0.5f;
	Location.Y += static_cast<float>(OffsetY) * TileSpacing * 0.5f;
	Location.Z = WaterBlockingHeight * 0.5f;
	return FTransform(FQuat::Identity, Location, Scale);
}

int32 AUOUWaterIceGridActor::GetCollisionChunkCountX() const
{
	return FMath::DivideAndRoundUp(GridSizeX, CollisionChunkSize);
}

int32 AUOUWaterIceGridActor::GetCollisionChunkCountY() const
{
	return FMath::DivideAndRoundUp(GridSizeY, CollisionChunkSize);
}

int32 AUOUWaterIceGridActor::GetCollisionChunkIndex(int32 TileX, int32 TileY) const
{
	if (TileX < 0 || TileX >= GridSizeX || TileY < 0 || TileY >= GridSizeY)
	{
		return INDEX_NONE;
	}
	return (TileY / CollisionChunkSize) * GetCollisionChunkCountX() + TileX / CollisionChunkSize;
}

void AUOUWaterIceGridActor::DestroyDynamicCollisionChunkComponents()
{
	for (UInstancedStaticMeshComponent* Component : IceCollisionChunkComponents)
	{
		if (Component != nullptr && Component != MergedIceCollisionInstances)
		{
			Component->DestroyComponent();
		}
	}
	for (UInstancedStaticMeshComponent* Component : WaterCollisionChunkComponents)
	{
		if (Component != nullptr && Component != WaterBlockingInstances)
		{
			Component->DestroyComponent();
		}
	}
	IceCollisionChunkComponents.Reset();
	WaterCollisionChunkComponents.Reset();
}

void AUOUWaterIceGridActor::EnsureCollisionChunkComponents()
{
	const int32 RequiredChunkCount = GetCollisionChunkCountX() * GetCollisionChunkCountY();
	if (IceCollisionChunkComponents.Num() == RequiredChunkCount &&
		WaterCollisionChunkComponents.Num() == RequiredChunkCount)
	{
		return;
	}

	DestroyDynamicCollisionChunkComponents();
	IceCollisionChunkComponents.Add(MergedIceCollisionInstances);
	WaterCollisionChunkComponents.Add(WaterBlockingInstances);
	for (int32 ChunkIndex = 1; ChunkIndex < RequiredChunkCount; ++ChunkIndex)
	{
		UInstancedStaticMeshComponent* IceChunk = NewObject<UInstancedStaticMeshComponent>(
			this,
			*FString::Printf(TEXT("IceCollisionChunk_%d"), ChunkIndex),
			RF_Transient);
		IceChunk->SetupAttachment(RootScene);
		IceChunk->SetStaticMesh(MergedIceCollisionInstances->GetStaticMesh());
		IceChunk->SetGenerateOverlapEvents(false);
		IceChunk->SetMobility(EComponentMobility::Movable);
		IceChunk->SetHiddenInGame(true);
		IceChunk->SetVisibility(false, true);
		IceChunk->SetCanEverAffectNavigation(false);
		AddInstanceComponent(IceChunk);
		IceChunk->RegisterComponent();
		IceCollisionChunkComponents.Add(IceChunk);

		UInstancedStaticMeshComponent* WaterChunk = NewObject<UInstancedStaticMeshComponent>(
			this,
			*FString::Printf(TEXT("WaterCollisionChunk_%d"), ChunkIndex),
			RF_Transient);
		WaterChunk->SetupAttachment(RootScene);
		WaterChunk->SetStaticMesh(WaterBlockingInstances->GetStaticMesh());
		WaterChunk->SetGenerateOverlapEvents(false);
		WaterChunk->SetMobility(EComponentMobility::Movable);
		WaterChunk->SetHiddenInGame(true);
		WaterChunk->SetVisibility(false, true);
		WaterChunk->SetCanEverAffectNavigation(false);
		AddInstanceComponent(WaterChunk);
		WaterChunk->RegisterComponent();
		WaterCollisionChunkComponents.Add(WaterChunk);
	}
	ConfigureInstanceCollision();
}

void AUOUWaterIceGridActor::RebuildWaterBoundaryCollisionChunk(int32 ChunkIndex)
{
	if (!WaterCollisionChunkComponents.IsValidIndex(ChunkIndex))
	{
		return;
	}

	UInstancedStaticMeshComponent* Component = WaterCollisionChunkComponents[ChunkIndex];
	Component->ClearInstances();
	static constexpr int32 NeighborOffsets[4][2] =
	{
		{ -1, 0 },
		{ 1, 0 },
		{ 0, -1 },
		{ 0, 1 }
	};

	const int32 ChunkX = ChunkIndex % GetCollisionChunkCountX();
	const int32 ChunkY = ChunkIndex / GetCollisionChunkCountX();
	const int32 StartX = ChunkX * CollisionChunkSize;
	const int32 StartY = ChunkY * CollisionChunkSize;
	const int32 EndX = FMath::Min(StartX + CollisionChunkSize, GridSizeX);
	const int32 EndY = FMath::Min(StartY + CollisionChunkSize, GridSizeY);
	for (int32 TileY = StartY; TileY < EndY; ++TileY)
	{
		for (int32 TileX = StartX; TileX < EndX; ++TileX)
		{
			const int32 TileIndex = GetTileIndex(TileX, TileY);
			if (Tiles[TileIndex].Phase != EUOUWaterIcePhase::Water)
			{
				continue;
			}
			for (const int32* Offset : NeighborOffsets)
			{
				const int32 NeighborIndex = GetTileIndex(TileX + Offset[0], TileY + Offset[1]);
				const bool bNeighborIsWater = Tiles.IsValidIndex(NeighborIndex) &&
					Tiles[NeighborIndex].Phase == EUOUWaterIcePhase::Water;
				if (!bNeighborIsWater)
				{
					Component->AddInstance(MakeWaterBoundaryTransform(TileIndex, Offset[0], Offset[1]));
				}
			}
		}
	}
}

void AUOUWaterIceGridActor::ResolvePlayerReferences()
{
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);
	CachedPlayerActor = PlayerActor;
	CachedUmbrellaComponent = PlayerActor != nullptr
		? PlayerActor->FindComponentByClass<UUOUUmbrellaComponent>()
		: nullptr;
}

void AUOUWaterIceGridActor::ResolvePlayerOverlapWithNewIce(int32 TileIndex)
{
	if (!bResolvePlayerOverlapOnFreeze || !Tiles.IsValidIndex(TileIndex))
	{
		return;
	}

	AActor* PlayerActor = CachedPlayerActor.Get();
	if (PlayerActor == nullptr)
	{
		ResolvePlayerReferences();
		PlayerActor = CachedPlayerActor.Get();
	}
	UCapsuleComponent* Capsule = PlayerActor != nullptr
		? PlayerActor->FindComponentByClass<UCapsuleComponent>()
		: nullptr;
	if (Capsule == nullptr)
	{
		return;
	}

	const FVector TileLocalLocation = GetTileLocalLocation(TileIndex);
	const FVector PlayerLocalLocation = GetActorTransform().InverseTransformPosition(PlayerActor->GetActorLocation());
	const FVector ActorScale = GetActorScale3D().GetAbs();
	const float MinimumHorizontalScale = FMath::Max(0.01f, FMath::Min(ActorScale.X, ActorScale.Y));
	const float CapsuleRadiusLocal = Capsule->GetScaledCapsuleRadius() / MinimumHorizontalScale;
	const float TileHalfExtent = TileSpacing * TileFillRatio * 0.5f;
	if (FMath::Abs(PlayerLocalLocation.X - TileLocalLocation.X) > TileHalfExtent + CapsuleRadiusLocal ||
		FMath::Abs(PlayerLocalLocation.Y - TileLocalLocation.Y) > TileHalfExtent + CapsuleRadiusLocal)
	{
		return;
	}

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float PlayerBottomWorldZ = PlayerActor->GetActorLocation().Z - CapsuleHalfHeight;
	const FVector IceSurfaceWorldLocation = GetActorTransform().TransformPosition(
		FVector(TileLocalLocation.X, TileLocalLocation.Y, TileHeight));
	const float TargetBottomWorldZ = IceSurfaceWorldLocation.Z + PlayerFreezeSurfaceClearance;
	if (PlayerBottomWorldZ >= TargetBottomWorldZ)
	{
		return;
	}

	FVector CorrectedLocation = PlayerActor->GetActorLocation();
	CorrectedLocation.Z += TargetBottomWorldZ - PlayerBottomWorldZ;
	PlayerActor->SetActorLocation(CorrectedLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

FTransform AUOUWaterIceGridActor::MakeMergedIceCollisionTransform(
	int32 StartX,
	int32 StartY,
	int32 Width,
	int32 Height) const
{
	const UStaticMesh* CollisionMesh = MergedIceCollisionInstances != nullptr
		? MergedIceCollisionInstances->GetStaticMesh()
		: nullptr;
	const FVector MeshSize = CollisionMesh != nullptr
		? CollisionMesh->GetBoundingBox().GetSize()
		: FVector(100.0f);
	const float DesiredSizeX = (Width - 1) * TileSpacing + TileSpacing * TileFillRatio;
	const float DesiredSizeY = (Height - 1) * TileSpacing + TileSpacing * TileFillRatio;
	const FVector DesiredSize(DesiredSizeX, DesiredSizeY, TileHeight);
	const FVector Scale(
		DesiredSize.X / FMath::Max(1.0f, MeshSize.X),
		DesiredSize.Y / FMath::Max(1.0f, MeshSize.Y),
		DesiredSize.Z / FMath::Max(1.0f, MeshSize.Z));
	const int32 EndTileIndex = GetTileIndex(StartX + Width - 1, StartY + Height - 1);
	const int32 StartTileIndex = GetTileIndex(StartX, StartY);
	FVector Location = (GetTileLocalLocation(StartTileIndex) + GetTileLocalLocation(EndTileIndex)) * 0.5f;
	Location.Z = TileHeight * 0.5f;
	return FTransform(FQuat::Identity, Location, Scale);
}

void AUOUWaterIceGridActor::RebuildMergedIceCollisionChunk(int32 ChunkIndex)
{
	if (!IceCollisionChunkComponents.IsValidIndex(ChunkIndex))
	{
		return;
	}

	UInstancedStaticMeshComponent* Component = IceCollisionChunkComponents[ChunkIndex];
	Component->ClearInstances();
	if (!bEnableIceCollision || Tiles.Num() != GridSizeX * GridSizeY)
	{
		return;
	}

	const int32 ChunkX = ChunkIndex % GetCollisionChunkCountX();
	const int32 ChunkY = ChunkIndex / GetCollisionChunkCountX();
	const int32 StartX = ChunkX * CollisionChunkSize;
	const int32 StartY = ChunkY * CollisionChunkSize;
	const int32 EndX = FMath::Min(StartX + CollisionChunkSize, GridSizeX);
	const int32 EndY = FMath::Min(StartY + CollisionChunkSize, GridSizeY);
	TBitArray<> ConsumedTiles(false, Tiles.Num());
	for (int32 TileY = StartY; TileY < EndY; ++TileY)
	{
		for (int32 TileX = StartX; TileX < EndX; ++TileX)
		{
			const int32 StartTileIndex = GetTileIndex(TileX, TileY);
			if (!Tiles.IsValidIndex(StartTileIndex) || ConsumedTiles[StartTileIndex] ||
				Tiles[StartTileIndex].Phase != EUOUWaterIcePhase::Ice)
			{
				continue;
			}

			int32 RectangleWidth = 0;
			while (TileX + RectangleWidth < EndX)
			{
				const int32 CandidateIndex = GetTileIndex(TileX + RectangleWidth, TileY);
				if (ConsumedTiles[CandidateIndex] || Tiles[CandidateIndex].Phase != EUOUWaterIcePhase::Ice)
				{
					break;
				}
				++RectangleWidth;
			}

			int32 RectangleHeight = 1;
			while (TileY + RectangleHeight < EndY)
			{
				bool bCanExtendRectangle = true;
				for (int32 OffsetX = 0; OffsetX < RectangleWidth; ++OffsetX)
				{
					const int32 CandidateIndex = GetTileIndex(TileX + OffsetX, TileY + RectangleHeight);
					if (ConsumedTiles[CandidateIndex] || Tiles[CandidateIndex].Phase != EUOUWaterIcePhase::Ice)
					{
						bCanExtendRectangle = false;
						break;
					}
				}
				if (!bCanExtendRectangle)
				{
					break;
				}
				++RectangleHeight;
			}

			for (int32 OffsetY = 0; OffsetY < RectangleHeight; ++OffsetY)
			{
				for (int32 OffsetX = 0; OffsetX < RectangleWidth; ++OffsetX)
				{
					ConsumedTiles[GetTileIndex(TileX + OffsetX, TileY + OffsetY)] = true;
				}
			}

			Component->AddInstance(
				MakeMergedIceCollisionTransform(TileX, TileY, RectangleWidth, RectangleHeight));
		}
	}
}

void AUOUWaterIceGridActor::RebuildAllCollisionChunks()
{
	EnsureCollisionChunkComponents();
	TSet<int32> AllChunks;
	const int32 ChunkCount = GetCollisionChunkCountX() * GetCollisionChunkCountY();
	for (int32 ChunkIndex = 0; ChunkIndex < ChunkCount; ++ChunkIndex)
	{
		AllChunks.Add(ChunkIndex);
	}
	RebuildCollisionChunks(AllChunks);
}

void AUOUWaterIceGridActor::RebuildCollisionChunks(const TSet<int32>& ChunkIndices)
{
	for (const int32 ChunkIndex : ChunkIndices)
	{
		if (IceCollisionChunkComponents.IsValidIndex(ChunkIndex) &&
			WaterCollisionChunkComponents.IsValidIndex(ChunkIndex))
		{
			RebuildMergedIceCollisionChunk(ChunkIndex);
			RebuildWaterBoundaryCollisionChunk(ChunkIndex);
			++CollisionChunksRebuiltSinceLog;
		}
	}
}

void AUOUWaterIceGridActor::AddAffectedCollisionChunks(int32 TileIndex, TSet<int32>& OutChunkIndices) const
{
	if (!Tiles.IsValidIndex(TileIndex))
	{
		return;
	}
	const int32 TileX = TileIndex % GridSizeX;
	const int32 TileY = TileIndex / GridSizeX;
	static constexpr int32 Offsets[5][2] =
	{
		{ 0, 0 }, { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }
	};
	for (const int32* Offset : Offsets)
	{
		const int32 ChunkIndex = GetCollisionChunkIndex(TileX + Offset[0], TileY + Offset[1]);
		if (ChunkIndex != INDEX_NONE)
		{
			OutChunkIndices.Add(ChunkIndex);
		}
	}
}

void AUOUWaterIceGridActor::RefreshLightSources()
{
	TArray<TWeakObjectPtr<UUOULightExposureSourceComponent>> FoundLightSources;
	for (TObjectIterator<UUOULightExposureSourceComponent> It; It; ++It)
	{
		UUOULightExposureSourceComponent* Source = *It;
		if (IsValid(Source) && Source->GetWorld() == GetWorld() && Source->IsRegistered())
		{
			FoundLightSources.Add(Source);
		}
	}

	bool bSourcesChanged = CachedLightSources.Num() != FoundLightSources.Num();
	if (!bSourcesChanged)
	{
		for (const TWeakObjectPtr<UUOULightExposureSourceComponent>& Source : CachedLightSources)
		{
			if (!FoundLightSources.Contains(Source))
			{
				bSourcesChanged = true;
				break;
			}
		}
	}
	if (bSourcesChanged)
	{
		UnbindLightSources();
		CachedLightSources = MoveTemp(FoundLightSources);
		for (const TWeakObjectPtr<UUOULightExposureSourceComponent>& SourcePtr : CachedLightSources)
		{
			if (UUOULightExposureSourceComponent* Source = SourcePtr.Get())
			{
				Source->OnLightPathsUpdated.AddUniqueDynamic(this, &AUOUWaterIceGridActor::HandleLightPathsUpdated);
			}
		}
		bLightIntensityCacheDirty = true;
	}
	NextLightSourceRefreshTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() + 1.0f : 0.0f;
}

void AUOUWaterIceGridActor::UnbindLightSources()
{
	for (const TWeakObjectPtr<UUOULightExposureSourceComponent>& SourcePtr : CachedLightSources)
	{
		if (UUOULightExposureSourceComponent* Source = SourcePtr.Get())
		{
			Source->OnLightPathsUpdated.RemoveDynamic(this, &AUOUWaterIceGridActor::HandleLightPathsUpdated);
		}
	}
}

void AUOUWaterIceGridActor::HandleLightPathsUpdated(const TArray<FUOULightPathData>&)
{
	// 연속 이동 이벤트는 즉시 전체 격자를 재계산하지 않고 임계값과 최대 갱신 주기를 적용합니다.
	bLightCacheRefreshRequested = true;
	LastLightPathUpdateTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
}

void AUOUWaterIceGridActor::RefreshTileLightIntensityCache()
{
	if (Tiles.Num() != GridSizeX * GridSizeY)
	{
		return;
	}

	const FTransform ActorTransform = GetActorTransform();
	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		const FVector TileWorldLocation = ActorTransform.TransformPosition(GetTileLocalLocation(TileIndex));
		FTileRuntime& Tile = Tiles[TileIndex];
		const float PreviousIntensity = Tile.CachedLightIntensity;
		Tile.CachedLightIntensity = CalculateTileLightIntensity(TileWorldLocation, false);
		if (PreviousIntensity > 0.0f || Tile.CachedLightIntensity > 0.0f)
		{
			ActiveTileIndices.Add(TileIndex);
		}
	}
	CaptureLightPathSnapshot();
	bLightIntensityCacheDirty = false;
	bLightCacheRefreshRequested = false;
	NextAllowedLightCacheRefreshTime = GetWorld() != nullptr
		? GetWorld()->GetTimeSeconds() + LightCacheMinimumRefreshInterval
		: 0.0f;
	++LightCacheRefreshesSinceLog;
}

void AUOUWaterIceGridActor::CaptureLightPathSnapshot()
{
	CachedLightPathSnapshot.Reset();
	for (const TWeakObjectPtr<UUOULightExposureSourceComponent>& SourcePtr : CachedLightSources)
	{
		const UUOULightExposureSourceComponent* Source = SourcePtr.Get();
		if (Source == nullptr)
		{
			continue;
		}
		for (const FUOULightPathData& Path : Source->GetLightPathsView())
		{
			for (const FUOULightPathSegmentData& Segment : Path.Segments)
			{
				FLightSegmentSnapshot& Snapshot = CachedLightPathSnapshot.AddDefaulted_GetRef();
				Snapshot.Start = Segment.Start;
				Snapshot.Direction = Segment.Direction.GetSafeNormal();
				Snapshot.Length = Segment.Length;
				Snapshot.StartRadius = Segment.StartRadius;
				Snapshot.EndRadius = Segment.EndRadius;
				Snapshot.Intensity = Segment.Intensity;
			}
		}
	}
}

bool AUOUWaterIceGridActor::HaveLightPathsChangedBeyondThreshold() const
{
	TArray<FLightSegmentSnapshot> CurrentSegments;
	for (const TWeakObjectPtr<UUOULightExposureSourceComponent>& SourcePtr : CachedLightSources)
	{
		const UUOULightExposureSourceComponent* Source = SourcePtr.Get();
		if (Source == nullptr)
		{
			continue;
		}
		for (const FUOULightPathData& Path : Source->GetLightPathsView())
		{
			for (const FUOULightPathSegmentData& Segment : Path.Segments)
			{
				FLightSegmentSnapshot& Snapshot = CurrentSegments.AddDefaulted_GetRef();
				Snapshot.Start = Segment.Start;
				Snapshot.Direction = Segment.Direction.GetSafeNormal();
				Snapshot.Length = Segment.Length;
				Snapshot.StartRadius = Segment.StartRadius;
				Snapshot.EndRadius = Segment.EndRadius;
				Snapshot.Intensity = Segment.Intensity;
			}
		}
	}
	if (CurrentSegments.Num() != CachedLightPathSnapshot.Num())
	{
		return true;
	}

	const float DirectionDotThreshold = FMath::Cos(FMath::DegreesToRadians(LightCacheDirectionThreshold));
	for (int32 Index = 0; Index < CurrentSegments.Num(); ++Index)
	{
		const FLightSegmentSnapshot& Current = CurrentSegments[Index];
		const FLightSegmentSnapshot& Cached = CachedLightPathSnapshot[Index];
		if (FVector::DistSquared(Current.Start, Cached.Start) > FMath::Square(LightCachePositionThreshold) ||
			FVector::DotProduct(Current.Direction, Cached.Direction) < DirectionDotThreshold ||
			!FMath::IsNearlyEqual(Current.Length, Cached.Length, LightCachePositionThreshold) ||
			!FMath::IsNearlyEqual(Current.StartRadius, Cached.StartRadius, LightCachePositionThreshold) ||
			!FMath::IsNearlyEqual(Current.EndRadius, Cached.EndRadius, LightCachePositionThreshold) ||
			!FMath::IsNearlyEqual(Current.Intensity, Cached.Intensity, 0.01f))
		{
			return true;
		}
	}
	return false;
}

float AUOUWaterIceGridActor::CalculateTileLightIntensity(
	const FVector& TileWorldLocation,
	bool bCheckPlayerOcclusion) const
{
	float MaximumIntensity = 0.0f;
	const float TileMargin = TileSpacing * 0.707107f;
	for (const TWeakObjectPtr<UUOULightExposureSourceComponent>& SourcePtr : CachedLightSources)
	{
		const UUOULightExposureSourceComponent* Source = SourcePtr.Get();
		if (Source == nullptr)
		{
			continue;
		}
		for (const FUOULightPathData& Path : Source->GetLightPathsView())
		{
			for (const FUOULightPathSegmentData& Segment : Path.Segments)
			{
				++LightSegmentTestsSinceLog;
				if (Segment.Length <= KINDA_SMALL_NUMBER || Segment.Intensity <= 0.0f)
				{
					continue;
				}
				const float AxialDistance = FVector::DotProduct(TileWorldLocation - Segment.Start, Segment.Direction);
				if (AxialDistance < -TileMargin || AxialDistance > Segment.Length + TileMargin)
				{
					continue;
				}
				const float ClampedAxialDistance = FMath::Clamp(AxialDistance, 0.0f, Segment.Length);
				const float Alpha = ClampedAxialDistance / Segment.Length;
				const float BeamRadius = FMath::Lerp(Segment.StartRadius, Segment.EndRadius, Alpha);
				const FVector ClosestPoint = Segment.Start + Segment.Direction * ClampedAxialDistance;
				if (FVector::DistSquared(TileWorldLocation, ClosestPoint) > FMath::Square(BeamRadius + TileMargin))
				{
					continue;
				}
				const FVector RayStart = Segment.StartRadius <= KINDA_SMALL_NUMBER
					? Segment.Start
					: TileWorldLocation - Segment.Direction * FMath::Max(0.0f, AxialDistance);
				if (!bCheckPlayerOcclusion || !IsPlayerBlockingLight(RayStart, TileWorldLocation))
				{
					MaximumIntensity = FMath::Max(MaximumIntensity, Segment.Intensity);
				}
			}
		}
	}
	return MaximumIntensity;
}

bool AUOUWaterIceGridActor::IsPlayerBlockingLight(const FVector& RayStart, const FVector& TileWorldLocation) const
{
	if (GetWorld() == nullptr || RayStart.Equals(TileWorldLocation, 1.0f))
	{
		return false;
	}
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WaterIceGridPlayerShade), false, this);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByObjectType(Hit, RayStart, TileWorldLocation, ObjectQueryParams, QueryParams) &&
		Hit.GetActor() == CachedPlayerActor.Get();
}

void AUOUWaterIceGridActor::AddUmbrellaAffectedTiles(TSet<int32>& OutTileIndices) const
{
	const AActor* PlayerActor = CachedPlayerActor.Get();
	const UUOUUmbrellaComponent* Umbrella = CachedUmbrellaComponent.Get();
	if (PlayerActor == nullptr || Umbrella == nullptr || !Umbrella->HasUmbrella())
	{
		return;
	}

	const int32 Radius = Umbrella->IsClosed() ? ClosedUmbrellaRadiusInTiles : OpenUmbrellaRadiusInTiles;
	const FVector PlayerLocalLocation = GetActorTransform().InverseTransformPosition(PlayerActor->GetActorLocation());
	const float HalfFieldX = GridSizeX * TileSpacing * 0.5f;
	const float HalfFieldY = GridSizeY * TileSpacing * 0.5f;
	const float ShoreInteractionReach = (static_cast<float>(Radius) + 1.0f) * TileSpacing;
	if (FMath::Abs(PlayerLocalLocation.X) > HalfFieldX + ShoreInteractionReach ||
		FMath::Abs(PlayerLocalLocation.Y) > HalfFieldY + ShoreInteractionReach)
	{
		return;
	}

	const int32 CenterX = FMath::Clamp(
		FMath::RoundToInt(PlayerLocalLocation.X / TileSpacing + (GridSizeX - 1) * 0.5f),
		0,
		GridSizeX - 1);
	const int32 CenterY = FMath::Clamp(
		FMath::RoundToInt(PlayerLocalLocation.Y / TileSpacing + (GridSizeY - 1) * 0.5f),
		0,
		GridSizeY - 1);
	for (int32 OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
	{
		for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
		{
			const int32 TileIndex = GetTileIndex(CenterX + OffsetX, CenterY + OffsetY);
			if (TileIndex != INDEX_NONE)
			{
				OutTileIndices.Add(TileIndex);
			}
		}
	}
}

void AUOUWaterIceGridActor::UpdateGrid()
{
	if (GetWorld() == nullptr || Tiles.Num() != GridSizeX * GridSizeY)
	{
		return;
	}
	if (!CachedPlayerActor.IsValid() || !CachedUmbrellaComponent.IsValid())
	{
		ResolvePlayerReferences();
	}
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime >= NextLightSourceRefreshTime)
	{
		RefreshLightSources();
	}
	if (!bLightIntensityCacheDirty && bLightCacheRefreshRequested &&
		CurrentTime >= NextAllowedLightCacheRefreshTime)
	{
		const bool bMovementSettled = CurrentTime - LastLightPathUpdateTime >= LightCacheSettleDelay;
		bLightIntensityCacheDirty = bMovementSettled || HaveLightPathsChangedBeyondThreshold();
	}
	if (bLightIntensityCacheDirty)
	{
		RefreshTileLightIntensityCache();
	}

	int32 PlayerTileX = INDEX_NONE;
	int32 PlayerTileY = INDEX_NONE;
	const AActor* PlayerActor = CachedPlayerActor.Get();
	const bool bPlayerInsideField = PlayerActor != nullptr &&
		WorldLocationToTileCoordinates(PlayerActor->GetActorLocation(), PlayerTileX, PlayerTileY);
	const int32 PlayerOcclusionRadius = FMath::Max(OpenUmbrellaRadiusInTiles, ClosedUmbrellaRadiusInTiles) + 1;
	TSet<int32> UmbrellaTileIndices;
	AddUmbrellaAffectedTiles(UmbrellaTileIndices);
	for (const int32 TileIndex : UmbrellaTileIndices)
	{
		ActiveTileIndices.Add(TileIndex);
	}

	TSet<int32> DirtyCollisionChunks;
	TArray<int32> TilesToDeactivate;
	const TArray<int32> TilesToUpdate = ActiveTileIndices.Array();
	for (const int32 TileIndex : TilesToUpdate)
	{
		if (!Tiles.IsValidIndex(TileIndex))
		{
			continue;
		}
		FTileRuntime& Tile = Tiles[TileIndex];
		const FVector TileWorldLocation = GetActorTransform().TransformPosition(GetTileLocalLocation(TileIndex));
		const int32 TileX = TileIndex % GridSizeX;
		const int32 TileY = TileIndex / GridSizeX;
		const bool bInsideUmbrellaArea = UmbrellaTileIndices.Contains(TileIndex);

		// 우산이 덮는 셀은 입사광보다 차광·냉각 판정을 우선합니다.
		// 빛 가열을 먼저 적용하면 플레이어 몸에 직접 가려진 중앙 셀만 얼어
		// 펼친 우산의 3x3 범위가 한 칸처럼 보일 수 있습니다.
		if (bInsideUmbrellaArea)
		{
			Tile.Temperature -= UmbrellaCoolingRate * Tile.CoolingRateMultiplier * UpdateInterval;
		}
		else
		{
			float LightIntensity = Tile.CachedLightIntensity;
			const bool bNearPlayer = bPlayerInsideField &&
				FMath::Abs(TileX - PlayerTileX) <= PlayerOcclusionRadius &&
				FMath::Abs(TileY - PlayerTileY) <= PlayerOcclusionRadius;
			if (LightIntensity > 0.0f && bNearPlayer)
			{
				// 움직이는 플레이어의 차광은 주변 셀만 실시간으로 다시 검사합니다.
				LightIntensity = CalculateTileLightIntensity(TileWorldLocation, true);
			}

			if (LightIntensity > 0.0f)
			{
				Tile.Temperature += LightHeatingRate * Tile.HeatingRateMultiplier * LightIntensity * UpdateInterval;
			}
			else
			{
				const float RecoveryMultiplier = AmbientTemperature >= Tile.Temperature
					? Tile.HeatingRateMultiplier
					: Tile.CoolingRateMultiplier;
				Tile.Temperature = FMath::FInterpConstantTo(
					Tile.Temperature,
					AmbientTemperature,
					UpdateInterval,
					TemperatureRecoveryRate * RecoveryMultiplier);
			}
		}
		Tile.Temperature = FMath::Clamp(Tile.Temperature, MinimumTemperature, MaximumTemperature);

		const EUOUWaterIcePhase PreviousPhase = Tile.Phase;
		EvaluateTilePhase(TileIndex, false);
		if (Tile.Phase != PreviousPhase)
		{
			AddAffectedCollisionChunks(TileIndex, DirtyCollisionChunks);
		}

		const bool bNeedsFurtherUpdates = bInsideUmbrellaArea || Tile.CachedLightIntensity > 0.0f ||
			!FMath::IsNearlyEqual(Tile.Temperature, AmbientTemperature, 0.01f);
		if (!bNeedsFurtherUpdates)
		{
			TilesToDeactivate.Add(TileIndex);
		}
	}
	for (const int32 TileIndex : TilesToDeactivate)
	{
		ActiveTileIndices.Remove(TileIndex);
	}
	if (DirtyCollisionChunks.Num() > 0)
	{
		WaterInstances->MarkRenderStateDirty();
		IceInstances->MarkRenderStateDirty();
		RebuildCollisionChunks(DirtyCollisionChunks);
	}
	LogLocalPerformanceStats();
}

void AUOUWaterIceGridActor::LogLocalPerformanceStats()
{
#if !UE_BUILD_SHIPPING
	if (!bEnableLocalPerformanceLogging || GetWorld() == nullptr)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < NextPerformanceLogTime)
	{
		return;
	}

	int32 IceCollisionInstanceCount = 0;
	for (const UInstancedStaticMeshComponent* Component : IceCollisionChunkComponents)
	{
		if (Component != nullptr)
		{
			IceCollisionInstanceCount += Component->GetInstanceCount();
		}
	}

	int32 WaterBoundaryInstanceCount = 0;
	for (const UInstancedStaticMeshComponent* Component : WaterCollisionChunkComponents)
	{
		if (Component != nullptr)
		{
			WaterBoundaryInstanceCount += Component->GetInstanceCount();
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WaterIceGrid][LocalPerf] ActiveTiles=%d/%d LightCacheRefreshes=%d LightSegmentTests=%lld CollisionChunksRebuilt=%d IceCollisionInstances=%d WaterBoundaryInstances=%d"),
		ActiveTileIndices.Num(),
		Tiles.Num(),
		LightCacheRefreshesSinceLog,
		static_cast<long long>(LightSegmentTestsSinceLog),
		CollisionChunksRebuiltSinceLog,
		IceCollisionInstanceCount,
		WaterBoundaryInstanceCount);

	LightCacheRefreshesSinceLog = 0;
	LightSegmentTestsSinceLog = 0;
	CollisionChunksRebuiltSinceLog = 0;
	NextPerformanceLogTime = CurrentTime + PerformanceLogInterval;
#endif
}

void AUOUWaterIceGridActor::EvaluateTilePhase(int32 TileIndex, bool bMarkRenderStateDirty)
{
	if (!Tiles.IsValidIndex(TileIndex))
	{
		return;
	}

	FTileRuntime& Tile = Tiles[TileIndex];
	const EUOUWaterIcePhase PreviousPhase = Tile.Phase;
	if (Tile.Phase == EUOUWaterIcePhase::Water && Tile.Temperature <= FreezeTemperature)
	{
		Tile.Phase = EUOUWaterIcePhase::Ice;
	}
	else if (Tile.Phase == EUOUWaterIcePhase::Ice && Tile.Temperature >= MeltTemperature)
	{
		Tile.Phase = EUOUWaterIcePhase::Water;
	}

	if (Tile.Phase != PreviousPhase)
	{
		if (PreviousPhase == EUOUWaterIcePhase::Water && Tile.Phase == EUOUWaterIcePhase::Ice)
		{
			ResolvePlayerOverlapWithNewIce(TileIndex);
		}
		ApplyTilePhase(TileIndex, bMarkRenderStateDirty);
		if (bMarkRenderStateDirty)
		{
			TSet<int32> DirtyCollisionChunks;
			AddAffectedCollisionChunks(TileIndex, DirtyCollisionChunks);
			RebuildCollisionChunks(DirtyCollisionChunks);
		}
	}
}

void AUOUWaterIceGridActor::ApplyTilePhase(int32 TileIndex, bool bMarkRenderStateDirty)
{
	const FVector LocalLocation = GetTileLocalLocation(TileIndex);
	const FTransform HiddenTransform(FQuat::Identity, LocalLocation, FVector::ZeroVector);
	const bool bIsWater = Tiles[TileIndex].Phase == EUOUWaterIcePhase::Water;
	WaterInstances->UpdateInstanceTransform(TileIndex, bIsWater ? MakeTileTransform(TileIndex, EUOUWaterIcePhase::Water) : HiddenTransform, false, bMarkRenderStateDirty, true);
	IceInstances->UpdateInstanceTransform(TileIndex, bIsWater ? HiddenTransform : MakeTileTransform(TileIndex, EUOUWaterIcePhase::Ice), false, bMarkRenderStateDirty, true);
}
