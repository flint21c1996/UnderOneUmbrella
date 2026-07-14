// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Pour/UOUPourDropActor.h"

#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"

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

	struct FPourReceiverCandidate
	{
		UObject* ReceiverObject = nullptr;
		int32 Priority = 0;
		int32 HierarchyDepth = 0;
		FString StableKey;
	};

	bool IsPourReceiverObject(const UObject* Object)
	{
		return IsValid(Object)
			&& Object->GetClass()->ImplementsInterface(UUOUPourReceiver::StaticClass());
	}

	AActor* ResolvePourReceiverActor(UObject* ReceiverObject)
	{
		if (AActor* ReceiverActor = Cast<AActor>(ReceiverObject))
		{
			return ReceiverActor;
		}

		if (UActorComponent* ReceiverComponent = Cast<UActorComponent>(ReceiverObject))
		{
			return ReceiverComponent->GetOwner();
		}

		return nullptr;
	}

	void AddPourReceiverCandidate(
		UObject* ReceiverObject,
		const FUOUPourInputContext& Context,
		int32 HierarchyDepth,
		bool bRequireLocationAcceptance,
		TArray<FPourReceiverCandidate>& OutCandidates)
	{
		if (!IsPourReceiverObject(ReceiverObject)
			|| OutCandidates.ContainsByPredicate(
				[ReceiverObject](const FPourReceiverCandidate& Candidate)
				{
					return Candidate.ReceiverObject == ReceiverObject;
				}))
		{
			return;
		}

		const bool bCanAccept = bRequireLocationAcceptance
			? IUOUPourReceiver::Execute_CanAcceptPourAtLocation(ReceiverObject, Context)
			: IUOUPourReceiver::Execute_CanAcceptPour(ReceiverObject, Context);
		if (!bCanAccept)
		{
			return;
		}

		FPourReceiverCandidate& Candidate = OutCandidates.AddDefaulted_GetRef();
		Candidate.ReceiverObject = ReceiverObject;
		Candidate.Priority = IUOUPourReceiver::Execute_GetPourReceivePriority(ReceiverObject);
		Candidate.HierarchyDepth = HierarchyDepth;
		Candidate.StableKey = ReceiverObject->GetPathName();
	}

	bool TryReceivePourFromCandidates(
		TArray<FPourReceiverCandidate>& Candidates,
		const FUOUPourInputContext& Context,
		FUOUPourReceiveResult& OutResult)
	{
		Candidates.Sort(
			[](const FPourReceiverCandidate& Left, const FPourReceiverCandidate& Right)
			{
				if (Left.Priority != Right.Priority)
				{
					return Left.Priority > Right.Priority;
				}

				if (Left.HierarchyDepth != Right.HierarchyDepth)
				{
					return Left.HierarchyDepth < Right.HierarchyDepth;
				}

				return Left.StableKey.Compare(Right.StableKey) < 0;
			});

		for (const FPourReceiverCandidate& Candidate : Candidates)
		{
			FUOUPourReceiveResult Result = IUOUPourReceiver::Execute_TryReceivePour(Candidate.ReceiverObject, Context);
			if (!Result.bAccepted)
			{
				continue;
			}

			if (!IsValid(Result.ReceiverObject.Get()))
			{
				Result.ReceiverObject = Candidate.ReceiverObject;
			}
			if (!IsValid(Result.ReceiverActor.Get()))
			{
				Result.ReceiverActor = ResolvePourReceiverActor(Candidate.ReceiverObject);
			}

			OutResult = MoveTemp(Result);
			return true;
		}

		return false;
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

	const FVector ImpactLocation = GetActorLocation();
	const FUOUPourInputContext PourContext = BuildPourInputContext(ImpactLocation, CurrentWorldDirection);
	FUOUPourReceiveResult ReceiveResult;
	if (!TryDeliverWaterAtLocation(PourContext, ReceiveResult))
	{
		return;
	}

	const FVector ImpactNormal = -CurrentWorldDirection.GetSafeNormal();
	LastReceiverType = ReceiveResult.ReceiverType;
	LastReceiverId = ReceiveResult.ReceiverId;
	bHasDeliveredWater = true;
	SpawnImpactSplash(ImpactLocation, ImpactNormal, true);
	OnPourDropImpacted.Broadcast(this, ReceiveResult.ReceiverActor.Get(), ImpactLocation, ReceiveResult.ReceiverType, true);

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
		|| CollisionComponent == nullptr)
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

	FUOUPourReceiveResult ReceiveResult;
	const bool bDeliveredWater = TryDeliverWater(OtherActor, ImpactLocation, ReceiveResult);

	LastReceiverType = ReceiveResult.ReceiverType;
	LastReceiverId = ReceiveResult.ReceiverId;
	bHasDeliveredWater = bDeliveredWater;
	SpawnImpactSplash(ImpactLocation, ImpactNormal, bDeliveredWater);
	OnPourDropImpacted.Broadcast(this, OtherActor, ImpactLocation, ReceiveResult.ReceiverType, bDeliveredWater);

	if ((bDeliveredWater && bDestroyOnFirstValidReceiver)
		|| (!bDeliveredWater && bIsBlockingImpact && bDestroyOnBlockingHitWithoutReceiver))
	{
		Destroy();
	}
}

