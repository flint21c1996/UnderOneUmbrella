// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULumenStaticRayVisualActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const FName StaticRayBeamColorParameter(TEXT("BeamColor"));
	const FName StaticRayVariationColorParameter(TEXT("VariationColor"));
	const FName StaticRayEmissiveIntensityParameter(TEXT("EmissiveIntensity"));
	const FName StaticRayOpacityParameter(TEXT("Opacity"));
	const FName StaticRayVariationAmountParameter(TEXT("VariationAmount"));
	const FName StaticRayVariationSpeedParameter(TEXT("VariationSpeed"));
	const FName StaticRayVariationScaleParameter(TEXT("VariationScale"));
	const FName JunctionClipStartEnabledParameter(TEXT("JunctionClipStartEnabled"));
	const FName JunctionClipStartPositionParameter(TEXT("JunctionClipStartPosition"));
	const FName JunctionClipStartNormalParameter(TEXT("JunctionClipStartNormal"));
	const FName JunctionClipEndEnabledParameter(TEXT("JunctionClipEndEnabled"));
	const FName JunctionClipEndPositionParameter(TEXT("JunctionClipEndPosition"));
	const FName JunctionClipEndNormalParameter(TEXT("JunctionClipEndNormal"));
	const FName JunctionClipFeatherParameter(TEXT("JunctionClipFeather"));

	void ApplyJunctionClipParameters(
		UMaterialInstanceDynamic* Material,
		const FUOULightBeamVisualSegmentData& SegmentData)
	{
		if (Material == nullptr)
		{
			return;
		}

		Material->SetScalarParameterValue(
			JunctionClipStartEnabledParameter,
			SegmentData.bUseStartJunctionClip ? 1.0f : 0.0f);
		Material->SetVectorParameterValue(
			JunctionClipStartPositionParameter,
			FLinearColor(SegmentData.StartJunctionPlanePosition));
		Material->SetVectorParameterValue(
			JunctionClipStartNormalParameter,
			FLinearColor(SegmentData.StartJunctionPlaneNormal));
		Material->SetScalarParameterValue(
			JunctionClipEndEnabledParameter,
			SegmentData.bUseEndJunctionClip ? 1.0f : 0.0f);
		Material->SetVectorParameterValue(
			JunctionClipEndPositionParameter,
			FLinearColor(SegmentData.EndJunctionPlanePosition));
		Material->SetVectorParameterValue(
			JunctionClipEndNormalParameter,
			FLinearColor(SegmentData.EndJunctionPlaneNormal));
		Material->SetScalarParameterValue(
			JunctionClipFeatherParameter,
			FMath::Max(0.0f, SegmentData.JunctionClipFeather));
	}

	struct FLumenStaticRayLayer
	{
		int32 Shape = 0;
		float Brightness = 1.0f;
		FLinearColor Color = FLinearColor::White;
		FVector Position = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector Scale = FVector::OneVector;
	};

	struct FLumenStaticRayPreset
	{
		TArray<FLumenStaticRayLayer> Layers;
		float GlobalScale = 0.8f;
		float GlobalBrightness = 0.368f;
		float VariationSpeed = 1.0f;
		float VariationScale = 10.0f;
		bool bUseVariation = false;
	};

