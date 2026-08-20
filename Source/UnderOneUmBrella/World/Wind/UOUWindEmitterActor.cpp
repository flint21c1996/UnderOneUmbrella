// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Wind/UOUWindEmitterActor.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Debug/UOUDevelopmentToolsBuild.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraParameterStore.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "World/Wind/UOUWindInteractionSurfaceComponent.h"
#include "World/Wind/UOUWindReceivableInterface.h"

namespace UOUWindEmitterActorPrivate
{
	const FName GeneratedWindPathPreviewTag(
		TEXT("UOU.GeneratedWindPathPreview"));
	const FName GeneratedWindVFXTag(
		TEXT("UOU.GeneratedWindVFX"));

	FName NormalizeNiagaraParameterName(FName ParameterName)
	{
		FString ParameterNameString = ParameterName.ToString();
		if (ParameterNameString.StartsWith(TEXT("User.")))
		{
			ParameterNameString.RightChopInline(5);
			return FName(*ParameterNameString);
		}

		return ParameterName;
	}

	bool HasFloatParameter(const UNiagaraComponent* Effect, FName ParameterName)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return false;
		}

		TArray<FName, TInlineAllocator<2>> NamesToCheck;
		NamesToCheck.AddUnique(ParameterName);
		NamesToCheck.AddUnique(NormalizeNiagaraParameterName(ParameterName));

		const UNiagaraSystem* System = Effect->GetAsset();
		for (const FName NameToCheck : NamesToCheck)
		{
			const FNiagaraVariable QueryParameter(
				FNiagaraTypeDefinition::GetFloatDef(),
				NameToCheck);
			if ((System != nullptr
					&& System->GetExposedParameters().FindParameterVariable(QueryParameter, false) != nullptr)
				|| Effect->GetOverrideParameters().FindParameterVariable(QueryParameter, false) != nullptr)
			{
				return true;
			}
		}

		return false;
	}

	bool SetFloatParameterIfPresent(
		UNiagaraComponent* Effect,
		FName ParameterName,
		float Value)
	{
		if (!HasFloatParameter(Effect, ParameterName))
		{
			return false;
		}

		Effect->SetVariableFloat(NormalizeNiagaraParameterName(ParameterName), Value);
		return true;
	}

	bool SetColorParameterIfPresent(
		UNiagaraComponent* Effect,
		FName ParameterName,
		const FLinearColor& Value)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return false;
		}

		TArray<FName, TInlineAllocator<2>> NamesToCheck;
		NamesToCheck.AddUnique(ParameterName);
		NamesToCheck.AddUnique(NormalizeNiagaraParameterName(ParameterName));

		const UNiagaraSystem* System = Effect->GetAsset();
		for (const FName NameToCheck : NamesToCheck)
		{
			const FNiagaraVariable QueryParameter(
				FNiagaraTypeDefinition::GetColorDef(),
				NameToCheck);
			if ((System != nullptr
					&& System->GetExposedParameters().FindParameterVariable(QueryParameter, false) != nullptr)
				|| Effect->GetOverrideParameters().FindParameterVariable(QueryParameter, false) != nullptr)
			{
				Effect->SetVariableLinearColor(
					NormalizeNiagaraParameterName(ParameterName),
					Value);
				return true;
			}
		}

		return false;
	}
}

AUOUWindEmitterActor::AUOUWindEmitterActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	EmitterVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmitterVisual"));
	EmitterVisual->SetupAttachment(RootScene);
	EmitterVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WindOrigin = CreateDefaultSubobject<UArrowComponent>(TEXT("WindOrigin"));
	WindOrigin->SetupAttachment(RootScene);
	WindOrigin->ArrowColor = FColor::Cyan;
	WindOrigin->ArrowSize = 2.0f;

	WindRangePreview = CreateDefaultSubobject<UBoxComponent>(TEXT("WindRangePreview"));
	WindRangePreview->SetupAttachment(WindOrigin);
	WindRangePreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WindRangePreview->SetGenerateOverlapEvents(false);
	WindRangePreview->SetCanEverAffectNavigation(false);
	WindRangePreview->SetHiddenInGame(true);
	WindRangePreview->ShapeColor = FColor::Cyan;
	WindRangePreview->SetLineThickness(2.0f);
	WindRangePreview->bIsEditorOnly = true;

	ReceiverObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ReceiverObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
	ReceiverObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
}

void AUOUWindEmitterActor::BeginPlay()
{
	Super::BeginPlay();
	ValidateSettings();
	InitializePulseCycleState();
	RefreshWindVFX();
	SetWindVFXActive(IsWindBlowing());
	SetActorTickEnabled(bWindEnabled);
	RebuildWindPath();
}

void AUOUWindEmitterActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearGeneratedWindVFX();
	Super::EndPlay(EndPlayReason);
}

void AUOUWindEmitterActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld())
	{
		// Blueprint SCS 컴포넌트는 native OnConstruction 이후에 준비될 수 있으므로
		// 에디터 프리뷰에서는 현재 Niagara 에셋과 범위를 계속 동기화합니다.
		RefreshWindVFX();
		return;
	}

	if (!bWindEnabled || DeltaSeconds <= 0.0f)
	{
		return;
	}

	UpdatePulseCycle(DeltaSeconds);
	if (!IsWindBlowing())
	{
		LastAffectedReceiverCount = 0;
		return;
	}

	RebuildWindPath();
	ApplyWindToReceivers(DeltaSeconds);
	DrawWindDebug();
}

void AUOUWindEmitterActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ValidateSettings();
	UpdateWindRangePreview();
	RefreshWindVFX();
}

void AUOUWindEmitterActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	RefreshWindVFX();
}

#if WITH_EDITOR
bool AUOUWindEmitterActor::ShouldTickIfViewportsOnly() const
{
	return true;
}
#endif

void AUOUWindEmitterActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		SetWindEnabled(true);
		break;
	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		SetWindEnabled(false);
		break;
	case EOUUPuzzleResultAction::Toggle:
		SetWindEnabled(!bWindEnabled);
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

FText AUOUWindEmitterActor::GetDebugSummaryText_Implementation() const
{
	const TArray<FString> DebugLines = {
		FString::Printf(TEXT("Wind Enabled: %s"), bWindEnabled ? TEXT("true") : TEXT("false")),
		FString::Printf(
			TEXT("Blowing: %s / Pulse: %s / Remaining: %.2fs"),
			IsWindBlowing() ? TEXT("true") : TEXT("false"),
			bUsePulseCycle ? TEXT("true") : TEXT("false"),
			PulseRuntimeState.TimeRemaining),
		FString::Printf(TEXT("Segments: %d / Reflections: %d"), WindPathSegments.Num(), FMath::Max(0, WindPathSegments.Num() - 1)),
		FString::Printf(TEXT("Affected Receivers: %d"), LastAffectedReceiverCount),
		FString::Printf(
			TEXT("Range: %.0f / Radius: %.0f / Accel: %.0f / Max Accel: %.0f / Max Speed: %.0f"),
			MaxWindDistance,
			WindRadius,
			WindAcceleration,
			MaximumWindAcceleration,
			MaximumWindSpeed),
		FString::Printf(
			TEXT("Entry Speed: %.0f-%.0f / Initial Boost: %.0f / Fall Conversion: %.2f"),
			MinimumWindEntrySpeed,
			MaximumWindEntrySpeed,
			InitialWindVelocityBoost,
			FallingMomentumConversion)
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

EUOUDebugCategory AUOUWindEmitterActor::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

void AUOUWindEmitterActor::SetWindEnabled(bool bNewEnabled)
{
	if (bWindEnabled == bNewEnabled)
	{
		return;
	}

	const bool bWasBlowing = IsWindBlowing();
	bWindEnabled = bNewEnabled;
	SetActorTickEnabled(bWindEnabled);

	if (bWindEnabled)
	{
		InitializePulseCycleState();
	}
	RefreshWindPathForCurrentState();

	HandleWindPhaseChanged(bWasBlowing);
}

bool AUOUWindEmitterActor::IsWindBlowing() const
{
	return bWindEnabled && (!bUsePulseCycle || PulseRuntimeState.bIsBlowing);
}

void AUOUWindEmitterActor::SetPulseCycleEnabled(bool bNewPulseCycleEnabled)
{
	if (bUsePulseCycle == bNewPulseCycleEnabled)
	{
		return;
	}

	const bool bWasBlowing = IsWindBlowing();
	bUsePulseCycle = bNewPulseCycleEnabled;
	InitializePulseCycleState();
	RefreshWindPathForCurrentState();

	HandleWindPhaseChanged(bWasBlowing);
}

void AUOUWindEmitterActor::ResetPulseCycle()
{
	const bool bWasBlowing = IsWindBlowing();
	InitializePulseCycleState();

	if (HasActorBegunPlay())
	{
		RefreshWindPathForCurrentState();
	}

	HandleWindPhaseChanged(bWasBlowing);
}

void AUOUWindEmitterActor::InitializePulseCycleState()
{
	if (bUsePulseCycle)
	{
		PulseRuntimeState.Reset(
			bStartCycleWithWind,
			WindOnDuration,
			WindOffDuration);
	}
	else
	{
		PulseRuntimeState.bIsBlowing = true;
		PulseRuntimeState.TimeRemaining = 0.0f;
	}
}

void AUOUWindEmitterActor::RebuildWindPath()
{
	RebuildWindPathInternal(false);
}

void AUOUWindEmitterActor::RebuildWindPathPreview()
{
	ValidateSettings();

#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (World != nullptr && !World->IsGameWorld())
	{
		RebuildWindPathInternal(true);
		RebuildEditorWindPathPreviewComponents();
		return;
	}
#endif

	RebuildWindPath();
}

void AUOUWindEmitterActor::RebuildWindPathInternal(
	bool bIgnoreRuntimeWindState)
{
	WindPathSegments.Reset();

	UWorld* World = GetWorld();
	if ((!bIgnoreRuntimeWindState && !IsWindBlowing())
		|| World == nullptr
		|| WindOrigin == nullptr
		|| MaxWindDistance <= 0.0f
		|| WindAcceleration <= 0.0f
		|| MaximumWindAcceleration <= 0.0f
		|| MaximumWindSpeed <= 0.0f)
	{
		ClearGeneratedWindVFX();
		OnWindPathChanged.Broadcast();
		return;
	}

	FVector SegmentStart = WindOrigin->GetComponentLocation();
	FVector SegmentDirection = WindOrigin->GetForwardVector().GetSafeNormal();
	float RemainingDistance = MaxWindDistance;
	float CurrentStrength = WindAcceleration;

	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	UPrimitiveComponent* PreviousReflectionSurface = nullptr;

	for (int32 ReflectionIndex = 0;
		ReflectionIndex <= MaxReflections
			&& RemainingDistance > KINDA_SMALL_NUMBER
			&& CurrentStrength >= MinimumReflectedStrength;
		++ReflectionIndex)
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOUWindPathTrace), false, this);
		QueryParams.AddIgnoredActor(this);
		if (PlayerCharacter != nullptr)
		{
			QueryParams.AddIgnoredActor(PlayerCharacter);
		}
		if (PreviousReflectionSurface != nullptr)
		{
			QueryParams.AddIgnoredComponent(PreviousReflectionSurface);
		}

		const FVector TraceEnd = SegmentStart + SegmentDirection * RemainingDistance;
		FHitResult Hit;
		const bool bBlockingHit = World->LineTraceSingleByChannel(
			Hit,
			SegmentStart,
			TraceEnd,
			WindTraceChannel,
			QueryParams);

		const FVector SegmentEnd = bBlockingHit ? Hit.ImpactPoint : TraceEnd;
		FUOUWindPathSegment& Segment = WindPathSegments.AddDefaulted_GetRef();
		Segment.Start = SegmentStart;
		Segment.End = SegmentEnd;
		Segment.Direction = SegmentDirection;
		Segment.Strength = CurrentStrength;
		Segment.ReflectionIndex = ReflectionIndex;
		Segment.HitActor = bBlockingHit ? Hit.GetActor() : nullptr;

		if (!bBlockingHit || ReflectionIndex >= MaxReflections)
		{
			break;
		}

		UUOUWindInteractionSurfaceComponent* InteractionSurface =
			Cast<UUOUWindInteractionSurfaceComponent>(Hit.GetComponent());
		if (InteractionSurface == nullptr || !InteractionSurface->CanReflectWind())
		{
			break;
		}

		const FVector OutgoingDirection =
			InteractionSurface->GetOutgoingDirection(SegmentDirection, Hit.ImpactNormal);
		if (OutgoingDirection.IsNearlyZero())
		{
			break;
		}

		const float TravelledDistance = FVector::Distance(SegmentStart, SegmentEnd);
		RemainingDistance = FMath::Max(0.0f, RemainingDistance - TravelledDistance);
		CurrentStrength *= FMath::Clamp(InteractionSurface->StrengthRetention, 0.0f, 1.0f);
		SegmentDirection = OutgoingDirection;
		SegmentStart =
			Hit.ImpactPoint
			+ SegmentDirection * FMath::Max(0.1f, InteractionSurface->ReflectionStartPadding);
		PreviousReflectionSurface = InteractionSurface;
	}

	RefreshWindVFX();
	OnWindPathChanged.Broadcast();
}