FUOUPourInputContext AUOUPourDropActor::BuildPourInputContext(const FVector& WorldLocation, const FVector& WorldDirection) const
{
	FUOUPourInputContext Context;
	Context.Volume = FMath::Max(CurrentVolume, 0.0f);
	Context.Duration = FMath::Max(CurrentDuration, 0.0f);
	Context.WorldDirection = WorldDirection.GetSafeNormal();
	Context.WorldLocation = WorldLocation;
	Context.bHasValidWorldLocation = true;
	Context.LocationAcceptanceTolerance = FMath::Max(CollisionRadius, WaterBasinDeliveryVerticalTolerance);
	Context.InstigatorActor = SourceInstigatorActor;
	Context.bPropagateToConnectedTargets = bCurrentApplyToConnectedWaterBasinGroup;
	return Context;
}

bool AUOUPourDropActor::TryDeliverWater(AActor* HitActor, const FVector& ImpactLocation, FUOUPourReceiveResult& OutResult)
{
	OutResult = FUOUPourReceiveResult();
	if (CurrentVolume <= 0.0f)
	{
		return false;
	}

	const FVector DeliveryWorldDirection = ProjectileMovement != nullptr && !ProjectileMovement->Velocity.IsNearlyZero()
		? ProjectileMovement->Velocity.GetSafeNormal()
		: CurrentWorldDirection;
	const FUOUPourInputContext Context = BuildPourInputContext(ImpactLocation, DeliveryWorldDirection);

	TArray<FPourReceiverCandidate> Candidates;
	TSet<AActor*> VisitedActors;
	AActor* CurrentActor = HitActor;
	int32 HierarchyDepth = 0;
	while (IsValid(CurrentActor) && !VisitedActors.Contains(CurrentActor))
	{
		VisitedActors.Add(CurrentActor);
		AddPourReceiverCandidate(CurrentActor, Context, HierarchyDepth, false, Candidates);

		TInlineComponentArray<UActorComponent*> ActorComponents(CurrentActor);
		for (UActorComponent* ActorComponent : ActorComponents)
		{
			AddPourReceiverCandidate(ActorComponent, Context, HierarchyDepth, false, Candidates);
		}

		CurrentActor = ResolveParentActor(CurrentActor);
		++HierarchyDepth;
	}

	if (TryReceivePourFromCandidates(Candidates, Context, OutResult))
	{
		return true;
	}

	return TryDeliverWaterAtLocation(Context, OutResult);
}

bool AUOUPourDropActor::TryDeliverWaterAtLocation(const FUOUPourInputContext& Context, FUOUPourReceiveResult& OutResult)
{
	OutResult = FUOUPourReceiveResult();
	if (Context.Volume <= 0.0f)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	TArray<FPourReceiverCandidate> Candidates;
	for (TObjectIterator<UActorComponent> It; It; ++It)
	{
		UActorComponent* ReceiverComponent = *It;
		if (!IsValid(ReceiverComponent) || ReceiverComponent->GetWorld() != World)
		{
			continue;
		}

		AddPourReceiverCandidate(ReceiverComponent, Context, 0, true, Candidates);
	}

	return TryReceivePourFromCandidates(Candidates, Context, OutResult);
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
