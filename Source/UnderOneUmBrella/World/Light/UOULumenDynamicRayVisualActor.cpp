// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULumenDynamicRayVisualActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName LumenRayBeamColorParameter(TEXT("BeamColor"));
	const FName LumenRayEmissiveIntensityParameter(TEXT("EmissiveIntensity"));
	const FName LumenRayOpacityParameter(TEXT("Opacity"));
	const FName LumenRayLengthParameter(TEXT("RayLength"));
	const FName DynamicRayJunctionClipStartEnabledParameter(TEXT("JunctionClipStartEnabled"));
	const FName DynamicRayJunctionClipStartPositionParameter(TEXT("JunctionClipStartPosition"));
	const FName DynamicRayJunctionClipStartNormalParameter(TEXT("JunctionClipStartNormal"));
	const FName DynamicRayJunctionClipEndEnabledParameter(TEXT("JunctionClipEndEnabled"));
	const FName DynamicRayJunctionClipEndPositionParameter(TEXT("JunctionClipEndPosition"));
	const FName DynamicRayJunctionClipEndNormalParameter(TEXT("JunctionClipEndNormal"));
	const FName DynamicRayJunctionClipFeatherParameter(TEXT("JunctionClipFeather"));
	constexpr float SourcePresetLength = 15.0f;

	void ApplyDynamicRayJunctionClipParameters(
		UMaterialInstanceDynamic* Material,
		const FUOULightBeamVisualSegmentData& SegmentData)
	{
		if (Material == nullptr)
		{
			return;
		}

		Material->SetScalarParameterValue(DynamicRayJunctionClipStartEnabledParameter, SegmentData.bUseStartJunctionClip ? 1.0f : 0.0f);
		Material->SetVectorParameterValue(DynamicRayJunctionClipStartPositionParameter, FLinearColor(SegmentData.StartJunctionPlanePosition));
		Material->SetVectorParameterValue(DynamicRayJunctionClipStartNormalParameter, FLinearColor(SegmentData.StartJunctionPlaneNormal));
		Material->SetScalarParameterValue(DynamicRayJunctionClipEndEnabledParameter, SegmentData.bUseEndJunctionClip ? 1.0f : 0.0f);
		Material->SetVectorParameterValue(DynamicRayJunctionClipEndPositionParameter, FLinearColor(SegmentData.EndJunctionPlanePosition));
		Material->SetVectorParameterValue(DynamicRayJunctionClipEndNormalParameter, FLinearColor(SegmentData.EndJunctionPlaneNormal));
		Material->SetScalarParameterValue(DynamicRayJunctionClipFeatherParameter, FMath::Max(0.0f, SegmentData.JunctionClipFeather));
	}

	struct FLumenRayLayer
	{
		int32 Shape = 0;
		float Brightness = 0.2f;
		FVector2D Position = FVector2D::ZeroVector;
		FVector2D Scale = FVector2D(1.0f, 1.0f);
		float Length = SourcePresetLength;
	};

	using FLumenRayPreset = TArray<FLumenRayLayer>;

	const TArray<FLumenRayPreset>& GetPresets()
	{
		static const TArray<FLumenRayPreset> Presets = {
			{{0, 0.186f, {0.0f, 0.0f}, {1.0f, 1.0f}, 15.0f}},
			{{0, 0.186f, {0.38f, -0.33f}, {0.43f, 0.61f}, 15.0f}, {7, 0.17f, {0.0f, 0.0f}, {1.0f, 1.0f}, 15.0f}, {0, 0.118f, {-0.34f, 0.21f}, {0.43f, 0.61f}, 15.0f}, {0, 0.153f, {0.0f, 0.0f}, {1.0f, 1.0f}, 15.0f}},
			{{2, 0.227f, {0.0f, 0.0f}, {1.0f, 1.0f}, 15.0f}},
			{{6, 0.227f, {0.0f, 0.0f}, {1.0f, 1.0f}, 15.0f}, {6, 0.159f, {-0.25f, 0.0f}, {1.0f, 0.6f}, 9.98f}, {6, 0.159f, {-0.08f, 0.27f}, {1.0f, 0.6f}, 9.98f}, {6, 0.159f, {0.25f, -0.04f}, {1.0f, 0.75f}, 16.2f}},
			{{4, 0.186f, {0.0f, 0.0f}, {1.0f, 1.0f}, 15.0f}},
			{{7, 0.1f, {-1.48f, 0.0f}, {1.0f, 1.0f}, 11.76f}, {7, 0.1f, {0.0f, 0.0f}, {1.0f, 1.0f}, 15.0f}, {7, 0.1f, {0.12f, 0.17f}, {0.5f, 0.5f}, 16.24f}, {7, 0.1f, {-0.23f, -0.4f}, {0.5f, 0.5f}, 10.6f}},
			{{7, 0.114f, {0.0f, 0.0f}, {1.0f, 1.0f}, 15.0f}, {7, 0.122f, {-0.45f, 0.69f}, {0.5f, 0.5f}, 10.0f}, {7, 0.122f, {-0.45f, -0.18f}, {0.5f, 0.5f}, 17.0f}, {7, 0.122f, {0.5f, 0.0f}, {0.5f, 0.5f}, 17.0f}},
			{{2, 0.173f, {0.0f, 0.0f}, {1.0f, 1.0f}, 9.2f}, {2, 0.173f, {0.46f, -1.01f}, {1.0f, 1.0f}, 11.22f}, {2, 0.173f, {0.46f, 0.62f}, {1.0f, 1.0f}, 8.4f}, {2, 0.173f, {1.21f, -0.66f}, {1.0f, 1.0f}, 11.6f}, {2, 0.173f, {-0.49f, 0.34f}, {1.0f, 1.0f}, 15.0f}}
		};
		return Presets;
	}
}