#if WITH_EDITOR
void AUOUWindEmitterActor::RebuildEditorWindPathPreviewComponents()
{
	ClearEditorWindPathPreviewComponents();

	if (WindRangePreview != nullptr)
	{
		WindRangePreview->SetVisibility(false);
	}

	if (!bShowWindRangePreview || RootScene == nullptr)
	{
		return;
	}

	EditorWindPathPreviewComponents.Reserve(
		WindPathSegments.Num() * 2);

	for (const FUOUWindPathSegment& Segment : WindPathSegments)
	{
		const float SegmentLength = Segment.GetLength();
		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FColor SegmentColor =
			Segment.ReflectionIndex == 0
				? FColor::Cyan
				: FColor::MakeRedToGreenColorFromScalar(
					FMath::Clamp(
						Segment.Strength
							/ FMath::Max(
								WindAcceleration,
								KINDA_SMALL_NUMBER),
						0.0f,
						1.0f));
		const FQuat SegmentRotation =
			FRotationMatrix::MakeFromX(
				Segment.Direction.GetSafeNormal())
				.ToQuat();

		UBoxComponent* RangePreview =
			NewObject<UBoxComponent>(
				this,
				*FString::Printf(
					TEXT("WindPathRangePreview_%02d"),
					Segment.ReflectionIndex),
				RF_Transient | RF_TextExportTransient);
		if (RangePreview != nullptr)
		{
			RangePreview->CreationMethod =
				EComponentCreationMethod::UserConstructionScript;
			RangePreview->ComponentTags.AddUnique(
				UOUWindEmitterActorPrivate::
					GeneratedWindPathPreviewTag);
			RangePreview->SetupAttachment(RootScene);
			RangePreview->SetMobility(
				EComponentMobility::Movable);
			RangePreview->SetCollisionEnabled(
				ECollisionEnabled::NoCollision);
			RangePreview->SetGenerateOverlapEvents(false);
			RangePreview->SetCanEverAffectNavigation(false);
			RangePreview->SetHiddenInGame(true);
			RangePreview->SetIsVisualizationComponent(true);
			RangePreview->ShapeColor = SegmentColor;
			RangePreview->SetLineThickness(2.0f);
			RangePreview->SetBoxExtent(
				FVector(
					SegmentLength * 0.5f,
					WindRadius,
					WindRadius),
				false);
			RangePreview->RegisterComponent();
			RangePreview->SetWorldLocationAndRotation(
				(Segment.Start + Segment.End) * 0.5f,
				SegmentRotation);
			EditorWindPathPreviewComponents.Add(
				RangePreview);
		}

		UArrowComponent* DirectionPreview =
			NewObject<UArrowComponent>(
				this,
				*FString::Printf(
					TEXT("WindPathDirectionPreview_%02d"),
					Segment.ReflectionIndex),
				RF_Transient | RF_TextExportTransient);
		if (DirectionPreview != nullptr)
		{
			DirectionPreview->CreationMethod =
				EComponentCreationMethod::UserConstructionScript;
			DirectionPreview->ComponentTags.AddUnique(
				UOUWindEmitterActorPrivate::
					GeneratedWindPathPreviewTag);
			DirectionPreview->SetupAttachment(RootScene);
			DirectionPreview->SetMobility(
				EComponentMobility::Movable);
			DirectionPreview->SetCollisionEnabled(
				ECollisionEnabled::NoCollision);
			DirectionPreview->SetHiddenInGame(true);
			DirectionPreview->SetIsVisualizationComponent(true);
			DirectionPreview->SetArrowFColor(SegmentColor);
			DirectionPreview->SetArrowSize(1.5f);
			DirectionPreview->SetArrowLength(SegmentLength);
			DirectionPreview->RegisterComponent();
			DirectionPreview->SetWorldLocationAndRotation(
				Segment.Start,
				SegmentRotation);
			EditorWindPathPreviewComponents.Add(
				DirectionPreview);
		}
	}
}

