// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Temperature/UOUWaterIcePhaseActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Player/UOUUmbrellaComponent.h"
#include "TimerManager.h"
#include "World/Light/UOULightExposureReceiverComponent.h"

AUOUWaterIcePhaseActor::AUOUWaterIcePhaseActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.AddUnique(TEXT("UOUWaterIceLightOccluder"));

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	// 격자 간격은 100cm로 유지하되 타일 자체는 가로·세로 절반 크기로 표시합니다.
	// 루트의 Z 스케일은 유지하여 상태별 높이와 얼음 높이 패턴에는 영향을 주지 않습니다.
	RootScene->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.0f));

	WaterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterMesh"));
	WaterMesh->SetupAttachment(RootScene);
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	IceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IceMesh"));
	IceMesh->SetupAttachment(RootScene);
	IceMesh->SetVisibility(false, true);
	IceMesh->SetHiddenInGame(true, true);
	IceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IceMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	WaterBlockingVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WaterBlockingVolume"));
	WaterBlockingVolume->SetupAttachment(RootScene);
	// 기본 100cm 큐브의 가장자리보다 살짝 안쪽으로 넣어 인접 타일에서
	// 플레이어가 불필요하게 막히지 않도록 합니다. 위쪽 여유는 물 타일 진입 차단용입니다.
	WaterBlockingVolume->InitBoxExtent(FVector(48.0f, 48.0f, 100.0f));
	WaterBlockingVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	WaterBlockingVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WaterBlockingVolume->SetCollisionObjectType(ECC_WorldDynamic);
	WaterBlockingVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	WaterBlockingVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	WaterBlockingVolume->SetGenerateOverlapEvents(false);

	LightReceiverVolume = CreateDefaultSubobject<USphereComponent>(TEXT("LightReceiverVolume"));
	LightReceiverVolume->SetupAttachment(RootScene);
	// 100cm 타일보다 큰 판정 구체는 인접 타일까지 침범하고 가장자리 샘플을
	// 실제 메시 밖에 배치하므로, 타일 안쪽에 들어오는 크기로 제한합니다.
	LightReceiverVolume->InitSphereRadius(48.0f);
	LightReceiverVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LightReceiverVolume->SetCollisionObjectType(ECC_WorldDynamic);
	LightReceiverVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	LightReceiverVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	LightReceiverVolume->SetGenerateOverlapEvents(false);

	TemperatureReceiver = CreateDefaultSubobject<UUOULightExposureReceiverComponent>(TEXT("TemperatureReceiver"));
	TemperatureReceiver->bAutoFindReceiverTransform = false;
	TemperatureReceiver->bStartAtAmbientTemperature = true;
	TemperatureReceiver->AmbientTemperature = -10.0f;
	TemperatureReceiver->CurrentTemperature = 10.0f;
	TemperatureReceiver->MinTemperature = -10.0f;
	TemperatureReceiver->MaxTemperature = 10.0f;
	TemperatureReceiver->TemperatureRisePerIntensity = 10.0f;
	TemperatureReceiver->TemperatureRecoveryRate = 5.0f;
	TemperatureReceiver->bRecoverToAmbientWhenNotExposed = true;
	TemperatureReceiver->bUseReceiverVolumeSampling = true;
	TemperatureReceiver->RequiredReceiverSampleHits = 2;
	TemperatureReceiver->bUseReceiverSampleHysteresis = true;
	TemperatureReceiver->RequiredReceiverSampleHitsWhileLit = 1;
	TemperatureReceiver->bUsePawnOcclusion = true;
	TemperatureReceiver->ReceiverTransformReference.ComponentProperty =
		GET_MEMBER_NAME_CHECKED(AUOUWaterIcePhaseActor, LightReceiverVolume);
	TemperatureReceiver->ReceiverVolumeReference.ComponentProperty =
		GET_MEMBER_NAME_CHECKED(AUOUWaterIcePhaseActor, LightReceiverVolume);
}

void AUOUWaterIcePhaseActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ValidateSettings();
	CurrentPhase = InitialPhase;
	ApplyPhaseState(false, CurrentPhase);
}

