// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Cinematics/UOUCinematicCameraTargetActor.h"

#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

AUOUCinematicCameraTargetActor::AUOUCinematicCameraTargetActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	RootScene->SetMobility(EComponentMobility::Movable);

	TargetDirectionMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("TargetDirectionMarker"));
	TargetDirectionMarker->bEditableWhenInherited = false;
	TargetDirectionMarker->SetupAttachment(RootScene);
	TargetDirectionMarker->SetMobility(EComponentMobility::Movable);
	TargetDirectionMarker->SetHiddenInGame(true);
	TargetDirectionMarker->ArrowColor = FColor(0, 200, 120);
	TargetDirectionMarker->ArrowSize = 1.5f;

	TargetCameraPreview = CreateDefaultSubobject<UCameraComponent>(TEXT("TargetCameraPreview"));
	TargetCameraPreview->bEditableWhenInherited = false;
	TargetCameraPreview->SetupAttachment(RootScene);
	TargetCameraPreview->SetMobility(EComponentMobility::Movable);
	TargetCameraPreview->SetActive(false);
	TargetCameraPreview->SetHiddenInGame(true);
	TargetCameraPreview->SetProjectionMode(ProjectionMode.GetValue());
	TargetCameraPreview->SetOrthoWidth(OrthographicWidth);
	TargetCameraPreview->SetFieldOfView(FieldOfView);
	TargetCameraPreview->AspectRatio = AspectRatio;
	TargetCameraPreview->bConstrainAspectRatio = bConstrainAspectRatio;
}

ECameraProjectionMode::Type AUOUCinematicCameraTargetActor::ResolveProjectionMode(ECameraProjectionMode::Type CameraDefault) const
{
	if (!bOverrideProjectionMode)
	{
		return CameraDefault;
	}

	return ProjectionMode.GetValue();
}

float AUOUCinematicCameraTargetActor::ResolveOrthographicWidth(float CameraDefault) const
{
	return bOverrideOrthographicWidth
		? FMath::Max(1.0f, OrthographicWidth)
		: FMath::Max(1.0f, CameraDefault);
}

float AUOUCinematicCameraTargetActor::ResolveFieldOfView(float CameraDefault) const
{
	return bOverrideFieldOfView
		? FMath::Clamp(FieldOfView, 5.0f, 170.0f)
		: FMath::Clamp(CameraDefault, 5.0f, 170.0f);
}

float AUOUCinematicCameraTargetActor::ResolveAspectRatio(float CameraDefault) const
{
	return bOverrideAspectRatio
		? FMath::Max(0.1f, AspectRatio)
		: FMath::Max(0.1f, CameraDefault);
}

bool AUOUCinematicCameraTargetActor::ResolveConstrainAspectRatio(bool bCameraDefault) const
{
	return bOverrideConstrainAspectRatio ? bConstrainAspectRatio : bCameraDefault;
}

float AUOUCinematicCameraTargetActor::ResolveMoveDuration(float CameraDefault) const
{
	return bOverrideMoveDuration ? FMath::Max(0.0f, MoveDuration) : FMath::Max(0.0f, CameraDefault);
}

void AUOUCinematicCameraTargetActor::SyncPreviewCamera(
	ECameraProjectionMode::Type InProjectionMode,
	float InOrthographicWidth,
	float InFieldOfView,
	float InAspectRatio,
	bool bInConstrainAspectRatio)
{
	if (TargetCameraPreview == nullptr)
	{
		return;
	}

	TargetCameraPreview->SetProjectionMode(InProjectionMode);
	TargetCameraPreview->SetOrthoWidth(FMath::Max(1.0f, InOrthographicWidth));
	TargetCameraPreview->SetFieldOfView(FMath::Clamp(InFieldOfView, 5.0f, 170.0f));
	TargetCameraPreview->AspectRatio = FMath::Max(0.1f, InAspectRatio);
	TargetCameraPreview->bConstrainAspectRatio = bInConstrainAspectRatio;
}

void AUOUCinematicCameraTargetActor::SetTargetCameraPreviewVisible(bool bVisible)
{
	if (TargetCameraPreview != nullptr)
	{
		TargetCameraPreview->SetVisibility(bVisible, true);
	}

	if (TargetDirectionMarker != nullptr)
	{
		TargetDirectionMarker->SetVisibility(bVisible, true);
	}
}

void AUOUCinematicCameraTargetActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SyncPreviewCamera(
		ProjectionMode.GetValue(),
		OrthographicWidth,
		FieldOfView,
		AspectRatio,
		bConstrainAspectRatio);
}

#if WITH_EDITOR
void AUOUCinematicCameraTargetActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SyncPreviewCamera(
		ProjectionMode.GetValue(),
		OrthographicWidth,
		FieldOfView,
		AspectRatio,
		bConstrainAspectRatio);
}
#endif