#define LAYER(Shape, Brightness, R, G, B, A, PX, PY, PZ, RX, RY, RZ, SX, SY, SZ) \
	{Shape, Brightness, FLinearColor(R, G, B, A), FVector(PX, PY, PZ), FRotator(RY, RZ, RX), FVector(SX, SY, SZ)}

	const TArray<FLumenStaticRayPreset>& GetStaticRayPresets()
	{
		static const TArray<FLumenStaticRayPreset> Presets = {
			{{LAYER(30,1,1,1,1,.24705882, .88,3.39,0, 0,0,0, 1,1,.85), LAYER(30,1,1,1,1,.24705882, -.83,0,.58, 0,0,0, 1.2,1.2,.85), LAYER(30,1,1,1,1,.24705882, -.95,0,0, 0,0,0, 1,1,1), LAYER(30,1,1,1,1,.24705882, 2.41,0,-.31, 0,0,0, 1,1,1.27), LAYER(30,1,1,1,1,.24705882, -.88,0,-1.17, 0,0,0, 1,1,1.27)},.8,.368,1,10,true},
			{{LAYER(1,1,2,2,2,1, 0,0,0, 0,0,0, 1,-.05,1), LAYER(1,1,1,1,1,1, 0,0,0, 0,0,0, 1,.29,1), LAYER(3,.275,1,1,1,1, 0,0,3.1, 0,0,0, 1,.89,1.12)},.8,.368,1,10,true},
			{{LAYER(4,.407,1,1,1,1, 0,0,0, 0,0,0, 1,1,1), LAYER(5,.252,1,1,1,1, -.89,-.51,0, 0,0,0, 1,1,1), LAYER(5,.269,1,1,1,1, .89,1.65,0, 0,0,0, 1,.81,1), LAYER(5,.221,1,1,1,1, .89,-1.23,0, 0,0,0, 1,.81,1)},.8,.368,1,10,true},
			{{LAYER(14,.203,1,1,1,1, 0,0,3.13, 0,0,0, 2,2,1.4), LAYER(16,.393,1,1,1,1, -.76,.45,0, 0,0,0, 1,1,1), LAYER(15,.393,1,1,1,1, .43,-.32,0, 0,0,0, 1.4,1.4,1)},.8,.368,1,10,true},
			{{LAYER(25,1,1,1,1,1, 0,0,0, 0,0,0, 1,1,1), LAYER(22,1,.509434,.509434,.509434,1, 0,0,0, 0,0,0, 2,2,1), LAYER(22,1,1,1,1,1, -.78,.56,0, 0,0,0, 1,1,1), LAYER(22,1,1,1,1,1, .86,.14,0, 0,0,0, 1,1,1)},.8,.368,1,10,true},
			{{LAYER(30,1,.61,.61,.61,1, 0,0,0, 0,0,0, 1,1,1), LAYER(34,1,.681,.681,.681,1, 1.42,.31,0, 0,0,0, 1,1,1), LAYER(34,1,.5471698,.5471698,.5471698,1, -.55,.7,-1.03, 0,0,0, 1,1,.9)},.8,.368,1,10,true},
			{{LAYER(30,1,.6320754,.6320754,.6320754,1, 0,0,0, 70,-45,0, 1,1,1.59), LAYER(30,1,.7264151,.7264151,.7264151,1, -1.37,0,-2.3, 70,-45,0, 1,1,1.15), LAYER(30,1,.7169812,.7169812,.7169812,1, -1.72,0,2.87, 70,-45,0, 1,1,1.38), LAYER(30,1,.6981132,.6981132,.6981132,1, 3.81,0,3.96, 70,-45,0, 1,.79,1.46), LAYER(30,1,.6981132,.6981132,.6981132,1, 2.68,0,0, 70,-45,0, 1.24,1.18,1.22)},.8,.368,1,10,false},
			{{LAYER(42,.317,1,1,1,1, 0,0,0, 90,0,0, 2.2,2.2,2.2), LAYER(37,.314,1,1,1,1, 0,0,0, 90,0,0, 2,2,2)},.8,.368,1,10,false},
			{{LAYER(11,1,1,1,1,1, 0,0,0, 90,90,0, 1,1,1), LAYER(8,.147,1,1,1,1, -4.2,1.38,0, 107.35,90,0, .7,.7,.7), LAYER(4,.052,1,1,1,1, -1.3,1.38,0, 98.16,90,0, .7,.7,.7), LAYER(4,.06,1,1,1,1, 2.96,1.38,0, 76.3,90,0, .7,.7,.7)},.8,.368,1,10,false},
			{{LAYER(30,1,1,1,1,1, 0,0,0, 0,0,0, 1,1,1)},.8,.368,1,10,false},
			{{LAYER(15,1,1,1,1,1, 0,0,0, 0,0,0, 1,1,1)},.8,.368,1,10,false},
			{{LAYER(22,1,1,1,1,1, 0,0,0, 0,0,0, 1,1,1)},.8,.368,1,10,false},
			{{LAYER(4,1,1,1,1,1, 0,0,0, 0,0,0, 1,1,1)},.8,.368,1,10,false},
			{{LAYER(34,1,1,1,1,1, 0,0,0, 0,0,0, 1,1,1)},.8,.368,1,10,false},
			{{LAYER(25,1,1,1,1,1, 0,0,0, 0,0,0, 1,1,1)},.8,.368,1,10,false},
			{{LAYER(18,1,1,1,1,1, 0,0,0, 0,0,0, 1,1,1)},.8,.368,1,10,false},
			{{LAYER(21,1,1,1,1,1, 0,0,0, 0,0,0, 1,1,1)},.8,.368,1,10,false},
			{{LAYER(42,.286,1,1,1,1, 0,0,0, 0,0,0, 2.2,2.2,2.2), LAYER(42,.496,1,1,1,1, 0,0,0, 0,0,0, 1,1,1), LAYER(42,.461,1,1,1,1, 0,0,0, 0,0,0, 1.8,1.8,1.8), LAYER(41,.3,1,1,1,1, 0,0,0, 0,0,0, 2,2,2)},.8,.368,1,10,true},
			{{LAYER(18,1,1,1,1,1, 0,0,14.28, 0,0,0, 1,1,1), LAYER(42,.342,1,1,1,1, 0,0,0, 0,0,0, 2,2,2)},.8,.368,1,10,true}
		};
		return Presets;
	}

