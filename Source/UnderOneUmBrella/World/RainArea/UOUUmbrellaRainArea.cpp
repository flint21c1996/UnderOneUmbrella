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
#include "UObject/UObjectIterator.h"
#include "World/Environment/UOUEnvironmentVisualComponent.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

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

void AUOUUmbrellaRainArea::SetWaterBasinRainFillEnabled(bool bEnabled)
{
	bEnableWaterBasinRainFill = bEnabled;
}

bool AUOUUmbrellaRainArea::IsWaterBasinRainFillEnabled() const
{
	return bEnableWaterBasinRainFill;
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
	const FName PropertyName = PropertyChangedEvent.Property != nullptr
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	bHasExplicitRainEffectSystemSelection |= PropertyName == GET_MEMBER_NAME_CHECKED(AUOUUmbrellaRainArea, RainEffectSystem);
	bHasExplicitGroundSplashEffectSystemSelection |= PropertyName == GET_MEMBER_NAME_CHECKED(AUOUUmbrellaRainArea, GroundSplashEffectSystem);

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
	FRotator RainBlockerWorldRotation = FRotator::ZeroRotator;
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
				RainBlockerWorldRotation = CandidateBlockerWorldRotation;
				RainBlockerHalfExtent = CandidateBlockerHalfExtent;
			}
		}
	}

	ApplyRainToWaterBasinTargets(
		DeltaSeconds,
		bHasRainBlocker,
		RainBlockerWorldCenter,
		RainBlockerWorldRotation,
		RainBlockerHalfExtent);

	ApplyEnvironmentVisualRainBlocker(
		bHasRainBlocker,
		RainBlockerWorldCenter,
		RainBlockerHalfExtent,
		bHasRainBlocker ? RainVisualIntensity : 0.0f);
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualEffectSystems()
{
	if (RainEffectSystem != nullptr)
	{
		bHasExplicitRainEffectSystemSelection = true;
	}
	else if (!bHasExplicitRainEffectSystemSelection && PrimaryRainEffect != nullptr)
	{
		RainEffectSystem = PrimaryRainEffect->GetAsset();
		bHasExplicitRainEffectSystemSelection = RainEffectSystem != nullptr;
	}

	if (GroundSplashEffectSystem != nullptr)
	{
		bHasExplicitGroundSplashEffectSystemSelection = true;
	}
	else if (!bHasExplicitGroundSplashEffectSystemSelection && SecondaryRainEffect != nullptr)
	{
		GroundSplashEffectSystem = SecondaryRainEffect->GetAsset();
		bHasExplicitGroundSplashEffectSystemSelection = GroundSplashEffectSystem != nullptr;
	}

	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectSystems(RainEffectSystem, GroundSplashEffectSystem);
	}
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualSettings()
{
	ApplyEnvironmentVisualEffectSystems();
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

void AUOUUmbrellaRainArea::ApplyRainToWaterBasinTargets(float DeltaSeconds, bool bHasRainBlocker, const FVector& RainBlockerWorldCenter, const FRotator& RainBlockerWorldRotation, const FVector& RainBlockerHalfExtent) const
{
	if (!bEnableWaterBasinRainFill)
	{
		return;
	}

	const float RainAmount = FMath::Max(0.0f, RainFillRate) * FMath::Max(0.0f, DeltaSeconds);
	if (RainAmount <= 0.0f || RainVolume == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	TSet<UUOUWaterBasinTargetComponent*> ProcessedTargets;
	for (TObjectIterator<UUOUWaterBasinTargetComponent> It; It; ++It)
	{
		UUOUWaterBasinTargetComponent* Target = *It;
		if (!IsValid(Target) || Target->GetWorld() != World || ProcessedTargets.Contains(Target)
			|| !Target->CanReceiveRainFill())
		{
			continue;
		}

		AActor* TargetOwner = Target->GetOwner();
		if (!IsValid(TargetOwner)
			|| !IsActorInsideRainVolume(TargetOwner)
			|| (bHasRainBlocker && IsActorBlockedByRainBlocker(TargetOwner, RainBlockerWorldCenter, RainBlockerWorldRotation, RainBlockerHalfExtent)))
		{
			continue;
		}

		TArray<UUOUWaterBasinTargetComponent*> Group;
		Target->GetConnectedGroup(Group);
		for (UUOUWaterBasinTargetComponent* GroupTarget : Group)
		{
			if (IsValid(GroupTarget))
			{
				ProcessedTargets.Add(GroupTarget);
			}
		}

		FUOUWaterBasinInputContext InputContext;
		InputContext.Volume = RainAmount;
		InputContext.Duration = RainAmount;
		InputContext.Source = EUOUWaterBasinInputSource::Rain;
		InputContext.WorldDirection = -RainVolume->GetUpVector();
		InputContext.WorldLocation = TargetOwner->GetActorLocation();
		InputContext.InstigatorActor = const_cast<AUOUUmbrellaRainArea*>(this);
		InputContext.bApplyToConnectedGroup = true;
		Target->ReceiveWaterInput(InputContext);
	}
}

bool AUOUUmbrellaRainArea::IsActorInsideRainVolume(const AActor* Actor) const
{
	if (RainVolume == nullptr || !IsValid(Actor))
	{
		return false;
	}

	FVector ActorOrigin = FVector::ZeroVector;
	FVector ActorExtent = FVector::ZeroVector;
	Actor->GetActorBounds(false, ActorOrigin, ActorExtent);
	if (ActorExtent.IsNearlyZero())
	{
		ActorOrigin = Actor->GetActorLocation();
	}

	const FVector LocalPoint = RainVolume->GetComponentTransform().InverseTransformPosition(ActorOrigin);
	const FVector BoxExtent = RainVolume->GetUnscaledBoxExtent();
	return FMath::Abs(LocalPoint.X) <= BoxExtent.X
		&& FMath::Abs(LocalPoint.Y) <= BoxExtent.Y
		&& FMath::Abs(LocalPoint.Z) <= BoxExtent.Z;
}

bool AUOUUmbrellaRainArea::IsActorBlockedByRainBlocker(const AActor* Actor, const FVector& BlockerWorldCenter, const FRotator& BlockerWorldRotation, const FVector& BlockerHalfExtent) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	FVector ActorOrigin = FVector::ZeroVector;
	FVector ActorExtent = FVector::ZeroVector;
	Actor->GetActorBounds(false, ActorOrigin, ActorExtent);
	if (ActorExtent.IsNearlyZero())
	{
		ActorOrigin = Actor->GetActorLocation();
	}

	const FVector SafeHalfExtent(
		FMath::Max(0.0f, BlockerHalfExtent.X),
		FMath::Max(0.0f, BlockerHalfExtent.Y),
		FMath::Max(0.0f, BlockerHalfExtent.Z));
	if (SafeHalfExtent.X <= KINDA_SMALL_NUMBER || SafeHalfExtent.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FTransform BlockerTransform(BlockerWorldRotation, BlockerWorldCenter);
	const FVector LocalPoint = BlockerTransform.InverseTransformPosition(ActorOrigin);
	return FMath::Abs(LocalPoint.X) <= SafeHalfExtent.X
		&& FMath::Abs(LocalPoint.Y) <= SafeHalfExtent.Y
		&& LocalPoint.Z <= SafeHalfExtent.Z;
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
