// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightBeamMeshVisualActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float EngineBasicShapeRadius = 50.0f;
	constexpr float EngineBasicShapeLength = 100.0f;
	const FName BeamColorParameter(TEXT("BeamColor"));
	const FName EmissiveIntensityParameter(TEXT("EmissiveIntensity"));
	const FName OpacityParameter(TEXT("Opacity"));
}

AUOULightBeamMeshVisualActor::AUOULightBeamMeshVisualActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	BeamMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
	BeamMeshComponent->SetupAttachment(RootScene);
	ConfigureMeshComponent(BeamMeshComponent);

	CoreBeamMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreBeamMesh"));
	CoreBeamMeshComponent->SetupAttachment(RootScene);
	ConfigureMeshComponent(CoreBeamMeshComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		CylinderMesh = CylinderMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshFinder(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshFinder.Succeeded())
	{
		ConeMesh = ConeMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> OriginalLumenConeMeshFinder(
		TEXT("/Game/UOU/Effects/StylizedLightFX/StaticScatters/"
			"_LUMENRAY42_Spotlight_Scatter_1._LUMENRAY42_Spotlight_Scatter_1"));
	if (OriginalLumenConeMeshFinder.Succeeded())
	{
		OriginalLumenConeMesh = OriginalLumenConeMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BeamMaterialFinder(
		TEXT("/Game/UOU/Effects/StylizedLightFX/Materials/M_SLF_Beam_Master.M_SLF_Beam_Master"));
	if (BeamMaterialFinder.Succeeded())
	{
		BeamMaterial = BeamMaterialFinder.Object;
	}

	BeamMeshComponent->SetStaticMesh(CylinderMesh);
	CoreBeamMeshComponent->SetStaticMesh(CylinderMesh);
}

void AUOULightBeamMeshVisualActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BeamMeshComponent->SetTranslucentSortPriority(BaseTranslucencySortPriority);
	CoreBeamMeshComponent->SetTranslucentSortPriority(BaseTranslucencySortPriority + 1);
	CoreBeamMeshComponent->SetVisibility(bEnableCoreLayer, true);
	SetActorTickEnabled(bEnableVariation);
	EnsureDynamicMaterials();
}

void AUOULightBeamMeshVisualActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bEnableVariation && GetWorld() != nullptr)
	{
		UpdateAnimatedMaterialParameters(GetWorld()->GetTimeSeconds());
	}
}

void AUOULightBeamMeshVisualActor::ApplyLightBeamSegment_Implementation(
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	const FVector SafeDirection = SegmentData.Direction.GetSafeNormal();
	const bool bHasRenderableSegment =
		SegmentData.Length > KINDA_SMALL_NUMBER && !SafeDirection.IsNearlyZero();
	if (!bHasRenderableSegment)
	{
		SetLightBeamVisualActive_Implementation(false);
		return;
	}

	const bool bUseCone = bUseConeForExpandingSegments &&
		ConeMesh != nullptr &&
		FMath::Abs(SegmentData.EndRadius - SegmentData.StartRadius) > CylinderRadiusTolerance;

	UStaticMesh* SelectedMesh = CylinderMesh;
	if (bUseCone)
	{
		SelectedMesh = bUseOriginalLumenConeMesh && OriginalLumenConeMesh != nullptr
			? OriginalLumenConeMesh
			: ConeMesh;
	}

	BeamMeshComponent->SetStaticMesh(SelectedMesh);
	CoreBeamMeshComponent->SetStaticMesh(SelectedMesh);
	ApplyMeshTransform(SegmentData, bUseCone, SelectedMesh);
	ApplyMaterialParameters(SegmentData);
	BeamMeshComponent->SetTranslucentSortPriority(
		BaseTranslucencySortPriority + (SegmentData.bReflected ? 1 : 0));
	CoreBeamMeshComponent->SetTranslucentSortPriority(
		BaseTranslucencySortPriority + (SegmentData.bReflected ? 1 : 0) + 1);
	SetLightBeamVisualActive_Implementation(true);
}

void AUOULightBeamMeshVisualActor::SetLightBeamVisualActive_Implementation(const bool bActive)
{
	SetActorHiddenInGame(!bActive);
	BeamMeshComponent->SetVisibility(bActive, true);
	CoreBeamMeshComponent->SetVisibility(bActive && bEnableCoreLayer, true);
}

void AUOULightBeamMeshVisualActor::ConfigureMeshComponent(UStaticMeshComponent* MeshComponent) const
{
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCastShadow(false);
	MeshComponent->bReceivesDecals = false;
	MeshComponent->SetTranslucentSortPriority(BaseTranslucencySortPriority);
}

void AUOULightBeamMeshVisualActor::EnsureDynamicMaterials()
{
	if (BeamMaterial == nullptr)
	{
		DynamicBeamMaterial = nullptr;
		DynamicCoreMaterial = nullptr;
		DynamicMaterialSource.Reset();
		return;
	}

	if (DynamicBeamMaterial == nullptr ||
		DynamicCoreMaterial == nullptr ||
		DynamicMaterialSource.Get() != BeamMaterial)
	{
		DynamicBeamMaterial = UMaterialInstanceDynamic::Create(BeamMaterial, this);
		DynamicCoreMaterial = UMaterialInstanceDynamic::Create(BeamMaterial, this);
		DynamicMaterialSource = BeamMaterial;
	}

	if (DynamicBeamMaterial != nullptr)
	{
		BeamMeshComponent->SetMaterial(0, DynamicBeamMaterial);
	}

	if (DynamicCoreMaterial != nullptr)
	{
		CoreBeamMeshComponent->SetMaterial(0, DynamicCoreMaterial);
	}
}

