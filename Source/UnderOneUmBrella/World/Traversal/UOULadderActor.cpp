// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Traversal/UOULadderActor.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AUOULadderActor::AUOULadderActor()
{
	constexpr float DefaultTopHeight = 400.0f;

	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootScene);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);

	const FRotator DefaultCharacterFacing(0.0f, 180.0f, 0.0f);

	BottomStandingAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("BottomStandingAnchor"));
	BottomStandingAnchor->SetupAttachment(RootScene);
	BottomStandingAnchor->SetRelativeLocation(FVector(100.0f, 0.0f, 96.0f));
	BottomStandingAnchor->SetRelativeRotation(DefaultCharacterFacing);
	BottomStandingAnchor->ArrowColor = FColor::Green;
	BottomStandingAnchor->ArrowSize = 0.75f;
	BottomStandingAnchor->SetHiddenInGame(true);

	BottomClimbAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("BottomClimbAnchor"));
	BottomClimbAnchor->SetupAttachment(RootScene);
	BottomClimbAnchor->SetRelativeLocation(FVector(55.0f, 0.0f, 96.0f));
	BottomClimbAnchor->SetRelativeRotation(DefaultCharacterFacing);
	BottomClimbAnchor->ArrowColor = FColor::Blue;
	BottomClimbAnchor->ArrowSize = 0.75f;
	BottomClimbAnchor->SetHiddenInGame(true);

	TopClimbAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("TopClimbAnchor"));
	TopClimbAnchor->SetupAttachment(RootScene);
	TopClimbAnchor->SetRelativeLocation(FVector(55.0f, 0.0f, DefaultTopHeight));
	TopClimbAnchor->SetRelativeRotation(DefaultCharacterFacing);
	TopClimbAnchor->ArrowColor = FColor(255, 128, 0);
	TopClimbAnchor->ArrowSize = 0.75f;
	TopClimbAnchor->SetHiddenInGame(true);

	TopStandingAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("TopStandingAnchor"));
	TopStandingAnchor->SetupAttachment(RootScene);
	TopStandingAnchor->SetRelativeLocation(FVector(-80.0f, 0.0f, DefaultTopHeight + 96.0f));
	TopStandingAnchor->SetRelativeRotation(DefaultCharacterFacing);
	TopStandingAnchor->ArrowColor = FColor::Magenta;
	TopStandingAnchor->ArrowSize = 0.75f;
	TopStandingAnchor->SetHiddenInGame(true);

	DetectionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionVolume"));
	DetectionVolume->SetupAttachment(RootScene);
	DetectionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DetectionVolume->SetGenerateOverlapEvents(true);
	DetectionVolume->SetHiddenInGame(true);
	DetectionVolume->ShapeColor = FColor::Cyan;

	BottomExitMarker = CreateDefaultSubobject<UBoxComponent>(TEXT("BottomExitMarker"));
	BottomExitMarker->SetupAttachment(RootScene);
	BottomExitMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BottomExitMarker->SetGenerateOverlapEvents(false);
	BottomExitMarker->SetHiddenInGame(true);
	BottomExitMarker->ShapeColor = FColor::Red;
	BottomExitMarker->SetRelativeLocation(FVector(0.0f, 0.0f, 99.0f));

	TopExitMarker = CreateDefaultSubobject<UBoxComponent>(TEXT("TopExitMarker"));
	TopExitMarker->SetupAttachment(RootScene);
	TopExitMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TopExitMarker->SetGenerateOverlapEvents(false);
	TopExitMarker->SetHiddenInGame(true);
	TopExitMarker->ShapeColor = FColor::Yellow;
	TopExitMarker->SetRelativeLocation(FVector(0.0f, 0.0f, DefaultTopHeight - 3.0f));

	ApplyDetectionVolumeSettings();
}

void AUOULadderActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDetectionVolumeSettings();
}

FVector AUOULadderActor::GetClimbLocationNear(const FVector& WorldLocation) const
{
	const FVector BottomLocation = GetBottomClimbLocation();
	const FVector TopLocation = GetTopClimbLocation();
	const FVector ClimbRail = TopLocation - BottomLocation;
	const float RailSizeSquared = ClimbRail.SizeSquared();
	if (RailSizeSquared <= UE_SMALL_NUMBER)
	{
		return BottomLocation;
	}

	const float Alpha = FMath::Clamp(
		FVector::DotProduct(WorldLocation - BottomLocation, ClimbRail) / RailSizeSquared,
		0.0f,
		1.0f);
	return FMath::Lerp(BottomLocation, TopLocation, Alpha);
}

FVector AUOULadderActor::GetClimbDirection() const
{
	return (GetTopClimbLocation() - GetBottomClimbLocation()).GetSafeNormal();
}

FRotator AUOULadderActor::GetClimbingRotationNear(const FVector& WorldLocation) const
{
	const FVector BottomLocation = GetBottomClimbLocation();
	const FVector ClimbRail = GetTopClimbLocation() - BottomLocation;
	const float RailSizeSquared = ClimbRail.SizeSquared();
	const float Alpha = RailSizeSquared > UE_SMALL_NUMBER
		? FMath::Clamp(FVector::DotProduct(WorldLocation - BottomLocation, ClimbRail) / RailSizeSquared, 0.0f, 1.0f)
		: 0.0f;
	return FQuat::Slerp(
		GetBottomClimbRotation().Quaternion(),
		GetTopClimbRotation().Quaternion(),
		Alpha).Rotator();
}

FVector AUOULadderActor::GetBottomStandingLocation() const
{
	return BottomStandingAnchor != nullptr ? BottomStandingAnchor->GetComponentLocation() : GetActorLocation();
}

