// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/RainArea/UOUUmbrellaRainArea.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Puzzle/Water/UOUWaterWheelRainConditionComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Environment/UOUEnvironmentVisualComponent.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

namespace
{
	constexpr float MaxRainAreaFlowSpeed = 3000.0f;

	float NormalizeRainAreaFlowSpeed(float FlowSpeed, EUOURainAreaFlowDirection FlowDirection)
	{
		const float SpeedMagnitude = FMath::Clamp(FMath::Abs(FlowSpeed), 0.0f, MaxRainAreaFlowSpeed);
		return FlowDirection == EUOURainAreaFlowDirection::Upward ? SpeedMagnitude : -SpeedMagnitude;
	}

	void BuildRainSampleBasis(const FVector& RainDirection, FVector& OutBasisX, FVector& OutBasisY)
	{
		const FVector SafeDirection = RainDirection.GetSafeNormal();
		OutBasisX = FVector::CrossProduct(SafeDirection, FVector::UpVector);
		if (OutBasisX.IsNearlyZero())
		{
			OutBasisX = FVector::CrossProduct(SafeDirection, FVector::RightVector);
		}

		OutBasisX = OutBasisX.GetSafeNormal();
		OutBasisY = FVector::CrossProduct(SafeDirection, OutBasisX).GetSafeNormal();
	}
}

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
	RainFallSpeed = NormalizeRainAreaFlowSpeed(RainFallSpeed, FlowDirection);
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
	RainFallSpeed = NormalizeRainAreaFlowSpeed(RainFallSpeed, FlowDirection);
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
	RainFallSpeed = NormalizeRainAreaFlowSpeed(RainFallSpeed, FlowDirection);
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
	bool bHasVisualRainBlocker = false;
	FVector VisualRainBlockerWorldCenter = FVector::ZeroVector;
	FVector VisualRainBlockerHalfExtent = FVector::ZeroVector;

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

			FVector CandidateVisualBlockerWorldCenter = FVector::ZeroVector;
			FRotator CandidateVisualBlockerWorldRotation = FRotator::ZeroRotator;
			FVector CandidateVisualBlockerHalfExtent = FVector::ZeroVector;
			const bool bBlocksGameplayRain = UmbrellaComponent->IsBlockingRain();
			const bool bBlocksRainVisual = bBlocksGameplayRain || UmbrellaComponent->IsUpsideDown();
			if (bBlocksRainVisual
				&& UmbrellaComponent->TryGetGameplayRainBlockerVolumeData(CandidateVisualBlockerWorldCenter, CandidateVisualBlockerWorldRotation, CandidateVisualBlockerHalfExtent)
				&& CandidateVisualBlockerHalfExtent.SizeSquared() > VisualRainBlockerHalfExtent.SizeSquared())
			{
				bHasVisualRainBlocker = true;
				VisualRainBlockerWorldCenter = CandidateVisualBlockerWorldCenter;
				VisualRainBlockerHalfExtent = CandidateVisualBlockerHalfExtent;
			}

			FVector CandidateGameplayBlockerWorldCenter = FVector::ZeroVector;
			FRotator CandidateGameplayBlockerWorldRotation = FRotator::ZeroRotator;
			FVector CandidateGameplayBlockerHalfExtent = FVector::ZeroVector;
			if (bBlocksGameplayRain
				&& UmbrellaComponent->TryGetGameplayRainBlockerVolumeData(CandidateGameplayBlockerWorldCenter, CandidateGameplayBlockerWorldRotation, CandidateGameplayBlockerHalfExtent)
				&& CandidateGameplayBlockerHalfExtent.SizeSquared() > RainBlockerHalfExtent.SizeSquared())
			{
				bHasRainBlocker = true;
				RainBlockerWorldCenter = CandidateGameplayBlockerWorldCenter;
				RainBlockerWorldRotation = CandidateGameplayBlockerWorldRotation;
				RainBlockerHalfExtent = CandidateGameplayBlockerHalfExtent;
			}
		}
	}

	ApplyRainToWaterBasinTargets(
		DeltaSeconds,
		bHasRainBlocker,
		RainBlockerWorldCenter,
		RainBlockerWorldRotation,
		RainBlockerHalfExtent);

	ApplyRainToWaterWheelTargets(
		DeltaSeconds,
		bHasRainBlocker,
		RainBlockerWorldCenter,
		RainBlockerWorldRotation,
		RainBlockerHalfExtent);

	ApplyEnvironmentVisualRainBlocker(
		bHasVisualRainBlocker,
		VisualRainBlockerWorldCenter,
		VisualRainBlockerHalfExtent,
		bHasVisualRainBlocker ? RainVisualIntensity : 0.0f);
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
	const bool bFlowUpward = FlowDirection == EUOURainAreaFlowDirection::Upward;
	const FVector RainSpawnPlaneWorldPosition = bFlowUpward
		? VolumeCenter - VolumeUp * BoxExtent.Z
		: VolumeCenter + VolumeUp * BoxExtent.Z;
	const FVector GroundSplashWorldPosition = bFlowUpward
		? VolumeCenter + VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset)
		: VolumeCenter - VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset);

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

	RainFallSpeed = NormalizeRainAreaFlowSpeed(RainFallSpeed, FlowDirection);
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
			|| !DoesActorBoundsOverlapRainVolume(TargetOwner)
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
		InputContext.WorldDirection = FlowDirection == EUOURainAreaFlowDirection::Upward
			? RainVolume->GetUpVector()
			: -RainVolume->GetUpVector();
		InputContext.WorldLocation = TargetOwner->GetActorLocation();
		InputContext.InstigatorActor = const_cast<AUOUUmbrellaRainArea*>(this);
		InputContext.bApplyToConnectedGroup = true;
		Target->ReceiveWaterInput(InputContext);
	}
}

