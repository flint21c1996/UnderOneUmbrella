// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/HeatWire/UOUHeatWireActor.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Puzzle/HeatWire/UOUHeatWireComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Light/UOULightExposureReceiverComponent.h"

namespace
{
	constexpr TCHAR DefaultHeatWireSegmentMeshPath[] = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const FName GeneratedHeatWireSegmentTag(TEXT("UOU.GeneratedHeatWireSegment"));

	bool ProgressRangesOverlap(float AStart, float AEnd, float BStart, float BEnd)
	{
		return AStart <= BEnd + KINDA_SMALL_NUMBER && AEnd + KINDA_SMALL_NUMBER >= BStart;
	}

	void SetScalarParameterIfNamed(UMaterialInstanceDynamic* MaterialInstance, FName ParameterName, float Value)
	{
		if (MaterialInstance != nullptr && !ParameterName.IsNone())
		{
			MaterialInstance->SetScalarParameterValue(ParameterName, Value);
		}
	}
}

AUOUHeatWireActor::AUOUHeatWireActor()
{
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	HeatWirePath = CreateDefaultSubobject<USplineComponent>(TEXT("HeatWirePath"));
	HeatWirePath->SetupAttachment(RootScene);
	HeatWirePath->ClearSplinePoints(false);
	HeatWirePath->AddSplinePoint(FVector(-150.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	HeatWirePath->AddSplinePoint(FVector(150.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	HeatWirePath->SetSplinePointType(0, ESplinePointType::Linear, false);
	HeatWirePath->SetSplinePointType(1, ESplinePointType::Linear, false);
	HeatWirePath->UpdateSpline();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(DefaultHeatWireSegmentMeshPath);
	if (CylinderMeshFinder.Succeeded())
	{
		HeatWireSegmentMesh = CylinderMeshFinder.Object;
	}

	LightReceiverComponent = CreateDefaultSubobject<UUOULightExposureReceiverComponent>(TEXT("LightReceiverComponent"));
	LightReceiverComponent->bAutoFindReceiverTransform = true;

	HeatWireComponent = CreateDefaultSubobject<UUOUHeatWireComponent>(TEXT("HeatWireComponent"));
	HeatWireComponent->bAutoFindLightReceiver = true;
	HeatWireComponent->HeatWirePathComponent = HeatWirePath;
}

void AUOUHeatWireActor::BeginPlay()
{
	Super::BeginPlay();

	RebuildHeatWireVisualSegments();
	BindHeatWireVisualEvents();
	RefreshHeatWireVisualState();

	if (bRegisterDebugProvider)
	{
		if (UWorld* World = GetWorld())
		{
			if (UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>())
			{
				DebugSubsystem->RegisterDebugProvider(this);
			}
		}
	}
}

void AUOUHeatWireActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindHeatWireVisualEvents();

	if (UWorld* World = GetWorld())
	{
		if (UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>())
		{
			DebugSubsystem->UnregisterDebugProvider(this);
		}
	}

	ClearHeatWireVisualSegments();

	Super::EndPlay(EndPlayReason);
}

void AUOUHeatWireActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (HeatWireComponent != nullptr)
	{
		HeatWireComponent->HeatWirePathComponent = HeatWirePath;
	}

	RebuildHeatWireVisualSegments();
	RefreshHeatWireVisualState();
}

#if WITH_EDITOR
void AUOUHeatWireActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RebuildHeatWireVisualSegments();
	RefreshHeatWireVisualState();
}

void AUOUHeatWireActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	RebuildHeatWireVisualSegments();
	RefreshHeatWireVisualState();
}
#endif

