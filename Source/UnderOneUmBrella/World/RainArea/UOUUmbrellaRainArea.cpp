// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/RainArea/UOUUmbrellaRainArea.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Environment/UOUEnvironmentVisualComponent.h"

AUOUUmbrellaRainArea::AUOUUmbrellaRainArea()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	RainVisual = CreateDefaultSubobject<UUOUEnvironmentVisualComponent>(TEXT("RainVisual"));
	RainVisual->SetupAttachment(RootScene);

	PrimaryRainEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PrimaryRainEffect"));
	PrimaryRainEffect->SetupAttachment(RainVisual);
	PrimaryRainEffect->SetAutoActivate(false);

	SecondaryRainEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SecondaryRainEffect"));
	SecondaryRainEffect->SetupAttachment(RainVisual);
	SecondaryRainEffect->SetAutoActivate(false);

	RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);

	RainVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("RainVolume"));
	RainVolume->SetupAttachment(RootScene);
	RainVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RainVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	RainVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RainVolume->SetGenerateOverlapEvents(true);
	RainVolume->SetBoxExtent(FVector(250.0f, 250.0f, 200.0f));

	PreviewVolumeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewVolumeMesh"));
	PreviewVolumeMesh->SetupAttachment(RootScene);
	PreviewVolumeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewVolumeMesh->SetGenerateOverlapEvents(false);
	PreviewVolumeMesh->SetCastShadow(false);
	PreviewVolumeMesh->SetHiddenInGame(false);
	PreviewVolumeMesh->SetVisibility(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		PreviewVolumeMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

}

void AUOUUmbrellaRainArea::BeginPlay()
{
	Super::BeginPlay();
	RainFillRate = FMath::Max(0.0f, RainFillRate);
	RainVisualIntensity = FMath::Clamp(RainVisualIntensity, 0.0f, 1.0f);
	RainSpawnRate = FMath::Max(0.0f, RainSpawnRate);
	GroundSplashIntensityMultiplier = FMath::Max(0.0f, GroundSplashIntensityMultiplier);
	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);
	}
	ApplyPreviewSettings();
	ApplyEnvironmentVisualSettings();
}

void AUOUUmbrellaRainArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RainFillRate = FMath::Max(0.0f, RainFillRate);
	RainVisualIntensity = FMath::Clamp(RainVisualIntensity, 0.0f, 1.0f);
	RainSpawnRate = FMath::Max(0.0f, RainSpawnRate);
	GroundSplashIntensityMultiplier = FMath::Max(0.0f, GroundSplashIntensityMultiplier);
	RainFallSpeed = FMath::Clamp(RainFallSpeed, -3000.0f, 0.0f);
	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);
	}
	ApplyPreviewSettings();
	ApplyEnvironmentVisualSettings();
}

#if WITH_EDITOR
void AUOUUmbrellaRainArea::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RainFillRate = FMath::Max(0.0f, RainFillRate);
	RainVisualIntensity = FMath::Clamp(RainVisualIntensity, 0.0f, 1.0f);
	RainSpawnRate = FMath::Max(0.0f, RainSpawnRate);
	GroundSplashIntensityMultiplier = FMath::Max(0.0f, GroundSplashIntensityMultiplier);
	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);
	}
	ApplyPreviewSettings();
	ApplyEnvironmentVisualSettings();
}

void AUOUUmbrellaRainArea::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);
	}
	ApplyPreviewSettings();
	ApplyEnvironmentVisualSettings();
}
#endif

