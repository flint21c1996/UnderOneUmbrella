// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Pour/UOUPourRotationReactionComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

UUOUPourRotationReactionComponent::UUOUPourRotationReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUPourRotationReactionComponent::OnRegister()
{
	Super::OnRegister();

	CacheBaseRotationIfNeeded();
	CacheBaseMovementLocationIfNeeded();
}

void UUOUPourRotationReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheBaseRotationIfNeeded();
	CacheBaseMovementLocationIfNeeded();
	BindToPourReceiver();
}

void UUOUPourRotationReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromPourReceiver();

	Super::EndPlay(EndPlayReason);
}

void UUOUPourRotationReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bRotationReactionEnabled)
	{
		return;
	}

	UpdateInterpolatedRotation(DeltaTime);
}

void UUOUPourRotationReactionComponent::ResetRotationReaction(bool bApplyBaseRotation)
{
	bHasCachedBaseRotation = false;
	bHasCachedBaseMovementLocation = false;
	CacheBaseRotationIfNeeded();
	CacheBaseMovementLocationIfNeeded();

	TargetAngleDegrees = 0.0f;
	CurrentAppliedAngleDegrees = 0.0f;
	CurrentMovementDistance = 0.0f;
	if (bApplyBaseRotation)
	{
		ApplyRotationAngle(CurrentAppliedAngleDegrees);
	}
}

void UUOUPourRotationReactionComponent::SetRotationReactionEnabled(bool bEnabled)
{
	bRotationReactionEnabled = bEnabled;
}

bool UUOUPourRotationReactionComponent::IsRotationReactionEnabled() const
{
	return bRotationReactionEnabled;
}

void UUOUPourRotationReactionComponent::HandlePourReceived(UUOUPourReceiverComponent* Receiver, const FUOUPourInputContext& PourContext)
{
	if (!bRotationReactionEnabled || Receiver == nullptr || PourContext.Volume <= 0.0f)
	{
		return;
	}

	const float AngleDelta = RotationAmountMode == EUOUPourRotationAmountMode::Event
		? DegreesPerPourEvent
		: DegreesPerPourSecond * FMath::Max(PourContext.Duration, 0.0f);
	if (FMath::IsNearlyZero(AngleDelta))
	{
		return;
	}

	const float RotationSign = ResolveRotationSign(PourContext);
	if (FMath::IsNearlyZero(RotationSign))
	{
		return;
	}

	SetTargetRotationAngle(TargetAngleDegrees + AngleDelta * RotationSign);
}

UUOUPourReceiverComponent* UUOUPourRotationReactionComponent::ResolvePourReceiverComponent() const
{
	if (IsValid(PourReceiverComponent))
	{
		return PourReceiverComponent.Get();
	}

	const AActor* Owner = GetOwner();
	if (bAutoFindPourReceiverComponent && Owner != nullptr)
	{
		return Owner->FindComponentByClass<UUOUPourReceiverComponent>();
	}

	return nullptr;
}

USceneComponent* UUOUPourRotationReactionComponent::ResolveRotationTargetComponent() const
{
	if (IsValid(RotationTargetComponent))
	{
		return RotationTargetComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindRotationTargetComponent())
	{
		return FoundComponent;
	}

	return bUseOwnerRootWhenTargetMissing && GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
}

USceneComponent* UUOUPourRotationReactionComponent::ResolveMovementTargetComponent() const
{
	if (IsValid(MovementTargetComponent))
	{
		return MovementTargetComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindMovementTargetComponent())
	{
		return FoundComponent;
	}

	return bUseOwnerRootWhenMovementTargetMissing && GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
}

USceneComponent* UUOUPourRotationReactionComponent::FindRotationTargetComponent() const
{
	return FindSceneComponentByNameOrTag(RotationTargetComponentName);
}

USceneComponent* UUOUPourRotationReactionComponent::FindMovementTargetComponent() const
{
	return FindSceneComponentByNameOrTag(MovementTargetComponentName);
}

const USceneComponent* UUOUPourRotationReactionComponent::ResolveTorqueCenterComponent(const USceneComponent* TargetComponent) const
{
	if (IsValid(TorqueCenterComponent))
	{
		return TorqueCenterComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindSceneComponentByNameOrTag(TorqueCenterComponentName))
	{
		return FoundComponent;
	}

	return TargetComponent;
}

