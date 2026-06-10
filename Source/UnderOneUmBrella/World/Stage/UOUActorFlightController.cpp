// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUActorFlightController.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"

AUOUActorFlightController::AUOUActorFlightController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
}

void AUOUActorFlightController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsFlying)
	{
		return;
	}

	FlightElapsedTime += DeltaSeconds;

	bool bAllFinished = true;
	for (FUOUActorFlightRuntimeItem& Item : FlightItems)
	{
		if (Item.bFinished)
		{
			continue;
		}

		AActor* FlightActor = Item.Actor.Get();
		if (FlightActor == nullptr)
		{
			Item.bFinished = true;
			continue;
		}

		const float LocalTime = FlightElapsedTime - Item.StartDelay;
		if (LocalTime <= 0.0f)
		{
			bAllFinished = false;
			continue;
		}

		const float RawAlpha = FMath::Clamp(LocalTime / FMath::Max(FlightDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
		ApplyFlightTransform(Item, RawAlpha);

		if (RawAlpha >= 1.0f)
		{
			MarkActorFinished(Item);
			continue;
		}

		bAllFinished = false;
	}

	if (bAllFinished)
	{
		FinishFlight();
	}
}

void AUOUActorFlightController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bIsFlying && bRestorePreparedStateOnCancel)
	{
		RestorePreparedActorStates();
	}

	Super::EndPlay(EndPlayReason);
}

void AUOUActorFlightController::StartFlight()
{
	if (bIsFlying)
	{
		return;
	}

	BuildFlightItems();
	if (FlightItems.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UOU Actor Flight Controller '%s' has no valid FlightActors."), *GetNameSafe(this));
		return;
	}

	FlightElapsedTime = 0.0f;
	bIsFlying = true;
	SetActorTickEnabled(true);

	OnFlightStarted.Broadcast(this);
}

