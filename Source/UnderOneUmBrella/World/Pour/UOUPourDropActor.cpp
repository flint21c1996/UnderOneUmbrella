// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Pour/UOUPourDropActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/UOUWaterContainerComponent.h"
#include "Puzzle/Water/UOUWaterWheelRainConditionComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"
#include "World/Pour/UOUPourReceiverComponent.h"
#include "World/WaterTarget/UOUUmbrellaWaterTarget.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

namespace
{
	AActor* ResolveParentActor(AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return nullptr;
		}

		if (AActor* AttachParentActor = Actor->GetAttachParentActor())
		{
			return AttachParentActor;
		}

		AActor* ParentActor = Actor->GetParentActor();
		return ParentActor != Actor ? ParentActor : nullptr;
	}

	template<typename ComponentType>
	ComponentType* FindComponentOnActorOrParent(AActor* Actor, AActor*& OutOwnerActor)
	{
		OutOwnerActor = nullptr;
		AActor* CurrentActor = Actor;
		while (IsValid(CurrentActor))
		{
			if (ComponentType* Component = CurrentActor->FindComponentByClass<ComponentType>())
			{
				OutOwnerActor = CurrentActor;
				return Component;
			}

			CurrentActor = ResolveParentActor(CurrentActor);
		}

		return nullptr;
	}

	AUOUUmbrellaWaterTarget* FindUmbrellaWaterTargetOnActorOrParent(AActor* Actor)
	{
		if (AUOUUmbrellaWaterTarget* WaterTarget = Cast<AUOUUmbrellaWaterTarget>(Actor))
		{
			return WaterTarget;
		}

		return Cast<AUOUUmbrellaWaterTarget>(ResolveParentActor(Actor));
	}
}

AUOUPourDropActor::AUOUPourDropActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->InitSphereRadius(CollisionRadius);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMeshFinder.Object);
	}

	TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(CollisionComponent);
	TrailEffect->SetAutoActivate(bActivateTrailEffect);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
}

void AUOUPourDropActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyCollisionSettings();
	ApplyVisualSettings();
	ApplyMovementSettings();
}

void AUOUPourDropActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyCollisionSettings();
	ApplyVisualSettings();
	ApplyMovementSettings();
	IgnoreSourceActor();

	if (DropLifeSpan > 0.0f)
	{
		SetLifeSpan(DropLifeSpan);
	}

	CollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &AUOUPourDropActor::HandleCollisionBeginOverlap);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AUOUPourDropActor::HandleCollisionBeginOverlap);

	ProjectileMovement->OnProjectileStop.RemoveDynamic(this, &AUOUPourDropActor::HandleProjectileStop);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AUOUPourDropActor::HandleProjectileStop);

	if (!bHasInitializedVelocity)
	{
		ApplyLaunchVelocity();
	}
}

void AUOUPourDropActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DrawDebugCollisionRadius();

	if (bHasDeliveredWater || CurrentVolume <= 0.0f)
	{
		return;
	}

	EUOUPourDropReceiverType ReceiverType = EUOUPourDropReceiverType::None;
	AActor* ReceiverActor = nullptr;
	const FVector ImpactLocation = GetActorLocation();
	if (!TryDeliverWaterToBasinAtLocation(ImpactLocation, ReceiverType, ReceiverActor))
	{
		return;
	}

	const FVector ImpactNormal = -CurrentWorldDirection.GetSafeNormal();
	LastReceiverType = ReceiverType;
	bHasDeliveredWater = true;
	SpawnImpactSplash(ImpactLocation, ImpactNormal, true);
	OnPourDropImpacted.Broadcast(this, ReceiverActor, ImpactLocation, ReceiverType, true);

	if (bDestroyOnFirstValidReceiver)
	{
		Destroy();
	}
}

void AUOUPourDropActor::InitializePourDrop(const FUOUPourDropContext& DropContext)
{
	CurrentVolume = FMath::Max(0.0f, DropContext.Volume);
	CurrentDuration = FMath::Max(0.0f, DropContext.Duration);
	CurrentWorldDirection = DropContext.WorldDirection.GetSafeNormal();
	if (CurrentWorldDirection.IsNearlyZero())
	{
		CurrentWorldDirection = GetActorForwardVector();
	}

	SourceInstigatorActor = DropContext.InstigatorActor;
	bCurrentApplyToConnectedWaterBasinGroup = DropContext.bApplyToConnectedWaterBasinGroup;
	ApplyContextVisualSettings(DropContext.VisualSettings);
	IgnoreSourceActor();
	ApplyCollisionSettings();
	ApplyVisualSettings();
	ApplyMovementSettings();
	SetLifeSpan(FMath::Max(0.0f, DropLifeSpan));
	ApplyLaunchVelocity();
}

