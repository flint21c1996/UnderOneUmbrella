// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformTargetActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

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
	TargetPreviewMesh->bEditableWhenInherited = true;
	TargetPreviewMesh->SetupAttachment(RootScene);
	TargetPreviewMesh->SetMobility(EComponentMobility::Movable);
	TargetPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TargetPreviewMesh->SetGenerateOverlapEvents(false);
	TargetPreviewMesh->SetHiddenInGame(true);
	TargetPreviewMesh->SetCastShadow(false);
	TargetPreviewMesh->SetRenderCustomDepth(true);
	ApplyPreviewMaterialSettings(nullptr);
}

void AUOUFloorPlatformTargetActor::SyncPreviewFromMesh(const UStaticMeshComponent* SourceMesh)
{
	if (SourceMesh == nullptr || TargetPreviewMesh == nullptr)
	{
		return;
	}

	TargetPreviewMesh->SetStaticMesh(SourceMesh->GetStaticMesh());
	TargetPreviewMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());

	ApplyPreviewMaterialSettings(SourceMesh);
}

void AUOUFloorPlatformTargetActor::SetTargetPreviewMeshVisible(bool bVisible)
{
	if (TargetPreviewMesh == nullptr)
	{
		return;
	}

	TargetPreviewMesh->SetVisibility(bVisible, true);
}

EUOUFloorPlatformRotationMode AUOUFloorPlatformTargetActor::ResolveRotationMode(EUOUFloorPlatformRotationMode PlatformDefault) const
{
	switch (StepRotationMode)
	{
	case EUOUFloorPlatformStepRotationMode::TransformLerp:
		return EUOUFloorPlatformRotationMode::TransformLerp;
	case EUOUFloorPlatformStepRotationMode::Hinge:
		return EUOUFloorPlatformRotationMode::Hinge;
	case EUOUFloorPlatformStepRotationMode::UsePlatformDefault:
	default:
		return PlatformDefault;
	}
}

EUOUFloorPlatformHingeEdge AUOUFloorPlatformTargetActor::ResolveHingeEdge(EUOUFloorPlatformHingeEdge PlatformDefault) const
{
	return StepRotationMode == EUOUFloorPlatformStepRotationMode::Hinge
		? StepHingeEdge
		: PlatformDefault;
}

FVector AUOUFloorPlatformTargetActor::ResolveCustomHingeLocalOffset(const FVector& PlatformDefault) const
{
	return StepRotationMode == EUOUFloorPlatformStepRotationMode::Hinge
		? StepCustomHingeLocalOffset
		: PlatformDefault;
}

void AUOUFloorPlatformTargetActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyPreviewMaterialSettings(nullptr);
}

#if WITH_EDITOR
void AUOUFloorPlatformTargetActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplyPreviewMaterialSettings(nullptr);
}
#endif

void AUOUFloorPlatformTargetActor::ApplyPreviewMaterialSettings(const UStaticMeshComponent* SourceMesh)
{
	if (TargetPreviewMesh == nullptr)
	{
		return;
	}

	// Overlay Material은 실제 머티리얼 슬롯 변경을 가려서, 미리보기 머티리얼은 슬롯에 직접 적용합니다.
	TargetPreviewMesh->SetOverlayMaterial(nullptr);

	if (!bOverrideTargetPreviewMeshMaterial)
	{
		return;
	}

	const int32 MaterialCount = FMath::Max(1, TargetPreviewMesh->GetNumMaterials());
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInterface* PreviewMaterial = TargetPreviewMaterial.Get();
		if (PreviewMaterial == nullptr && SourceMesh != nullptr)
		{
			PreviewMaterial = SourceMesh->GetMaterial(MaterialIndex);
		}

		if (PreviewMaterial != nullptr)
		{
			TargetPreviewMesh->SetMaterial(MaterialIndex, PreviewMaterial);
		}
	}
}
