// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUPlayerBlockingWallActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AUOUPlayerBlockingWallActor::AUOUPlayerBlockingWallActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	BlockingVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingVolume"));
	BlockingVolume->SetupAttachment(RootScene);
	BlockingVolume->SetBoxExtent(WallExtent);
	BlockingVolume->SetCanEverAffectNavigation(false);

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(RootScene);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetGenerateOverlapEvents(false);
	PreviewMesh->SetCastShadow(false);
	PreviewMesh->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		PreviewMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PreviewMaterialFinder(TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent"));
	if (PreviewMaterialFinder.Succeeded())
	{
		PreviewMaterial = PreviewMaterialFinder.Object;
		PreviewMesh->SetMaterial(0, PreviewMaterial);
	}

	ApplyCollisionSettings();
	ApplyPreviewSettings();
}

void AUOUPlayerBlockingWallActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyCollisionSettings();
	ApplyPreviewSettings();
}

void AUOUPlayerBlockingWallActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyCollisionSettings();
	ApplyPreviewSettings();
}

void AUOUPlayerBlockingWallActor::ApplyCollisionSettings()
{
	if (BlockingVolume == nullptr)
	{
		return;
	}

	BlockingVolume->SetBoxExtent(WallExtent);
	BlockingVolume->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingVolume->SetCollisionObjectType(ECC_WorldStatic);
	BlockingVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingVolume->SetCollisionResponseToChannel(BlockedChannel, ECR_Block);
	BlockingVolume->SetGenerateOverlapEvents(false);
}

void AUOUPlayerBlockingWallActor::ApplyPreviewSettings()
{
	if (PreviewMesh == nullptr || BlockingVolume == nullptr)
	{
		return;
	}

	if (bOverridePreviewMeshMaterial && PreviewMaterial != nullptr)
	{
		PreviewMesh->SetMaterial(0, PreviewMaterial);
	}

	// 엔진 기본 Cube는 한 변이 100cm라서 Box Extent를 50으로 나누면 충돌 박스와 같은 크기가 됩니다.
	const FVector SafeExtent(
		FMath::Max(1.0f, WallExtent.X),
		FMath::Max(1.0f, WallExtent.Y),
		FMath::Max(1.0f, WallExtent.Z));

	PreviewMesh->SetRelativeLocation(BlockingVolume->GetRelativeLocation());
	PreviewMesh->SetRelativeRotation(BlockingVolume->GetRelativeRotation());
	PreviewMesh->SetRelativeScale3D(SafeExtent / 50.0f);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const UWorld* World = GetWorld();
	const bool bIsGameWorld = World != nullptr && World->IsGameWorld();
	const bool bShouldShowPreview = bIsGameWorld ? bShowPreviewInGame : bShowPreviewInEditor;

	PreviewMesh->SetVisibility(bShouldShowPreview, true);
	PreviewMesh->SetHiddenInGame(!bShowPreviewInGame);
}
