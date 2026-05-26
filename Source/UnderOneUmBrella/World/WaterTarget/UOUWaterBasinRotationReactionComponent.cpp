// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinRotationReactionComponent.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

namespace
{
	const TCHAR* GetInputSourceDebugName(EUOUWaterBasinInputSource Source)
	{
		switch (Source)
		{
		case EUOUWaterBasinInputSource::PlayerPour:
			return TEXT("PlayerPour");
		case EUOUWaterBasinInputSource::Rain:
			return TEXT("Rain");
		case EUOUWaterBasinInputSource::Script:
			return TEXT("Script");
		case EUOUWaterBasinInputSource::Unknown:
		default:
			return TEXT("Unknown");
		}
	}
}

UUOUWaterBasinRotationReactionComponent::UUOUWaterBasinRotationReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	ValueSource = EUOUWaterBasinReactionValueSource::WaterFillRatio;
	CompareMode = EUOUWaterBasinReactionCompareMode::GreaterOrEqual;
	ThresholdValue = 0.0f;
}

void UUOUWaterBasinRotationReactionComponent::OnRegister()
{
	Super::OnRegister();

	CacheBaseRotationIfNeeded();
}

void UUOUWaterBasinRotationReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheBaseRotationIfNeeded();
	BindToWaterInputTarget();
}

void UUOUWaterBasinRotationReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromWaterInputTarget();

	Super::EndPlay(EndPlayReason);
}

void UUOUWaterBasinRotationReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateInterpolatedRotation(DeltaTime);
	DrawInputDirectionDebug();
}

void UUOUWaterBasinRotationReactionComponent::ResetRotationReaction(bool bResetObservedValue, bool bApplyBaseRotation)
{
	bHasCachedBaseRotation = false;
	CacheBaseRotationIfNeeded();

	if (bResetObservedValue)
	{
		bHasObservedValue = false;
		LastObservedValue = 0.0f;
	}

	TargetAngleDegrees = 0.0f;
	CurrentAppliedAngleDegrees = 0.0f;
	if (bApplyBaseRotation)
	{
		ApplyRotationAngle(CurrentAppliedAngleDegrees);
	}
}

void UUOUWaterBasinRotationReactionComponent::OnWaterBasinReactionStateUpdated_Implementation(const FUOUWaterBasinReactionContext& Context)
{
	CacheBaseRotationIfNeeded();
	BindToWaterInputTarget();

	const float CurrentValue = Context.CurrentValue;
	if (RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput)
	{
		LastObservedValue = CurrentValue;
		bHasObservedValue = true;
		return;
	}

	if (!bHasObservedValue)
	{
		bHasObservedValue = true;
		LastObservedValue = CurrentValue;

		if (RotationMode == EUOUWaterBasinRotationReactionMode::AbsoluteByValue)
		{
			SetTargetRotationAngle(CurrentValue * DegreesPerValueUnit);
		}
		return;
	}

	const bool bCanApplyRotation = !bRequireConditionSatisfied || Context.bIsSatisfied;
	if (!bCanApplyRotation)
	{
		LastObservedValue = CurrentValue;
		return;
	}

	if (RotationMode == EUOUWaterBasinRotationReactionMode::AbsoluteByValue)
	{
		SetTargetRotationAngle(CurrentValue * DegreesPerValueUnit);
		LastObservedValue = CurrentValue;
		return;
	}

	const float DeltaValue = CurrentValue - LastObservedValue;
	LastObservedValue = CurrentValue;
	if (DeltaValue <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	SetTargetRotationAngle(TargetAngleDegrees + (DeltaValue * DegreesPerValueUnit));
}

void UUOUWaterBasinRotationReactionComponent::HandleWaterInputReceived(UUOUWaterBasinTargetComponent* Target, const FUOUWaterBasinInputContext& InputContext)
{
	if (RotationMode != EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput
		|| Target == nullptr
		|| InputContext.Volume <= 0.0f)
	{
		return;
	}

	const FUOUWaterBasinReactionContext Context = GetLastReactionContext();
	if (bRequireConditionSatisfied && !Context.bIsSatisfied)
	{
		return;
	}

	const float DirectionSign = ResolveInputRotationSign(InputContext);
	CacheInputDirectionDebug(InputContext, DirectionSign);
	if (FMath::IsNearlyZero(DirectionSign))
	{
		return;
	}

	SetTargetRotationAngle(TargetAngleDegrees + (InputContext.Volume * DegreesPerInputVolume * DirectionSign));
}

USceneComponent* UUOUWaterBasinRotationReactionComponent::ResolveRotationTargetComponent() const
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

USceneComponent* UUOUWaterBasinRotationReactionComponent::FindRotationTargetComponent() const
{
	return FindSceneComponentByNameOrTag(RotationTargetComponentName);
}

const USceneComponent* UUOUWaterBasinRotationReactionComponent::ResolveInputDirectionReferenceComponent(const USceneComponent* TargetComponent) const
{
	if (IsValid(InputDirectionReferenceComponent))
	{
		return InputDirectionReferenceComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindSceneComponentByNameOrTag(InputDirectionReferenceComponentName))
	{
		return FoundComponent;
	}

	return bUseRotationTargetAsInputDirectionReferenceWhenMissing && IsValid(TargetComponent) ? TargetComponent : nullptr;
}

USceneComponent* UUOUWaterBasinRotationReactionComponent::FindSceneComponentByNameOrTag(FName ComponentName) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || ComponentName.IsNone())
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

