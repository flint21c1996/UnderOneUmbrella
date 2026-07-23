// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WindArea/UOUUmbrellaWindArea.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UObject/ConstructorHelpers.h"

AUOUUmbrellaWindArea::AUOUUmbrellaWindArea()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	WindVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WindVolume"));
	WindVolume->SetupAttachment(RootScene);
	WindVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WindVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	WindVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WindVolume->SetGenerateOverlapEvents(true);
	WindVolume->SetBoxExtent(FVector(250.0f, 250.0f, 200.0f));

	WindTarget = CreateDefaultSubobject<UArrowComponent>(TEXT("WindTarget"));
	WindTarget->SetupAttachment(RootScene);
	WindTarget->SetRelativeLocation(FVector(1000.0f, 0.0f, 300.0f));
	WindTarget->ArrowColor = FColor::Green;
	WindTarget->ArrowSize = 1.5f;

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

void AUOUUmbrellaWindArea::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RefreshPreview();

	if (!bWindEnabled || WindVolume == nullptr || WindTarget == nullptr)
	{
		TrackedPlayers.Reset();
		return;
	}

	RefreshTrackedPlayers();
	ApplyWindToTrackedPlayers(DeltaSeconds);

	if (bDrawWindDebug && GetWorld() != nullptr)
	{
		DrawDebugDirectionalArrow(
			GetWorld(),
			WindVolume->GetComponentLocation(),
			WindTarget->GetComponentLocation(),
			80.0f,
			FColor::Cyan,
			false,
			0.0f,
			0,
			4.0f);
	}
}

void AUOUUmbrellaWindArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPreview();
}

void AUOUUmbrellaWindArea::RefreshPreview()
{
	if (WindVolume == nullptr || WindTarget == nullptr)
	{
		return;
	}

	const FVector DirectionToTarget =
		WindTarget->GetComponentLocation() - WindVolume->GetComponentLocation();

	if (WindDirectionArrow != nullptr && !DirectionToTarget.IsNearlyZero())
	{
		WindDirectionArrow->SetWorldLocation(WindVolume->GetComponentLocation());
		WindDirectionArrow->SetWorldRotation(DirectionToTarget.Rotation());
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

void AUOUUmbrellaWindArea::RefreshTrackedPlayers()
{
	TArray<AActor*> OverlappingActors;
	WindVolume->GetOverlappingActors(OverlappingActors, AUOUCharacter::StaticClass());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		AUOUCharacter* Character = Cast<AUOUCharacter>(OverlappingActor);
		if (Character == nullptr)
		{
			continue;
		}

		const UUOUUmbrellaComponent* UmbrellaComponent =
			Character->FindComponentByClass<UUOUUmbrellaComponent>();
		if (UmbrellaComponent != nullptr && UmbrellaComponent->IsOpen())
		{
			TrackedPlayers.Add(TWeakObjectPtr<AUOUCharacter>(Character));
		}
	}
}

void AUOUUmbrellaWindArea::ApplyWindToTrackedPlayers(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f || MoveSpeed <= 0.0f)
	{
		return;
	}

	for (auto PlayerIterator = TrackedPlayers.CreateIterator(); PlayerIterator; ++PlayerIterator)
	{
		AUOUCharacter* Character = PlayerIterator->Get();
		if (Character == nullptr)
		{
			PlayerIterator.RemoveCurrent();
			continue;
		}

		const UUOUUmbrellaComponent* UmbrellaComponent =
			Character->FindComponentByClass<UUOUUmbrellaComponent>();
		if (UmbrellaComponent == nullptr || !UmbrellaComponent->IsOpen())
		{
			PlayerIterator.RemoveCurrent();
			continue;
		}

		const FVector ToTarget = WindTarget->GetComponentLocation() - Character->GetActorLocation();
		const float DistanceToTarget = ToTarget.Size();
		if (DistanceToTarget <= AcceptanceRadius || ToTarget.IsNearlyZero())
		{
			if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
			{
				MovementComponent->StopMovementImmediately();
			}

			PlayerIterator.RemoveCurrent();
			continue;
		}

		// 남은 거리가 짧으면 한 프레임에 목표를 지나치지 않도록 마지막 이동 속도만 줄입니다.
		const float DesiredSpeed = FMath::Min(MoveSpeed, DistanceToTarget / DeltaSeconds);
		const FVector DesiredVelocity = ToTarget.GetSafeNormal() * DesiredSpeed;
		Character->LaunchCharacter(DesiredVelocity, true, true);
	}
}
