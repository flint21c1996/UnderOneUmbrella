// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOURotatableMirrorActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Light/UOULightInteractionSurfaceComponent.h"
#include "World/Light/UOURotatableMirrorComponent.h"

AUOURotatableMirrorActor::AUOURotatableMirrorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MirrorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("MirrorPivot"));
	SetRootComponent(MirrorPivot);
	MirrorPivot->SetMobility(EComponentMobility::Movable);

	MirrorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MirrorMesh"));
	MirrorMesh->SetupAttachment(MirrorPivot);
	MirrorMesh->SetMobility(EComponentMobility::Movable);
	MirrorMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	MirrorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MirrorMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	MirrorMesh->SetGenerateOverlapEvents(false);
	MirrorMesh->SetRelativeScale3D(FVector(0.1f, 2.0f, 2.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		MirrorMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

	LightInteractionSurface =
		CreateDefaultSubobject<UUOULightInteractionSurfaceComponent>(TEXT("LightInteractionSurface"));
	LightInteractionSurface->SetupAttachment(MirrorPivot);
	LightInteractionSurface->SetMobility(EComponentMobility::Movable);
	LightInteractionSurface->SetBoxExtent(FVector(6.0f, 100.0f, 100.0f));
	LightInteractionSurface->LightInteractionMode = EUOULightInteractionMode::Reflecting;
	// 회전된 얇은 Box의 모서리나 옆면이 트레이스에 맞아도 반사 법선이 튀지 않도록
	// 실제 충돌 면 법선 대신 거울 피벗과 함께 회전하는 고정된 앞 방향을 사용합니다.
	LightInteractionSurface->ReflectionNormalMode = EUOULightReflectionNormalMode::ComponentForward;
	LightInteractionSurface->ReflectionFrontNormalMode =
		EUOULightReflectionFrontNormalMode::ComponentForward;
	// 거울보다 넓은 빛은 일부만 잘라 반사하지 않으며, 일반 거울처럼 표면에서 차단합니다.
	LightInteractionSurface->bRequireFullBeamFootprint = true;
	LightInteractionSurface->MaximumReflectionIncidenceAngle = 89.0f;
	LightInteractionSurface->RetainedMaximumReflectionIncidenceAngle = 95.0f;
	LightInteractionSurface->BeamFootprintOverflowAllowancePercent = 20.0f;
	// 평행광 중심축이 거울을 조금 빗나가도 실제 빛 단면이 충분히 걸치면 반사합니다.
	// 단면 수용 비율은 bRequireFullBeamFootprint와 돌출 허용값으로 별도 제한합니다.
	LightInteractionSurface->bAllowEdgeOnlyCylinderReflection = true;

	PushVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("PushVolume"));
	PushVolume->SetupAttachment(MirrorPivot);
	PushVolume->SetMobility(EComponentMobility::Movable);
	PushVolume->SetBoxExtent(FVector(100.0f, 140.0f, 120.0f));
	PushVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PushVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	PushVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PushVolume->SetGenerateOverlapEvents(true);

	PushHandleLeft = CreateDefaultSubobject<USceneComponent>(TEXT("PushHandleLeft"));
	PushHandleLeft->SetupAttachment(MirrorPivot);
	PushHandleLeft->SetRelativeLocation(FVector(12.0f, -80.0f, 0.0f));
	PushHandleLeft->ComponentTags.Add(TEXT("MirrorPushHandle"));

	PushHandleRight = CreateDefaultSubobject<USceneComponent>(TEXT("PushHandleRight"));
	PushHandleRight->SetupAttachment(MirrorPivot);
	PushHandleRight->SetRelativeLocation(FVector(12.0f, 80.0f, 0.0f));
	PushHandleRight->ComponentTags.Add(TEXT("MirrorPushHandle"));

	RotatableMirror =
		CreateDefaultSubobject<UUOURotatableMirrorComponent>(TEXT("RotatableMirror"));
	RotatableMirror->bAutoFindPushVolume = true;
	RotatableMirror->PreferredPushVolumeName = PushVolume->GetFName();
	RotatableMirror->bConfigurePushVolumeCollision = true;
	RotatableMirror->LocalRotationAxis = FVector::UpVector;
}

void AUOURotatableMirrorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (LightInteractionSurface != nullptr)
	{
		// 기존 Blueprint 컴포넌트 템플릿에 false가 저장되어 있어도 회전 거울 정책을 일관되게 적용합니다.
		LightInteractionSurface->bAllowEdgeOnlyCylinderReflection = true;
	}
	SyncLightInteractionSurfaceToMirrorMesh();
}

void AUOURotatableMirrorActor::SetReflectionIncidenceAngles(
	float StartMaximumAngleDegrees,
	float RetainedMaximumAngleDegrees)
{
	if (LightInteractionSurface != nullptr)
	{
		LightInteractionSurface->SetReflectionIncidenceAngles(
			StartMaximumAngleDegrees,
			RetainedMaximumAngleDegrees);
	}
}

void AUOURotatableMirrorActor::SetBeamFootprintOverflowAllowance(
	float OverflowAllowancePercent)
{
	if (LightInteractionSurface != nullptr)
	{
		LightInteractionSurface->SetBeamFootprintOverflowAllowance(
			OverflowAllowancePercent);
	}
}

void AUOURotatableMirrorActor::SyncLightInteractionSurfaceToMirrorMesh() const
{
	if (!bSyncLightSurfaceToMirrorMesh || MirrorMesh == nullptr ||
		LightInteractionSurface == nullptr || MirrorMesh->GetStaticMesh() == nullptr)
	{
		return;
	}

	const FBox MeshLocalBounds = MirrorMesh->GetStaticMesh()->GetBoundingBox();
	if (!MeshLocalBounds.IsValid)
	{
		return;
	}

	const FTransform MeshRelativeTransform = MirrorMesh->GetRelativeTransform();
	const FVector MeshScale = MeshRelativeTransform.GetScale3D().GetAbs();
	const FVector SurfaceCenter =
		MeshRelativeTransform.TransformPosition(MeshLocalBounds.GetCenter());
	const FVector MeshExtent = MeshLocalBounds.GetExtent() * MeshScale;

	EUOUMirrorSurfaceNormalAxis ResolvedNormalAxis = LightSurfaceNormalAxis;
	if (ResolvedNormalAxis == EUOUMirrorSurfaceNormalAxis::Auto)
	{
		ResolvedNormalAxis = EUOUMirrorSurfaceNormalAxis::X;
		if (MeshExtent.Y < MeshExtent.X && MeshExtent.Y <= MeshExtent.Z)
		{
			ResolvedNormalAxis = EUOUMirrorSurfaceNormalAxis::Y;
		}
		else if (MeshExtent.Z < MeshExtent.X && MeshExtent.Z < MeshExtent.Y)
		{
			ResolvedNormalAxis = EUOUMirrorSurfaceNormalAxis::Z;
		}
	}

	FQuat SurfaceRotation = MeshRelativeTransform.GetRotation();
	FVector SurfaceExtent = MeshExtent;
	switch (ResolvedNormalAxis)
	{
	case EUOUMirrorSurfaceNormalAxis::Y:
		// 반사 컴포넌트의 로컬 +X를 메시의 로컬 +Y에 맞춥니다.
		SurfaceRotation *= FQuat(FVector::UpVector, HALF_PI);
		SurfaceExtent = FVector(MeshExtent.Y, MeshExtent.X, MeshExtent.Z);
		break;
	case EUOUMirrorSurfaceNormalAxis::Z:
		// 반사 컴포넌트의 로컬 +X를 메시의 로컬 +Z에 맞춥니다.
		SurfaceRotation *= FQuat::FindBetweenNormals(FVector::ForwardVector, FVector::UpVector);
		SurfaceExtent = FVector(MeshExtent.Z, MeshExtent.Y, MeshExtent.X);
		break;
	case EUOUMirrorSurfaceNormalAxis::X:
	case EUOUMirrorSurfaceNormalAxis::Auto:
	default:
		break;
	}
	SurfaceExtent.X += FMath::Max(0.0f, LightSurfaceThicknessPadding);

	LightInteractionSurface->SetRelativeLocationAndRotation(
		SurfaceCenter,
		SurfaceRotation);
	LightInteractionSurface->SetRelativeScale3D(FVector::OneVector);
	LightInteractionSurface->SetBoxExtent(SurfaceExtent);
}