void UUOUWaterBasinRotationReactionComponent::BindToWaterInputTarget()
{
	UUOUWaterBasinTargetComponent* TargetComponent = nullptr;
	const FUOUWaterBasinReactionContext Context = GetLastReactionContext();
	if (IsValid(Context.WaterBasinTarget))
	{
		TargetComponent = Context.WaterBasinTarget;
	}
	else if (IsValid(Context.WaterTileActor))
	{
		TargetComponent = Context.WaterTileActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
	}
	else if (IsValid(TargetWaterTileActor))
	{
		TargetComponent = TargetWaterTileActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
	}

	if (BoundInputWaterBasinTarget == TargetComponent)
	{
		return;
	}

	UnbindFromWaterInputTarget();

	BoundInputWaterBasinTarget = TargetComponent;
	if (BoundInputWaterBasinTarget)
	{
		BoundInputWaterBasinTarget->OnWaterInputReceived.AddUniqueDynamic(this, &UUOUWaterBasinRotationReactionComponent::HandleWaterInputReceived);
	}
}

void UUOUWaterBasinRotationReactionComponent::UnbindFromWaterInputTarget()
{
	if (BoundInputWaterBasinTarget)
	{
		BoundInputWaterBasinTarget->OnWaterInputReceived.RemoveDynamic(this, &UUOUWaterBasinRotationReactionComponent::HandleWaterInputReceived);
		BoundInputWaterBasinTarget = nullptr;
	}
}

void UUOUWaterBasinRotationReactionComponent::CacheBaseRotationIfNeeded()
{
	if (bHasCachedBaseRotation)
	{
		return;
	}

	USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	if (!TargetComponent)
	{
		return;
	}

	BaseRelativeRotation = TargetComponent->GetRelativeRotation().Quaternion();
	BaseWorldRotation = TargetComponent->GetComponentQuat();
	bHasCachedBaseRotation = true;
}

void UUOUWaterBasinRotationReactionComponent::SetTargetRotationAngle(float NewTargetAngleDegrees)
{
	TargetAngleDegrees = ClampRotationAngle(NewTargetAngleDegrees);
	if (bUseRotationInterpolation)
	{
		return;
	}

	CurrentAppliedAngleDegrees = TargetAngleDegrees;
	ApplyRotationAngle(CurrentAppliedAngleDegrees);
}

