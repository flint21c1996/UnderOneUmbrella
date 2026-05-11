// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Pickups/UOUUmbrellaPickupActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Actor.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UObject/ConstructorHelpers.h"

AUOUUmbrellaPickupActor::AUOUUmbrellaPickupActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	PickupTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PickupTrigger"));
	PickupTrigger->SetupAttachment(RootScene);
	PickupTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupTrigger->SetGenerateOverlapEvents(true);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootScene);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMeshFinder.Object);
	}
}

void AUOUUmbrellaPickupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TriggerRadius = FMath::Max(0.0f, TriggerRadius);
	PickupTrigger->SetSphereRadius(TriggerRadius);
	ApplyVisualMeshPlacement();
}

void AUOUUmbrellaPickupActor::BeginPlay()
{
	Super::BeginPlay();

	TriggerRadius = FMath::Max(0.0f, TriggerRadius);
	HoverAmplitude = FMath::Max(0.0f, HoverAmplitude);
	HoverSpeed = FMath::Max(0.0f, HoverSpeed);

	PickupTrigger->SetSphereRadius(TriggerRadius);
	ApplyVisualMeshPlacement();
	PickupTrigger->OnComponentBeginOverlap.AddDynamic(this, &AUOUUmbrellaPickupActor::HandlePickupTriggerBeginOverlap);
}

void AUOUUmbrellaPickupActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bUseSimpleHoverMotion || VisualMesh == nullptr)
	{
		return;
	}

	const float Time = GetGameTimeSinceCreation();
	FVector RelativeLocation = MeshRelativeOffset;
	RelativeLocation.Z += FMath::Sin(Time * HoverSpeed) * HoverAmplitude;
	VisualMesh->SetRelativeLocation(RelativeLocation);
}

void AUOUUmbrellaPickupActor::ApplyVisualMeshPlacement()
{
	if (VisualMesh == nullptr)
	{
		return;
	}

	VisualMesh->SetRelativeLocation(MeshRelativeOffset);
	VisualMesh->SetRelativeRotation(MeshRelativeRotation);
	VisualMesh->SetRelativeScale3D(MeshRelativeScale);
}

void AUOUUmbrellaPickupActor::HandlePickupTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!TryGiveUmbrellaToActor(OtherActor))
	{
		return;
	}

	if (bDestroyOnPickup)
	{
		Destroy();
	}
}

bool AUOUUmbrellaPickupActor::TryGiveUmbrellaToActor(AActor* OtherActor)
{
	if (OtherActor == nullptr)
	{
		return false;
	}

	UUOUUmbrellaComponent* UmbrellaComponent = OtherActor->FindComponentByClass<UUOUUmbrellaComponent>();
	if (UmbrellaComponent == nullptr || UmbrellaComponent->bHasUmbrella)
	{
		return false;
	}

	UmbrellaComponent->AcquireUmbrellaFromMeshComponent(VisualMesh);
	return true;
}