void AUOUActorFlightController::RestartFlight()
{
	bIsFlying = false;
	SetActorTickEnabled(false);

	if (FlightItems.Num() > 0)
	{
		for (FUOUActorFlightRuntimeItem& Item : FlightItems)
		{
			AActor* FlightActor = Item.Actor.Get();
			if (FlightActor == nullptr)
			{
				continue;
			}

			FlightActor->SetActorTransform(Item.StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
			FlightActor->SetActorHiddenInGame(Item.bWasHidden);
			Item.bFinished = false;
		}

		RestorePreparedActorStates();
	}

	FlightElapsedTime = 0.0f;
	StartFlight();
}

void AUOUActorFlightController::StopFlight()
{
	if (!bIsFlying)
	{
		if (bRestorePreparedStateOnCancel)
		{
			RestorePreparedActorStates();
		}
		return;
	}

	bIsFlying = false;
	SetActorTickEnabled(false);

	if (bRestorePreparedStateOnCancel)
	{
		RestorePreparedActorStates();
	}

	OnFlightStopped.Broadcast(this);
}

void AUOUActorFlightController::ResetFlightActorsToStart()
{
	bIsFlying = false;
	SetActorTickEnabled(false);

	for (FUOUActorFlightRuntimeItem& Item : FlightItems)
	{
		AActor* FlightActor = Item.Actor.Get();
		if (FlightActor == nullptr)
		{
			continue;
		}

		FlightActor->SetActorTransform(Item.StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
		FlightActor->SetActorHiddenInGame(Item.bWasHidden);
		Item.bFinished = false;
	}

	RestorePreparedActorStates();
	FlightElapsedTime = 0.0f;
}

void AUOUActorFlightController::RestorePreparedActorStates()
{
	for (FUOUActorFlightRuntimeItem& Item : FlightItems)
	{
		RestoreActorState(Item);
	}
}

bool AUOUActorFlightController::IsFlying() const
{
	return bIsFlying;
}

void AUOUActorFlightController::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		StartFlight();
		break;

	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		StopFlight();
		break;

	case EOUUPuzzleResultAction::Toggle:
		if (IsFlying())
		{
			StopFlight();
			return;
		}
		StartFlight();
		break;

	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

#if WITH_EDITOR
bool AUOUActorFlightController::ShouldTickIfViewportsOnly() const
{
	return bIsFlying;
}
#endif

void AUOUActorFlightController::BuildFlightItems()
{
	FlightItems.Reset();

	FRandomStream RandomStream(bUseFixedRandomSeed ? RandomSeed : FMath::Rand());
	const FTransform TargetTransform = ResolveTargetTransform();
	const FVector TargetLocation = TargetTransform.GetLocation();

	TArray<AActor*> OrderedActors;
	OrderedActors.Reserve(FlightActors.Num());
	for (const TObjectPtr<AActor>& FlightActor : FlightActors)
	{
		if (FlightActor != nullptr && FlightActor != this)
		{
			OrderedActors.Add(FlightActor);
		}
	}

	SortFlightActors(OrderedActors, TargetLocation, RandomStream);

	const int32 ActorCount = OrderedActors.Num();
	for (int32 Index = 0; Index < ActorCount; ++Index)
	{
		AActor* FlightActor = OrderedActors[Index];
		if (FlightActor == nullptr)
		{
			continue;
		}

		FUOUActorFlightRuntimeItem Item;
		Item.Actor = FlightActor;
		Item.StartTransform = FlightActor->GetActorTransform();
		Item.bWasHidden = FlightActor->IsHidden();

		const FVector StartLocation = Item.StartTransform.GetLocation();
		const FVector TargetScatterOffset = TargetScatterRadius > 0.0f
			? RandomStream.VRand() * RandomStream.FRandRange(0.0f, TargetScatterRadius)
			: FVector::ZeroVector;
		const FVector EndLocation = TargetLocation + TargetScatterOffset;

		const FQuat EndRotation = bMatchTargetRotation ? TargetTransform.GetRotation() : Item.StartTransform.GetRotation();
		const FVector EndScale = Item.StartTransform.GetScale3D() * FMath::Max(0.0f, EndScaleMultiplier);
		Item.EndTransform = FTransform(EndRotation, EndLocation, EndScale);

		const float OrderAlpha = ActorCount > 1 ? static_cast<float>(Index) / static_cast<float>(ActorCount - 1) : 0.0f;
		Item.StartDelay = OrderAlpha * FMath::Max(0.0f, TotalStaggerTime)
			+ RandomStream.FRandRange(0.0f, FMath::Max(0.0f, RandomStartDelayMax));

		FVector TravelDirection = (EndLocation - StartLocation).GetSafeNormal();
		if (TravelDirection.IsNearlyZero())
		{
			TravelDirection = FlightActor->GetActorForwardVector().GetSafeNormal();
		}
		if (TravelDirection.IsNearlyZero())
		{
			TravelDirection = FVector::ForwardVector;
		}

		FVector SideAxis = FVector::CrossProduct(FVector::UpVector, TravelDirection).GetSafeNormal();
		if (SideAxis.IsNearlyZero())
		{
			SideAxis = FlightActor->GetActorRightVector().GetSafeNormal();
		}
		if (SideAxis.IsNearlyZero())
		{
			SideAxis = FVector::RightVector;
		}

		const float LocalArcHeight = FMath::Max(0.0f, ArcHeight + RandomStream.FRandRange(-ArcHeightRandomRange, ArcHeightRandomRange));
		const float SideOffsetA = RandomStream.FRandRange(-SideSpread, SideSpread);
		const float SideOffsetB = RandomStream.FRandRange(-SideSpread * 0.5f, SideSpread * 0.5f);

		Item.ControlPointA = StartLocation + FVector::UpVector * LocalArcHeight + SideAxis * SideOffsetA;
		Item.ControlPointB = EndLocation
			- TravelDirection * FMath::Max(0.0f, TargetApproachDistance)
			+ FVector::UpVector * (LocalArcHeight * 0.35f)
			+ SideAxis * SideOffsetB;

		Item.SpinAxis = RandomStream.VRand().GetSafeNormal();
		if (Item.SpinAxis.IsNearlyZero())
		{
			Item.SpinAxis = FVector::UpVector;
		}

		const float MinSpin = FMath::Min(SpinMinDegrees, SpinMaxDegrees);
		const float MaxSpin = FMath::Max(SpinMinDegrees, SpinMaxDegrees);
		const float SpinSign = RandomStream.RandRange(0, 1) == 0 ? -1.0f : 1.0f;
		Item.SpinDegrees = RandomStream.FRandRange(MinSpin, MaxSpin) * SpinSign;

		PrepareActorForFlight(Item);
		FlightItems.Add(MoveTemp(Item));
	}
}

void AUOUActorFlightController::SortFlightActors(TArray<AActor*>& Actors, const FVector& TargetLocation, FRandomStream& RandomStream) const
{
	switch (FlightOrder)
	{
	case EUOUActorFlightOrder::NearTargetFirst:
		Actors.Sort([TargetLocation](const AActor& Left, const AActor& Right)
		{
			return FVector::DistSquared(Left.GetActorLocation(), TargetLocation) < FVector::DistSquared(Right.GetActorLocation(), TargetLocation);
		});
		break;

	case EUOUActorFlightOrder::FarTargetFirst:
		Actors.Sort([TargetLocation](const AActor& Left, const AActor& Right)
		{
			return FVector::DistSquared(Left.GetActorLocation(), TargetLocation) > FVector::DistSquared(Right.GetActorLocation(), TargetLocation);
		});
		break;

	case EUOUActorFlightOrder::Random:
		for (int32 Index = Actors.Num() - 1; Index > 0; --Index)
		{
			const int32 SwapIndex = RandomStream.RandRange(0, Index);
			Actors.Swap(Index, SwapIndex);
		}
		break;

	case EUOUActorFlightOrder::ArrayOrder:
	default:
		break;
	}
}

void AUOUActorFlightController::PrepareActorForFlight(FUOUActorFlightRuntimeItem& Item)
{
	AActor* FlightActor = Item.Actor.Get();
	if (FlightActor == nullptr)
	{
		return;
	}

	if (bUnhideActorsOnStart)
	{
		FlightActor->SetActorHiddenInGame(false);
	}

	TArray<USceneComponent*> SceneComponents;
	FlightActor->GetComponents(SceneComponents);

	Item.ComponentStates.Reset(SceneComponents.Num());
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent == nullptr)
		{
			continue;
		}

		FUOUActorFlightComponentRuntimeState ComponentState;
		ComponentState.Component = SceneComponent;
		ComponentState.Mobility = SceneComponent->Mobility;

		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent))
		{
			ComponentState.bWasPrimitive = true;
			ComponentState.CollisionEnabled = PrimitiveComponent->GetCollisionEnabled();
			ComponentState.bWasSimulatingPhysics = PrimitiveComponent->IsSimulatingPhysics();
			ComponentState.bGravityEnabled = PrimitiveComponent->IsGravityEnabled();
			ComponentState.LinearVelocity = PrimitiveComponent->GetPhysicsLinearVelocity();
			ComponentState.AngularVelocityInDegrees = PrimitiveComponent->GetPhysicsAngularVelocityInDegrees();
		}

		Item.ComponentStates.Add(ComponentState);

		if (bForceMovableBeforeFlight && SceneComponent->Mobility != EComponentMobility::Movable)
		{
			SceneComponent->SetMobility(EComponentMobility::Movable);
		}

		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent))
		{
			if (bDisablePhysicsDuringFlight)
			{
				if (bClearPhysicsVelocityBeforeFlight && PrimitiveComponent->IsSimulatingPhysics())
				{
					PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
					PrimitiveComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				}
				PrimitiveComponent->SetSimulatePhysics(false);
			}

			if (bDisableGravityDuringFlight)
			{
				PrimitiveComponent->SetEnableGravity(false);
			}

			if (bOverrideCollisionDuringFlight)
			{
				PrimitiveComponent->SetCollisionEnabled(CollisionDuringFlight);
			}
		}
	}
}