#undef LAYER

	const TCHAR* StaticShapeNames[] = {
		TEXT("_LUMENRAY00_2D_Laser_Ray_1"), TEXT("_LUMENRAY01_2D_Laser_Ray_2"), TEXT("_LUMENRAY02_2D_Laser_Ray_3"), TEXT("_LUMENRAY03_2D_Laser_Ray_4"),
		TEXT("_LUMENRAY04_2D_Ray_1"), TEXT("_LUMENRAY05_2D_Ray_2"), TEXT("_LUMENRAY06_2D_Ray_3"), TEXT("_LUMENRAY07_2D_Ray_4"), TEXT("_LUMENRAY08_2D_Ray_5"), TEXT("_LUMENRAY09_2D_Ray_6"),
		TEXT("_LUMENRAY10_2D_Spot_Ray_1"), TEXT("_LUMENRAY11_2D_Spot_Ray_2"), TEXT("_LUMENRAY12_2D_Spot_Ray_3"), TEXT("_LUMENRAY13_2D_Spot_Ray_4"),
		TEXT("_LUMENRAY14_Cylindrical_Scatter_1"), TEXT("_LUMENRAY15_Cylindrical_Scatter_2"), TEXT("_LUMENRAY16_Cylindrical_Scatter_3"), TEXT("_LUMENRAY17_Cylindrical_Scatter_4"), TEXT("_LUMENRAY18_Cylindrical_Scatter_5"), TEXT("_LUMENRAY19_Cylindrical_Scatter_6"), TEXT("_LUMENRAY20_Cylindrical_Scatter_7"), TEXT("_LUMENRAY21_Cylindrical_Scatter_8"),
		TEXT("_LUMENRAY22_Prism_Scatter_1"), TEXT("_LUMENRAY23_Prism_Scatter_2"), TEXT("_LUMENRAY24_Prism_Scatter_3"), TEXT("_LUMENRAY25_Prism_Scatter_4"), TEXT("_LUMENRAY26_Prism_Scatter_5"), TEXT("_LUMENRAY27_Prism_Scatter_6"), TEXT("_LUMENRAY28_Prism_Scatter_7"), TEXT("_LUMENRAY29_Prism_Scatter_8"),
		TEXT("_LUMENRAY30_Rayleigh_Scatter_1"), TEXT("_LUMENRAY31_Rayleigh_Scatter_2"), TEXT("_LUMENRAY32_Rayleigh_Scatter_3"), TEXT("_LUMENRAY33_Rayleigh_Scatter_5"), TEXT("_LUMENRAY34_Rayleigh_Scatter_5"), TEXT("_LUMENRAY35_Rayleigh_Scatter_6"), TEXT("_LUMENRAY36_Rayleigh_Scatter_7"),
		TEXT("_LUMENRAY37_Spotlight_Rays_1"), TEXT("_LUMENRAY38_Spotlight_Rays_2"), TEXT("_LUMENRAY39_Spotlight_Rays_3"), TEXT("_LUMENRAY40_Spotlight_Rays_4"), TEXT("_LUMENRAY41_Spotlight_Rays_5"),
		TEXT("_LUMENRAY42_Spotlight_Scatter_1"), TEXT("_LUMENRAY43_Spotlight_Scatter_2"), TEXT("_LUMENRAY44_Spotlight_Scatter_3"), TEXT("_LUMENRAY45_Spotlight_Scatter_4"), TEXT("_LUMENRAY46_Spotlight_Scatter_5"), TEXT("_LUMENRAY47_Spotlight_Scatter_6"), TEXT("_LUMENRAY48_Spotlight_Scatter_7"), TEXT("_LUMENRAY49_Spotlight_Scatter_8"), TEXT("_LUMENRAY50_Solid_Scatter_1")
	};
}