void AUOUPourDropActor::HandleCollisionBeginOverlap(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent*,
	int32,
	bool,
	const FHitResult& SweepResult)
{
	HandleImpact(SweepResult, OtherActor, false);
}

void AUOUPourDropActor::HandleProjectileStop(const FHitResult& ImpactResult)
{
	HandleImpact(ImpactResult, ImpactResult.GetActor(), true);
}

void AUOUPourDropActor::ApplyCollisionSettings()
{
	if (CollisionComponent == nullptr)
	{
		return;
	}

	CollisionRadius = FMath::Max(0.0f, CollisionRadius);
	CollisionComponent->SetSphereRadius(CollisionRadius);
}

void AUOUPourDropActor::ApplyVisualSettings()
{
	if (VisualMesh != nullptr)
	{
		if (VisualMeshAsset != nullptr)
		{
			VisualMesh->SetStaticMesh(VisualMeshAsset);
		}

		for (int32 MaterialIndex = 0; MaterialIndex < VisualMaterials.Num(); ++MaterialIndex)
		{
			if (VisualMaterials[MaterialIndex] != nullptr)
			{
				VisualMesh->SetMaterial(MaterialIndex, VisualMaterials[MaterialIndex]);
			}
		}

		const FVector SafeVisualScale(
			FMath::Max(0.0f, VisualMeshRelativeScale.X),
			FMath::Max(0.0f, VisualMeshRelativeScale.Y),
			FMath::Max(0.0f, VisualMeshRelativeScale.Z));
		VisualMesh->SetRelativeLocation(VisualMeshRelativeOffset);
		VisualMesh->SetRelativeRotation(VisualMeshRelativeRotation);
		VisualMesh->SetRelativeScale3D(SafeVisualScale);
		VisualMesh->SetVisibility(bShowDebugVisualMesh, true);
		VisualMesh->SetHiddenInGame(!bShowDebugVisualMesh, true);
	}

	if (TrailEffect != nullptr)
	{
		if (TrailEffectAsset != nullptr)
		{
			TrailEffect->SetAsset(TrailEffectAsset);
		}

		TrailEffect->SetAutoActivate(bActivateTrailEffect);
		if (bActivateTrailEffect)
		{
			TrailEffect->Activate(true);
		}
		else
		{
			TrailEffect->DeactivateImmediate();
		}
	}
}

void AUOUPourDropActor::ApplyContextVisualSettings(const FUOUPourDropVisualSettings& VisualSettings)
{
	if (VisualSettings.VisualMesh != nullptr)
	{
		VisualMeshAsset = VisualSettings.VisualMesh;
	}
	if (!VisualSettings.VisualMaterials.IsEmpty())
	{
		VisualMaterials = VisualSettings.VisualMaterials;
	}

	if (VisualSettings.TrailEffect != nullptr)
	{
		TrailEffectAsset = VisualSettings.TrailEffect;
	}

	if (VisualSettings.ImpactSplashEffect != nullptr)
	{
		ImpactSplashEffect = VisualSettings.ImpactSplashEffect;
	}

	if (!VisualSettings.bOverrideDropActorSettings)
	{
		return;
	}

	VisualMeshRelativeScale = VisualSettings.VisualMeshRelativeScale;
	VisualMeshRelativeOffset = VisualSettings.VisualMeshRelativeOffset;
	VisualMeshRelativeRotation = VisualSettings.VisualMeshRelativeRotation;
	bShowDebugVisualMesh = VisualSettings.bShowDebugVisualMesh;
	bDrawDebugCollisionRadius = VisualSettings.bDrawDebugCollisionRadius;

	bActivateTrailEffect = VisualSettings.bActivateTrailEffect;

	CollisionRadius = VisualSettings.CollisionRadius;
	WaterBasinDeliveryVerticalTolerance = VisualSettings.WaterBasinDeliveryVerticalTolerance;
	InitialSpeed = VisualSettings.InitialSpeed;
	MaxSpeed = VisualSettings.MaxSpeed;
	GravityScale = VisualSettings.GravityScale;
	bUseVerticalDescent = VisualSettings.bUseVerticalDescent;
	DropLifeSpan = VisualSettings.DropLifeSpan;

	bDestroyOnFirstValidReceiver = VisualSettings.bDestroyOnFirstValidReceiver;
	bDestroyOnBlockingHitWithoutReceiver = VisualSettings.bDestroyOnBlockingHitWithoutReceiver;
	bSpawnSplashOnlyWhenDelivered = VisualSettings.bSpawnSplashOnlyWhenDelivered;
	ImpactSplashScale = VisualSettings.ImpactSplashScale;
	ImpactSplashScaleParameterName = VisualSettings.ImpactSplashScaleParameterName;
}