void AUOUUmbrellaRainArea::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RainVolume == nullptr)
	{
		return;
	}

	ApplyEnvironmentVisualState();
	DrawRainVisualDebug();

	TArray<AActor*> OverlappingActors;
	RainVolume->GetOverlappingActors(OverlappingActors);

	bool bHasRainBlocker = false;
	FVector RainBlockerWorldCenter = FVector::ZeroVector;
	FVector RainBlockerHalfExtent = FVector::ZeroVector;

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (OverlappingActor == nullptr)
		{
			continue;
		}

		if (UUOUUmbrellaComponent* UmbrellaComponent = OverlappingActor->FindComponentByClass<UUOUUmbrellaComponent>())
		{
			if (RainFillRate > 0.0f)
			{
				UmbrellaComponent->ApplyRainExposure(RainFillRate * DeltaSeconds);
			}

			FVector CandidateBlockerWorldCenter = FVector::ZeroVector;
			FRotator CandidateBlockerWorldRotation = FRotator::ZeroRotator;
			FVector CandidateBlockerHalfExtent = FVector::ZeroVector;
			if (UmbrellaComponent->IsBlockingRain()
				&& UmbrellaComponent->TryGetRainBlockerVolumeData(CandidateBlockerWorldCenter, CandidateBlockerWorldRotation, CandidateBlockerHalfExtent)
				&& CandidateBlockerHalfExtent.SizeSquared() > RainBlockerHalfExtent.SizeSquared())
			{
				bHasRainBlocker = true;
				RainBlockerWorldCenter = CandidateBlockerWorldCenter;
				RainBlockerHalfExtent = CandidateBlockerHalfExtent;
			}
		}
	}

	ApplyEnvironmentVisualRainBlocker(
		bHasRainBlocker,
		RainBlockerWorldCenter,
		RainBlockerHalfExtent,
		bHasRainBlocker ? RainVisualIntensity : 0.0f);
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualSettings()
{
	ApplyEnvironmentVisualGeometry();
	ApplyEnvironmentVisualState();
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualGeometry()
{
	if (RainVisual == nullptr || RainVolume == nullptr)
	{
		return;
	}

	const FVector BoxExtent = RainVolume->GetScaledBoxExtent();
	const FVector VolumeCenter = RainVolume->GetComponentLocation();
	const FVector VolumeUp = RainVolume->GetUpVector();
	const FVector RainSpawnPlaneWorldPosition = VolumeCenter + VolumeUp * BoxExtent.Z;
	const FVector GroundSplashWorldPosition = VolumeCenter - VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset);

	RainVisual->SetWorldLocationAndRotation(VolumeCenter, RainVolume->GetComponentRotation());

	const FTransform VisualTransform = RainVisual->GetComponentTransform();
	const FVector RainLocalPosition = VisualTransform.InverseTransformPosition(RainSpawnPlaneWorldPosition);
	const FVector GroundSplashLocalPosition = VisualTransform.InverseTransformPosition(GroundSplashWorldPosition);
	const FVector RainKillVolumeLocalCenter = FVector::ZeroVector;
	const FRotator EffectLocalRotation = FRotator::ZeroRotator;
	const FVector2D AreaSize(BoxExtent.X * 2.0f, BoxExtent.Y * 2.0f);
	const FVector KillVolumeSize(BoxExtent.X * 2.0f, BoxExtent.Y * 2.0f, BoxExtent.Z * 2.0f);

	RainVisual->ConfigureRainVisual(
		RainLocalPosition,
		GroundSplashLocalPosition,
		EffectLocalRotation,
		AreaSize,
		RainKillVolumeLocalCenter,
		KillVolumeSize);
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualState()
{
	if (RainVisual == nullptr)
	{
		return;
	}

	const float PrimaryIntensity = FMath::Clamp(RainVisualIntensity, 0.0f, 1.0f);
	const float SecondaryIntensity = FMath::Clamp(RainVisualIntensity * GroundSplashIntensityMultiplier, 0.0f, 1.0f);

	RainVisual->SetRainSpawnRate(RainSpawnRate);
	RainVisual->SetRainFallSpeed(RainFallSpeed);
	RainVisual->SetVisualIntensities(PrimaryIntensity, SecondaryIntensity);
	RainVisual->SetVisualsEnabled(bEnableRainVisuals);
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualRainBlocker(bool bIsBlocking, const FVector& BlockerWorldCenter, const FVector& BlockerHalfExtent, float BlockerIntensity)
{
	if (RainVisual == nullptr)
	{
		return;
	}

	const FTransform VisualTransform = RainVisual->GetComponentTransform();
	const FVector BlockerLocalCenter = bIsBlocking
		? VisualTransform.InverseTransformPosition(BlockerWorldCenter)
		: FVector::ZeroVector;

	RainVisual->SetRainBlockerData(
		bIsBlocking,
		BlockerLocalCenter,
		BlockerHalfExtent,
		BlockerIntensity);
}

void AUOUUmbrellaRainArea::DrawRainVisualDebug() const
{
	if (!bDrawRainVisualDebug
		|| !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::VFX)
		|| RainVolume == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const FVector BoxExtent = RainVolume->GetScaledBoxExtent();
	const FVector VolumeCenter = RainVolume->GetComponentLocation();
	const FQuat VolumeRotation = RainVolume->GetComponentQuat();
	const FVector VolumeUp = RainVolume->GetUpVector();
	const FVector RainSpawnPlaneWorldPosition = VolumeCenter + VolumeUp * BoxExtent.Z;
	const FVector GroundSplashWorldPosition = VolumeCenter - VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset);
	const FVector VisualAreaHalfExtent(BoxExtent.X, BoxExtent.Y, 2.0f);
	const float Thickness = FMath::Max(0.0f, RainVisualDebugThickness);
	const float LifeTime = 0.0f;
	const FColor VFXDebugColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::VFX, FColor::Cyan);

	DrawDebugBox(
		World,
		VolumeCenter,
		BoxExtent,
		VolumeRotation,
		VFXDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugBox(
		World,
		RainSpawnPlaneWorldPosition,
		VisualAreaHalfExtent,
		VolumeRotation,
		VFXDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugBox(
		World,
		GroundSplashWorldPosition,
		VisualAreaHalfExtent,
		VolumeRotation,
		VFXDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugLine(
		World,
		GroundSplashWorldPosition,
		RainSpawnPlaneWorldPosition,
		VFXDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugString(
		World,
		RainSpawnPlaneWorldPosition + VolumeUp * 20.0f,
		FString::Printf(
			TEXT("RainAreaSize %.1f x %.1f"),
			BoxExtent.X * 2.0f,
			BoxExtent.Y * 2.0f),
		nullptr,
		VFXDebugColor,
		LifeTime,
		false,
		1.0f);
}

void AUOUUmbrellaRainArea::ApplyPreviewSettings()
{
	if (PreviewVolumeMesh == nullptr || RainVolume == nullptr)
	{
		return;
	}

	PreviewVolumeMesh->SetVisibility(bShowEditorPreview);
	PreviewVolumeMesh->SetHiddenInGame(!bShowPreviewInGame);

	if (PreviewMaterial != nullptr)
	{
		PreviewVolumeMesh->SetMaterial(0, PreviewMaterial);
	}

	const FVector BoxExtent = RainVolume->GetUnscaledBoxExtent();
	const FVector BaseScale(
		BoxExtent.X / 50.0f,
		BoxExtent.Y / 50.0f,
		BoxExtent.Z / 50.0f);

	PreviewVolumeMesh->SetRelativeLocation(RainVolume->GetRelativeLocation());
	PreviewVolumeMesh->SetRelativeRotation(RainVolume->GetRelativeRotation());

	if (bAutoFitPreviewScaleToRainVolume)
	{
		PreviewVolumeMesh->SetRelativeScale3D(FVector(
			BaseScale.X * PreviewScaleMultiplier.X,
			BaseScale.Y * PreviewScaleMultiplier.Y,
			BaseScale.Z * PreviewScaleMultiplier.Z));
	}
	else
	{
		PreviewVolumeMesh->SetRelativeScale3D(ManualPreviewRelativeScale);
	}
}