AUOULumenStaticRayVisualActor::AUOULumenStaticRayVisualActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// 플레이어 카메라 회전 보간이 끝난 뒤 빌보드 방향을 계산해 한 프레임 지연으로 인한 떨림을 방지합니다.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetActorEnableCollision(false);
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	CameraFacingRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraFacingRoot"));
	CameraFacingRoot->SetupAttachment(RootScene);

	for (int32 Index = 0; Index < MaxLayerCount; ++Index)
	{
		UStaticMeshComponent* Layer = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("StaticRayLayer%d"), Index + 1));
		Layer->SetupAttachment(CameraFacingRoot);
		LayerComponents.Add(Layer);
	}

	for (const TCHAR* ShapeName : StaticShapeNames)
	{
		const FString Path = FString::Printf(TEXT("/Game/UOU/Effects/StylizedLightFX/StaticScatters/%s.%s"), ShapeName, ShapeName);
		ShapeMeshes.Add(LoadObject<UStaticMesh>(nullptr, *Path));
	}

	RayMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UOU/Effects/StylizedLightFX/Materials/M_SLF_StaticRay_Master_V3.M_SLF_StaticRay_Master_V3"));
	if (RayMaterial == nullptr)
	{
		RayMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UOU/Effects/StylizedLightFX/Materials/M_SLF_Beam_Master.M_SLF_Beam_Master"));
	}
	ConfigureComponents();
}

void AUOULumenStaticRayVisualActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureComponents();
	if (bPreviewInEditor)
	{
		FUOULightBeamVisualSegmentData PreviewData;
		PreviewData.Start = GetActorLocation();
		PreviewData.Direction = GetActorUpVector();
		PreviewData.End = PreviewData.Start + PreviewData.Direction * PreviewLength;
		PreviewData.Length = PreviewLength;
		PreviewData.StartRadius = PreviewRadius;
		PreviewData.EndRadius = PreviewRadius;
		PreviewData.Intensity = 1.0f;
		PreviewData.Color = FLinearColor::White;
		ApplyPreset(PreviewData);
	}
}

void AUOULumenStaticRayVisualActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCameraFacing();
	UpdateMaterialParameters(GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f);
}

void AUOULumenStaticRayVisualActor::ConfigureComponents()
{
	DynamicMaterials.SetNum(MaxLayerCount);
	for (int32 Index = 0; Index < LayerComponents.Num(); ++Index)
	{
		UStaticMeshComponent* Layer = LayerComponents[Index];
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetGenerateOverlapEvents(false);
		Layer->SetCastShadow(false);
		Layer->bReceivesDecals = false;
		Layer->SetTranslucentSortPriority(30 + Index);
		Layer->SetMaterial(0, RayMaterial);
		DynamicMaterials[Index] = nullptr;
	}
}

void AUOULumenStaticRayVisualActor::EnsureDynamicMaterials()
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

void AUOULumenStaticRayVisualActor::ApplyLightBeamSegment_Implementation(const FUOULightBeamVisualSegmentData& SegmentData)
{
	if (SegmentData.Length <= KINDA_SMALL_NUMBER || SegmentData.Direction.IsNearlyZero())
	{
		SetLightBeamVisualActive_Implementation(false);
		return;
	}
	SetActorLocationAndRotation(SegmentData.Start, FRotationMatrix::MakeFromZ(SegmentData.Direction.GetSafeNormal()).Rotator());
	ApplyPreset(SegmentData);
	SetLightBeamVisualActive_Implementation(true);
}