void AUOUPourDropActor::ApplyMovementSettings()
{
	if (ProjectileMovement == nullptr)
	{
		return;
	}

	InitialSpeed = FMath::Max(0.0f, InitialSpeed);
	MaxSpeed = FMath::Max(InitialSpeed, MaxSpeed);
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
}

void AUOUPourDropActor::ApplyLaunchVelocity()
{
	if (ProjectileMovement == nullptr)
	{
		return;
	}

	const FVector LaunchDirection = ResolveLaunchDirection();

	ProjectileMovement->Velocity = LaunchDirection * FMath::Max(0.0f, InitialSpeed);
	CurrentWorldDirection = LaunchDirection;
	bHasInitializedVelocity = true;
}

FVector AUOUPourDropActor::ResolveLaunchDirection() const
{
	if (bUseVerticalDescent)
	{
		return FVector::DownVector;
	}

	FVector LaunchDirection = CurrentWorldDirection.GetSafeNormal();
	if (LaunchDirection.IsNearlyZero())
	{
		LaunchDirection = GetActorForwardVector();
	}

	return LaunchDirection.IsNearlyZero() ? FVector::DownVector : LaunchDirection;
}

void AUOUPourDropActor::IgnoreSourceActor()
{
	if (CollisionComponent == nullptr)
	{
		return;
	}

	if (AActor* OwnerActor = GetOwner())
	{
		CollisionComponent->IgnoreActorWhenMoving(OwnerActor, true);
	}

	if (SourceInstigatorActor != nullptr)
	{
		CollisionComponent->IgnoreActorWhenMoving(SourceInstigatorActor, true);
	}
}

void AUOUPourDropActor::DrawDebugCollisionRadius() const
{
	if (!bDrawDebugCollisionRadius
		|| CollisionComponent == nullptr
		|| !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Player))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float DebugRadius = FMath::Max(1.0f, CollisionComponent->GetScaledSphereRadius());
	const FColor DebugColor = bHasDeliveredWater ? FColor::Green : FColor::Cyan;
	DrawDebugSphere(
		World,
		CollisionComponent->GetComponentLocation(),
		DebugRadius,
		24,
		DebugColor,
		false,
		0.0f,
		0,
		1.5f);
}