void AUOUUmbrellaRainArea::ApplyRainToWaterWheelTargets(float DeltaSeconds, bool bHasRainBlocker, const FVector& RainBlockerWorldCenter, const FRotator& RainBlockerWorldRotation, const FVector& RainBlockerHalfExtent)
{
	bLastWaterWheelRainInputTickRan = true;
	LastWaterWheelActorScanCount = 0;
	LastWaterWheelComponentCount = 0;
	LastWaterWheelValidComponentCount = 0;
	LastWaterWheelCatchSampleCount = 0;
	LastWaterWheelAcceptedSampleCount = 0;
	LastWaterWheelDeliveredStrength = 0.0f;
	LastWaterWheelSampleLocation = FVector::ZeroVector;
	LastWaterWheelRainDebugReason = TEXT("Running");

	if (!bEnableWaterWheelRainInput)
	{
		LastWaterWheelRainDebugReason = TEXT("Disabled");
		return;
	}

	const float SafeDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	const float RainStrength = FMath::Max(0.0f, RainFillRate);
	if (SafeDeltaSeconds <= 0.0f || RainStrength <= KINDA_SMALL_NUMBER || RainVolume == nullptr)
	{
		LastWaterWheelRainDebugReason = FString::Printf(
			TEXT("Skipped: Delta %.3f Rain %.3f Volume %s"),
			SafeDeltaSeconds,
			RainStrength,
			RainVolume != nullptr ? TEXT("Y") : TEXT("N"));
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		LastWaterWheelRainDebugReason = TEXT("Skipped: No World");
		return;
	}

	const FVector RainDirection = FlowDirection == EUOURainAreaFlowDirection::Upward
		? RainVolume->GetUpVector()
		: -RainVolume->GetUpVector();
	int32 RainInputDisabledComponentCount = 0;
	int32 InactiveComponentCount = 0;
	int32 NoOwnerComponentCount = 0;
	int32 OtherCannotReceiveComponentCount = 0;

	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		++LastWaterWheelActorScanCount;
		if (!IsValid(Actor))
		{
			continue;
		}

		TInlineComponentArray<UUOUWaterWheelRainConditionComponent*> WaterWheelComponents(Actor);
		LastWaterWheelComponentCount += WaterWheelComponents.Num();
		for (UUOUWaterWheelRainConditionComponent* WaterWheel : WaterWheelComponents)
		{
			if (!IsValid(WaterWheel))
			{
				continue;
			}

			if (!WaterWheel->bRainInputEnabled)
			{
				++RainInputDisabledComponentCount;
				continue;
			}

			if (!WaterWheel->IsActive())
			{
				++InactiveComponentCount;
				continue;
			}

			if (WaterWheel->GetOwner() == nullptr)
			{
				++NoOwnerComponentCount;
				continue;
			}

			if (!WaterWheel->CanReceiveRainInput())
			{
				++OtherCannotReceiveComponentCount;
				continue;
			}
			++LastWaterWheelValidComponentCount;

			TArray<FUOUWaterWheelRainCatchSample> CatchSamples;
			WaterWheel->GetRainCatchSamples(CatchSamples);
			LastWaterWheelCatchSampleCount += CatchSamples.Num();
			for (const FUOUWaterWheelRainCatchSample& CatchSample : CatchSamples)
			{
				LastWaterWheelSampleLocation = CatchSample.WorldLocation;
				const float RainScale = CalculateWaterWheelCatchRainScale(
					CatchSample,
					bHasRainBlocker,
					RainBlockerWorldCenter,
					RainBlockerWorldRotation,
					RainBlockerHalfExtent);

				if (bDrawWaterWheelRainInputDebug)
				{
					const FColor DebugColor =
						CatchSample.Weight <= KINDA_SMALL_NUMBER
							? FColor::Yellow
							: (RainScale > KINDA_SMALL_NUMBER ? FColor::Green : FColor::Red);
					DrawDebugSphere(
						World,
						CatchSample.WorldLocation,
						FMath::Max(8.0f, CatchSample.CoverageRadius * 0.15f),
						12,
						DebugColor,
						false,
						WaterWheelRainInputDebugLifeTime,
						0,
						2.0f);
					DrawDebugString(
						World,
						CatchSample.WorldLocation + FVector(0.0f, 0.0f, 22.0f),
						FString::Printf(TEXT("WheelRain %.2f W %.2f"), RainScale, CatchSample.Weight),
						nullptr,
						DebugColor,
						WaterWheelRainInputDebugLifeTime,
						true,
						0.85f);
				}

				if (CatchSample.Weight <= KINDA_SMALL_NUMBER || RainScale <= KINDA_SMALL_NUMBER)
				{
					continue;
				}

				FUOUWaterWheelRainInputContext InputContext;
				InputContext.Strength = RainStrength * CatchSample.Weight * RainScale;
				InputContext.Duration = SafeDeltaSeconds;
				InputContext.WorldLocation = CatchSample.WorldLocation;
				InputContext.WorldDirection = RainDirection;
				InputContext.bHasValidWorldLocation = true;
				InputContext.InstigatorActor = const_cast<AUOUUmbrellaRainArea*>(this);
				WaterWheel->ReceiveRainInput(InputContext);
				++LastWaterWheelAcceptedSampleCount;
				LastWaterWheelDeliveredStrength += InputContext.Strength;
			}
		}
	}

	if (LastWaterWheelComponentCount == 0)
	{
		LastWaterWheelRainDebugReason = TEXT("No WaterWheel Components");
	}
	else if (LastWaterWheelValidComponentCount == 0)
	{
		LastWaterWheelRainDebugReason = FString::Printf(
			TEXT("Cannot Receive: RainOff %d Inactive %d NoOwner %d Other %d"),
			RainInputDisabledComponentCount,
			InactiveComponentCount,
			NoOwnerComponentCount,
			OtherCannotReceiveComponentCount);
	}
	else if (LastWaterWheelCatchSampleCount == 0)
	{
		LastWaterWheelRainDebugReason = TEXT("No Catch Samples");
	}
	else if (LastWaterWheelAcceptedSampleCount == 0)
	{
		LastWaterWheelRainDebugReason = TEXT("All Samples Outside Or Blocked");
	}
	else
	{
		LastWaterWheelRainDebugReason = FString::Printf(
			TEXT("Delivered %.2f from %d samples"),
			LastWaterWheelDeliveredStrength,
			LastWaterWheelAcceptedSampleCount);
	}

	if (bDrawWaterWheelRainInputDebug && RainVolume != nullptr)
	{
		const FColor SummaryColor = LastWaterWheelAcceptedSampleCount > 0 ? FColor::Green : FColor::Red;
		const FVector SummaryLocation =
			RainVolume->GetComponentLocation()
			+ RainVolume->GetUpVector() * (RainVolume->GetScaledBoxExtent().Z + 80.0f);
		DrawDebugString(
			World,
			SummaryLocation,
			FString::Printf(
				TEXT("WheelRain: %s\nActors %d Components %d Valid %d Samples %d Accepted %d"),
				*LastWaterWheelRainDebugReason,
				LastWaterWheelActorScanCount,
				LastWaterWheelComponentCount,
				LastWaterWheelValidComponentCount,
				LastWaterWheelCatchSampleCount,
				LastWaterWheelAcceptedSampleCount),
			nullptr,
			SummaryColor,
			WaterWheelRainInputDebugLifeTime,
			true,
			0.9f);
	}
}