USceneComponent* UUOUPourRotationReactionComponent::FindSceneComponentByNameOrTag(FName ComponentName) const
{
	const AActor* Owner = GetOwner();
	if (Owner == nullptr || ComponentName.IsNone())
	{
		return nullptr;
	}

	const FString TargetName = ComponentName.ToString();
	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!IsValid(SceneComponent))
		{
			continue;
		}

		if (SceneComponent->GetFName() == ComponentName
			|| SceneComponent->ComponentTags.Contains(ComponentName)
			|| SceneComponent->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void UUOUPourRotationReactionComponent::BindToPourReceiver()
{
	UUOUPourReceiverComponent* ResolvedReceiver = ResolvePourReceiverComponent();
	if (BoundPourReceiverComponent == ResolvedReceiver)
	{
		return;
	}

	UnbindFromPourReceiver();

	BoundPourReceiverComponent = ResolvedReceiver;
	if (BoundPourReceiverComponent != nullptr)
	{
		BoundPourReceiverComponent->OnPourReceived.AddUniqueDynamic(this, &UUOUPourRotationReactionComponent::HandlePourReceived);
	}
}

void UUOUPourRotationReactionComponent::UnbindFromPourReceiver()
{
	if (BoundPourReceiverComponent != nullptr)
	{
		BoundPourReceiverComponent->OnPourReceived.RemoveDynamic(this, &UUOUPourRotationReactionComponent::HandlePourReceived);
		BoundPourReceiverComponent = nullptr;
	}
}

void UUOUPourRotationReactionComponent::CacheBaseRotationIfNeeded()
{
	if (bHasCachedBaseRotation)
	{
		return;
	}

	USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	if (TargetComponent == nullptr)
	{
		return;
	}

	BaseRelativeRotation = TargetComponent->GetRelativeRotation().Quaternion();
	BaseWorldRotation = TargetComponent->GetComponentQuat();
	bHasCachedBaseRotation = true;
}

void UUOUPourRotationReactionComponent::CacheBaseMovementLocationIfNeeded()
{
	if (!bDriveMovementFromRotation || bHasCachedBaseMovementLocation)
	{
		return;
	}

	USceneComponent* TargetComponent = ResolveMovementTargetComponent();
	if (TargetComponent == nullptr)
	{
		return;
	}

	BaseRelativeMovementLocation = TargetComponent->GetRelativeLocation();
	BaseWorldMovementLocation = TargetComponent->GetComponentLocation();
	bHasCachedBaseMovementLocation = true;
}

void UUOUPourRotationReactionComponent::SetTargetRotationAngle(float NewTargetAngleDegrees)
{
	TargetAngleDegrees = ClampRotationAngle(NewTargetAngleDegrees);
	if (bUseRotationInterpolation)
	{
		return;
	}

	CurrentAppliedAngleDegrees = TargetAngleDegrees;
	ApplyRotationAngle(CurrentAppliedAngleDegrees);
}

void UUOUPourRotationReactionComponent::UpdateInterpolatedRotation(float DeltaTime)
{
	if (!bUseRotationInterpolation)
	{
		return;
	}

	const float Duration = FMath::Max(DeltaTime, 0.0f);
	const float RotationSpeed = FMath::Max(RotationSpeedDegreesPerSecond, 0.0f);
	if (Duration <= 0.0f || RotationSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float NextAngle = FMath::FInterpConstantTo(
		CurrentAppliedAngleDegrees,
		TargetAngleDegrees,
		Duration,
		RotationSpeed);
	if (FMath::IsNearlyEqual(NextAngle, CurrentAppliedAngleDegrees, KINDA_SMALL_NUMBER))
	{
		return;
	}

	CurrentAppliedAngleDegrees = NextAngle;
	ApplyRotationAngle(CurrentAppliedAngleDegrees);
}

void UUOUPourRotationReactionComponent::ApplyRotationAngle(float AngleDegrees)
{
	USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	const FVector SafeAxis = RotationAxis.GetSafeNormal();
	if (TargetComponent != nullptr && !SafeAxis.IsNearlyZero())
	{
		CacheBaseRotationIfNeeded();

		const FQuat RotationDelta(SafeAxis, FMath::DegreesToRadians(AngleDegrees));
		if (RotationSpace == EUOUPourRotationReactionSpace::World)
		{
			TargetComponent->SetWorldRotation((RotationDelta * BaseWorldRotation).Rotator());
		}
		else
		{
			TargetComponent->SetRelativeRotation((BaseRelativeRotation * RotationDelta).Rotator());
		}
	}

	ApplyDrivenMovement(AngleDegrees);
}

void UUOUPourRotationReactionComponent::ApplyDrivenMovement(float AngleDegrees)
{
	if (!bDriveMovementFromRotation)
	{
		return;
	}

	USceneComponent* TargetComponent = ResolveMovementTargetComponent();
	if (TargetComponent == nullptr)
	{
		return;
	}

	const FVector SafeAxis = MovementAxis.GetSafeNormal();
	if (SafeAxis.IsNearlyZero())
	{
		return;
	}

	CacheBaseMovementLocationIfNeeded();

	float MovementDistance = AngleDegrees * DistancePerRotationDegree;
	if (bClampMovementDistance)
	{
		const float MinDistance = FMath::Min(MinMovementDistance, MaxMovementDistance);
		const float MaxDistance = FMath::Max(MinMovementDistance, MaxMovementDistance);
		MovementDistance = FMath::Clamp(MovementDistance, MinDistance, MaxDistance);
	}

	CurrentMovementDistance = MovementDistance;

	if (MovementSpace == EUOUPourRotationDrivenMovementSpace::World)
	{
		TargetComponent->SetWorldLocation(BaseWorldMovementLocation + SafeAxis * MovementDistance);
		return;
	}

	TargetComponent->SetRelativeLocation(BaseRelativeMovementLocation + SafeAxis * MovementDistance);
}

float UUOUPourRotationReactionComponent::ResolveRotationSign(const FUOUPourInputContext& PourContext) const
{
	switch (RotationDirectionMode)
	{
	case EUOUPourRotationDirectionMode::FixedNegative:
		return -1.0f;
	case EUOUPourRotationDirectionMode::ByPourTorque:
		return ResolvePourTorqueRotationSign(PourContext);
	case EUOUPourRotationDirectionMode::FixedPositive:
	default:
		return 1.0f;
	}
}

float UUOUPourRotationReactionComponent::ResolvePourTorqueRotationSign(const FUOUPourInputContext& PourContext) const
{
	if (!PourContext.bHasValidWorldLocation || PourContext.WorldDirection.IsNearlyZero())
	{
		return 0.0f;
	}

	const USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	const USceneComponent* CenterComponent = ResolveTorqueCenterComponent(TargetComponent);
	if (CenterComponent == nullptr)
	{
		return 0.0f;
	}

	const FVector AxisWorld = ResolveWorldRotationAxis(TargetComponent);
	const FVector Radius = PourContext.WorldLocation - CenterComponent->GetComponentLocation();
	const FVector ForceDirection = PourContext.WorldDirection.GetSafeNormal();
	if (AxisWorld.IsNearlyZero() || Radius.IsNearlyZero() || ForceDirection.IsNearlyZero())
	{
		return 0.0f;
	}

	const float TorqueAlongAxis = FVector::DotProduct(FVector::CrossProduct(Radius, ForceDirection), AxisWorld.GetSafeNormal());
	if (FMath::Abs(TorqueAlongAxis) <= FMath::Max(TorqueDeadZone, 0.0f))
	{
		return 0.0f;
	}

	const float TorqueSign = FMath::Sign(TorqueAlongAxis);
	return bInvertPourTorqueDirection ? -TorqueSign : TorqueSign;
}

FVector UUOUPourRotationReactionComponent::ResolveWorldRotationAxis(const USceneComponent* TargetComponent) const
{
	const FVector SafeAxis = RotationAxis.GetSafeNormal();
	if (SafeAxis.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	if (RotationSpace == EUOUPourRotationReactionSpace::World || TargetComponent == nullptr)
	{
		return SafeAxis;
	}

	return TargetComponent->GetComponentTransform().TransformVectorNoScale(SafeAxis).GetSafeNormal();
}

float UUOUPourRotationReactionComponent::ClampRotationAngle(float AngleDegrees) const
{
	if (!bClampRotationAngle)
	{
		return AngleDegrees;
	}

	const float MinAngle = FMath::Min(MinRotationAngleDegrees, MaxRotationAngleDegrees);
	const float MaxAngle = FMath::Max(MinRotationAngleDegrees, MaxRotationAngleDegrees);
	return FMath::Clamp(AngleDegrees, MinAngle, MaxAngle);
}
