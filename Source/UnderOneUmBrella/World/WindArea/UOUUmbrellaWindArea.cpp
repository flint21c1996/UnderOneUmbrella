// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WindArea/UOUUmbrellaWindArea.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUPlayerSplineTravelComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UObject/ConstructorHelpers.h"

AUOUUmbrellaWindArea::AUOUUmbrellaWindArea()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	WindVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WindVolume"));
	WindVolume->SetupAttachment(RootScene);
	WindVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WindVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	WindVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WindVolume->SetGenerateOverlapEvents(true);
	WindVolume->SetBoxExtent(FVector(250.0f, 250.0f, 200.0f));
	WindVolume->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&AUOUUmbrellaWindArea::HandleWindVolumeBeginOverlap);
	WindVolume->OnComponentEndOverlap.AddUniqueDynamic(
		this,
		&AUOUUmbrellaWindArea::HandleWindVolumeEndOverlap);

	WindPath = CreateDefaultSubobject<USplineComponent>(TEXT("WindPath"));
	WindPath->SetupAttachment(RootScene);
	WindPath->ClearSplinePoints(false);
	WindPath->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
	WindPath->AddSplinePoint(FVector(1000.0f, 0.0f, 300.0f), ESplineCoordinateSpace::Local, true);

	WindDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("WindDirectionArrow"));
	WindDirectionArrow->SetupAttachment(RootScene);
	WindDirectionArrow->ArrowColor = FColor::Cyan;
	WindDirectionArrow->ArrowSize = 2.0f;

	PreviewVolumeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewVolumeMesh"));
	PreviewVolumeMesh->SetupAttachment(RootScene);
	PreviewVolumeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewVolumeMesh->SetGenerateOverlapEvents(false);
	PreviewVolumeMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		PreviewVolumeMesh->SetStaticMesh(CubeMeshFinder.Object);
	}
}

void AUOUUmbrellaWindArea::BeginPlay()
{
	Super::BeginPlay();
	RefreshPreview();

	if (WindVolume != nullptr)
	{
		TArray<AActor*> InitiallyOverlappingPlayers;
		WindVolume->GetOverlappingActors(
			InitiallyOverlappingPlayers,
			AUOUCharacter::StaticClass());
		if (!InitiallyOverlappingPlayers.IsEmpty())
		{
			SetOverlappingPlayer(Cast<AUOUCharacter>(InitiallyOverlappingPlayers[0]));
		}
	}

	RefreshTickEnabled();
}

void AUOUUmbrellaWindArea::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDrawWindDebug
		&& GetWorld() != nullptr
		&& WindPath != nullptr
		&& WindPath->GetNumberOfSplinePoints() >= 2)
	{
		constexpr int32 DebugSegmentCount = 32;
		const float SplineLength = WindPath->GetSplineLength();
		FVector PreviousLocation =
			WindPath->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);

		for (int32 SegmentIndex = 1; SegmentIndex <= DebugSegmentCount; ++SegmentIndex)
		{
			const float Distance = SplineLength * SegmentIndex / DebugSegmentCount;
			const FVector SegmentLocation =
				WindPath->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			DrawDebugLine(GetWorld(), PreviousLocation, SegmentLocation, FColor::Cyan, false, 0.0f, 0, 4.0f);
			PreviousLocation = SegmentLocation;
		}
	}
}

void AUOUUmbrellaWindArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPreview();
}

void AUOUUmbrellaWindArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetOverlappingPlayer(nullptr);
	Super::EndPlay(EndPlayReason);
}

void AUOUUmbrellaWindArea::RefreshPreview()
{
	if (WindVolume == nullptr
		|| WindPath == nullptr
		|| WindPath->GetNumberOfSplinePoints() <= 0)
	{
		return;
	}

	const FVector PathStart =
		WindPath->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
	const FVector InitialPathDirection =
		WindPath->GetDirectionAtSplinePoint(0, ESplineCoordinateSpace::World);
	const FVector DirectionToPathStart = PathStart - WindVolume->GetComponentLocation();

	if (WindDirectionArrow != nullptr)
	{
		WindDirectionArrow->SetWorldLocation(WindVolume->GetComponentLocation());

		const FVector PreviewDirection = !DirectionToPathStart.IsNearlyZero()
			? DirectionToPathStart
			: InitialPathDirection;
		if (!PreviewDirection.IsNearlyZero())
		{
			WindDirectionArrow->SetWorldRotation(PreviewDirection.Rotation());
		}
	}

	if (PreviewVolumeMesh != nullptr)
	{
		// 엔진 기본 Cube는 한 변이 100cm이므로 BoxExtent를 50으로 나눠 동일한 크기로 맞춥니다.
		PreviewVolumeMesh->SetRelativeTransform(WindVolume->GetRelativeTransform());
		PreviewVolumeMesh->SetRelativeScale3D(
			(WindVolume->GetUnscaledBoxExtent() / 50.0f) * WindVolume->GetRelativeScale3D());
		PreviewVolumeMesh->SetHiddenInGame(!bShowPreviewInGame);
		PreviewVolumeMesh->SetVisibility(true);
	}
}

void AUOUUmbrellaWindArea::RefreshTickEnabled()
{
	SetActorTickEnabled(bDrawWindDebug);
}

void AUOUUmbrellaWindArea::SetOverlappingPlayer(AUOUCharacter* Character)
{
	AUOUCharacter* PreviousCharacter = OverlappingPlayer.Get();
	if (PreviousCharacter == Character)
	{
		return;
	}

	if (PreviousCharacter != nullptr)
	{
		PreviousCharacter->OnCharacterJumped.RemoveDynamic(
			this,
			&AUOUUmbrellaWindArea::HandleOverlappingPlayerJumped);
	}

	OverlappingPlayer = Character;

	if (Character != nullptr)
	{
		Character->OnCharacterJumped.AddUniqueDynamic(
			this,
			&AUOUUmbrellaWindArea::HandleOverlappingPlayerJumped);
	}

	RefreshTickEnabled();
}

void AUOUUmbrellaWindArea::HandleOverlappingPlayerJumped()
{
	AUOUCharacter* Character = OverlappingPlayer.Get();
	const UUOUUmbrellaComponent* UmbrellaComponent = Character != nullptr
		? Character->FindComponentByClass<UUOUUmbrellaComponent>()
		: nullptr;
	if (!bWindEnabled
		|| MoveSpeed <= 0.0f
		|| WindPath == nullptr
		|| WindPath->GetNumberOfSplinePoints() < 2
		|| UmbrellaComponent == nullptr
		|| !UmbrellaComponent->IsOpen())
	{
		return;
	}

	if (UUOUPlayerSplineTravelComponent* TravelComponent =
		Character->GetSplineTravelComponent())
	{
		TravelComponent->StartTravel(
			WindPath,
			MoveSpeed,
			AcceptanceRadius,
			this,
			true);
	}
}

void AUOUUmbrellaWindArea::HandleWindVolumeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (AUOUCharacter* Character = Cast<AUOUCharacter>(OtherActor))
	{
		SetOverlappingPlayer(Character);
	}
}

void AUOUUmbrellaWindArea::HandleWindVolumeEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (OtherActor == OverlappingPlayer.Get())
	{
		SetOverlappingPlayer(nullptr);
	}
}
