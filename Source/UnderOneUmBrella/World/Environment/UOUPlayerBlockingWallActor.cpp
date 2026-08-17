// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUPlayerBlockingWallActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/UOUDevelopmentDebugDrawContext.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AUOUPlayerBlockingWallActor::AUOUPlayerBlockingWallActor()
{
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

void AUOUPlayerBlockingWallActor::EnableWall()
{
	SetWallEnabled(true);
}

void AUOUPlayerBlockingWallActor::DisableWall()
{
	SetWallEnabled(false);
}

void AUOUPlayerBlockingWallActor::ToggleWall()
{
	SetWallEnabled(!bWallEnabled);
}

void AUOUPlayerBlockingWallActor::SetWallEnabled(bool bNewEnabled)
{
	bWallEnabled = bNewEnabled;
	bHasAppliedPreviewMaterialState = false;

	ApplyCollisionSettings();
	ApplyPreviewSettings();
}

bool AUOUPlayerBlockingWallActor::IsWallEnabled() const
{
	return bWallEnabled;
}

EUOUDebugCategory AUOUPlayerBlockingWallActor::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

#if UOU_WITH_DEVELOPMENT_TOOLS
void AUOUPlayerBlockingWallActor::GatherDevelopmentDebugDraw(
	IUOUDevelopmentDebugDrawContext& Context) const
{
	if (BlockingVolume == nullptr)
	{
		return;
	}

	const FColor StateColor = (
		IsWallEnabled()
			? EnabledPreviewColor
			: DisabledPreviewColor).ToFColor(true);
	Context.DrawBox(
		BlockingVolume->GetComponentLocation(),
		BlockingVolume->GetScaledBoxExtent(),
		BlockingVolume->GetComponentQuat(),
		StateColor,
		4.0f);
}
#endif

void AUOUPlayerBlockingWallActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		DisableWall();
		break;
	case EOUUPuzzleResultAction::Deactivate:
		EnableWall();
		break;
	case EOUUPuzzleResultAction::Toggle:
		ToggleWall();
		break;
	case EOUUPuzzleResultAction::None:
	case EOUUPuzzleResultAction::Pause:
	case EOUUPuzzleResultAction::Resume:
	default:
		break;
	}
}

void AUOUPlayerBlockingWallActor::ApplyCollisionSettings()
{
	if (BlockingVolume == nullptr)
	{
		return;
	}

	BlockingVolume->SetBoxExtent(WallExtent);
	BlockingVolume->SetCollisionObjectType(ECC_WorldStatic);
	BlockingVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingVolume->SetCollisionResponseToChannel(BlockedChannel, ECR_Block);
	BlockingVolume->SetGenerateOverlapEvents(false);

	if (!bWallEnabled)
	{
		BlockingVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	BlockingVolume->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AUOUPlayerBlockingWallActor::ApplyPreviewSettings()
{
	if (PreviewMesh == nullptr || BlockingVolume == nullptr)
	{
		return;
	}

	ApplyPreviewMaterialSettings();

	// 엔진 기본 Cube는 전체 크기가 100cm라서 Box Extent를 50으로 나누면 충돌 박스와 같은 크기가 됩니다.
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
	const bool bShouldShowPreview = bIsGameWorld
		? bShowPreviewInGame
		: bShowPreviewInEditor;

	PreviewMesh->SetVisibility(bShouldShowPreview, true);
	PreviewMesh->SetHiddenInGame(!bShouldShowPreview);

}

void AUOUPlayerBlockingWallActor::ApplyPreviewMaterialSettings()
{
	if (PreviewMesh == nullptr || !bOverridePreviewMeshMaterial || PreviewMaterial == nullptr)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const bool bCanCreateRuntimeMaterial = World != nullptr && World->IsGameWorld() && !HasAnyFlags(RF_ClassDefaultObject);
	if (!bCanCreateRuntimeMaterial)
	{
		PreviewMaterialInstance = nullptr;
		PreviewMaterialInstanceSource = nullptr;
		bHasAppliedPreviewMaterialState = false;
		PreviewMesh->SetMaterial(0, PreviewMaterial);
		return;
	}

	if (PreviewMaterialInstance == nullptr || PreviewMaterialInstanceSource != PreviewMaterial)
	{
		PreviewMaterialInstance = PreviewMesh->CreateDynamicMaterialInstance(0, PreviewMaterial);
		PreviewMaterialInstanceSource = PreviewMaterial;
		bHasAppliedPreviewMaterialState = false;
	}

	if (PreviewMaterialInstance == nullptr)
	{
		PreviewMesh->SetMaterial(0, PreviewMaterial);
		return;
	}

	const FLinearColor PreviewColor = bWallEnabled ? EnabledPreviewColor : DisabledPreviewColor;
	if (bHasAppliedPreviewMaterialState && AppliedPreviewColor.Equals(PreviewColor))
	{
		return;
	}

	AppliedPreviewColor = PreviewColor;
	bHasAppliedPreviewMaterialState = true;

	PreviewMaterialInstance->SetVectorParameterValue(TEXT("Color"), PreviewColor);
	PreviewMaterialInstance->SetVectorParameterValue(TEXT("BaseColor"), PreviewColor);
	PreviewMaterialInstance->SetVectorParameterValue(TEXT("Tint"), PreviewColor);
	PreviewMaterialInstance->SetScalarParameterValue(TEXT("Opacity"), PreviewColor.A);
	PreviewMaterialInstance->SetScalarParameterValue(TEXT("Alpha"), PreviewColor.A);
}