void UUOUWaterBasinRotationReactionComponent::UpdateInterpolatedRotation(float DeltaTime)
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

void UUOUWaterBasinRotationReactionComponent::ApplyRotationAngle(float AngleDegrees)
{
	USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	if (!TargetComponent)
	{
		return;
	}

	const FVector SafeAxis = RotationAxis.GetSafeNormal();
	if (SafeAxis.IsNearlyZero())
	{
		return;
	}

	CacheBaseRotationIfNeeded();

	const FQuat RotationDelta(SafeAxis, FMath::DegreesToRadians(AngleDegrees));
	if (RotationSpace == EUOUWaterBasinRotationReactionSpace::World)
	{
		TargetComponent->SetWorldRotation((RotationDelta * BaseWorldRotation).Rotator());
		return;
	}

	TargetComponent->SetRelativeRotation((BaseRelativeRotation * RotationDelta).Rotator());
}

void UUOUWaterBasinRotationReactionComponent::CacheInputDirectionDebug(const FUOUWaterBasinInputContext& InputContext, float DirectionSign)
{
	bHasLastInputDirectionDebug = true;
	LastInputDebugWorldLocation = InputContext.WorldLocation;
	LastInputDebugWorldDirection = InputContext.WorldDirection.GetSafeNormal();
	LastInputDebugRotationSign = DirectionSign;
	LastInputDebugVolume = InputContext.Volume;
	LastInputDebugSource = InputContext.Source;
}

void UUOUWaterBasinRotationReactionComponent::DrawInputDirectionDebug() const
{
	if (!bDrawInputDirectionDebug)
	{
		return;
	}

	UWorld* World = GetWorld();
	const USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	if (World == nullptr || TargetComponent == nullptr)
	{
		return;
	}

	const float DrawScale = FMath::Max(InputDirectionDebugDrawScale, 0.0f);
	const float SphereRadius = FMath::Max(InputDirectionDebugSphereRadius, 0.0f);
	const float Thickness = FMath::Max(InputDirectionDebugThickness, 0.0f);
	const float ArrowSize = FMath::Max(DrawScale * 0.2f, 8.0f);
	const USceneComponent* ReferenceComponent = ResolveInputDirectionReferenceComponent(TargetComponent);
	const FVector Center = ReferenceComponent != nullptr ? ReferenceComponent->GetComponentLocation() : TargetComponent->GetComponentLocation();
	const FVector AxisWorld = ResolveWorldRotationAxis(ReferenceComponent != nullptr ? ReferenceComponent : TargetComponent);
	const FVector ReferenceDirection = ResolveInputDirectionReferenceWorldDirection(ReferenceComponent);
	const FVector SideDirection = ResolveInputDirectionSideWorldDirection(ReferenceComponent, TargetComponent);

	DrawDebugSphere(World, Center, SphereRadius, 16, FColor::Yellow, false, 0.0f, 0, Thickness);

	if (!AxisWorld.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			World,
			Center,
			Center + AxisWorld * DrawScale,
			ArrowSize,
			FColor::Cyan,
			false,
			0.0f,
			0,
			Thickness);
	}

	if (!ReferenceDirection.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			World,
			Center,
			Center + ReferenceDirection * DrawScale,
			ArrowSize,
			FColor::Magenta,
			false,
			0.0f,
			0,
			Thickness);
	}

	if (!SideDirection.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			World,
			Center,
			Center + SideDirection * DrawScale,
			ArrowSize,
			FColor::Purple,
			false,
			0.0f,
			0,
			Thickness);
	}

	if (bDrawInputDirectionDebugLabel)
	{
		DrawDebugString(
			World,
			Center + FVector(0.0f, 0.0f, SphereRadius * 2.0f),
			TEXT("Input Direction Reference / Axis / Right"),
			nullptr,
			FColor::Yellow,
			0.0f,
			true);
	}

	if (!bHasLastInputDirectionDebug)
	{
		return;
	}

	const FVector InputLocation = LastInputDebugWorldLocation.IsNearlyZero() ? Center : LastInputDebugWorldLocation;
	const FVector InputDirection = LastInputDebugWorldDirection.IsNearlyZero()
		? FVector::ForwardVector
		: LastInputDebugWorldDirection;
	const FColor ResponseColor = LastInputDebugRotationSign > 0.0f
		? FColor::Green
		: (LastInputDebugRotationSign < 0.0f ? FColor::Red : FColor::White);

	DrawDebugSphere(World, InputLocation, SphereRadius, 16, FColor::Orange, false, 0.0f, 0, Thickness);
	DrawDebugLine(World, Center, InputLocation, FColor::Yellow, false, 0.0f, 0, Thickness);
	DrawDebugDirectionalArrow(
		World,
		InputLocation,
		InputLocation + InputDirection * DrawScale,
		ArrowSize,
		FColor::Blue,
		false,
		0.0f,
		0,
		Thickness);

	if (!SideDirection.IsNearlyZero() && !FMath::IsNearlyZero(LastInputDebugRotationSign))
	{
		const FVector ResponseDirection = SideDirection * LastInputDebugRotationSign;
		DrawDebugDirectionalArrow(
			World,
			InputLocation,
			InputLocation + ResponseDirection * DrawScale,
			ArrowSize,
			ResponseColor,
			false,
			0.0f,
			0,
			Thickness);
	}

	if (bDrawInputDirectionDebugLabel)
	{
		const FString DebugText = FString::Printf(
			TEXT("Water Input\nSource: %s\nVolume: %.2f\nSign: %.0f"),
			GetInputSourceDebugName(LastInputDebugSource),
			LastInputDebugVolume,
			LastInputDebugRotationSign);
		DrawDebugString(
			World,
			InputLocation + FVector(0.0f, 0.0f, SphereRadius * 2.0f),
			DebugText,
			nullptr,
			ResponseColor,
			0.0f,
			true);
	}
}

