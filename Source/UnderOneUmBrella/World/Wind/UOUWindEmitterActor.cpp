// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Wind/UOUWindEmitterActor.h"

#include "Components/ArrowComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "World/Wind/UOUWindInteractionSurfaceComponent.h"
#include "World/Wind/UOUWindReceivableInterface.h"

AUOUWindEmitterActor::AUOUWindEmitterActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	FanVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FanVisual"));
	FanVisual->SetupAttachment(RootScene);
	FanVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WindOrigin = CreateDefaultSubobject<UArrowComponent>(TEXT("WindOrigin"));
	WindOrigin->SetupAttachment(RootScene);
	WindOrigin->ArrowColor = FColor::Cyan;
	WindOrigin->ArrowSize = 2.0f;

	ReceiverObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ReceiverObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
	ReceiverObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
}

void AUOUWindEmitterActor::BeginPlay()
{
	Super::BeginPlay();
	ValidateSettings();
	InitializePulseCycleState();
	if (WindOrigin != nullptr)
	{
		WindOrigin->SetHiddenInGame(!bShowDirectionArrowInGame);
	}
	SetActorTickEnabled(bWindEnabled);
	RebuildWindPath();
}

void AUOUWindEmitterActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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
	if (WindOrigin != nullptr)
	{
		WindOrigin->SetHiddenInGame(!bShowDirectionArrowInGame);
	}
}

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

TArray<FString> AUOUWindEmitterActor::GetPuzzleDebugInfo_Implementation() const
{
	return {
		FString::Printf(TEXT("Wind Enabled: %s"), bWindEnabled ? TEXT("true") : TEXT("false")),
		FString::Printf(
			TEXT("Blowing: %s / Pulse: %s / Remaining: %.2fs"),
			IsWindBlowing() ? TEXT("true") : TEXT("false"),
			bUsePulseCycle ? TEXT("true") : TEXT("false"),
			PulseRuntimeState.TimeRemaining),
		FString::Printf(TEXT("Segments: %d / Reflections: %d"), WindPathSegments.Num(), FMath::Max(0, WindPathSegments.Num() - 1)),
		FString::Printf(TEXT("Affected Receivers: %d"), LastAffectedReceiverCount),
		FString::Printf(TEXT("Range: %.0f / Radius: %.0f / Strength: %.2f"), MaxWindDistance, WindRadius, BaseStrength)
	};
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
		RebuildWindPath();
	}
	else
	{
		WindPathSegments.Reset();
		LastAffectedReceiverCount = 0;
		OnWindPathChanged.Broadcast();
	}

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

	if (IsWindBlowing())
	{
		RebuildWindPath();
	}
	else
	{
		WindPathSegments.Reset();
		LastAffectedReceiverCount = 0;
		OnWindPathChanged.Broadcast();
	}

	HandleWindPhaseChanged(bWasBlowing);
}

void AUOUWindEmitterActor::ResetPulseCycle()
{
	const bool bWasBlowing = IsWindBlowing();
	InitializePulseCycleState();

	if (HasActorBegunPlay())
	{
		if (IsWindBlowing())
		{
			RebuildWindPath();
		}
		else
		{
			WindPathSegments.Reset();
			LastAffectedReceiverCount = 0;
			OnWindPathChanged.Broadcast();
		}
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
	WindPathSegments.Reset();

	UWorld* World = GetWorld();
	if (!IsWindBlowing()
		|| World == nullptr
		|| WindOrigin == nullptr
		|| MaxWindDistance <= 0.0f
		|| BaseStrength <= 0.0f)
	{
		OnWindPathChanged.Broadcast();
		return;
	}

	FVector SegmentStart = WindOrigin->GetComponentLocation();
	FVector SegmentDirection = WindOrigin->GetForwardVector().GetSafeNormal();
	float RemainingDistance = MaxWindDistance;
	float CurrentStrength = BaseStrength;

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

	OnWindPathChanged.Broadcast();
}

void AUOUWindEmitterActor::ValidateSettings()
{
	MaxWindDistance = FMath::Max(0.0f, MaxWindDistance);
	WindRadius = FMath::Max(1.0f, WindRadius);
	BaseStrength = FMath::Max(0.0f, BaseStrength);
	WindOnDuration = FMath::Max(0.05f, WindOnDuration);
	WindOffDuration = FMath::Max(0.05f, WindOffDuration);
	MaxReflections = FMath::Clamp(MaxReflections, 0, 8);
	MinimumReflectedStrength = FMath::Max(0.0f, MinimumReflectedStrength);
	DebugDrawTime = FMath::Max(0.0f, DebugDrawTime);
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

	if (IsWindBlowing())
	{
		RebuildWindPath();
	}
	else
	{
		WindPathSegments.Reset();
		LastAffectedReceiverCount = 0;
		OnWindPathChanged.Broadcast();
	}

	HandleWindPhaseChanged(bWasBlowing);
}

void AUOUWindEmitterActor::HandleWindPhaseChanged(bool bWasBlowing)
{
	const bool bIsBlowing = IsWindBlowing();
	if (bWasBlowing != bIsBlowing)
	{
		OnWindPhaseChanged.Broadcast(bIsBlowing);
	}
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

				FUOUWindExposureData ExposureData;
				ExposureData.SourceActor = this;
				ExposureData.Direction = Segment.Direction;
				ExposureData.ClosestPointOnPath = ClosestPoint;
				ExposureData.Strength = FinalStrength;
				ExposureData.DeltaTime = DeltaSeconds;
				ExposureData.ReflectionIndex = Segment.ReflectionIndex;

				FUOUWindExposureData* ExistingExposure = BestExposureByReceiver.Find(Receiver);
				if (ExistingExposure == nullptr || FinalStrength > ExistingExposure->Strength)
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

void AUOUWindEmitterActor::DrawWindDebug() const
{
	if (!bDrawWindDebug || GetWorld() == nullptr)
	{
		return;
	}

	for (const FUOUWindPathSegment& Segment : WindPathSegments)
	{
		const FColor SegmentColor = Segment.ReflectionIndex == 0
			? FColor::Cyan
			: FColor::MakeRedToGreenColorFromScalar(
				FMath::Clamp(Segment.Strength / FMath::Max(BaseStrength, KINDA_SMALL_NUMBER), 0.0f, 1.0f));

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
}

FCollisionObjectQueryParams AUOUWindEmitterActor::BuildReceiverObjectQueryParams() const
{
	FCollisionObjectQueryParams QueryParams;
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