void AUOUWindEmitterActor::ClearEditorWindPathPreviewComponents()
{
	TArray<USceneComponent*> ExistingSceneComponents;
	GetComponents<USceneComponent>(ExistingSceneComponents);

	for (USceneComponent* ExistingComponent :
		ExistingSceneComponents)
	{
		if (ExistingComponent != nullptr
			&& ExistingComponent->ComponentTags.Contains(
				UOUWindEmitterActorPrivate::
					GeneratedWindPathPreviewTag))
		{
			ExistingComponent->DestroyComponent();
		}
	}

	EditorWindPathPreviewComponents.Reset();
}
#endif

void AUOUWindEmitterActor::ValidateSettings()
{
	MaxWindDistance = FMath::Max(0.0f, MaxWindDistance);
	WindRadius = FMath::Max(1.0f, WindRadius);
	WindAcceleration = FMath::Max(0.0f, WindAcceleration);
	MaximumWindAcceleration = FMath::Max(0.0f, MaximumWindAcceleration);
	MaximumWindSpeed = FMath::Max(0.0f, MaximumWindSpeed);
	MaximumWindEntrySpeed = FMath::Clamp(
		MaximumWindEntrySpeed,
		0.0f,
		MaximumWindSpeed);
	MinimumWindEntrySpeed = FMath::Clamp(
		MinimumWindEntrySpeed,
		0.0f,
		MaximumWindEntrySpeed);
	FallingMomentumConversion =
		FMath::Clamp(FallingMomentumConversion, 0.0f, 1.0f);
	InitialWindVelocityBoost =
		FMath::Max(0.0f, InitialWindVelocityBoost);
	WindOnDuration = FMath::Max(0.05f, WindOnDuration);
	WindOffDuration = FMath::Max(0.05f, WindOffDuration);
	MaxReflections = FMath::Clamp(MaxReflections, 0, 8);
	MinimumReflectedStrength = FMath::Max(0.0f, MinimumReflectedStrength);
	DebugDrawTime = FMath::Max(0.0f, DebugDrawTime);
	MinimumWindVFXBoundsSize = FMath::Max(1.0f, MinimumWindVFXBoundsSize);
	MaximumWindVFXAutoScale = FMath::Max(1.0f, MaximumWindVFXAutoScale);
	WindVFXLengthCoverage = FMath::Clamp(WindVFXLengthCoverage, 0.1f, 1.5f);
	WindVFXLengthParameterRatio = FMath::Max(0.01f, WindVFXLengthParameterRatio);
	WindVFXJointOverlap = FMath::Max(0.0f, WindVFXJointOverlap);
	WindVFXRateReferenceDistance = FMath::Max(1.0f, WindVFXRateReferenceDistance);
	WindVFXRateReferenceRadius = FMath::Max(1.0f, WindVFXRateReferenceRadius);
	WindVFXBaseRateScale = FMath::Max(0.0f, WindVFXBaseRateScale);
	MinimumWindVFXRateScale = FMath::Max(0.0f, MinimumWindVFXRateScale);
	MaximumWindVFXRateScale = FMath::Max(MinimumWindVFXRateScale, MaximumWindVFXRateScale);
}

