// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformTargetActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AUOUFloorPlatformTargetActor::AUOUFloorPlatformTargetActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PreviewMaterialFinder(TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent"));
	if (PreviewMaterialFinder.Succeeded())
	{
		TargetPreviewMaterial = PreviewMaterialFinder.Object;
	}

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	RootScene->SetMobility(EComponentMobility::Movable);

	TargetOriginMarker = CreateDefaultSubobject<USphereComponent>(TEXT("TargetOriginMarker"));
	TargetOriginMarker->bEditableWhenInherited = false;
	TargetOriginMarker->SetupAttachment(RootScene);
	TargetOriginMarker->SetMobility(EComponentMobility::Movable);
	TargetOriginMarker->SetSphereRadius(34.0f);
	TargetOriginMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TargetOriginMarker->SetGenerateOverlapEvents(false);
	TargetOriginMarker->SetHiddenInGame(true);
	TargetOriginMarker->ShapeColor = FColor::Green;

	TargetPreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetPreviewMesh"));
	TargetPreviewMesh->bEditableWhenInherited = false;
	TargetPreviewMesh->SetupAttachment(RootScene);
	TargetPreviewMesh->SetMobility(EComponentMobility::Movable);
	TargetPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TargetPreviewMesh->SetGenerateOverlapEvents(false);
	TargetPreviewMesh->SetHiddenInGame(true);
	TargetPreviewMesh->SetCastShadow(false);
	TargetPreviewMesh->SetRenderCustomDepth(true);
	TargetPreviewMesh->SetOverlayMaterial(TargetPreviewMaterial);
}

void AUOUFloorPlatformTargetActor::SyncPreviewFromMesh(const UStaticMeshComponent* SourceMesh)
{
	if (SourceMesh == nullptr || TargetPreviewMesh == nullptr)
	{
		return;
	}

	TargetPreviewMesh->SetStaticMesh(SourceMesh->GetStaticMesh());
	TargetPreviewMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());

	const int32 MaterialCount = SourceMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInterface* PreviewMaterial = TargetPreviewMaterial.Get();
		if (PreviewMaterial == nullptr)
		{
			PreviewMaterial = SourceMesh->GetMaterial(MaterialIndex);
		}

		TargetPreviewMesh->SetMaterial(MaterialIndex, PreviewMaterial);
	}
}

void AUOUFloorPlatformTargetActor::SetTargetPreviewMeshVisible(bool bVisible)
{
	if (TargetPreviewMesh == nullptr)
	{
		return;
	}

	TargetPreviewMesh->SetVisibility(bVisible, true);
}