void AUOUHeatWireActor::RebuildHeatWireVisualSegments()
{
	ValidateVisualSettings();
	ClearHeatWireVisualSegments();

	if (RootScene == nullptr || HeatWirePath == nullptr)
	{
		return;
	}

	UStaticMesh* SegmentMesh = ResolveHeatWireSegmentMesh();
	if (SegmentMesh == nullptr)
	{
		return;
	}

	const float SplineLength = HeatWirePath->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const int32 SegmentCount = FMath::Clamp(
		FMath::CeilToInt(SplineLength / VisualSegmentLength),
		1,
		MaxVisualSegmentCount);

	HeatWireVisualSegments.Reserve(SegmentCount);
	HeatWireVisualSegmentMaterialInstances.Reserve(SegmentCount);
	HeatWireVisualSegmentMaterialSources.Reserve(SegmentCount);
	HeatWireVisualSegmentStartProgresses.Reserve(SegmentCount);
	HeatWireVisualSegmentEndProgresses.Reserve(SegmentCount);

	const float RollRadians = FMath::DegreesToRadians(VisualSegmentRollDegrees);
	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const float StartDistance = SplineLength * static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float EndDistance = SplineLength * static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
		const float SegmentDistance = FMath::Max(EndDistance - StartDistance, 1.0f);

		const FVector StartLocation = HeatWirePath->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
		const FVector EndLocation = HeatWirePath->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
		const FVector StartTangent = HeatWirePath->GetDirectionAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local) * SegmentDistance;
		const FVector EndTangent = HeatWirePath->GetDirectionAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local) * SegmentDistance;

		USplineMeshComponent* SegmentComponent = NewObject<USplineMeshComponent>(
			this,
			*FString::Printf(TEXT("HeatWireSegment_%02d"), SegmentIndex + 1));
		if (SegmentComponent == nullptr)
		{
			continue;
		}

		SegmentComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		SegmentComponent->ComponentTags.AddUnique(GeneratedHeatWireSegmentTag);
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

		if (DryHeatWireMaterial != nullptr)
		{
			SegmentComponent->SetMaterial(0, DryHeatWireMaterial);
		}

		SegmentComponent->RegisterComponent();
		SegmentComponent->UpdateMesh();

		HeatWireVisualSegments.Add(SegmentComponent);
		HeatWireVisualSegmentMaterialInstances.Add(nullptr);
		HeatWireVisualSegmentMaterialSources.Add(nullptr);
		HeatWireVisualSegmentStartProgresses.Add(StartDistance / SplineLength);
		HeatWireVisualSegmentEndProgresses.Add(EndDistance / SplineLength);
	}
}

void AUOUHeatWireActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	if (HeatWireComponent != nullptr)
	{
		HeatWireComponent->ApplyPuzzleResult_Implementation(Action);
	}
}

void AUOUHeatWireActor::Interact_Implementation(AActor* Interactor)
{
	if (HeatWireComponent != nullptr)
	{
		HeatWireComponent->Interact_Implementation(Interactor);
	}
}

EUOUDebugCategory AUOUHeatWireActor::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

bool AUOUHeatWireActor::IsDebugProviderEnabled_Implementation() const
{
	return bRegisterDebugProvider;
}

FText AUOUHeatWireActor::GetDebugDisplayName_Implementation() const
{
#if WITH_EDITOR
	return FText::FromString(GetActorLabel());
#else
	return FText::FromString(GetName());
#endif
}