float UUOUWaterBasinRotationReactionComponent::ResolveInputRotationSign(const FUOUWaterBasinInputContext& InputContext) const
{
	switch (GetInputDirectionPolicy(InputContext.Source))
	{
	case EUOUWaterBasinRotationInputDirectionPolicy::Ignore:
		return 0.0f;
	case EUOUWaterBasinRotationInputDirectionPolicy::FixedNegative:
		return -1.0f;
	case EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection:
		return ResolveInputDirectionSign(InputContext);
	case EUOUWaterBasinRotationInputDirectionPolicy::FixedPositive:
	default:
		return 1.0f;
	}
}

float UUOUWaterBasinRotationReactionComponent::ResolveInputDirectionSign(const FUOUWaterBasinInputContext& InputContext) const
{
	const float DeadZone = FMath::Max(InputDirectionDeadZone, 0.0f);
	const USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	const USceneComponent* ReferenceComponent = ResolveInputDirectionReferenceComponent(TargetComponent);
	const FVector Center = ReferenceComponent != nullptr
		? ReferenceComponent->GetComponentLocation()
		: (TargetComponent != nullptr ? TargetComponent->GetComponentLocation() : FVector::ZeroVector);
	const FVector SideDirection = ResolveInputDirectionSideWorldDirection(ReferenceComponent, TargetComponent);
	if (SideDirection.IsNearlyZero())
	{
		return 0.0f;
	}

	const FVector InputOffset = InputContext.WorldLocation - Center;
	const FVector AxisWorld = ResolveWorldRotationAxis(ReferenceComponent != nullptr ? ReferenceComponent : TargetComponent);
	const FVector PlanarInputOffset = AxisWorld.IsNearlyZero()
		? InputOffset
		: InputOffset - AxisWorld * FVector::DotProduct(InputOffset, AxisWorld);
	const FVector InputSideDirection = PlanarInputOffset.GetSafeNormal();
	if (InputSideDirection.IsNearlyZero())
	{
		return 0.0f;
	}

	const float SideSignValue = FVector::DotProduct(InputSideDirection, SideDirection);
	if (FMath::Abs(SideSignValue) > DeadZone)
	{
		return ApplyInputDirectionSignInversion(FMath::Sign(SideSignValue));
	}

	return 0.0f;
}

