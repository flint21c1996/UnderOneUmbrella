// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Traversal/UOULadderActor.h"

#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

AUOULadderActor::AUOULadderActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootScene);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetCanEverAffectNavigation(false);

	LadderSegments = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("LadderSegments"));
	LadderSegments->SetupAttachment(RootScene);
	LadderSegments->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LadderSegments->SetGenerateOverlapEvents(false);
	LadderSegments->SetCanEverAffectNavigation(false);
	LadderSegments->SetVisibility(false);
	LadderSegments->SetHiddenInGame(true);

	DetectionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionVolume"));
	DetectionVolume->SetupAttachment(RootScene);
	DetectionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetectionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionVolume->SetGenerateOverlapEvents(false);
	DetectionVolume->SetHiddenInGame(true);
	DetectionVolume->ShapeColor = FColor::Red;
	DetectionVolume->SetBoxExtent(FVector(140.0f, 70.0f, 200.0f));
	DetectionVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));

	BottomEntryVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BottomEntryVolume"));
	BottomEntryVolume->SetupAttachment(RootScene);
	BottomEntryVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BottomEntryVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	BottomEntryVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BottomEntryVolume->SetGenerateOverlapEvents(true);
	BottomEntryVolume->SetHiddenInGame(true);
	BottomEntryVolume->ShapeColor = FColor::Green;
	BottomEntryVolume->SetBoxExtent(FVector(140.0f, 70.0f, 100.0f));
	BottomEntryVolume->SetRelativeTransform(BottomEntryTransform);

	TopEntryVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TopEntryVolume"));
	TopEntryVolume->SetupAttachment(RootScene);
	TopEntryVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TopEntryVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TopEntryVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TopEntryVolume->SetGenerateOverlapEvents(true);
	TopEntryVolume->SetHiddenInGame(true);
	TopEntryVolume->ShapeColor = FColor::Blue;
	TopEntryVolume->SetBoxExtent(FVector(140.0f, 70.0f, 140.0f));
	TopEntryVolume->SetRelativeTransform(TopEntryTransform);

	RefreshLadderLayout();
}

void AUOULadderActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshLadderLayout();
	SetActorTickEnabled(bShowTraversalDebugInGame);
}

void AUOULadderActor::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(bShowTraversalDebugInGame);
}

void AUOULadderActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bShowTraversalDebugInGame)
	{
		DrawTraversalDebug();
	}
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
	return GetActorTransform().TransformPosition(BottomEntryTransform.GetLocation());
}

FVector AUOULadderActor::GetTopStandingLocation() const
{
	return GetActorTransform().TransformPosition(TopEntryTransform.GetLocation());
}

FVector AUOULadderActor::GetBottomExitLocation() const
{
	return GetActorTransform().TransformPosition(BottomExitTransform.GetLocation());
}

FVector AUOULadderActor::GetTopExitLocation() const
{
	return GetActorTransform().TransformPosition(TopExitTransform.GetLocation());
}

FVector AUOULadderActor::GetBottomClimbLocation() const
{
	return GetActorTransform().TransformPosition(BottomClimbTransform.GetLocation());
}

FVector AUOULadderActor::GetTopClimbLocation() const
{
	return GetActorTransform().TransformPosition(TopClimbTransform.GetLocation());
}

FRotator AUOULadderActor::GetBottomStandingRotation() const
{
	return GetActorTransform().TransformRotation(BottomEntryTransform.GetRotation()).Rotator();
}

FRotator AUOULadderActor::GetTopStandingRotation() const
{
	return GetActorTransform().TransformRotation(TopEntryTransform.GetRotation()).Rotator();
}

FRotator AUOULadderActor::GetBottomExitRotation() const
{
	return GetActorTransform().TransformRotation(BottomExitTransform.GetRotation()).Rotator();
}

FRotator AUOULadderActor::GetTopExitRotation() const
{
	return GetActorTransform().TransformRotation(TopExitTransform.GetRotation()).Rotator();
}

FRotator AUOULadderActor::GetBottomClimbRotation() const
{
	return GetActorTransform().TransformRotation(BottomClimbTransform.GetRotation()).Rotator();
}