void AUOULumenStaticRayVisualActor::ApplyPreset(const FUOULightBeamVisualSegmentData& SegmentData)
{
	EnsureDynamicMaterials();
	const int32 EffectivePreset = SegmentData.LumenStaticRayPresetOverride > 0
		? SegmentData.LumenStaticRayPresetOverride
		: Preset;
	const FLumenStaticRayPreset& Selected = GetStaticRayPresets()[FMath::Clamp(EffectivePreset, 1, 19) - 1];
	const float Radius = FMath::Max(
		KINDA_SMALL_NUMBER,
		FMath::Max(SegmentData.StartRadius, SegmentData.EndRadius) * BeamWidthScale);
	CurrentColor = SegmentData.Color;
	CurrentIntensity = SegmentData.Intensity * SegmentData.VisualBrightnessMultiplier * EmissiveIntensityScale;
	CurrentOpacity = FMath::Clamp(OpacityScale * SegmentData.VisualOpacityMultiplier, 0.0f, 1.0f);
	LayerBaseOpacities.SetNumZeroed(MaxLayerCount);
	float MaxLayerRadiusScale = KINDA_SMALL_NUMBER;
	float MaxLayerLengthScale = KINDA_SMALL_NUMBER;
	for (const FLumenStaticRayLayer& Layer : Selected.Layers)
	{
		MaxLayerRadiusScale = FMath::Max(
			MaxLayerRadiusScale,
			FMath::Max(FMath::Abs(Layer.Scale.X), FMath::Abs(Layer.Scale.Y)));
		MaxLayerLengthScale = FMath::Max(MaxLayerLengthScale, FMath::Abs(Layer.Scale.Z));
	}

	for (int32 Index = 0; Index < LayerComponents.Num(); ++Index)
	{
		UStaticMeshComponent* Component = LayerComponents[Index];
		const bool bActive = Selected.Layers.IsValidIndex(Index);
		Component->SetVisibility(bActive, true);
		if (!bActive)
		{
			continue;
		}
		const FLumenStaticRayLayer& Layer = Selected.Layers[Index];
		UStaticMesh* Mesh = ShapeMeshes.IsValidIndex(Layer.Shape) ? ShapeMeshes[Layer.Shape] : nullptr;
		Component->SetStaticMesh(Mesh);

		// Unity Y-up 좌표를 Unreal Z-up 좌표로 변환합니다.
		const FVector ConvertedPosition(Layer.Position.Z, Layer.Position.X, Layer.Position.Y);
		Component->SetRelativeLocation(ConvertedPosition * Radius * Selected.GlobalScale);
		// FBX 임포트 과정에서 Unity Y-up 메시 축이 Unreal Z-up으로 이미 변환됩니다.
		// 프리셋의 Unity 회전을 다시 적용하면 메시가 광선 축에서 90도 벗어날 수 있으므로,
		// 런타임 구간의 로컬 Z축을 길이 축으로 사용합니다.
		Component->SetRelativeRotation(FRotator::ZeroRotator);

		const FBoxSphereBounds MeshBounds = Mesh != nullptr
			? Mesh->GetBounds()
			: FBoxSphereBounds(FVector::ZeroVector, FVector(50.0f), 50.0f);
		const FVector NativeExtent = MeshBounds.BoxExtent.ComponentMax(FVector(KINDA_SMALL_NUMBER));
		const float NativeRadius = FMath::Max(NativeExtent.X, NativeExtent.Y);
		const float NativeLength = NativeExtent.Z * 2.0f;
		const FVector NormalizedLayerScale(
			FMath::Abs(Layer.Scale.X) / MaxLayerRadiusScale,
			FMath::Abs(Layer.Scale.Y) / MaxLayerRadiusScale,
			FMath::Abs(Layer.Scale.Z) / MaxLayerLengthScale);
		const FVector FitScale(
			Radius / NativeRadius,
			Radius / NativeRadius,
			SegmentData.Length / NativeLength);
		Component->SetRelativeScale3D(FitScale * NormalizedLayerScale);
		// 서브메시가 원본 FBX의 공통 피벗을 유지한 채 분리되어 있으므로,
		// 바운드 중심의 횡방향 오프셋을 제거하고 시작점에서 광선 구간이 시작되게 맞춥니다.
		const FVector ScaledBoundsOrigin = MeshBounds.Origin * Component->GetRelativeScale3D();
		Component->AddRelativeLocation(FVector(
			-ScaledBoundsOrigin.X,
			-ScaledBoundsOrigin.Y,
			-ScaledBoundsOrigin.Z + SegmentData.Length * 0.5f));
		if (DynamicMaterials.IsValidIndex(Index) && DynamicMaterials[Index] != nullptr)
		{
			const FLinearColor LayerColor = CurrentColor * Layer.Color;
			DynamicMaterials[Index]->SetVectorParameterValue(StaticRayBeamColorParameter, LayerColor);
			DynamicMaterials[Index]->SetVectorParameterValue(StaticRayVariationColorParameter, LayerColor);
			DynamicMaterials[Index]->SetScalarParameterValue(StaticRayEmissiveIntensityParameter, CurrentIntensity * Selected.GlobalBrightness * Layer.Brightness);
			// Unity 원본은 전역 밝기와 레이어 밝기를 Emissive가 아닌 최종 Alpha에 곱한다.
			// 같은 계산을 사용해 여러 겹의 메시가 불투명한 흰 덩어리처럼 보이지 않게 한다.
			LayerBaseOpacities[Index] = FMath::Clamp(
				CurrentOpacity * Selected.GlobalBrightness * Layer.Brightness * Layer.Color.A,
				0.0f,
				1.0f);
			DynamicMaterials[Index]->SetScalarParameterValue(StaticRayOpacityParameter, LayerBaseOpacities[Index]);
			DynamicMaterials[Index]->SetScalarParameterValue(StaticRayVariationAmountParameter, bUseVariation && Selected.bUseVariation ? 1.0f : 0.0f);
			DynamicMaterials[Index]->SetScalarParameterValue(StaticRayVariationSpeedParameter, Selected.VariationSpeed);
			DynamicMaterials[Index]->SetScalarParameterValue(StaticRayVariationScaleParameter, Selected.VariationScale);
			ApplyJunctionClipParameters(DynamicMaterials[Index], SegmentData);
		}
	}
	UpdateCameraFacing();
	UpdateMaterialParameters(GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f);
}