void AUOUActorFlightController::RestoreActorState(FUOUActorFlightRuntimeItem& Item) const
{
	for (const FUOUActorFlightComponentRuntimeState& ComponentState : Item.ComponentStates)
	{
		USceneComponent* SceneComponent = ComponentState.Component.Get();
		if (SceneComponent == nullptr)
		{
			continue;
		}

		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent))
		{
			if (bOverrideCollisionDuringFlight)
			{
				PrimitiveComponent->SetCollisionEnabled(ComponentState.CollisionEnabled);
			}

			if (bDisableGravityDuringFlight)
			{
				PrimitiveComponent->SetEnableGravity(ComponentState.bGravityEnabled);
			}

			if (bDisablePhysicsDuringFlight)
			{
				PrimitiveComponent->SetSimulatePhysics(ComponentState.bWasSimulatingPhysics);
				if (bRestorePhysicsVelocityWhenRestoringState && ComponentState.bWasSimulatingPhysics)
				{
					PrimitiveComponent->SetPhysicsLinearVelocity(ComponentState.LinearVelocity);
					PrimitiveComponent->SetPhysicsAngularVelocityInDegrees(ComponentState.AngularVelocityInDegrees);
				}
			}
		}

		if (bForceMovableBeforeFlight && SceneComponent->Mobility != ComponentState.Mobility)
		{
			SceneComponent->SetMobility(ComponentState.Mobility);
		}
	}
}