float AUOUUmbrellaRainArea::CalculateWaterWheelCatchRainScale(
	const FUOUWaterWheelRainCatchSample& CatchSample,
	bool bHasRainBlocker,
	const FVector& RainBlockerWorldCenter,
	const FRotator& RainBlockerWorldRotation,
	const FVector& RainBlockerHalfExtent) const
{
	if (RainVolume == nullptr)
	{
		return 0.0f;
	}

	const auto CanReceiveAtLocation = [this, bHasRainBlocker, &RainBlockerWorldCenter, &RainBlockerWorldRotation, &RainBlockerHalfExtent](const FVector& WorldLocation)
	{
		return IsWorldLocationInsideRainVolume(WorldLocation)
			&& (!bHasRainBlocker
				|| !IsWorldLocationBlockedByRainBlocker(
					WorldLocation,
					RainBlockerWorldCenter,
					RainBlockerWorldRotation,
					RainBlockerHalfExtent));
	};

	const bool bCanReceiveAtCenter = CanReceiveAtLocation(CatchSample.WorldLocation);
	if (bRequireWaterWheelCatchPointCenterInsideRainVolume && !bCanReceiveAtCenter)
	{
		return 0.0f;
	}

	const float SafeCoverageRadius = FMath::Max(0.0f, CatchSample.CoverageRadius);
	if (SafeCoverageRadius <= KINDA_SMALL_NUMBER)
	{
		return bCanReceiveAtCenter
			? CalculateRainVolumeCenterStrength(CatchSample.WorldLocation)
			: 0.0f;
	}

	const FVector RainDirection = FlowDirection == EUOURainAreaFlowDirection::Upward
		? RainVolume->GetUpVector()
		: -RainVolume->GetUpVector();

	FVector BasisX = FVector::RightVector;
	FVector BasisY = FVector::ForwardVector;
	BuildRainSampleBasis(RainDirection, BasisX, BasisY);

	struct FRainCoverageSampleOffset
	{
		FVector2D Offset;
		float Weight = 1.0f;
	};

	const FRainCoverageSampleOffset SampleOffsets[] = {
		{ FVector2D(0.0f, 0.0f), 1.5f },
		{ FVector2D(1.0f, 0.0f), 1.0f },
		{ FVector2D(-1.0f, 0.0f), 1.0f },
		{ FVector2D(0.0f, 1.0f), 1.0f },
		{ FVector2D(0.0f, -1.0f), 1.0f },
		{ FVector2D(0.7071f, 0.7071f), 0.75f },
		{ FVector2D(0.7071f, -0.7071f), 0.75f },
		{ FVector2D(-0.7071f, 0.7071f), 0.75f },
		{ FVector2D(-0.7071f, -0.7071f), 0.75f }
	};

	float WeightedRainScale = 0.0f;
	float TotalWeight = 0.0f;
	for (const FRainCoverageSampleOffset& SampleOffset : SampleOffsets)
	{
		const FVector SampleLocation =
			CatchSample.WorldLocation
			+ BasisX * (SampleOffset.Offset.X * SafeCoverageRadius)
			+ BasisY * (SampleOffset.Offset.Y * SafeCoverageRadius);
		TotalWeight += SampleOffset.Weight;

		if (!CanReceiveAtLocation(SampleLocation))
		{
			continue;
		}

		WeightedRainScale += SampleOffset.Weight * CalculateRainVolumeCenterStrength(SampleLocation);
	}

	return TotalWeight > KINDA_SMALL_NUMBER ? WeightedRainScale / TotalWeight : 0.0f;
}