AUOULumenDynamicRayVisualActor::AUOULumenDynamicRayVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	for (int32 Index = 0; Index < MaxLayerCount; ++Index)
	{
		UStaticMeshComponent* Layer = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("RayLayer%d"), Index + 1));
		Layer->SetupAttachment(RootScene);
		LayerComponents.Add(Layer);
	}

	static const TCHAR* MeshPaths[] = {
		TEXT("/Game/UOU/Effects/StylizedLightFX/DynamicScatters/_LUMENDYNRAY00_Dynamic_Shaft_1._LUMENDYNRAY00_Dynamic_Shaft_1"),
		TEXT("/Game/UOU/Effects/StylizedLightFX/DynamicScatters/_LUMENDYNRAY01_Dynamic_Shaft_1_Hollow._LUMENDYNRAY01_Dynamic_Shaft_1_Hollow"),
		TEXT("/Game/UOU/Effects/StylizedLightFX/DynamicScatters/_LUMENDYNRAY02_Dynamic_Shaft_2._LUMENDYNRAY02_Dynamic_Shaft_2"),
		TEXT("/Game/UOU/Effects/StylizedLightFX/DynamicScatters/_LUMENDYNRAY03_Dynamic_Shaft_2_Hollow._LUMENDYNRAY03_Dynamic_Shaft_2_Hollow"),
		TEXT("/Game/UOU/Effects/StylizedLightFX/DynamicScatters/_LUMENDYNRAY04_Dynamic_Shaft_3._LUMENDYNRAY04_Dynamic_Shaft_3"),
		TEXT("/Game/UOU/Effects/StylizedLightFX/DynamicScatters/_LUMENDYNRAY05_Dynamic_Shaft_3_Hollow._LUMENDYNRAY05_Dynamic_Shaft_3_Hollow"),
		TEXT("/Game/UOU/Effects/StylizedLightFX/DynamicScatters/_LUMENDYNRAY06_Dynamic_Shaft_4._LUMENDYNRAY06_Dynamic_Shaft_4"),
		TEXT("/Game/UOU/Effects/StylizedLightFX/DynamicScatters/_LUMENDYNRAY07_Dynamic_Shaft_1_Gradient._LUMENDYNRAY07_Dynamic_Shaft_1_Gradient")
	};
	for (const TCHAR* MeshPath : MeshPaths)
	{
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(MeshPath);
		ShapeMeshes.Add(Finder.Succeeded() ? Finder.Object : nullptr);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
		TEXT("/Game/UOU/Effects/StylizedLightFX/Materials/M_SLF_Beam_Master.M_SLF_Beam_Master"));
	if (MaterialFinder.Succeeded())
	{
		RayMaterial = MaterialFinder.Object;
	}

	ConfigureComponents();
}

void AUOULumenDynamicRayVisualActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureComponents();
}

void AUOULumenDynamicRayVisualActor::ConfigureComponents()
{
	if (RayMaterial == nullptr || RayMaterial->GetName() == TEXT("M_SLF_LumenDynamicRay_Master"))
	{
		RayMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Game/UOU/Effects/StylizedLightFX/Materials/M_SLF_Beam_Master.M_SLF_Beam_Master"));
	}

	DynamicMaterials.SetNum(MaxLayerCount);
	for (int32 Index = 0; Index < LayerComponents.Num(); ++Index)
	{
		UStaticMeshComponent* Layer = LayerComponents[Index];
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetGenerateOverlapEvents(false);
		Layer->SetCastShadow(false);
		Layer->bReceivesDecals = false;
		Layer->SetTranslucentSortPriority(20 + Index);
		DynamicMaterials[Index] = nullptr;
		Layer->SetMaterial(0, RayMaterial);
	}
}