void AUOULightBeamMeshVisualActor::ApplyMaterialParameters(
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	EnsureDynamicMaterials();
	if (DynamicBeamMaterial == nullptr || DynamicCoreMaterial == nullptr)
	{
		return;
	}

	CurrentBeamColor = SegmentData.Color;
	CurrentEmissiveIntensity = FMath::Max(
		0.0f,
		SegmentData.Intensity * EmissiveIntensityScale * SegmentData.VisualBrightnessMultiplier);
	DynamicBeamMaterial->SetVectorParameterValue(BeamColorParameter, CurrentBeamColor);
	DynamicCoreMaterial->SetVectorParameterValue(BeamColorParameter, CurrentBeamColor);
	const float InstanceOpacity = Opacity * SegmentData.VisualOpacityMultiplier;
	DynamicBeamMaterial->SetScalarParameterValue(OpacityParameter, FMath::Clamp(InstanceOpacity, 0.0f, 1.0f));
	DynamicCoreMaterial->SetScalarParameterValue(
		OpacityParameter,
		FMath::Clamp(InstanceOpacity * CoreOpacityMultiplier, 0.0f, 1.0f));
	UpdateAnimatedMaterialParameters(GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f);
}

void AUOULightBeamMeshVisualActor::UpdateAnimatedMaterialParameters(const float TimeSeconds)
{
	if (DynamicBeamMaterial == nullptr || DynamicCoreMaterial == nullptr)
	{
		return;
	}

	float VariationMultiplier = 1.0f;
	if (bEnableVariation && VariationSpeed > 0.0f && VariationAmount > 0.0f)
	{
		const float PrimaryWave = FMath::Sin(TimeSeconds * VariationSpeed * UE_TWO_PI);
		const float SecondaryWave = FMath::Sin(
			TimeSeconds * VariationSpeed * UE_TWO_PI * 0.37f + 1.7f);
		VariationMultiplier += VariationAmount * (PrimaryWave * 0.65f + SecondaryWave * 0.35f);
	}

	const float AnimatedIntensity = FMath::Max(0.0f, CurrentEmissiveIntensity * VariationMultiplier);
	DynamicBeamMaterial->SetScalarParameterValue(EmissiveIntensityParameter, AnimatedIntensity);
	DynamicCoreMaterial->SetScalarParameterValue(
		EmissiveIntensityParameter,
		AnimatedIntensity * CoreIntensityMultiplier);
}

void AUOULightBeamMeshVisualActor::ApplyMeshTransform(
	const FUOULightBeamVisualSegmentData& SegmentData,
	const bool bUseCone,
	const UStaticMesh* SelectedMesh)
{
	const FVector SafeDirection = SegmentData.Direction.GetSafeNormal();
	const FVector Midpoint = SegmentData.Start + SafeDirection * (SegmentData.Length * 0.5f);
	const float VisualRadius = FMath::Max(
		KINDA_SMALL_NUMBER,
		FMath::Max(SegmentData.StartRadius, SegmentData.EndRadius));
	float MeshRadius = EngineBasicShapeRadius;
	float MeshLength = EngineBasicShapeLength;
	FVector MeshBoundsCenter = FVector::ZeroVector;
	if (SelectedMesh != nullptr)
	{
		const FBox MeshBounds = SelectedMesh->GetBoundingBox();
		const FVector MeshSize = MeshBounds.GetSize();
		MeshRadius = FMath::Max(KINDA_SMALL_NUMBER, FMath::Max(MeshSize.X, MeshSize.Y) * 0.5f);
		MeshLength = FMath::Max(KINDA_SMALL_NUMBER, MeshSize.Z);
		MeshBoundsCenter = MeshBounds.GetCenter();
	}

	const float RadiusScale = VisualRadius / MeshRadius;
	const float LengthScale = SegmentData.Length / MeshLength;
	const FVector MeshAxis = bUseCone && bReverseConeAxis ? -SafeDirection : SafeDirection;
	const FRotator MeshRotation = FRotationMatrix::MakeFromZ(MeshAxis).Rotator();
	const FVector MeshScale(RadiusScale, RadiusScale, LengthScale);
	const FVector BoundsOffset = MeshRotation.RotateVector(MeshBoundsCenter * MeshScale);

	BeamMeshComponent->SetWorldLocationAndRotation(
		Midpoint - BoundsOffset,
		MeshRotation);
	BeamMeshComponent->SetWorldScale3D(MeshScale);
	CoreBeamMeshComponent->SetWorldLocationAndRotation(
		Midpoint - BoundsOffset,
		MeshRotation);
	CoreBeamMeshComponent->SetWorldScale3D(FVector(
		RadiusScale * CoreRadiusScale,
		RadiusScale * CoreRadiusScale,
		LengthScale * 0.98f));
}
