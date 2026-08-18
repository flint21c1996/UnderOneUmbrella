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
	// 거울보다 넓은 빛은 일부만 잘라 반사하지 않고 원래 경로로 통과시킵니다.
	LightInteractionSurface->bRequireFullBeamFootprint = true;

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