void AUOULumenDynamicRayVisualActor::EnsureDynamicMaterials()
{
	DynamicMaterials.SetNum(MaxLayerCount);
	for (int32 Index = 0; Index < LayerComponents.Num(); ++Index)
	{
		if (DynamicMaterials[Index] == nullptr && RayMaterial != nullptr)
		{
			DynamicMaterials[Index] = UMaterialInstanceDynamic::Create(RayMaterial, this);
			LayerComponents[Index]->SetMaterial(0, DynamicMaterials[Index]);
		}
	}
}

void AUOULumenDynamicRayVisualActor::ApplyLightBeamSegment_Implementation(
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	if (SegmentData.Length <= KINDA_SMALL_NUMBER || SegmentData.Direction.IsNearlyZero())
	{
		SetLightBeamVisualActive_Implementation(false);
		return;
	}

	SetActorLocationAndRotation(
		SegmentData.Start,
		FRotationMatrix::MakeFromZ(SegmentData.Direction.GetSafeNormal()).Rotator());
	ApplyPreset(SegmentData);
	SetLightBeamVisualActive_Implementation(true);
}

void AUOULumenDynamicRayVisualActor::ApplyPreset(
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	EnsureDynamicMaterials();
	const int32 EffectivePreset = SegmentData.LumenDynamicRayPresetOverride > 0
		? SegmentData.LumenDynamicRayPresetOverride
		: Preset;
	const FLumenRayPreset& SelectedPreset = GetPresets()[FMath::Clamp(EffectivePreset, 1, 8) - 1];
	const float Radius = FMath::Max(
		KINDA_SMALL_NUMBER,
		FMath::Max(SegmentData.StartRadius, SegmentData.EndRadius));

	for (int32 Index = 0; Index < LayerComponents.Num(); ++Index)
	{
		UStaticMeshComponent* Layer = LayerComponents[Index];
		const bool bLayerActive = SelectedPreset.IsValidIndex(Index);
		Layer->SetVisibility(bLayerActive, true);
		if (!bLayerActive)
		{
			continue;
		}

		const FLumenRayLayer& LayerData = SelectedPreset[Index];
		UStaticMesh* ShapeMesh = ShapeMeshes.IsValidIndex(LayerData.Shape)
			? ShapeMeshes[LayerData.Shape]
			: nullptr;
		Layer->SetStaticMesh(ShapeMesh);
		Layer->SetRelativeLocation(FVector(
			LayerData.Position.X * Radius,
			LayerData.Position.Y * Radius,
			0.0f));

		const float LayerLength = FMath::Min(
			SegmentData.Length,
			SegmentData.Length * (LayerData.Length / SourcePresetLength));
		const FVector MeshSize = ShapeMesh != nullptr
			? ShapeMesh->GetBounds().BoxExtent * 2.0f
			: FVector::OneVector;
		const float MeshRadiusX = FMath::Max(KINDA_SMALL_NUMBER, MeshSize.X * 0.5f);
		const float MeshRadiusY = FMath::Max(KINDA_SMALL_NUMBER, MeshSize.Y * 0.5f);
		const float MeshLength = FMath::Max(KINDA_SMALL_NUMBER, MeshSize.Z);
		Layer->SetRelativeScale3D(FVector(
			(Radius / MeshRadiusX) * LayerData.Scale.X,
			(Radius / MeshRadiusY) * LayerData.Scale.Y,
			LayerLength / MeshLength));

		if (!DynamicMaterials.IsValidIndex(Index) || DynamicMaterials[Index] == nullptr)
		{
			continue;
		}

		DynamicMaterials[Index]->SetVectorParameterValue(LumenRayBeamColorParameter, SegmentData.Color);
		DynamicMaterials[Index]->SetScalarParameterValue(
			LumenRayEmissiveIntensityParameter,
			SegmentData.Intensity * SegmentData.VisualBrightnessMultiplier *
			EmissiveIntensityScale * LayerData.Brightness);
		DynamicMaterials[Index]->SetScalarParameterValue(
			LumenRayOpacityParameter,
			FMath::Clamp(OpacityScale * SegmentData.VisualOpacityMultiplier, 0.0f, 1.0f));
		DynamicMaterials[Index]->SetScalarParameterValue(LumenRayLengthParameter, LayerLength);
		ApplyDynamicRayJunctionClipParameters(DynamicMaterials[Index], SegmentData);
	}
}

void AUOULumenDynamicRayVisualActor::SetLightBeamVisualActive_Implementation(const bool bActive)
{
	SetActorHiddenInGame(!bActive);
	for (UStaticMeshComponent* Layer : LayerComponents)
	{
		if (!bActive)
		{
			Layer->SetVisibility(false, true);
		}
	}
}