float AUOUUmbrellaRainArea::CalculateRainVolumeCenterStrength(const FVector& WorldLocation) const
{
	if (RainVolume == nullptr)
	{
		return 0.0f;
	}

	const FVector LocalLocation = RainVolume->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector BoxExtent = RainVolume->GetUnscaledBoxExtent();
	if (FMath::Abs(LocalLocation.X) > BoxExtent.X
		|| FMath::Abs(LocalLocation.Y) > BoxExtent.Y
		|| FMath::Abs(LocalLocation.Z) > BoxExtent.Z)
	{
		return 0.0f;
	}

	const float NormalizedX = BoxExtent.X > KINDA_SMALL_NUMBER
		? FMath::Abs(LocalLocation.X) / BoxExtent.X
		: 0.0f;
	const float NormalizedY = BoxExtent.Y > KINDA_SMALL_NUMBER
		? FMath::Abs(LocalLocation.Y) / BoxExtent.Y
		: 0.0f;
	const float EdgeAlpha = FMath::Clamp(FMath::Max(NormalizedX, NormalizedY), 0.0f, 1.0f);
	const float CenterAlpha = 1.0f - EdgeAlpha;
	const float SmoothCenterAlpha = CenterAlpha * CenterAlpha * (3.0f - 2.0f * CenterAlpha);
	const float SafeExponent = FMath::Max(0.1f, WaterWheelRainCenterFalloffExponent);
	const float CurvedCenterAlpha = FMath::Pow(SmoothCenterAlpha, SafeExponent);
	const float SafeEdgeStrength = FMath::Clamp(WaterWheelRainEdgeStrength, 0.0f, 1.0f);

	return FMath::Lerp(SafeEdgeStrength, 1.0f, CurvedCenterAlpha);
}