void AUOUWindEmitterActor::UpdateWindRangePreview()
{
	if (WindRangePreview == nullptr)
	{
		return;
	}

	const float SafeDistance = FMath::Max(0.0f, MaxWindDistance);
	const float SafeRadius = FMath::Max(1.0f, WindRadius);
	WindRangePreview->SetRelativeLocation(
		FVector(SafeDistance * 0.5f, 0.0f, 0.0f));
	WindRangePreview->SetBoxExtent(
		FVector(SafeDistance * 0.5f, SafeRadius, SafeRadius),
		false);
	WindRangePreview->SetVisibility(bShowWindRangePreview);
}

void AUOUWindEmitterActor::UpdatePulseCycle(float DeltaSeconds)
{
	if (!bUsePulseCycle || !bWindEnabled)
	{
		return;
	}

	const bool bWasBlowing = IsWindBlowing();
	const bool bPhaseChanged = PulseRuntimeState.Advance(
		DeltaSeconds,
		WindOnDuration,
		WindOffDuration);
	if (!bPhaseChanged)
	{
		return;
	}

	RefreshWindPathForCurrentState();

	HandleWindPhaseChanged(bWasBlowing);
}

void AUOUWindEmitterActor::HandleWindPhaseChanged(bool bWasBlowing)
{
	const bool bIsBlowing = IsWindBlowing();
	if (bWasBlowing != bIsBlowing)
	{
		SetWindVFXActive(bIsBlowing);
		OnWindPhaseChanged.Broadcast(bIsBlowing);
	}
}

float AUOUWindEmitterActor::GetWindVFXDisplayDistance() const
{
	if (bUseActualWindPathLengthForVFX && !WindPathSegments.IsEmpty())
	{
		return WindPathSegments[0].GetLength();
	}

	return MaxWindDistance;
}

void AUOUWindEmitterActor::ApplyWindVFXParameters(
	UNiagaraComponent* WindEffect,
	float DisplayDistance)
{
	if (WindEffect == nullptr)
	{
		return;
	}

	const float SafeDisplayDistance = FMath::Max(0.0f, DisplayDistance);
	const float CoveredDisplayDistance = SafeDisplayDistance * WindVFXLengthCoverage;
	if (bOverrideWindVFXColor)
	{
		UOUWindEmitterActorPrivate::SetColorParameterIfPresent(
			WindEffect,
			WindVFXColorParameterName,
			WindVFXColor);
	}
	if (bScaleWindVFXRateByVolume)
	{
		const float LengthRatio =
			SafeDisplayDistance / WindVFXRateReferenceDistance;
		const float RadiusRatio =
			WindRadius / WindVFXRateReferenceRadius;
		const float VolumeRatio =
			LengthRatio * FMath::Square(RadiusRatio);
		const float RateScale = FMath::Clamp(
			WindVFXBaseRateScale * VolumeRatio,
			MinimumWindVFXRateScale,
			MaximumWindVFXRateScale);
		UOUWindEmitterActorPrivate::SetFloatParameterIfPresent(
			WindEffect,
			WindVFXRateScaleParameterName,
			RateScale);
	}

	const float WindDiameter = WindRadius * 2.0f;
	const bool bHasLengthParameter =
		UOUWindEmitterActorPrivate::SetFloatParameterIfPresent(
			WindEffect,
			WindVFXLengthParameterName,
			CoveredDisplayDistance * WindVFXLengthParameterRatio);
	const bool bHasWidthParameter =
		UOUWindEmitterActorPrivate::SetFloatParameterIfPresent(
			WindEffect,
			WindVFXWidthParameterName,
			WindDiameter);
	const bool bHasHeightParameter =
		UOUWindEmitterActorPrivate::SetFloatParameterIfPresent(
			WindEffect,
			WindVFXHeightParameterName,
			WindDiameter);
	UOUWindEmitterActorPrivate::SetFloatParameterIfPresent(
		WindEffect,
		WindVFXMinRadiusParameterName,
		0.0f);
	const bool bHasMaxRadiusParameter =
		UOUWindEmitterActorPrivate::SetFloatParameterIfPresent(
			WindEffect,
			WindVFXMaxRadiusParameterName,
			WindRadius);

	FVector EffectScale = WindEffect->GetRelativeScale3D();
	bool bAdjustedEffectScale = false;
	const UNiagaraSystem* WindSystem = WindEffect->GetAsset();
	if (bFitMissingWindVFXDimensionsFromBounds && WindSystem != nullptr)
	{
		const FBox SystemBounds = WindSystem->GetFixedBounds();
		if (SystemBounds.IsValid != 0)
		{
			const FVector BoundsSize = SystemBounds.GetSize();
			auto CalculateAxisScale = [this](float DesiredSize, float SourceSize)
			{
				return FMath::Clamp(
					DesiredSize / FMath::Max(SourceSize, MinimumWindVFXBoundsSize),
					UE_KINDA_SMALL_NUMBER,
					MaximumWindVFXAutoScale);
			};

			if (!bHasLengthParameter)
			{
				EffectScale.X = CalculateAxisScale(CoveredDisplayDistance, BoundsSize.X);
				bAdjustedEffectScale = true;
			}
			if (!bHasWidthParameter && !bHasMaxRadiusParameter)
			{
				EffectScale.Y = CalculateAxisScale(WindDiameter, BoundsSize.Y);
				bAdjustedEffectScale = true;
			}
			if (!bHasHeightParameter && !bHasMaxRadiusParameter)
			{
				EffectScale.Z = CalculateAxisScale(WindDiameter, BoundsSize.Z);
				bAdjustedEffectScale = true;
			}
		}
	}

	if (bAdjustedEffectScale)
	{
		WindEffect->SetRelativeScale3D(EffectScale);
	}
}