FVector AUOULadderActor::GetTopStandingLocation() const
{
	return TopStandingAnchor != nullptr ? TopStandingAnchor->GetComponentLocation() : GetActorLocation();
}

FVector AUOULadderActor::GetBottomClimbLocation() const
{
	return BottomClimbAnchor != nullptr ? BottomClimbAnchor->GetComponentLocation() : GetActorLocation();
}

FVector AUOULadderActor::GetTopClimbLocation() const
{
	return TopClimbAnchor != nullptr ? TopClimbAnchor->GetComponentLocation() : GetActorLocation();
}

FRotator AUOULadderActor::GetBottomStandingRotation() const
{
	return BottomStandingAnchor != nullptr ? BottomStandingAnchor->GetComponentRotation() : GetActorRotation();
}

FRotator AUOULadderActor::GetTopStandingRotation() const
{
	return TopStandingAnchor != nullptr ? TopStandingAnchor->GetComponentRotation() : GetActorRotation();
}

FRotator AUOULadderActor::GetBottomClimbRotation() const
{
	return BottomClimbAnchor != nullptr ? BottomClimbAnchor->GetComponentRotation() : GetActorRotation();
}

FRotator AUOULadderActor::GetTopClimbRotation() const
{
	return TopClimbAnchor != nullptr ? TopClimbAnchor->GetComponentRotation() : GetActorRotation();
}

FVector AUOULadderActor::GetBottomEntryDirection() const
{
	const FVector Direction = FVector::VectorPlaneProject(
		GetBottomClimbLocation() - GetBottomStandingLocation(),
		GetActorUpVector()).GetSafeNormal();
	return Direction.IsNearlyZero() ? -GetOutwardNormal() : Direction;
}

FVector AUOULadderActor::GetTopEntryDirection() const
{
	const FVector Direction = FVector::VectorPlaneProject(
		GetTopClimbLocation() - GetTopStandingLocation(),
		GetActorUpVector()).GetSafeNormal();
	return Direction.IsNearlyZero() ? GetOutwardNormal() : Direction;
}

FVector AUOULadderActor::GetOutwardNormal() const
{
	return GetActorForwardVector().GetSafeNormal();
}

float AUOULadderActor::GetBottomClimbHeight() const
{
	return GetActorTransform().InverseTransformPosition(GetBottomClimbLocation()).Z;
}

float AUOULadderActor::GetTopClimbHeight() const
{
	return GetActorTransform().InverseTransformPosition(GetTopClimbLocation()).Z;
}

float AUOULadderActor::GetBottomExitHeight() const
{
	const float MinimumClimbHeight = FMath::Min(GetBottomClimbHeight(), GetTopClimbHeight());
	const float MaximumClimbHeight = FMath::Max(GetBottomClimbHeight(), GetTopClimbHeight());
	const float MarkerHeight = BottomExitMarker != nullptr
		? GetActorTransform().InverseTransformPosition(BottomExitMarker->GetComponentLocation()).Z
		: GetBottomClimbHeight();
	return FMath::Clamp(MarkerHeight, MinimumClimbHeight, MaximumClimbHeight);
}

float AUOULadderActor::GetTopExitHeight() const
{
	const float MinimumClimbHeight = FMath::Min(GetBottomClimbHeight(), GetTopClimbHeight());
	const float MaximumClimbHeight = FMath::Max(GetBottomClimbHeight(), GetTopClimbHeight());
	const float MarkerHeight = TopExitMarker != nullptr
		? GetActorTransform().InverseTransformPosition(TopExitMarker->GetComponentLocation()).Z
		: GetTopClimbHeight();
	return FMath::Clamp(MarkerHeight, MinimumClimbHeight, MaximumClimbHeight);
}

void AUOULadderActor::ApplyDetectionVolumeSettings()
{
	DetectionDepth = FMath::Max(20.0f, DetectionDepth);
	DetectionHalfWidth = FMath::Max(20.0f, DetectionHalfWidth);
	BottomEntryTolerance = FMath::Max(0.0f, BottomEntryTolerance);
	TopEntryTolerance = FMath::Max(0.0f, TopEntryTolerance);

	if (DetectionVolume == nullptr)
	{
		return;
	}

	const FTransform ActorTransform = GetActorTransform();
	const float BottomStandingHeight = ActorTransform.InverseTransformPosition(GetBottomStandingLocation()).Z;
	const float TopStandingHeight = ActorTransform.InverseTransformPosition(GetTopStandingLocation()).Z;
	const float DetectionBottom = FMath::Min(GetBottomClimbHeight(), BottomStandingHeight) - BottomEntryTolerance;
	const float DetectionTop = FMath::Max(GetTopClimbHeight(), TopStandingHeight) + TopEntryTolerance;
	const float DetectionHalfHeight = FMath::Max(20.0f, (DetectionTop - DetectionBottom) * 0.5f);
	DetectionVolume->SetBoxExtent(FVector(DetectionDepth, DetectionHalfWidth, DetectionHalfHeight));
	DetectionVolume->SetRelativeLocation(FVector(0.0f, 0.0f, (DetectionTop + DetectionBottom) * 0.5f));

	const FVector ExitMarkerExtent(DetectionDepth, DetectionHalfWidth, 2.0f);
	if (BottomExitMarker != nullptr)
	{
		BottomExitMarker->SetBoxExtent(ExitMarkerExtent);
	}

	if (TopExitMarker != nullptr)
	{
		TopExitMarker->SetBoxExtent(ExitMarkerExtent);
	}
}