FRotator AUOULadderActor::GetTopClimbRotation() const
{
	return GetActorTransform().TransformRotation(TopClimbTransform.GetRotation()).Rotator();
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

bool AUOULadderActor::IsEntryVolume(const UPrimitiveComponent* Component) const
{
	return Component != nullptr &&
		(Component == BottomEntryVolume || Component == TopEntryVolume);
}

void AUOULadderActor::SetLadderHeight(float NewLadderHeight)
{
	if (DetectionVolume != nullptr)
	{
		const float SafeHeight = FMath::Max(50.0f, NewLadderHeight);
		const FVector CurrentExtent = DetectionVolume->GetUnscaledBoxExtent();
		const FVector CurrentScale = DetectionVolume->GetRelativeScale3D();
		const float CurrentHalfHeight = CurrentExtent.Z * FMath::Abs(CurrentScale.Z);
		const float CurrentBottom =
			DetectionVolume->GetRelativeLocation().Z - CurrentHalfHeight;
		DetectionVolume->SetBoxExtent(FVector(CurrentExtent.X, CurrentExtent.Y, SafeHeight * 0.5f));
		DetectionVolume->SetRelativeScale3D(FVector(CurrentScale.X, CurrentScale.Y, 1.0f));
		FVector VolumeLocation = DetectionVolume->GetRelativeLocation();
		VolumeLocation.Z = CurrentBottom + SafeHeight * 0.5f;
		DetectionVolume->SetRelativeLocation(VolumeLocation);
	}
	RefreshLadderLayout();
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
	return FMath::Clamp(
		GetBottomClimbHeight() + BottomExitOffset,
		MinimumClimbHeight,
		MaximumClimbHeight);
}

float AUOULadderActor::GetTopExitHeight() const
{
	const float MinimumClimbHeight = FMath::Min(GetBottomClimbHeight(), GetTopClimbHeight());
	const float MaximumClimbHeight = FMath::Max(GetBottomClimbHeight(), GetTopClimbHeight());
	return FMath::Clamp(
		GetTopClimbHeight() - TopExitInset,
		MinimumClimbHeight,
		MaximumClimbHeight);
}

void AUOULadderActor::RefreshLadderLayout()
{
	UpdateLadderDimensionsFromVolume();
	LadderSegmentHeight = FMath::Max(1.0f, LadderSegmentHeight);
	TopExitInset = FMath::Max(0.0f, TopExitInset);
	BottomExitOffset = FMath::Max(0.0f, BottomExitOffset);

	RebuildLadderSegments();
	ApplyDetectionVolumeSettings();
}

void AUOULadderActor::UpdateLadderDimensionsFromVolume()
{
	if (DetectionVolume == nullptr)
	{
		LadderHeight = FMath::Max(50.0f, LadderHeight);
		return;
	}

	const float ScaledHalfHeight =
		DetectionVolume->GetUnscaledBoxExtent().Z *
		FMath::Abs(DetectionVolume->GetRelativeScale3D().Z);
	LadderHeight = FMath::Max(50.0f, ScaledHalfHeight * 2.0f);
}

void AUOULadderActor::RebuildLadderSegments()
{
	if (LadderSegments == nullptr)
	{
		return;
	}

	LadderSegments->ClearInstances();
	const bool bUseModularVisual = LadderSegments->GetStaticMesh() != nullptr;
	LadderSegments->SetVisibility(bUseModularVisual, true);
	LadderSegments->SetHiddenInGame(!bUseModularVisual);

	if (VisualMesh != nullptr)
	{
		VisualMesh->SetVisibility(!bUseModularVisual, true);
		VisualMesh->SetHiddenInGame(bUseModularVisual);
	}

	if (!bUseModularVisual)
	{
		return;
	}

	const int32 SegmentCount = FMath::Max(
		1,
		FMath::CeilToInt(LadderHeight / LadderSegmentHeight));
	const float LadderHalfHeight = DetectionVolume != nullptr
		? DetectionVolume->GetUnscaledBoxExtent().Z *
			FMath::Abs(DetectionVolume->GetRelativeScale3D().Z)
		: LadderHeight * 0.5f;
	const float LadderBottom = DetectionVolume != nullptr
		? DetectionVolume->GetRelativeLocation().Z - LadderHalfHeight
		: 0.0f;
	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const FVector SegmentLocation(
			0.0f,
			0.0f,
			LadderBottom + SegmentIndex * LadderSegmentHeight);
		LadderSegments->AddInstance(FTransform(SegmentLocation), false);
	}
}

void AUOULadderActor::ApplyDetectionVolumeSettings()
{
	DetectionDepth = FMath::Max(20.0f, DetectionDepth);
	DetectionHalfWidth = FMath::Max(20.0f, DetectionHalfWidth);

	if (DetectionVolume == nullptr)
	{
		return;
	}

	const FVector DetectionExtent = DetectionVolume->GetUnscaledBoxExtent();
	DetectionVolume->SetBoxExtent(FVector(
		DetectionDepth,
		DetectionHalfWidth,
		DetectionExtent.Z));

}

void AUOULadderActor::DrawTraversalDebug() const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	auto DrawTransform = [World](
		const FVector& Location,
		const FRotator& Rotation,
		const TCHAR* Label,
		const FColor& Color)
	{
		DrawDebugSphere(World, Location, 10.0f, 12, Color, false, 0.0f, 0, 1.5f);
		DrawDebugCoordinateSystem(World, Location, Rotation, 30.0f, false, 0.0f, 0, 1.5f);
		DrawDebugString(
			World,
			Location + FVector(0.0f, 0.0f, 18.0f),
			Label,
			nullptr,
			Color,
			0.0f,
			true,
			1.0f);
	};

	DrawTransform(GetBottomStandingLocation(), GetBottomStandingRotation(), TEXT("Bottom Entry"), FColor::Green);
	DrawTransform(GetBottomClimbLocation(), GetBottomClimbRotation(), TEXT("Bottom Climb"), FColor::Cyan);
	DrawTransform(GetBottomExitLocation(), GetBottomExitRotation(), TEXT("Bottom Exit"), FColor::Yellow);
	DrawTransform(GetTopStandingLocation(), GetTopStandingRotation(), TEXT("Top Entry"), FColor::Blue);
	DrawTransform(GetTopClimbLocation(), GetTopClimbRotation(), TEXT("Top Climb"), FColor(255, 128, 0));
	DrawTransform(GetTopExitLocation(), GetTopExitRotation(), TEXT("Top Exit"), FColor::Magenta);

	DrawDebugLine(
		World,
		GetBottomClimbLocation(),
		GetTopClimbLocation(),
		FColor::Cyan,
		false,
		0.0f,
		0,
		2.0f);

	auto DrawVolume = [World](const UBoxComponent* Volume, const FColor& Color)
	{
		if (Volume == nullptr)
		{
			return;
		}

		DrawDebugBox(
			World,
			Volume->GetComponentLocation(),
			Volume->GetScaledBoxExtent(),
			Volume->GetComponentQuat(),
			Color,
			false,
			0.0f,
			0,
			1.5f);
	};

	DrawVolume(DetectionVolume, FColor::Red);
	DrawVolume(BottomEntryVolume, FColor::Green);
	DrawVolume(TopEntryVolume, FColor::Blue);
#endif
}