void AUOUWindEmitterActor::RefreshWindVFX()
{
	if (!bAutoFitWindVFX || WindOrigin == nullptr)
	{
		ClearGeneratedWindVFX();
		return;
	}

	TInlineComponentArray<UNiagaraComponent*> WindEffects(this);
	UNiagaraComponent* TemplateEffect = nullptr;
	const float BaseDisplayDistance = GetWindVFXDisplayDistance();
	const float FirstSegmentEndOverlap =
		bCreateWindVFXForReflectedSegments && WindPathSegments.Num() > 1
			? WindVFXJointOverlap
			: 0.0f;

	for (UNiagaraComponent* WindEffect : WindEffects)
	{
		if (WindEffect == nullptr
			|| WindEffect->ComponentHasTag(UOUWindEmitterActorPrivate::GeneratedWindVFXTag))
		{
			continue;
		}

		if (TemplateEffect == nullptr)
		{
			TemplateEffect = WindEffect;
		}

		if (WindEffect->GetAttachParent() != WindOrigin)
		{
			WindEffect->AttachToComponent(
				WindOrigin,
				FAttachmentTransformRules::KeepWorldTransform);
		}

		if (bOffsetWindVFXByHalfDistance)
		{
			FVector EffectLocation = WindEffect->GetRelativeLocation();
			EffectLocation.X = BaseDisplayDistance * 0.5f + FirstSegmentEndOverlap * 0.5f;
			WindEffect->SetRelativeLocation(EffectLocation);
		}

		ApplyWindVFXParameters(
			WindEffect,
			BaseDisplayDistance + FirstSegmentEndOverlap);
	}

	RefreshReflectedWindVFX(TemplateEffect);
}

void AUOUWindEmitterActor::RefreshReflectedWindVFX(
	UNiagaraComponent* TemplateEffect)
{
	const int32 DesiredEffectCount =
		bCreateWindVFXForReflectedSegments
			&& bUseActualWindPathLengthForVFX
			&& TemplateEffect != nullptr
				? FMath::Max(0, WindPathSegments.Num() - 1)
				: 0;

	while (GeneratedWindVFXComponents.Num() > DesiredEffectCount)
	{
		if (UNiagaraComponent* EffectToRemove = GeneratedWindVFXComponents.Pop())
		{
			EffectToRemove->DestroyComponent();
		}
	}

	while (GeneratedWindVFXComponents.Num() < DesiredEffectCount)
	{
		const FName ComponentName = MakeUniqueObjectName(
			this,
			UNiagaraComponent::StaticClass(),
			TEXT("GeneratedWindVFX"));
		UNiagaraComponent* GeneratedEffect = NewObject<UNiagaraComponent>(
			this,
			ComponentName,
			RF_Transient | RF_TextExportTransient);
		if (GeneratedEffect == nullptr)
		{
			break;
		}

		GeneratedEffect->CreationMethod = EComponentCreationMethod::Instance;
		GeneratedEffect->ComponentTags.AddUnique(
			UOUWindEmitterActorPrivate::GeneratedWindVFXTag);
		GeneratedEffect->SetAutoActivate(false);
		GeneratedEffect->SetupAttachment(RootScene);
		AddInstanceComponent(GeneratedEffect);
		GeneratedEffect->RegisterComponent();
		GeneratedWindVFXComponents.Add(GeneratedEffect);
	}

	for (int32 GeneratedIndex = 0;
		GeneratedIndex < GeneratedWindVFXComponents.Num();
		++GeneratedIndex)
	{
		UNiagaraComponent* GeneratedEffect =
			GeneratedWindVFXComponents[GeneratedIndex];
		const int32 SegmentIndex = GeneratedIndex + 1;
		if (GeneratedEffect == nullptr
			|| !WindPathSegments.IsValidIndex(SegmentIndex))
		{
			continue;
		}

		if (GeneratedEffect->GetAsset() != TemplateEffect->GetAsset())
		{
			GeneratedEffect->SetAsset(TemplateEffect->GetAsset());
		}
		TemplateEffect->GetOverrideParameters().CopyParametersTo(
			GeneratedEffect->GetOverrideParameters(),
			false,
			FNiagaraParameterStore::EDataInterfaceCopyMethod::Value);

		const FUOUWindPathSegment& Segment = WindPathSegments[SegmentIndex];
		const FVector SegmentDirection = Segment.Direction.GetSafeNormal();
		const float StartOverlap = WindVFXJointOverlap;
		const float EndOverlap =
			SegmentIndex < WindPathSegments.Num() - 1
				? WindVFXJointOverlap
				: 0.0f;
		const float DisplayDistance =
			Segment.GetLength() + StartOverlap + EndOverlap;
		const FVector DisplayCenter =
			(Segment.Start + Segment.End) * 0.5f
			+ SegmentDirection * ((EndOverlap - StartOverlap) * 0.5f);
		const FQuat DisplayRotation =
			FRotationMatrix::MakeFromX(SegmentDirection).ToQuat();

		GeneratedEffect->SetWorldLocationAndRotation(
			DisplayCenter,
			DisplayRotation);
		GeneratedEffect->SetWorldScale3D(TemplateEffect->GetComponentScale());
		ApplyWindVFXParameters(GeneratedEffect, DisplayDistance);

		if (IsWindBlowing())
		{
			if (!GeneratedEffect->IsActive())
			{
				GeneratedEffect->Activate(true);
			}
		}
		else if (GeneratedEffect->IsActive())
		{
			GeneratedEffect->Deactivate();
		}
	}
}

