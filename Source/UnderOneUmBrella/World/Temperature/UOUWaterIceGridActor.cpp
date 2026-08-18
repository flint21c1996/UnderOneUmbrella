// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Temperature/UOUWaterIceGridActor.h"

#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
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

	WaterBlockingInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WaterBlockingInstances"));
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
		IceInstances->SetCollisionEnabled(
			bEnableIceCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		IceInstances->SetCollisionResponseToAllChannels(ECR_Block);
		IceInstances->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	if (WaterBlockingInstances != nullptr)
	{
		WaterBlockingInstances->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		WaterBlockingInstances->SetCollisionResponseToAllChannels(ECR_Ignore);
		WaterBlockingInstances->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
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
	if (WaterInstances == nullptr || IceInstances == nullptr || WaterBlockingInstances == nullptr)
	{
		return;
	}

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
	WaterBlockingInstances->ClearInstances();
	Tiles.SetNum(GridSizeX * GridSizeY);
	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		FTileRuntime& Tile = Tiles[TileIndex];
		Tile.Temperature = InitialTemperature;
		Tile.Phase = InitialTemperature <= FreezeTemperature
			? EUOUWaterIcePhase::Ice
			: EUOUWaterIcePhase::Water;
		const FVector LocalLocation = GetTileLocalLocation(TileIndex);
		const int32 TileX = TileIndex % GridSizeX;
		const int32 TileY = TileIndex / GridSizeX;
		FRandomStream HeightRandom(IceHeightPatternSeed + TileX * 73856093 + TileY * 19349663);
		Tile.IceHeightScale = HeightRandom.FRandRange(MinimumIceHeightScale, MaximumIceHeightScale);

		const FTransform WaterTransform = MakeTileTransform(TileIndex, EUOUWaterIcePhase::Water);
		const FTransform IceTransform = MakeTileTransform(TileIndex, EUOUWaterIcePhase::Ice);
		WaterInstances->AddInstance(Tile.Phase == EUOUWaterIcePhase::Water ? WaterTransform : FTransform(FQuat::Identity, LocalLocation, FVector::ZeroVector));
		IceInstances->AddInstance(Tile.Phase == EUOUWaterIcePhase::Ice ? IceTransform : FTransform(FQuat::Identity, LocalLocation, FVector::ZeroVector));
	}
	RebuildWaterBoundaryCollision();
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

void AUOUWaterIceGridActor::RebuildWaterBoundaryCollision()
{
	if (WaterBlockingInstances == nullptr)
	{
		return;
	}

	WaterBlockingInstances->ClearInstances();
	static constexpr int32 NeighborOffsets[4][2] =
	{
		{ -1, 0 },
		{ 1, 0 },
		{ 0, -1 },
		{ 0, 1 }
	};

	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		if (Tiles[TileIndex].Phase != EUOUWaterIcePhase::Water)
		{
			continue;
		}

		const int32 TileX = TileIndex % GridSizeX;
		const int32 TileY = TileIndex / GridSizeX;
		for (const int32* Offset : NeighborOffsets)
		{
			const int32 NeighborIndex = GetTileIndex(TileX + Offset[0], TileY + Offset[1]);
			const bool bNeighborIsWater = Tiles.IsValidIndex(NeighborIndex) &&
				Tiles[NeighborIndex].Phase == EUOUWaterIcePhase::Water;
			if (!bNeighborIsWater)
			{
				WaterBlockingInstances->AddInstance(
					MakeWaterBoundaryTransform(TileIndex, Offset[0], Offset[1]));
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

void AUOUWaterIceGridActor::RefreshLightSources()
{
	CachedLightSources.Reset();
	for (TObjectIterator<UUOULightExposureSourceComponent> It; It; ++It)
	{
		UUOULightExposureSourceComponent* Source = *It;
		if (IsValid(Source) && Source->GetWorld() == GetWorld() && Source->IsRegistered())
		{
			CachedLightSources.Add(Source);
		}
	}
	NextLightSourceRefreshTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() + 1.0f : 0.0f;
}

float AUOUWaterIceGridActor::CalculateTileLightIntensity(const FVector& TileWorldLocation) const
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
				if (!IsPlayerBlockingLight(RayStart, TileWorldLocation))
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

bool AUOUWaterIceGridActor::IsTileInsideUmbrellaArea(int32 TileX, int32 TileY) const
{
	const AActor* PlayerActor = CachedPlayerActor.Get();
	const UUOUUmbrellaComponent* Umbrella = CachedUmbrellaComponent.Get();
	if (PlayerActor == nullptr || Umbrella == nullptr || !Umbrella->HasUmbrella())
	{
		return false;
	}
	const int32 Radius = Umbrella->IsClosed() ? ClosedUmbrellaRadiusInTiles : OpenUmbrellaRadiusInTiles;
	const FVector PlayerLocalLocation = GetActorTransform().InverseTransformPosition(PlayerActor->GetActorLocation());
	const float HalfFieldX = GridSizeX * TileSpacing * 0.5f;
	const float HalfFieldY = GridSizeY * TileSpacing * 0.5f;
	const float ShoreInteractionReach = (static_cast<float>(Radius) + 1.0f) * TileSpacing;
	if (FMath::Abs(PlayerLocalLocation.X) > HalfFieldX + ShoreInteractionReach ||
		FMath::Abs(PlayerLocalLocation.Y) > HalfFieldY + ShoreInteractionReach)
	{
		return false;
	}

	// 물 필드 밖의 가장자리에서는 가장 가까운 셀을 기준으로 동결합니다.
	// 이를 허용하지 않으면 물 충돌 때문에 첫 얼음 타일을 만들 수 없어 필드에 진입할 수 없습니다.
	const int32 PlayerTileX = FMath::Clamp(
		FMath::RoundToInt(PlayerLocalLocation.X / TileSpacing + (GridSizeX - 1) * 0.5f),
		0,
		GridSizeX - 1);
	const int32 PlayerTileY = FMath::Clamp(
		FMath::RoundToInt(PlayerLocalLocation.Y / TileSpacing + (GridSizeY - 1) * 0.5f),
		0,
		GridSizeY - 1);
	return FMath::Abs(TileX - PlayerTileX) <= Radius && FMath::Abs(TileY - PlayerTileY) <= Radius;
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
	if (GetWorld()->GetTimeSeconds() >= NextLightSourceRefreshTime)
	{
		RefreshLightSources();
	}

	bool bAnyPhaseChanged = false;
	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		FTileRuntime& Tile = Tiles[TileIndex];
		const FVector TileWorldLocation = GetActorTransform().TransformPosition(GetTileLocalLocation(TileIndex));
		const int32 TileX = TileIndex % GridSizeX;
		const int32 TileY = TileIndex / GridSizeX;
		const bool bInsideUmbrellaArea = IsTileInsideUmbrellaArea(TileX, TileY);

		// 우산이 덮는 셀은 입사광보다 차광·냉각 판정을 우선합니다.
		// 빛 가열을 먼저 적용하면 플레이어 몸에 직접 가려진 중앙 셀만 얼어
		// 펼친 우산의 3x3 범위가 한 칸처럼 보일 수 있습니다.
		if (bInsideUmbrellaArea)
		{
			Tile.Temperature -= UmbrellaCoolingRate * UpdateInterval;
		}
		else if (const float LightIntensity = CalculateTileLightIntensity(TileWorldLocation); LightIntensity > 0.0f)
		{
			Tile.Temperature += LightHeatingRate * LightIntensity * UpdateInterval;
		}
		else
		{
			Tile.Temperature = FMath::FInterpConstantTo(Tile.Temperature, AmbientTemperature, UpdateInterval, TemperatureRecoveryRate);
		}
		Tile.Temperature = FMath::Clamp(Tile.Temperature, MinimumTemperature, MaximumTemperature);

		const EUOUWaterIcePhase PreviousPhase = Tile.Phase;
		EvaluateTilePhase(TileIndex, false);
		if (Tile.Phase != PreviousPhase)
		{
			bAnyPhaseChanged = true;
		}
	}
	if (bAnyPhaseChanged)
	{
		WaterInstances->MarkRenderStateDirty();
		IceInstances->MarkRenderStateDirty();
		RebuildWaterBoundaryCollision();
	}
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
		ApplyTilePhase(TileIndex, bMarkRenderStateDirty);
		if (bMarkRenderStateDirty)
		{
			RebuildWaterBoundaryCollision();
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