float UUOUWaterBasinRotationReactionComponent::ApplyInputDirectionSignInversion(float DirectionSign) const
{
	return bInvertInputDirectionRotationSign ? -DirectionSign : DirectionSign;
}

FVector UUOUWaterBasinRotationReactionComponent::ResolveInputDirectionReferenceWorldDirection(const USceneComponent* ReferenceComponent) const
{
	if (ReferenceComponent != nullptr)
	{
		return ReferenceComponent->GetForwardVector().GetSafeNormal();
	}

	return InputDirectionReferenceVector.GetSafeNormal();
}

FVector UUOUWaterBasinRotationReactionComponent::ResolveInputDirectionSideWorldDirection(
	const USceneComponent* ReferenceComponent,
	const USceneComponent* TargetComponent) const
{
	const FVector ForwardDirection = ResolveInputDirectionReferenceWorldDirection(ReferenceComponent);
	if (ForwardDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector AxisWorld = ResolveWorldRotationAxis(ReferenceComponent != nullptr ? ReferenceComponent : TargetComponent);
	if (AxisWorld.IsNearlyZero())
	{
		return ReferenceComponent != nullptr ? ReferenceComponent->GetRightVector().GetSafeNormal() : FVector::ZeroVector;
	}

	const FVector PlanarForwardDirection = (ForwardDirection - AxisWorld * FVector::DotProduct(ForwardDirection, AxisWorld)).GetSafeNormal();
	if (PlanarForwardDirection.IsNearlyZero())
	{
		return ReferenceComponent != nullptr ? ReferenceComponent->GetRightVector().GetSafeNormal() : FVector::ZeroVector;
	}

	return FVector::CrossProduct(AxisWorld, PlanarForwardDirection).GetSafeNormal();
}

EUOUWaterBasinRotationInputDirectionPolicy UUOUWaterBasinRotationReactionComponent::GetInputDirectionPolicy(EUOUWaterBasinInputSource Source) const
{
	switch (Source)
	{
	case EUOUWaterBasinInputSource::PlayerPour:
		return PlayerPourDirectionPolicy;
	case EUOUWaterBasinInputSource::Rain:
		return RainDirectionPolicy;
	case EUOUWaterBasinInputSource::Script:
	case EUOUWaterBasinInputSource::Unknown:
	default:
		return ScriptDirectionPolicy;
	}
}

FVector UUOUWaterBasinRotationReactionComponent::ResolveWorldRotationAxis(const USceneComponent* TargetComponent) const
{
	const FVector SafeAxis = RotationAxis.GetSafeNormal();
	if (SafeAxis.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	if (RotationSpace == EUOUWaterBasinRotationReactionSpace::World || TargetComponent == nullptr)
	{
		return SafeAxis;
	}

	return TargetComponent->GetComponentTransform().TransformVectorNoScale(SafeAxis).GetSafeNormal();
}

float UUOUWaterBasinRotationReactionComponent::ClampRotationAngle(float AngleDegrees) const
{
	if (!bClampRotationAngle)
	{
		return AngleDegrees;
	}

	const float MinAngle = FMath::Min(MinRotationAngleDegrees, MaxRotationAngleDegrees);
	const float MaxAngle = FMath::Max(MinRotationAngleDegrees, MaxRotationAngleDegrees);
	return FMath::Clamp(AngleDegrees, MinAngle, MaxAngle);
}