void AUOUWindEmitterActor::ClearGeneratedWindVFX()
{
	for (UNiagaraComponent* GeneratedEffect : GeneratedWindVFXComponents)
	{
		if (GeneratedEffect != nullptr)
		{
			GeneratedEffect->DestroyComponent();
		}
	}
	GeneratedWindVFXComponents.Reset();
}

void AUOUWindEmitterActor::SetWindVFXActive(bool bActive)
{
	TInlineComponentArray<UNiagaraComponent*> WindEffects(this);
	for (UNiagaraComponent* WindEffect : WindEffects)
	{
		if (WindEffect == nullptr)
		{
			continue;
		}

		if (bActive)
		{
			WindEffect->Activate(true);
		}
		else
		{
			WindEffect->Deactivate();
		}
	}
}

void AUOUWindEmitterActor::RefreshWindPathForCurrentState()
{
	if (IsWindBlowing())
	{
		RebuildWindPath();
		return;
	}

	ClearWindPath();
}

void AUOUWindEmitterActor::ClearWindPath()
{
	WindPathSegments.Reset();
	LastAffectedReceiverCount = 0;
	ClearGeneratedWindVFX();
	OnWindPathChanged.Broadcast();
}

void AUOUWindEmitterActor::ApplyWindToReceivers(float DeltaSeconds)
{
	LastAffectedReceiverCount = 0;

	UWorld* World = GetWorld();
	if (World == nullptr || WindPathSegments.IsEmpty() || DeltaSeconds <= 0.0f)
	{
		return;
	}

	const FCollisionObjectQueryParams ObjectQueryParams = BuildReceiverObjectQueryParams();
	if (!ObjectQueryParams.IsValid())
	{
		return;
	}

	TMap<UObject*, FUOUWindExposureData> BestExposureByReceiver;
	for (const FUOUWindPathSegment& Segment : WindPathSegments)
	{
		const float SegmentLength = Segment.GetLength();
		if (SegmentLength <= KINDA_SMALL_NUMBER || Segment.Strength <= 0.0f)
		{
			continue;
		}

		const FVector SegmentCenter = (Segment.Start + Segment.End) * 0.5f;
		const FQuat SegmentRotation = FRotationMatrix::MakeFromX(Segment.Direction).ToQuat();
		const FCollisionShape WindShape =
			FCollisionShape::MakeBox(FVector(SegmentLength * 0.5f, WindRadius, WindRadius));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOUWindReceiverOverlap), false, this);
		QueryParams.AddIgnoredActor(this);

		TArray<FOverlapResult> OverlapResults;
		if (!World->OverlapMultiByObjectType(
			OverlapResults,
			SegmentCenter,
			SegmentRotation,
			ObjectQueryParams,
			WindShape,
			QueryParams))
		{
			continue;
		}

		TSet<UObject*> SegmentReceivers;
		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			TArray<UObject*> Receivers;
			AppendWindReceivers(
				OverlapResult.GetActor(),
				OverlapResult.GetComponent(),
				Receivers);

			for (UObject* Receiver : Receivers)
			{
				if (Receiver == nullptr || SegmentReceivers.Contains(Receiver))
				{
					continue;
				}
				SegmentReceivers.Add(Receiver);

				const FVector ReceiverLocation =
					IUOUWindReceivableInterface::Execute_GetWindReceiverLocation(Receiver);
				const FVector ClosestPoint =
					FMath::ClosestPointOnSegment(ReceiverLocation, Segment.Start, Segment.End);
				const float DistanceFromPath = FVector::Distance(ReceiverLocation, ClosestPoint);
				if (DistanceFromPath > WindRadius)
				{
					continue;
				}

				const float RadialFactor = bUseRadialFalloff
					? 1.0f - FMath::Clamp(DistanceFromPath / WindRadius, 0.0f, 1.0f)
					: 1.0f;
				const float FinalStrength = Segment.Strength * RadialFactor;
				if (FinalStrength <= 0.0f)
				{
					continue;
				}

				const FUOUWindExposureData ExposureData =
					MakeExposureData(
						Segment,
						ClosestPoint,
						FinalStrength,
						DeltaSeconds);

				FUOUWindExposureData* ExistingExposure = BestExposureByReceiver.Find(Receiver);
				if (ExistingExposure == nullptr
					|| FinalStrength > ExistingExposure->Acceleration)
				{
					BestExposureByReceiver.Add(Receiver, ExposureData);
				}
			}
		}
	}

	for (const TPair<UObject*, FUOUWindExposureData>& ReceiverPair : BestExposureByReceiver)
	{
		if (IsValid(ReceiverPair.Key))
		{
			IUOUWindReceivableInterface::Execute_ReceiveWind(
				ReceiverPair.Key,
				ReceiverPair.Value);
			++LastAffectedReceiverCount;
		}
	}
}