FText AUOUHeatWireActor::GetDebugSummaryText_Implementation() const
{
	if (HeatWireComponent == nullptr)
	{
		return FText::FromString(TEXT("HeatWire: Missing Component"));
	}

	const TArray<FString> DebugLines = IUOUPuzzleDebugInfoProvider::Execute_GetPuzzleDebugInfo(HeatWireComponent);
	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

FVector AUOUHeatWireActor::GetDebugWorldLocation_Implementation() const
{
	return GetActorLocation() + DebugWorldLocationOffset;
}

void AUOUHeatWireActor::GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const
{
	OutConnections.Reset();
}

void AUOUHeatWireActor::ValidateVisualSettings()
{
	VisualSegmentLength = FMath::Max(1.0f, VisualSegmentLength);
	MaxVisualSegmentCount = FMath::Max(1, MaxVisualSegmentCount);
	VisualSegmentScale.X = FMath::Max(0.001f, VisualSegmentScale.X);
	VisualSegmentScale.Y = FMath::Max(0.001f, VisualSegmentScale.Y);
}

void AUOUHeatWireActor::ClearHeatWireVisualSegments()
{
	TArray<USplineMeshComponent*> ExistingSplineMeshComponents;
	GetComponents<USplineMeshComponent>(ExistingSplineMeshComponents);

	for (USplineMeshComponent* ExistingComponent : ExistingSplineMeshComponents)
	{
		if (ExistingComponent != nullptr && ExistingComponent->ComponentTags.Contains(GeneratedHeatWireSegmentTag))
		{
			ExistingComponent->DestroyComponent();
		}
	}

	HeatWireVisualSegments.Reset();
	HeatWireVisualSegmentMaterialInstances.Reset();
	HeatWireVisualSegmentMaterialSources.Reset();
	HeatWireVisualSegmentStartProgresses.Reset();
	HeatWireVisualSegmentEndProgresses.Reset();
}

UStaticMesh* AUOUHeatWireActor::ResolveHeatWireSegmentMesh()
{
	if (HeatWireSegmentMesh == nullptr && bUseDefaultCylinderSegmentMesh)
	{
		HeatWireSegmentMesh = LoadObject<UStaticMesh>(nullptr, DefaultHeatWireSegmentMeshPath);
	}

	return HeatWireSegmentMesh;
}

void AUOUHeatWireActor::RefreshHeatWireVisualState()
{
	if (HeatWireComponent == nullptr)
	{
		return;
	}

	const float BurnProgress = FMath::Clamp(HeatWireComponent->BurnProgress, 0.0f, 1.0f);
	for (int32 SegmentIndex = 0; SegmentIndex < HeatWireVisualSegments.Num(); ++SegmentIndex)
	{
		if (!HeatWireVisualSegmentStartProgresses.IsValidIndex(SegmentIndex) ||
			!HeatWireVisualSegmentEndProgresses.IsValidIndex(SegmentIndex))
		{
			continue;
		}

		const float SegmentStartProgress = HeatWireVisualSegmentStartProgresses[SegmentIndex];
		const float SegmentEndProgress = HeatWireVisualSegmentEndProgresses[SegmentIndex];
		const float SegmentLength = FMath::Max(SegmentEndProgress - SegmentStartProgress, KINDA_SMALL_NUMBER);
		const float SegmentBurnRatio = FMath::Clamp((BurnProgress - SegmentStartProgress) / SegmentLength, 0.0f, 1.0f);

		bool bBlocked = false;
		const float SegmentWetnessAlpha = GetSegmentWetnessAlpha(SegmentStartProgress, SegmentEndProgress, bBlocked);
		const bool bActiveHeat = HeatWireComponent->IsBurning()
			&& BurnProgress < 1.0f
			&& BurnProgress + KINDA_SMALL_NUMBER >= SegmentStartProgress
			&& BurnProgress <= SegmentEndProgress + KINDA_SMALL_NUMBER;
		ApplySegmentMaterialState(SegmentIndex, SegmentBurnRatio, SegmentWetnessAlpha, bBlocked, bActiveHeat);
	}
}

float AUOUHeatWireActor::GetSegmentWetnessAlpha(float SegmentStartProgress, float SegmentEndProgress, bool& bOutBlocked) const
{
	bOutBlocked = false;

	if (HeatWireComponent == nullptr)
	{
		return 0.0f;
	}

	float WetnessAlpha = 0.0f;
	for (int32 SectionIndex = 0; SectionIndex < HeatWireComponent->WetSections.Num(); ++SectionIndex)
	{
		const FUOUHeatWireWetSection& Section = HeatWireComponent->WetSections[SectionIndex];
		if (!ProgressRangesOverlap(SegmentStartProgress, SegmentEndProgress, Section.StartProgress, Section.EndProgress))
		{
			continue;
		}

		const float SectionWetnessAlpha = Section.MaxWetness > KINDA_SMALL_NUMBER
			? Section.Wetness / Section.MaxWetness
			: 0.0f;
		WetnessAlpha = FMath::Max(WetnessAlpha, SectionWetnessAlpha);

		if (SectionIndex == HeatWireComponent->BlockedSectionIndex && HeatWireComponent->IsBlockedByWetness())
		{
			bOutBlocked = true;
		}
	}

	return FMath::Clamp(WetnessAlpha, 0.0f, 1.0f);
}

UMaterialInterface* AUOUHeatWireActor::ResolveSegmentMaterial(
	float SegmentBurnRatio,
	float SegmentWetnessAlpha,
	bool bActiveHeat) const
{
	if (SegmentWetnessAlpha > KINDA_SMALL_NUMBER && WetHeatWireMaterial != nullptr)
	{
		return WetHeatWireMaterial;
	}

	if (bActiveHeat && ActiveHeatMaterial != nullptr)
	{
		return ActiveHeatMaterial;
	}

	if (SegmentBurnRatio >= 1.0f - KINDA_SMALL_NUMBER && BurnedHeatWireMaterial != nullptr)
	{
		return BurnedHeatWireMaterial;
	}

	return DryHeatWireMaterial;
}

void AUOUHeatWireActor::ApplySegmentMaterialState(
	int32 SegmentIndex,
	float SegmentBurnRatio,
	float SegmentWetnessAlpha,
	bool bBlocked,
	bool bActiveHeat)
{
	USplineMeshComponent* SegmentComponent = HeatWireVisualSegments.IsValidIndex(SegmentIndex)
		? HeatWireVisualSegments[SegmentIndex].Get()
		: nullptr;
	if (SegmentComponent == nullptr)
	{
		return;
	}

	if (!HeatWireVisualSegmentMaterialInstances.IsValidIndex(SegmentIndex) ||
		!HeatWireVisualSegmentMaterialSources.IsValidIndex(SegmentIndex))
	{
		return;
	}

	UMaterialInterface* DesiredMaterial = ResolveSegmentMaterial(SegmentBurnRatio, SegmentWetnessAlpha, bActiveHeat);
	UMaterialInstanceDynamic* DynamicMaterial = HeatWireVisualSegmentMaterialInstances[SegmentIndex];
	if (DynamicMaterial == nullptr || HeatWireVisualSegmentMaterialSources[SegmentIndex] != DesiredMaterial)
	{
		UMaterialInterface* SourceMaterial = DesiredMaterial;
		if (SourceMaterial == nullptr)
		{
			if (UStaticMesh* StaticMesh = SegmentComponent->GetStaticMesh())
			{
				SourceMaterial = StaticMesh->GetMaterial(0);
			}
		}

		if (SourceMaterial != nullptr)
		{
			SegmentComponent->SetMaterial(0, SourceMaterial);
			DynamicMaterial = SegmentComponent->CreateDynamicMaterialInstance(0, SourceMaterial);
		}
		else
		{
			DynamicMaterial = SegmentComponent->CreateDynamicMaterialInstance(0);
		}

		HeatWireVisualSegmentMaterialInstances[SegmentIndex] = DynamicMaterial;
		HeatWireVisualSegmentMaterialSources[SegmentIndex] = DesiredMaterial;
	}

	SetScalarParameterIfNamed(DynamicMaterial, BurnProgressParameterName, FMath::Clamp(HeatWireComponent->BurnProgress, 0.0f, 1.0f));
	SetScalarParameterIfNamed(DynamicMaterial, SegmentBurnRatioParameterName, SegmentBurnRatio);
	SetScalarParameterIfNamed(DynamicMaterial, WetnessParameterName, SegmentWetnessAlpha);
	SetScalarParameterIfNamed(DynamicMaterial, BlockedParameterName, bBlocked ? 1.0f : 0.0f);
	SetScalarParameterIfNamed(DynamicMaterial, ActiveHeatParameterName, bActiveHeat ? 1.0f : 0.0f);

	if (HeatWireVisualSegmentStartProgresses.IsValidIndex(SegmentIndex))
	{
		SetScalarParameterIfNamed(DynamicMaterial, SegmentStartProgressParameterName, HeatWireVisualSegmentStartProgresses[SegmentIndex]);
	}

	if (HeatWireVisualSegmentEndProgresses.IsValidIndex(SegmentIndex))
	{
		SetScalarParameterIfNamed(DynamicMaterial, SegmentEndProgressParameterName, HeatWireVisualSegmentEndProgresses[SegmentIndex]);
	}
}

void AUOUHeatWireActor::BindHeatWireVisualEvents()
{
	if (HeatWireComponent == nullptr)
	{
		return;
	}

	HeatWireComponent->OnHeatWireProgressChanged.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireProgressChanged);
	HeatWireComponent->OnWetSectionChanged.RemoveDynamic(this, &AUOUHeatWireActor::HandleWetSectionChanged);
	HeatWireComponent->OnBlockedSectionChanged.RemoveDynamic(this, &AUOUHeatWireActor::HandleBlockedSectionChanged);
	HeatWireComponent->OnHeatWireReset.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged);
	HeatWireComponent->OnHeatWireBurnedOut.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged);
	HeatWireComponent->OnHeatWireExtinguished.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged);
	HeatWireComponent->OnHeatWireIgnited.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireIgnited);

	HeatWireComponent->OnHeatWireProgressChanged.AddDynamic(this, &AUOUHeatWireActor::HandleHeatWireProgressChanged);
	HeatWireComponent->OnWetSectionChanged.AddDynamic(this, &AUOUHeatWireActor::HandleWetSectionChanged);
	HeatWireComponent->OnBlockedSectionChanged.AddDynamic(this, &AUOUHeatWireActor::HandleBlockedSectionChanged);
	HeatWireComponent->OnHeatWireReset.AddDynamic(this, &AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged);
	HeatWireComponent->OnHeatWireBurnedOut.AddDynamic(this, &AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged);
	HeatWireComponent->OnHeatWireExtinguished.AddDynamic(this, &AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged);
	HeatWireComponent->OnHeatWireIgnited.AddDynamic(this, &AUOUHeatWireActor::HandleHeatWireIgnited);

	bHeatWireVisualEventsBound = true;
}