void AUOUActorFlightController::ApplyFlightTransform(FUOUActorFlightRuntimeItem& Item, float RawAlpha) const
{
	AActor* FlightActor = Item.Actor.Get();
	if (FlightActor == nullptr)
	{
		return;
	}

	const float Alpha = ResolveFlightAlpha(RawAlpha);
	const FVector Location = EvaluateCubicBezier(
		Item.StartTransform.GetLocation(),
		Item.ControlPointA,
		Item.ControlPointB,
		Item.EndTransform.GetLocation(),
		Alpha);

	const FQuat StartRotation = Item.StartTransform.GetRotation();
	const FQuat EndRotation = Item.EndTransform.GetRotation();
	const FQuat BaseRotation = bMatchTargetRotation
		? FQuat::Slerp(StartRotation, EndRotation, Alpha).GetNormalized()
		: StartRotation;

	const FQuat SpinRotation(Item.SpinAxis.GetSafeNormal(), FMath::DegreesToRadians(Item.SpinDegrees * Alpha));
	const FVector Scale = FMath::Lerp(Item.StartTransform.GetScale3D(), Item.EndTransform.GetScale3D(), Alpha);
	const FTransform NewTransform((SpinRotation * BaseRotation).GetNormalized(), Location, Scale);

	FlightActor->SetActorTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void AUOUActorFlightController::MarkActorFinished(FUOUActorFlightRuntimeItem& Item) const
{
	Item.bFinished = true;

	AActor* FlightActor = Item.Actor.Get();
	if (FlightActor == nullptr)
	{
		return;
	}

	if (bHideActorsWhenFinished)
	{
		FlightActor->SetActorHiddenInGame(true);
	}
}

void AUOUActorFlightController::FinishFlight()
{
	bIsFlying = false;
	SetActorTickEnabled(false);

	if (bRestorePreparedStateOnFinish)
	{
		RestorePreparedActorStates();
	}

	OnFlightFinished.Broadcast(this);
}

FTransform AUOUActorFlightController::ResolveTargetTransform() const
{
	const AActor* ResolvedTargetActor = TargetActor != nullptr ? TargetActor.Get() : this;
	if (ResolvedTargetActor == nullptr)
	{
		return FTransform::Identity;
	}

	const FTransform TargetTransform = ResolvedTargetActor->GetActorTransform();
	return FTransform(
		TargetTransform.GetRotation(),
		TargetTransform.TransformPosition(TargetLocalOffset),
		TargetTransform.GetScale3D());
}

float AUOUActorFlightController::ResolveFlightAlpha(float RawAlpha) const
{
	const float ClampedAlpha = FMath::Clamp(RawAlpha, 0.0f, 1.0f);
	if (FlightCurve != nullptr)
	{
		return FMath::Clamp(FlightCurve->GetFloatValue(ClampedAlpha), 0.0f, 1.0f);
	}

	switch (EasingMode)
	{
	case EUOUActorFlightEasingMode::EaseIn:
		return ClampedAlpha * ClampedAlpha;

	case EUOUActorFlightEasingMode::EaseOut:
		return 1.0f - FMath::Square(1.0f - ClampedAlpha);

	case EUOUActorFlightEasingMode::EaseInOut:
		return FMath::InterpEaseInOut(0.0f, 1.0f, ClampedAlpha, 2.0f);

	case EUOUActorFlightEasingMode::Linear:
	default:
		return ClampedAlpha;
	}
}

FVector AUOUActorFlightController::EvaluateCubicBezier(const FVector& Point0, const FVector& Point1, const FVector& Point2, const FVector& Point3, float Alpha)
{
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const float OneMinusAlpha = 1.0f - ClampedAlpha;

	return OneMinusAlpha * OneMinusAlpha * OneMinusAlpha * Point0
		+ 3.0f * OneMinusAlpha * OneMinusAlpha * ClampedAlpha * Point1
		+ 3.0f * OneMinusAlpha * ClampedAlpha * ClampedAlpha * Point2
		+ ClampedAlpha * ClampedAlpha * ClampedAlpha * Point3;
}