FUOUWindExposureData AUOUWindEmitterActor::MakeExposureData(
	const FUOUWindPathSegment& Segment,
	const FVector& ClosestPoint,
	float FinalStrength,
	float DeltaSeconds)
{
	FUOUWindExposureData ExposureData;
	ExposureData.SourceActor = this;
	ExposureData.Direction = Segment.Direction;
	ExposureData.ClosestPointOnPath = ClosestPoint;
	ExposureData.Acceleration = FinalStrength;
	ExposureData.MaximumAcceleration = MaximumWindAcceleration;
	ExposureData.MaximumSpeed = MaximumWindSpeed;
	ExposureData.MinimumEntrySpeed = MinimumWindEntrySpeed;
	ExposureData.FallingMomentumConversion =
		FallingMomentumConversion;
	ExposureData.InitialVelocityBoost =
		InitialWindVelocityBoost;
	ExposureData.MaximumEntrySpeed =
		MaximumWindEntrySpeed;
	ExposureData.StrengthScale =
		FinalStrength
			/ FMath::Max(
				WindAcceleration,
				KINDA_SMALL_NUMBER);
	ExposureData.DeltaTime = DeltaSeconds;
	ExposureData.ReflectionIndex = Segment.ReflectionIndex;
	return ExposureData;
}

void AUOUWindEmitterActor::DrawWindDebug() const
{
#if UOU_WITH_DEVELOPMENT_TOOLS
	if (!bDrawWindDebug || GetWorld() == nullptr)
	{
		return;
	}

	for (const FUOUWindPathSegment& Segment : WindPathSegments)
	{
		const FColor SegmentColor = Segment.ReflectionIndex == 0
			? FColor::Cyan
			: FColor::MakeRedToGreenColorFromScalar(
				FMath::Clamp(
					Segment.Strength / FMath::Max(WindAcceleration, KINDA_SMALL_NUMBER),
					0.0f,
					1.0f));

		DrawDebugDirectionalArrow(
			GetWorld(),
			Segment.Start,
			Segment.End,
			60.0f,
			SegmentColor,
			false,
			DebugDrawTime,
			0,
			5.0f);
		DrawDebugSphere(
			GetWorld(),
			Segment.Start,
			WindRadius,
			16,
			SegmentColor,
			false,
			DebugDrawTime,
			0,
			1.0f);
		DrawDebugBox(
			GetWorld(),
			(Segment.Start + Segment.End) * 0.5f,
			FVector(Segment.GetLength() * 0.5f, WindRadius, WindRadius),
			FRotationMatrix::MakeFromX(Segment.Direction).ToQuat(),
			SegmentColor,
			false,
			DebugDrawTime,
			0,
			1.0f);
	}
#endif
}

FCollisionObjectQueryParams AUOUWindEmitterActor::BuildReceiverObjectQueryParams() const
{
	FCollisionObjectQueryParams QueryParams;
	// 플레이어는 동적 바람의 기본 수신체이므로 BP 배열이 비어도 항상 탐색합니다.
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);
	for (const TEnumAsByte<EObjectTypeQuery> ObjectType : ReceiverObjectTypes)
	{
		const ECollisionChannel CollisionChannel =
			UEngineTypes::ConvertToCollisionChannel(ObjectType);
		if (CollisionChannel != ECC_MAX)
		{
			QueryParams.AddObjectTypesToQuery(CollisionChannel);
		}
	}
	return QueryParams;
}

void AUOUWindEmitterActor::AppendWindReceivers(
	AActor* TargetActor,
	UPrimitiveComponent* TargetComponent,
	TArray<UObject*>& OutReceivers) const
{
	auto AppendIfReceivable = [&OutReceivers](UObject* Candidate)
	{
		if (Candidate != nullptr
			&& Candidate->GetClass()->ImplementsInterface(UUOUWindReceivableInterface::StaticClass()))
		{
			OutReceivers.AddUnique(Candidate);
		}
	};

	AppendIfReceivable(TargetComponent);
	AppendIfReceivable(TargetActor);

	if (TargetActor == nullptr)
	{
		return;
	}

	TArray<UActorComponent*> ActorComponents;
	TargetActor->GetComponents(ActorComponents);
	for (UActorComponent* ActorComponent : ActorComponents)
	{
		AppendIfReceivable(ActorComponent);
	}
}