void AUOUHeatWireActor::UnbindHeatWireVisualEvents()
{
	if (HeatWireComponent == nullptr || !bHeatWireVisualEventsBound)
	{
		return;
	}

	HeatWireComponent->OnHeatWireProgressChanged.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireProgressChanged);
	HeatWireComponent->OnWetSectionChanged.RemoveDynamic(this, &AUOUHeatWireActor::HandleWetSectionChanged);
	HeatWireComponent->OnBlockedSectionChanged.RemoveDynamic(this, &AUOUHeatWireActor::HandleBlockedSectionChanged);
	HeatWireComponent->OnHeatWireReset.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged);
	HeatWireComponent->OnHeatWireBurnedOut.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged);
	HeatWireComponent->OnHeatWireExtinguished.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged);
	HeatWireComponent->OnHeatWireIgnited.RemoveDynamic(this, &AUOUHeatWireActor::HandleHeatWireIgnited);

	bHeatWireVisualEventsBound = false;
}

void AUOUHeatWireActor::HandleHeatWireProgressChanged(float NewProgress, float RemainingTime)
{
	RefreshHeatWireVisualState();
}

void AUOUHeatWireActor::HandleWetSectionChanged(int32 SectionIndex, float NewWetness)
{
	RefreshHeatWireVisualState();
}

void AUOUHeatWireActor::HandleBlockedSectionChanged(int32 BlockedSectionIndex)
{
	RefreshHeatWireVisualState();
}

void AUOUHeatWireActor::HandleHeatWireSimpleVisualChanged()
{
	RefreshHeatWireVisualState();
}

void AUOUHeatWireActor::HandleHeatWireIgnited(AActor* Igniter)
{
	RefreshHeatWireVisualState();
}
