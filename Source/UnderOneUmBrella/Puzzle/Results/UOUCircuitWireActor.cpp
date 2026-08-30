// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Results/UOUCircuitWireActor.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName GeneratedCircuitWireSegmentTag(TEXT("UOU.GeneratedCircuitWireSegment"));
}

AUOUCircuitWireActor::AUOUCircuitWireActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	CircuitPath = CreateDefaultSubobject<USplineComponent>(TEXT("CircuitPath"));
	CircuitPath->SetupAttachment(RootScene);
	CircuitPath->ClearSplinePoints(false);
	CircuitPath->AddSplinePoint(FVector(-150.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	CircuitPath->AddSplinePoint(FVector(150.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	CircuitPath->SetSplinePointType(0, ESplinePointType::Linear, false);
	CircuitPath->SetSplinePointType(1, ESplinePointType::Linear, false);
	CircuitPath->UpdateSpline();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		WireSegmentMesh = CylinderMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> UnpoweredMaterialFinder(TEXT("/Game/UOU/DebugMaterials/M_BlackDebug.M_BlackDebug"));
	if (UnpoweredMaterialFinder.Succeeded())
	{
		UnpoweredMaterial = UnpoweredMaterialFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PoweredMaterialFinder(TEXT("/Game/UOU/DebugMaterials/M_Yellow.M_Yellow"));
	if (PoweredMaterialFinder.Succeeded())
	{
		PoweredMaterial = PoweredMaterialFinder.Object;
	}
}

void AUOUCircuitWireActor::BeginPlay()
{
	Super::BeginPlay();

	RebuildWireVisualSegments();
	ApplyTargetPowerState(bStartPowered, false);
}

void AUOUCircuitWireActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearWireVisualSegments();

	Super::EndPlay(EndPlayReason);
}

void AUOUCircuitWireActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	bIsPowered = bStartPowered;
	VisualPowerProgress = bIsPowered ? 1.0f : 0.0f;
	RebuildWireVisualSegments();
}

void AUOUCircuitWireActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float TargetProgress = bIsPowered ? 1.0f : 0.0f;
	const float SafeTravelDuration = FMath::Max(0.01f, PowerTravelDuration);
	VisualPowerProgress = FMath::FInterpConstantTo(
		VisualPowerProgress,
		TargetProgress,
		DeltaSeconds,
		1.0f / SafeTravelDuration);
	ApplyWireMaterial();

	if (FMath::IsNearlyEqual(VisualPowerProgress, TargetProgress, KINDA_SMALL_NUMBER))
	{
		VisualPowerProgress = TargetProgress;
		ApplyWireMaterial();
		SetActorTickEnabled(false);
	}
}

void AUOUCircuitWireActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		ApplyTargetPowerState(true, true);
		break;
	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		ApplyTargetPowerState(false, true);
		break;
	case EOUUPuzzleResultAction::Toggle:
		ApplyTargetPowerState(!bIsPowered, true);
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

void AUOUCircuitWireActor::RebuildWireVisualSegments()
{
	ClearWireVisualSegments();

	if (RootScene == nullptr || CircuitPath == nullptr)
	{
		return;
	}

	UStaticMesh* SegmentMesh = ResolveWireSegmentMesh();
	const float SplineLength = CircuitPath->GetSplineLength();
	if (SegmentMesh == nullptr || SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	VisualSegmentLength = FMath::Max(1.0f, VisualSegmentLength);
	MaxVisualSegmentCount = FMath::Max(1, MaxVisualSegmentCount);
	VisualSegmentScale.X = FMath::Max(0.001f, VisualSegmentScale.X);
	VisualSegmentScale.Y = FMath::Max(0.001f, VisualSegmentScale.Y);

	const int32 SegmentCount = FMath::Clamp(
		FMath::CeilToInt(SplineLength / VisualSegmentLength),
		1,
		MaxVisualSegmentCount);
	const float RollRadians = FMath::DegreesToRadians(VisualSegmentRollDegrees);
	WireVisualSegments.Reserve(SegmentCount);

	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const float StartDistance = SplineLength * static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float EndDistance = SplineLength * static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
		const float SegmentDistance = FMath::Max(EndDistance - StartDistance, 1.0f);

		const FVector StartLocation = CircuitPath->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
		const FVector EndLocation = CircuitPath->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
		const FVector StartTangent = CircuitPath->GetDirectionAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local) * SegmentDistance;
		const FVector EndTangent = CircuitPath->GetDirectionAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local) * SegmentDistance;

		USplineMeshComponent* SegmentComponent = NewObject<USplineMeshComponent>(
			this,
			*FString::Printf(TEXT("CircuitWireSegment_%03d"), SegmentIndex + 1));
		if (SegmentComponent == nullptr)
		{
			continue;
		}

		SegmentComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		SegmentComponent->ComponentTags.AddUnique(GeneratedCircuitWireSegmentTag);
		SegmentComponent->SetupAttachment(RootScene);
		SegmentComponent->SetMobility(EComponentMobility::Movable);
		SegmentComponent->SetStaticMesh(SegmentMesh);
		SegmentComponent->SetForwardAxis(SplineMeshForwardAxis.GetValue(), false);
		SegmentComponent->SetStartScale(VisualSegmentScale, false);
		SegmentComponent->SetEndScale(VisualSegmentScale, false);
		SegmentComponent->SetStartRoll(RollRadians, false);
		SegmentComponent->SetEndRoll(RollRadians, false);
		SegmentComponent->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, false);
		SegmentComponent->SetCastShadow(bCastVisualShadow);
		SegmentComponent->SetGenerateOverlapEvents(false);
		SegmentComponent->SetCanEverAffectNavigation(false);
		SegmentComponent->SetReceivesDecals(false);

		if (bEnableVisualCollision)
		{
			SegmentComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			SegmentComponent->SetCollisionObjectType(ECC_WorldDynamic);
			SegmentComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
			SegmentComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
		else
		{
			SegmentComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		SegmentComponent->RegisterComponent();
		SegmentComponent->UpdateMesh();
		WireVisualSegments.Add(SegmentComponent);
	}

	ApplyWireMaterial();
}