void AUOULumenStaticRayVisualActor::UpdateCameraFacing()
{
	if (CameraFacingRoot == nullptr)
	{
		return;
	}

	if (!bFaceCameraAroundBeamAxis)
	{
		CameraFacingRoot->SetRelativeRotation(FRotator::ZeroRotator);
		return;
	}

	const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (CameraManager == nullptr)
	{
		return;
	}

	// 로컬 Z 광선축은 유지하고, 레이어의 넓은 면만 카메라를 향하게 합니다.
	// 오소그래픽 카메라는 모든 투영 광선이 평행하므로 액터-카메라 위치 벡터가 아니라
	// 카메라 시선의 반대 방향을 사용해야 화면 회전 중 메시가 흔들리지 않습니다.
	const FMinimalViewInfo& CameraView = CameraManager->GetCameraCacheView();
	const FVector WorldToCamera = CameraView.ProjectionMode == ECameraProjectionMode::Orthographic
		? -CameraView.Rotation.Vector()
		: CameraView.Location - GetActorLocation();
	const FVector LocalToCamera = GetActorTransform().InverseTransformVectorNoScale(WorldToCamera);
	const FVector ProjectedToCamera(LocalToCamera.X, LocalToCamera.Y, 0.0f);
	if (ProjectedToCamera.IsNearlyZero())
	{
		return;
	}

	const float CameraYaw = FMath::RadiansToDegrees(
		FMath::Atan2(ProjectedToCamera.Y, ProjectedToCamera.X));
	CameraFacingRoot->SetRelativeRotation(FRotator(0.0f, CameraYaw - 90.0f, 0.0f));
}

void AUOULumenStaticRayVisualActor::UpdateMaterialParameters(const float TimeSeconds)
{
	float Fade = 1.0f;
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector CameraLocation = CameraManager->GetCameraLocation();
		if (bUseCameraDistanceFade)
		{
			const float Distance = FVector::Distance(GetActorLocation(), CameraLocation);
			Fade *= 1.0f - FMath::GetMappedRangeValueClamped(FVector2D(CameraDistanceFadeStart, CameraDistanceFadeEnd), FVector2D(0.0f, 1.0f), Distance);
		}
		if (bUseAngleFade)
		{
			const FVector ToCamera = (CameraLocation - GetActorLocation()).GetSafeNormal();
			const float Facing = FMath::Abs(FVector::DotProduct(GetActorForwardVector(), ToCamera));
			Fade *= FMath::GetMappedRangeValueClamped(FVector2D(AngleFadeStart, 1.0f), FVector2D(0.0f, 1.0f), Facing);
		}
	}

	for (int32 Index = 0; Index < DynamicMaterials.Num(); ++Index)
	{
		UMaterialInstanceDynamic* Material = DynamicMaterials[Index];
		if (Material != nullptr)
		{
			const float BaseOpacity = LayerBaseOpacities.IsValidIndex(Index) ? LayerBaseOpacities[Index] : CurrentOpacity;
			Material->SetScalarParameterValue(StaticRayOpacityParameter, BaseOpacity * Fade);
		}
	}
}

void AUOULumenStaticRayVisualActor::SetLightBeamVisualActive_Implementation(const bool bActive)
{
	SetActorHiddenInGame(!bActive);
	if (!bActive)
	{
		for (UStaticMeshComponent* Component : LayerComponents)
		{
			Component->SetVisibility(false, true);
		}
	}
}