void AUOUWaterIcePhaseActor::BeginPlay()
{
	Super::BeginPlay();

	ValidateSettings();
	CacheIceMeshBaseTransform();
	ApplyIceHeightVariation();

	if (TemperatureReceiver == nullptr)
	{
		SetPhase(InitialPhase);
		return;
	}

	if (ControlMode == EUOUWaterIceControlMode::PlayerUmbrellaArea)
	{
		TemperatureReceiver->OnTemperatureChanged.RemoveDynamic(
			this,
			&AUOUWaterIcePhaseActor::HandleTemperatureChanged);
		TemperatureReceiver->OnTemperatureChanged.AddDynamic(
			this,
			&AUOUWaterIcePhaseActor::HandleTemperatureChanged);

		LastEvaluatedTemperature = TemperatureReceiver->CurrentTemperature;
		CurrentPhase = ResolveInitialPhase(LastEvaluatedTemperature);
		ApplyPhaseState(false, CurrentPhase);

		ResolvePlayerReferences();
		RefreshPhaseFromUmbrellaArea();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				UmbrellaAreaRefreshTimerHandle,
				this,
				&AUOUWaterIcePhaseActor::RefreshPhaseFromUmbrellaArea,
				UmbrellaAreaRefreshInterval,
				true);
		}
		return;
	}

	TemperatureReceiver->OnTemperatureChanged.RemoveDynamic(
		this,
		&AUOUWaterIcePhaseActor::HandleTemperatureChanged);
	TemperatureReceiver->OnTemperatureChanged.AddDynamic(
		this,
		&AUOUWaterIcePhaseActor::HandleTemperatureChanged);

	LastEvaluatedTemperature = TemperatureReceiver->CurrentTemperature;
	CurrentPhase = ResolveInitialPhase(LastEvaluatedTemperature);
	ApplyPhaseState(false, CurrentPhase);
}

void AUOUWaterIcePhaseActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UmbrellaAreaRefreshTimerHandle);
	}

	if (TemperatureReceiver != nullptr)
	{
		TemperatureReceiver->OnTemperatureChanged.RemoveDynamic(
			this,
			&AUOUWaterIcePhaseActor::HandleTemperatureChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void AUOUWaterIcePhaseActor::RefreshPhaseFromTemperature()
{
	if (ControlMode != EUOUWaterIceControlMode::Temperature || TemperatureReceiver == nullptr)
	{
		return;
	}

	HandleTemperatureChanged(TemperatureReceiver->CurrentTemperature, LastEvaluatedTemperature);
}

void AUOUWaterIcePhaseActor::RefreshPhaseFromUmbrellaArea()
{
	if (ControlMode != EUOUWaterIceControlMode::PlayerUmbrellaArea)
	{
		return;
	}

	if (!IsValid(CachedPlayerActor) || !IsValid(CachedUmbrellaComponent))
	{
		ResolvePlayerReferences();
	}

	const bool bHasUsableUmbrella = IsValid(CachedUmbrellaComponent) &&
		CachedUmbrellaComponent->HasUmbrella();
	const int32 FreezeRadius = bHasUsableUmbrella && CachedUmbrellaComponent->IsOpen()
		? OpenUmbrellaRadiusInTiles
		: ClosedUmbrellaRadiusInTiles;
	const bool bInsideCoolingArea = bHasUsableUmbrella && IsValid(CachedPlayerActor) &&
		IsInsidePlayerTileRange(CachedPlayerActor->GetActorLocation(), FreezeRadius);

	// 우산은 상태를 즉시 바꾸지 않고 온도만 낮춥니다. 빛을 받는 동안에는
	// 냉각을 멈춰 기존 수광 온도 상승이 우선하도록 합니다.
	if (bInsideCoolingArea &&
		TemperatureReceiver != nullptr &&
		!TemperatureReceiver->IsReceivingLight())
	{
		TemperatureReceiver->ApplyTemperatureDelta(
			-UmbrellaCoolingRate * UmbrellaAreaRefreshInterval);
	}

}

void AUOUWaterIcePhaseActor::SetPhase(EUOUWaterIcePhase NewPhase)
{
	const EUOUWaterIcePhase PreviousPhase = CurrentPhase;
	CurrentPhase = NewPhase;
	ApplyPhaseState(CurrentPhase != PreviousPhase, PreviousPhase);
}

void AUOUWaterIcePhaseActor::HandleTemperatureChanged(float NewTemperature, float PreviousTemperature)
{
	LastEvaluatedTemperature = NewTemperature;

	if (CurrentPhase == EUOUWaterIcePhase::Water && NewTemperature <= FreezeTemperature)
	{
		SetPhase(EUOUWaterIcePhase::Ice);
		return;
	}

	if (CurrentPhase == EUOUWaterIcePhase::Ice && NewTemperature >= MeltTemperature)
	{
		SetPhase(EUOUWaterIcePhase::Water);
	}
}

void AUOUWaterIcePhaseActor::ValidateSettings()
{
	if (FreezeTemperature > MeltTemperature)
	{
		Swap(FreezeTemperature, MeltTemperature);
	}

	TileWorldSize = FMath::Max(1.0f, TileWorldSize);
	OpenUmbrellaRadiusInTiles = FMath::Max(0, OpenUmbrellaRadiusInTiles);
	ClosedUmbrellaRadiusInTiles = FMath::Max(0, ClosedUmbrellaRadiusInTiles);
	PlayerVerticalTolerance = FMath::Max(0.0f, PlayerVerticalTolerance);
	UmbrellaAreaRefreshInterval = FMath::Max(0.02f, UmbrellaAreaRefreshInterval);
	UmbrellaCoolingRate = FMath::Max(0.0f, UmbrellaCoolingRate);
	MinimumIceHeightScale = FMath::Clamp(MinimumIceHeightScale, 0.5f, 1.5f);
	MaximumIceHeightScale = FMath::Clamp(MaximumIceHeightScale, 0.5f, 1.5f);
	if (MinimumIceHeightScale > MaximumIceHeightScale)
	{
		Swap(MinimumIceHeightScale, MaximumIceHeightScale);
	}
}

void AUOUWaterIcePhaseActor::ApplyPhaseState(
	bool bBroadcastChange,
	EUOUWaterIcePhase PreviousPhase)
{
	const bool bWater = CurrentPhase == EUOUWaterIcePhase::Water;
	if (WaterMesh != nullptr)
	{
		WaterMesh->SetVisibility(bWater, true);
		WaterMesh->SetHiddenInGame(!bWater, true);
		WaterMesh->SetCollisionEnabled(
			bWater ? WaterCollisionEnabled.GetValue() : ECollisionEnabled::NoCollision);
	}

	if (IceMesh != nullptr)
	{
		IceMesh->SetVisibility(!bWater, true);
		IceMesh->SetHiddenInGame(bWater, true);
		IceMesh->SetCollisionEnabled(
			bWater ? ECollisionEnabled::NoCollision : IceCollisionEnabled.GetValue());
	}

	if (WaterBlockingVolume != nullptr)
	{
		WaterBlockingVolume->SetCollisionEnabled(
			bWater ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}

	if (bBroadcastChange)
	{
		OnPhaseChanged.Broadcast(CurrentPhase, PreviousPhase);
	}
}

EUOUWaterIcePhase AUOUWaterIcePhaseActor::ResolveInitialPhase(float Temperature) const
{
	if (Temperature <= FreezeTemperature)
	{
		return EUOUWaterIcePhase::Ice;
	}

	if (Temperature >= MeltTemperature)
	{
		return EUOUWaterIcePhase::Water;
	}

	return InitialPhase;
}

void AUOUWaterIcePhaseActor::ResolvePlayerReferences()
{
	CachedPlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);
	CachedUmbrellaComponent = IsValid(CachedPlayerActor)
		? CachedPlayerActor->FindComponentByClass<UUOUUmbrellaComponent>()
		: nullptr;
}

bool AUOUWaterIcePhaseActor::IsInsidePlayerTileRange(
	const FVector& PlayerLocation,
	int32 RadiusInTiles) const
{
	const FVector TileLocation = GetActorLocation();
	if (FMath::Abs(PlayerLocation.Z - TileLocation.Z) > PlayerVerticalTolerance)
	{
		return false;
	}

	const float SafeTileSize = FMath::Max(1.0f, TileWorldSize);
	const float MaximumHorizontalDistance =
		(static_cast<float>(FMath::Max(0, RadiusInTiles)) + 0.5f) * SafeTileSize;
	return FMath::Abs(PlayerLocation.X - TileLocation.X) <= MaximumHorizontalDistance &&
		FMath::Abs(PlayerLocation.Y - TileLocation.Y) <= MaximumHorizontalDistance;
}

void AUOUWaterIcePhaseActor::CacheIceMeshBaseTransform()
{
	if (IceMesh == nullptr || bHasCachedIceMeshBaseTransform)
	{
		return;
	}

	BaseIceMeshRelativeLocation = IceMesh->GetRelativeLocation();
	BaseIceMeshRelativeScale = IceMesh->GetRelativeScale3D();
	bHasCachedIceMeshBaseTransform = true;
}

void AUOUWaterIcePhaseActor::ApplyIceHeightVariation()
{
	if (IceMesh == nullptr || !bHasCachedIceMeshBaseTransform)
	{
		return;
	}

	const float HeightScale = bUseIceHeightVariation ? ResolveIceHeightScale() : 1.0f;
	FVector NewScale = BaseIceMeshRelativeScale;
	NewScale.Z *= HeightScale;

	FVector NewLocation = BaseIceMeshRelativeLocation;
	if (const UStaticMesh* StaticMesh = IceMesh->GetStaticMesh())
	{
		const float MeshBottom = StaticMesh->GetBoundingBox().Min.Z;
		NewLocation.Z -= MeshBottom * (NewScale.Z - BaseIceMeshRelativeScale.Z);
	}

	IceMesh->SetRelativeScale3D(NewScale);
	IceMesh->SetRelativeLocation(NewLocation);
}

float AUOUWaterIcePhaseActor::ResolveIceHeightScale() const
{
	const float SafeTileSize = FMath::Max(1.0f, TileWorldSize);
	const FVector Location = GetActorLocation();
	const int32 GridX = FMath::RoundToInt(Location.X / SafeTileSize);
	const int32 GridY = FMath::RoundToInt(Location.Y / SafeTileSize);
	const uint32 GridHash = HashCombine(
		HashCombine(GetTypeHash(GridX), GetTypeHash(GridY)),
		GetTypeHash(IceHeightPatternSeed));
	FRandomStream PatternStream(static_cast<int32>(GridHash));
	return PatternStream.FRandRange(MinimumIceHeightScale, MaximumIceHeightScale);
}