void AUOUPourDropActor::HandleImpact(const FHitResult& ImpactResult, AActor* OtherActor, bool bIsBlockingImpact)
{
	if (bHasDeliveredWater || ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	FVector ImpactLocation = GetActorLocation();
	if (ImpactResult.bBlockingHit || ImpactResult.bStartPenetrating)
	{
		ImpactLocation = FVector(
			ImpactResult.ImpactPoint.X,
			ImpactResult.ImpactPoint.Y,
			ImpactResult.ImpactPoint.Z);
	}

	FVector ImpactNormal = -CurrentWorldDirection.GetSafeNormal();
	if (!ImpactResult.ImpactNormal.IsNearlyZero())
	{
		ImpactNormal = FVector(
			ImpactResult.ImpactNormal.X,
			ImpactResult.ImpactNormal.Y,
			ImpactResult.ImpactNormal.Z);
	}

	EUOUPourDropReceiverType ReceiverType = EUOUPourDropReceiverType::None;
	const bool bDeliveredWater = TryDeliverWater(OtherActor, ImpactLocation, ReceiverType);

	LastReceiverType = ReceiverType;
	bHasDeliveredWater = bDeliveredWater;
	SpawnImpactSplash(ImpactLocation, ImpactNormal, bDeliveredWater);
	OnPourDropImpacted.Broadcast(this, OtherActor, ImpactLocation, ReceiverType, bDeliveredWater);

	if ((bDeliveredWater && bDestroyOnFirstValidReceiver)
		|| (!bDeliveredWater && bIsBlockingImpact && bDestroyOnBlockingHitWithoutReceiver))
	{
		Destroy();
	}
}

bool AUOUPourDropActor::TryDeliverWater(AActor* HitActor, const FVector& ImpactLocation, EUOUPourDropReceiverType& OutReceiverType)
{
	OutReceiverType = EUOUPourDropReceiverType::None;
	if (CurrentVolume <= 0.0f)
	{
		return false;
	}

	const FVector DeliveryWorldDirection = ProjectileMovement != nullptr && !ProjectileMovement->Velocity.IsNearlyZero()
		? ProjectileMovement->Velocity.GetSafeNormal()
		: CurrentWorldDirection;
	AActor* ReceiverOwner = nullptr;
	if (IsValid(HitActor))
	{
		if (UUOUWaterWheelRainConditionComponent* WaterWheelCondition = FindComponentOnActorOrParent<UUOUWaterWheelRainConditionComponent>(HitActor, ReceiverOwner))
		{
			if (WaterWheelCondition->CanReceivePouredWaterInput())
			{
				FUOUWaterWheelRainInputContext WaterWheelContext;
				WaterWheelContext.Strength = CurrentDuration > KINDA_SMALL_NUMBER
					? CurrentVolume / CurrentDuration
					: CurrentVolume;
				WaterWheelContext.Duration = CurrentDuration;
				WaterWheelContext.WorldDirection = DeliveryWorldDirection;
				WaterWheelContext.WorldLocation = ImpactLocation;
				WaterWheelContext.bHasValidWorldLocation = true;
				WaterWheelContext.InstigatorActor = SourceInstigatorActor;
				WaterWheelCondition->ReceivePouredWaterInput(WaterWheelContext);
				OutReceiverType = EUOUPourDropReceiverType::WaterWheel;
				return true;
			}
		}

		if (UUOUPourReceiverComponent* PourReceiver = FindComponentOnActorOrParent<UUOUPourReceiverComponent>(HitActor, ReceiverOwner))
		{
			if (PourReceiver->CanReceivePour())
			{
				FUOUPourInputContext PourContext;
				PourContext.Volume = CurrentVolume;
				PourContext.Duration = CurrentDuration;
				PourContext.WorldDirection = DeliveryWorldDirection;
				PourContext.WorldLocation = ImpactLocation;
				PourContext.bHasValidWorldLocation = true;
				PourContext.InstigatorActor = SourceInstigatorActor;
				PourReceiver->ReceivePourInput(PourContext);
				OutReceiverType = EUOUPourDropReceiverType::PurePourReceiver;
				return true;
			}
		}

		if (AUOUUmbrellaWaterTarget* WaterTarget = FindUmbrellaWaterTargetOnActorOrParent(HitActor))
		{
			WaterTarget->ReceiveWater(CurrentVolume);
			OutReceiverType = EUOUPourDropReceiverType::UmbrellaWaterTarget;
			return true;
		}

		if (UUOUWaterBasinTargetComponent* WaterBasinTarget = FindComponentOnActorOrParent<UUOUWaterBasinTargetComponent>(HitActor, ReceiverOwner))
		{
			FUOUWaterBasinInputContext InputContext;
			InputContext.Volume = CurrentVolume;
			InputContext.Duration = CurrentDuration;
			InputContext.Source = EUOUWaterBasinInputSource::PlayerPour;
			InputContext.WorldDirection = DeliveryWorldDirection;
			InputContext.WorldLocation = ImpactLocation;
			InputContext.bHasValidWorldLocation = WaterBasinTarget->IsWorldLocationInsideBasin(ImpactLocation);
			InputContext.InstigatorActor = SourceInstigatorActor;
			InputContext.bApplyToConnectedGroup = bCurrentApplyToConnectedWaterBasinGroup;
			InputContext.bImpactSplashHandledBySource = ShouldHandleImpactSplashAtSource(true);
			WaterBasinTarget->ReceiveWaterInput(InputContext);
			OutReceiverType = EUOUPourDropReceiverType::WaterBasinTarget;
			return true;
		}

		if (UUOUWaterContainerComponent* WaterContainer = FindComponentOnActorOrParent<UUOUWaterContainerComponent>(HitActor, ReceiverOwner))
		{
			WaterContainer->AddAmount(CurrentVolume);
			OutReceiverType = EUOUPourDropReceiverType::WaterContainer;
			return true;
		}
	}

	AActor* BasinReceiverActor = nullptr;
	return TryDeliverWaterToBasinAtLocation(ImpactLocation, OutReceiverType, BasinReceiverActor);
}

bool AUOUPourDropActor::TryDeliverWaterToBasinAtLocation(const FVector& ImpactLocation, EUOUPourDropReceiverType& OutReceiverType, AActor*& OutReceiverActor)
{
	OutReceiverType = EUOUPourDropReceiverType::None;
	OutReceiverActor = nullptr;
	if (CurrentVolume <= 0.0f)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	for (TObjectIterator<UUOUWaterBasinTargetComponent> It; It; ++It)
	{
		UUOUWaterBasinTargetComponent* WaterBasinTarget = *It;
		if (!IsValid(WaterBasinTarget)
			|| WaterBasinTarget->GetWorld() != World
			|| !IsWaterBasinDeliveryLocation(WaterBasinTarget, ImpactLocation))
		{
			continue;
		}

		FUOUWaterBasinInputContext InputContext;
		InputContext.Volume = CurrentVolume;
		InputContext.Duration = CurrentDuration;
		InputContext.Source = EUOUWaterBasinInputSource::PlayerPour;
		InputContext.WorldDirection = CurrentWorldDirection;
		InputContext.WorldLocation = ImpactLocation;
		InputContext.bHasValidWorldLocation = true;
		InputContext.InstigatorActor = SourceInstigatorActor;
		InputContext.bApplyToConnectedGroup = bCurrentApplyToConnectedWaterBasinGroup;
		InputContext.bImpactSplashHandledBySource = ShouldHandleImpactSplashAtSource(true);
		WaterBasinTarget->ReceiveWaterInput(InputContext);

		OutReceiverType = EUOUPourDropReceiverType::WaterBasinTarget;
		OutReceiverActor = WaterBasinTarget->GetOwner();
		return true;
	}

	return false;
}

bool AUOUPourDropActor::IsWaterBasinDeliveryLocation(const UUOUWaterBasinTargetComponent* WaterBasinTarget, const FVector& ImpactLocation) const
{
	if (!IsValid(WaterBasinTarget) || !WaterBasinTarget->IsWorldLocationInsideBasin(ImpactLocation))
	{
		return false;
	}

	const float DeliveryTolerance = FMath::Max(FMath::Max(CollisionRadius, WaterBasinDeliveryVerticalTolerance), 1.0f);
	const float BottomWorldZ = WaterBasinTarget->GetBottomWorldZ();
	const float SurfaceWorldZ = FMath::Max(WaterBasinTarget->WaterSurfaceWorldZ, BottomWorldZ);
	return ImpactLocation.Z >= BottomWorldZ - DeliveryTolerance
		&& ImpactLocation.Z <= SurfaceWorldZ + DeliveryTolerance;
}

bool AUOUPourDropActor::ShouldHandleImpactSplashAtSource(bool bDeliveredWater) const
{
	return ImpactSplashEffect != nullptr
		&& ImpactSplashScale > KINDA_SMALL_NUMBER
		&& (!bSpawnSplashOnlyWhenDelivered || bDeliveredWater);
}

void AUOUPourDropActor::SpawnImpactSplash(const FVector& ImpactLocation, const FVector& ImpactNormal, bool bDeliveredWater) const
{
	UWorld* World = GetWorld();
	if (World == nullptr || !ShouldHandleImpactSplashAtSource(bDeliveredWater))
	{
		return;
	}

	const FVector SafeNormal = ImpactNormal.IsNearlyZero() ? FVector::UpVector : ImpactNormal.GetSafeNormal();
	UNiagaraComponent* SplashComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		ImpactSplashEffect,
		ImpactLocation,
		SafeNormal.Rotation(),
		FVector::OneVector,
		true,
		false);

	if (SplashComponent == nullptr)
	{
		return;
	}

	if (!ImpactSplashScaleParameterName.IsNone())
	{
		SplashComponent->SetVariableFloat(ImpactSplashScaleParameterName, FMath::Max(0.0f, ImpactSplashScale));
	}

	SplashComponent->Activate(true);
}

bool AUOUPourDropActor::ShouldIgnoreActor(const AActor* OtherActor) const
{
	return OtherActor == nullptr
		|| OtherActor == this
		|| OtherActor == GetOwner()
		|| OtherActor == SourceInstigatorActor;
}