void AUOUCircuitWireActor::RefreshCircuitState()
{
	ApplyTargetPowerState(bIsPowered, false);
}

#if WITH_EDITOR
void AUOUCircuitWireActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RebuildWireVisualSegments();
	RefreshCircuitState();
}

void AUOUCircuitWireActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	RebuildWireVisualSegments();
	RefreshCircuitState();
}
#endif

void AUOUCircuitWireActor::ClearWireVisualSegments()
{
	TArray<USplineMeshComponent*> ExistingSplineMeshComponents;
	GetComponents<USplineMeshComponent>(ExistingSplineMeshComponents);
	for (USplineMeshComponent* ExistingComponent : ExistingSplineMeshComponents)
	{
		if (ExistingComponent != nullptr && ExistingComponent->ComponentHasTag(GeneratedCircuitWireSegmentTag))
		{
			ExistingComponent->DestroyComponent();
		}
	}

	WireVisualSegments.Reset();
}

void AUOUCircuitWireActor::ApplyWireMaterial()
{
	const int32 SegmentCount = WireVisualSegments.Num();
	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		USplineMeshComponent* SegmentComponent = WireVisualSegments[SegmentIndex];
		if (SegmentComponent != nullptr)
		{
			const float SegmentEndProgress = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			UMaterialInterface* DesiredMaterial = SegmentEndProgress <= VisualPowerProgress + KINDA_SMALL_NUMBER
				? PoweredMaterial.Get()
				: UnpoweredMaterial.Get();
			if (DesiredMaterial != nullptr)
			{
				SegmentComponent->SetMaterial(0, DesiredMaterial);
			}
		}
	}
}

UStaticMesh* AUOUCircuitWireActor::ResolveWireSegmentMesh() const
{
	return WireSegmentMesh.Get();
}

void AUOUCircuitWireActor::ApplyTargetPowerState(bool bNewPowered, bool bAllowTransition)
{
	bIsPowered = bNewPowered;

	if (!bAllowTransition || TransitionMode == EUOUCircuitWireTransitionMode::Instant)
	{
		VisualPowerProgress = bIsPowered ? 1.0f : 0.0f;
		SetActorTickEnabled(false);
		ApplyWireMaterial();
		return;
	}

	const float TargetProgress = bIsPowered ? 1.0f : 0.0f;
	if (FMath::IsNearlyEqual(VisualPowerProgress, TargetProgress, KINDA_SMALL_NUMBER))
	{
		VisualPowerProgress = TargetProgress;
		SetActorTickEnabled(false);
		ApplyWireMaterial();
		return;
	}

	SetActorTickEnabled(true);
}