bool AUOUUmbrellaRainArea::DoesActorBoundsOverlapRainVolume(const AActor* Actor) const
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

	const FTransform RainVolumeTransform = RainVolume->GetComponentTransform();
	const FVector BoxExtent = RainVolume->GetUnscaledBoxExtent();
	FBox ActorBoundsInRainVolumeLocal(ForceInit);

	for (int32 XIndex = 0; XIndex < 2; ++XIndex)
	{
		for (int32 YIndex = 0; YIndex < 2; ++YIndex)
		{
			for (int32 ZIndex = 0; ZIndex < 2; ++ZIndex)
			{
				const FVector CornerWorldLocation = ActorOrigin + FVector(
					XIndex == 0 ? -ActorExtent.X : ActorExtent.X,
					YIndex == 0 ? -ActorExtent.Y : ActorExtent.Y,
					ZIndex == 0 ? -ActorExtent.Z : ActorExtent.Z);

				ActorBoundsInRainVolumeLocal += RainVolumeTransform.InverseTransformPosition(CornerWorldLocation);
			}
		}
	}

	return ActorBoundsInRainVolumeLocal.Min.X <= BoxExtent.X
		&& ActorBoundsInRainVolumeLocal.Max.X >= -BoxExtent.X
		&& ActorBoundsInRainVolumeLocal.Min.Y <= BoxExtent.Y
		&& ActorBoundsInRainVolumeLocal.Max.Y >= -BoxExtent.Y
		&& ActorBoundsInRainVolumeLocal.Min.Z <= BoxExtent.Z
		&& ActorBoundsInRainVolumeLocal.Max.Z >= -BoxExtent.Z;
}

bool AUOUUmbrellaRainArea::IsWorldLocationInsideRainVolume(const FVector& WorldLocation) const
{
	if (RainVolume == nullptr)
	{
		return false;
	}

	const FVector LocalLocation = RainVolume->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector BoxExtent = RainVolume->GetUnscaledBoxExtent();
	return FMath::Abs(LocalLocation.X) <= BoxExtent.X
		&& FMath::Abs(LocalLocation.Y) <= BoxExtent.Y
		&& FMath::Abs(LocalLocation.Z) <= BoxExtent.Z;
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
	FBox ActorBoundsInBlockerLocal(ForceInit);

	for (int32 XIndex = 0; XIndex < 2; ++XIndex)
	{
		for (int32 YIndex = 0; YIndex < 2; ++YIndex)
		{
			for (int32 ZIndex = 0; ZIndex < 2; ++ZIndex)
			{
				const FVector CornerWorldLocation = ActorOrigin + FVector(
					XIndex == 0 ? -ActorExtent.X : ActorExtent.X,
					YIndex == 0 ? -ActorExtent.Y : ActorExtent.Y,
					ZIndex == 0 ? -ActorExtent.Z : ActorExtent.Z);

				ActorBoundsInBlockerLocal += BlockerTransform.InverseTransformPosition(CornerWorldLocation);
			}
		}
	}

	const bool bOverlapsBlockerArea = ActorBoundsInBlockerLocal.Min.X <= SafeHalfExtent.X
		&& ActorBoundsInBlockerLocal.Max.X >= -SafeHalfExtent.X
		&& ActorBoundsInBlockerLocal.Min.Y <= SafeHalfExtent.Y
		&& ActorBoundsInBlockerLocal.Max.Y >= -SafeHalfExtent.Y;

	return bOverlapsBlockerArea && ActorBoundsInBlockerLocal.Min.Z <= SafeHalfExtent.Z;
}

bool AUOUUmbrellaRainArea::IsWorldLocationBlockedByRainBlocker(const FVector& WorldLocation, const FVector& BlockerWorldCenter, const FRotator& BlockerWorldRotation, const FVector& BlockerHalfExtent) const
{
	const FVector SafeHalfExtent(
		FMath::Max(0.0f, BlockerHalfExtent.X),
		FMath::Max(0.0f, BlockerHalfExtent.Y),
		FMath::Max(0.0f, BlockerHalfExtent.Z));
	if (SafeHalfExtent.X <= KINDA_SMALL_NUMBER || SafeHalfExtent.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FTransform BlockerTransform(BlockerWorldRotation, BlockerWorldCenter);
	const FVector LocalLocation = BlockerTransform.InverseTransformPosition(WorldLocation);
	const bool bOverlapsBlockerArea = FMath::Abs(LocalLocation.X) <= SafeHalfExtent.X
		&& FMath::Abs(LocalLocation.Y) <= SafeHalfExtent.Y;
	if (!bOverlapsBlockerArea)
	{
		return false;
	}

	return FlowDirection == EUOURainAreaFlowDirection::Upward
		? LocalLocation.Z >= -SafeHalfExtent.Z
		: LocalLocation.Z <= SafeHalfExtent.Z;
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
	const bool bFlowUpward = FlowDirection == EUOURainAreaFlowDirection::Upward;
	const FVector RainSpawnPlaneWorldPosition = bFlowUpward
		? VolumeCenter - VolumeUp * BoxExtent.Z
		: VolumeCenter + VolumeUp * BoxExtent.Z;
	const FVector GroundSplashWorldPosition = bFlowUpward
		? VolumeCenter + VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset)
		: VolumeCenter - VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset);
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
	const FVector VolumeRelativeScale = RainVolume->GetRelativeScale3D();
	const FVector BaseScale(
		(BoxExtent.X / 50.0f) * VolumeRelativeScale.X,
		(BoxExtent.Y / 50.0f) * VolumeRelativeScale.Y,
		(BoxExtent.Z / 50.0f) * VolumeRelativeScale.Z);

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
