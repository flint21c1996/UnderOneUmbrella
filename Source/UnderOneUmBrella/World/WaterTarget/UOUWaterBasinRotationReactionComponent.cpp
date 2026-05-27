// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinRotationReactionComponent.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

namespace
{
	constexpr float MinRotationReactionWorldUnitsPerTile = 1.0f;

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

	FVector ProjectDirectionOnRotationPlane(const FVector& Direction, const FVector& AxisWorld)
	{
		const FVector SafeAxis = AxisWorld.GetSafeNormal();
		if (Direction.IsNearlyZero() || SafeAxis.IsNearlyZero())
		{
			return FVector::ZeroVector;
		}

		return (Direction - SafeAxis * FVector::DotProduct(Direction, SafeAxis)).GetSafeNormal();
	}

	float CalculateInputSideValueByCross(
		const FVector& Center,
		const FVector& InputLocation,
		const FVector& ForwardDirection,
		const FVector& AxisWorld)
	{
		const FVector SafeAxis = AxisWorld.GetSafeNormal();
		const FVector PlanarForwardDirection = ProjectDirectionOnRotationPlane(ForwardDirection, SafeAxis);
		const FVector PlanarInputDirection = ProjectDirectionOnRotationPlane(InputLocation - Center, SafeAxis);
		if (PlanarForwardDirection.IsNearlyZero() || PlanarInputDirection.IsNearlyZero() || SafeAxis.IsNearlyZero())
		{
			return 0.0f;
		}

		return FVector::DotProduct(SafeAxis, FVector::CrossProduct(PlanarForwardDirection, PlanarInputDirection));
	}

	void GetWaterInputTargets(
		const UUOUWaterBasinTargetComponent* Target,
		bool bApplyToConnectedGroup,
		TArray<UUOUWaterBasinTargetComponent*>& OutTargets)
	{
		OutTargets.Reset();
		if (!IsValid(Target))
		{
			return;
		}

		if (bApplyToConnectedGroup)
		{
			Target->GetConnectedGroup(OutTargets);
			return;
		}

		OutTargets.Add(const_cast<UUOUWaterBasinTargetComponent*>(Target));
	}

	float GetWaterInputTotalCapacity(const UUOUWaterBasinTargetComponent* Target, bool bApplyToConnectedGroup)
	{
		if (!IsValid(Target))
		{
			return 0.0f;
		}

		if (bApplyToConnectedGroup)
		{
			return Target->GetConnectedGroupDebugData().TotalCapacity;
		}

		return Target->GetCapacity();
	}

	float GetWaterInputTotalSurfaceArea(const UUOUWaterBasinTargetComponent* Target, bool bApplyToConnectedGroup)
	{
		TArray<UUOUWaterBasinTargetComponent*> Targets;
		GetWaterInputTargets(Target, bApplyToConnectedGroup, Targets);

		float TotalSurfaceArea = 0.0f;
		for (const UUOUWaterBasinTargetComponent* WaterTarget : Targets)
		{
			if (IsValid(WaterTarget))
			{
				TotalSurfaceArea += WaterTarget->GetSurfaceArea();
			}
		}

		return TotalSurfaceArea;
	}

	bool IsWaterInputTargetAlreadyFull(
		const UUOUWaterBasinTargetComponent* Target,
		const FUOUWaterBasinInputContext& InputContext,
		float FillRatioTolerance)
	{
		if (!IsValid(Target))
		{
			return false;
		}

		const float SafeTolerance = FMath::Max(FillRatioTolerance, KINDA_SMALL_NUMBER);
		const float FillRatio = InputContext.bApplyToConnectedGroup
			? Target->GetConnectedGroupDebugData().FillRatio
			: Target->CurrentFillRatio;
		return FillRatio >= 1.0f - SafeTolerance;
	}

	float ResolveAttemptedWaterInputVolumeDelta(
		const UUOUWaterBasinTargetComponent* Target,
		const FUOUWaterBasinInputContext& InputContext)
	{
		if (!IsValid(Target))
		{
			return 0.0f;
		}

		const float Duration = FMath::Max(InputContext.Duration, 0.0f);
		const bool bApplyToConnectedGroup = InputContext.bApplyToConnectedGroup;
		switch (Target->PouredWaterFillMode)
		{
		case EUOUWaterBasinPouredWaterFillMode::FillRatio:
		{
			const float RatioDelta = FMath::Max(Target->PouredWaterFillRatioPerSecond, 0.0f) * Duration;
			return GetWaterInputTotalCapacity(Target, bApplyToConnectedGroup) * RatioDelta;
		}

		case EUOUWaterBasinPouredWaterFillMode::WaterDepth:
		{
			const float DepthDelta = FMath::Max(Target->PouredWaterDepthPerSecond, 0.0f) * Duration;
			return GetWaterInputTotalSurfaceArea(Target, bApplyToConnectedGroup) * DepthDelta;
		}

		case EUOUWaterBasinPouredWaterFillMode::SurfaceWorldZ:
		{
			const float SurfaceDeltaWorld = FMath::Max(Target->PouredWaterSurfaceWorldZPerSecond, 0.0f) * Duration;
			const float DepthDelta = SurfaceDeltaWorld / FMath::Max(Target->WorldUnitsPerTile, MinRotationReactionWorldUnitsPerTile);
			return GetWaterInputTotalSurfaceArea(Target, bApplyToConnectedGroup) * DepthDelta;
		}

		case EUOUWaterBasinPouredWaterFillMode::Volume:
		default:
			return FMath::Max(InputContext.Volume, 0.0f);
		}
	}

	float ResolveAttemptedReactionValueDelta(
		const UUOUWaterBasinTargetComponent* Target,
		const FUOUWaterBasinInputContext& InputContext,
		EUOUWaterBasinReactionValueSource ValueSource)
	{
		const float VolumeDelta = ResolveAttemptedWaterInputVolumeDelta(Target, InputContext);
		if (VolumeDelta <= 0.0f)
		{
			return 0.0f;
		}

		const bool bApplyToConnectedGroup = InputContext.bApplyToConnectedGroup;
		switch (ValueSource)
		{
		case EUOUWaterBasinReactionValueSource::WaterVolume:
			return VolumeDelta;

		case EUOUWaterBasinReactionValueSource::WaterFillRatio:
		{
			const float Capacity = GetWaterInputTotalCapacity(Target, bApplyToConnectedGroup);
			return Capacity > KINDA_SMALL_NUMBER ? VolumeDelta / Capacity : 0.0f;
		}

		case EUOUWaterBasinReactionValueSource::WaterDepth:
		{
			const float SurfaceArea = GetWaterInputTotalSurfaceArea(Target, bApplyToConnectedGroup);
			return SurfaceArea > KINDA_SMALL_NUMBER ? VolumeDelta / SurfaceArea : 0.0f;
		}

		case EUOUWaterBasinReactionValueSource::WaterDepthWorld:
		case EUOUWaterBasinReactionValueSource::WaterSurfaceWorldZ:
		{
			const float SurfaceArea = GetWaterInputTotalSurfaceArea(Target, bApplyToConnectedGroup);
			const float DepthDelta = SurfaceArea > KINDA_SMALL_NUMBER ? VolumeDelta / SurfaceArea : 0.0f;
			return DepthDelta * FMath::Max(Target->WorldUnitsPerTile, MinRotationReactionWorldUnitsPerTile);
		}

		case EUOUWaterBasinReactionValueSource::PlatformWorldZ:
		default:
			return 0.0f;
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
	DrawInputSideDebug();
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
	const bool bRotateByEveryWaterInput = RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput;
	const bool bRotateByFullWaterInput = RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnIncrease && bRotateOnFullWaterInput;
	if ((!bRotateByEveryWaterInput && !bRotateByFullWaterInput)
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

	if (bRotateByFullWaterInput)
	{
		if (!IsWaterInputTargetAlreadyFull(Target, InputContext, Tolerance))
		{
			return;
		}

		const float AttemptedValueDelta = ResolveAttemptedReactionValueDelta(Target, InputContext, ValueSource);
		if (AttemptedValueDelta <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		SetTargetRotationAngle(TargetAngleDegrees + (AttemptedValueDelta * DegreesPerValueUnit));
		return;
	}

	const float RotationSign = ResolveInputRotationSign(InputContext);
	CacheInputSideDebug(InputContext, RotationSign);
	if (FMath::IsNearlyZero(RotationSign))
	{
		return;
	}

	SetTargetRotationAngle(TargetAngleDegrees + (InputContext.Volume * DegreesPerInputVolume * RotationSign));
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

const USceneComponent* UUOUWaterBasinRotationReactionComponent::ResolveInputSideCenterComponent(const USceneComponent* TargetComponent) const
{
	if (IsValid(InputSideCenterComponent))
	{
		return InputSideCenterComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindSceneComponentByNameOrTag(InputSideCenterComponentName))
	{
		return FoundComponent;
	}

	return IsValid(TargetComponent) ? TargetComponent : nullptr;
}

const USceneComponent* UUOUWaterBasinRotationReactionComponent::ResolveInputSideForwardReferenceComponent(const USceneComponent* CenterComponent) const
{
	if (IsValid(InputSideForwardReferenceComponent))
	{
		return InputSideForwardReferenceComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindSceneComponentByNameOrTag(InputSideForwardReferenceComponentName))
	{
		return FoundComponent;
	}

	return IsValid(CenterComponent) ? CenterComponent : nullptr;
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

void UUOUWaterBasinRotationReactionComponent::CacheInputSideDebug(const FUOUWaterBasinInputContext& InputContext, float RotationSign)
{
	bHasLastInputSideDebug = true;
	LastInputSideDebugWorldLocation = InputContext.WorldLocation;
	LastInputSideDebugRotationSign = RotationSign;
	LastInputSideDebugVolume = InputContext.Volume;
	LastInputSideDebugSource = InputContext.Source;
}

void UUOUWaterBasinRotationReactionComponent::DrawInputSideDebug() const
{
	if (!bDrawInputSideDebug)
	{
		return;
	}

	UWorld* World = GetWorld();
	const USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	if (World == nullptr || TargetComponent == nullptr)
	{
		return;
	}

	const float DrawScale = FMath::Max(InputSideDebugDrawScale, 0.0f);
	const float SphereRadius = FMath::Max(InputSideDebugSphereRadius, 0.0f);
	const float Thickness = FMath::Max(InputSideDebugThickness, 0.0f);
	const float ArrowSize = FMath::Max(DrawScale * 0.2f, 8.0f);
	const float DrawDuration = 0.05f;
	const USceneComponent* CenterComponent = ResolveInputSideCenterComponent(TargetComponent);
	const USceneComponent* ForwardReferenceComponent = ResolveInputSideForwardReferenceComponent(CenterComponent);
	const FVector Center = CenterComponent != nullptr ? CenterComponent->GetComponentLocation() : TargetComponent->GetComponentLocation();
	const FVector AxisWorld = ResolveWorldRotationAxis(CenterComponent != nullptr ? CenterComponent : TargetComponent);
	const FVector ForwardDirection = ResolveInputSideForwardWorldDirection(ForwardReferenceComponent);
	const FVector SideDirection = ResolveInputSideRightWorldDirection(ForwardReferenceComponent, CenterComponent != nullptr ? CenterComponent : TargetComponent);

	DrawDebugSphere(World, Center, SphereRadius, 16, FColor::Yellow, false, DrawDuration, 0, Thickness);

	if (!AxisWorld.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			World,
			Center,
			Center + AxisWorld * DrawScale,
			ArrowSize,
			FColor::Cyan,
			false,
			DrawDuration,
			0,
			Thickness);
	}

	if (!ForwardDirection.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			World,
			Center,
			Center + ForwardDirection * DrawScale,
			ArrowSize,
			FColor::Magenta,
			false,
			DrawDuration,
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
			DrawDuration,
			0,
			Thickness);
	}

	if (bDrawInputSideDebugLabel)
	{
		DrawDebugString(
			World,
			Center + FVector(0.0f, 0.0f, SphereRadius * 2.0f),
			TEXT("Input Side Center / Axis / Forward / Right"),
			nullptr,
			FColor::Yellow,
			DrawDuration,
			true);
	}

	if (!bHasLastInputSideDebug)
	{
		return;
	}

	const FVector InputLocation = LastInputSideDebugWorldLocation.IsNearlyZero() ? Center : LastInputSideDebugWorldLocation;
	const FVector InputOffset = InputLocation - Center;
	const FVector PlanarInputOffset = AxisWorld.IsNearlyZero()
		? InputOffset
		: InputOffset - AxisWorld * FVector::DotProduct(InputOffset, AxisWorld);
	const FVector InputPositionDirection = PlanarInputOffset.GetSafeNormal();
	const float SideValue = CalculateInputSideValueByCross(Center, InputLocation, ForwardDirection, AxisWorld);
	const float DeadZone = FMath::Max(InputSideDeadZone, 0.0f);
	const TCHAR* SideName = TEXT("Center");
	if (SideValue > DeadZone)
	{
		SideName = TEXT("Right");
	}
	else if (SideValue < -DeadZone)
	{
		SideName = TEXT("Left");
	}
	const FColor ResponseColor = LastInputSideDebugRotationSign > 0.0f
		? FColor::Green
		: (LastInputSideDebugRotationSign < 0.0f ? FColor::Red : FColor::White);

	DrawDebugSphere(World, InputLocation, SphereRadius, 16, FColor::Orange, false, DrawDuration, 0, Thickness);
	DrawDebugLine(World, Center, InputLocation, FColor::Yellow, false, DrawDuration, 0, Thickness);

	if (!InputPositionDirection.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			World,
			Center,
			Center + InputPositionDirection * DrawScale,
			ArrowSize,
			FColor::Orange,
			false,
			DrawDuration,
			0,
			Thickness);
	}

	if (!SideDirection.IsNearlyZero() && !FMath::IsNearlyZero(SideValue))
	{
		DrawDebugDirectionalArrow(
			World,
			Center,
			Center + SideDirection * SideValue * DrawScale,
			ArrowSize,
			ResponseColor,
			false,
			DrawDuration,
			0,
			Thickness);
	}

	if (!SideDirection.IsNearlyZero() && !FMath::IsNearlyZero(LastInputSideDebugRotationSign))
	{
		const FVector ResponseDirection = SideDirection * LastInputSideDebugRotationSign;
		DrawDebugDirectionalArrow(
			World,
			InputLocation,
			InputLocation + ResponseDirection * (DrawScale * 0.5f),
			ArrowSize,
			ResponseColor,
			false,
			DrawDuration,
			0,
			Thickness);
	}

	if (bDrawInputSideDebugLabel)
	{
		const FString DebugText = FString::Printf(
			TEXT("Water Input\nSource: %s\nVolume: %.2f\nSide: %s %.2f\nSign: %.0f"),
			GetInputSourceDebugName(LastInputSideDebugSource),
			LastInputSideDebugVolume,
			SideName,
			SideValue,
			LastInputSideDebugRotationSign);
		DrawDebugString(
			World,
			InputLocation + FVector(0.0f, 0.0f, SphereRadius * 2.0f),
			DebugText,
			nullptr,
			ResponseColor,
			DrawDuration,
			true);
	}
}

float UUOUWaterBasinRotationReactionComponent::ResolveInputRotationSign(const FUOUWaterBasinInputContext& InputContext) const
{
	switch (GetInputSidePolicy(InputContext.Source))
	{
	case EUOUWaterBasinRotationInputSidePolicy::Ignore:
		return 0.0f;
	case EUOUWaterBasinRotationInputSidePolicy::FixedNegative:
		return -1.0f;
	case EUOUWaterBasinRotationInputSidePolicy::ByInputSide:
		return ResolveInputSideSign(InputContext);
	case EUOUWaterBasinRotationInputSidePolicy::FixedPositive:
	default:
		return 1.0f;
	}
}

float UUOUWaterBasinRotationReactionComponent::ResolveInputSideSign(const FUOUWaterBasinInputContext& InputContext) const
{
	const float DeadZone = FMath::Max(InputSideDeadZone, 0.0f);
	const USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	const USceneComponent* CenterComponent = ResolveInputSideCenterComponent(TargetComponent);
	const USceneComponent* ForwardReferenceComponent = ResolveInputSideForwardReferenceComponent(CenterComponent);
	const USceneComponent* AxisComponent = CenterComponent != nullptr ? CenterComponent : TargetComponent;
	const FVector Center = CenterComponent != nullptr
		? CenterComponent->GetComponentLocation()
		: (TargetComponent != nullptr ? TargetComponent->GetComponentLocation() : FVector::ZeroVector);
	const FVector ForwardDirection = ResolveInputSideForwardWorldDirection(ForwardReferenceComponent);
	const FVector AxisWorld = ResolveWorldRotationAxis(AxisComponent);
	const float SideSignValue = CalculateInputSideValueByCross(Center, InputContext.WorldLocation, ForwardDirection, AxisWorld);
	if (FMath::Abs(SideSignValue) <= DeadZone)
	{
		return 0.0f;
	}

	const float SideSign = FMath::Sign(SideSignValue);
	return bRightSideInputRotatesPositive ? SideSign : -SideSign;
}

FVector UUOUWaterBasinRotationReactionComponent::ResolveInputSideForwardWorldDirection(const USceneComponent* ReferenceComponent) const
{
	if (ReferenceComponent != nullptr)
	{
		return ReferenceComponent->GetForwardVector().GetSafeNormal();
	}

	return FVector::ZeroVector;
}

FVector UUOUWaterBasinRotationReactionComponent::ResolveInputSideRightWorldDirection(
	const USceneComponent* ReferenceComponent,
	const USceneComponent* TargetComponent) const
{
	const FVector ForwardDirection = ResolveInputSideForwardWorldDirection(ReferenceComponent);
	if (ForwardDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector AxisWorld = ResolveWorldRotationAxis(TargetComponent);
	if (AxisWorld.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector PlanarForwardDirection = ProjectDirectionOnRotationPlane(ForwardDirection, AxisWorld);
	if (PlanarForwardDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	return FVector::CrossProduct(AxisWorld, PlanarForwardDirection).GetSafeNormal();
}

EUOUWaterBasinRotationInputSidePolicy UUOUWaterBasinRotationReactionComponent::GetInputSidePolicy(EUOUWaterBasinInputSource Source) const
{
	switch (Source)
	{
	case EUOUWaterBasinInputSource::PlayerPour:
		return PlayerPourInputSidePolicy;
	case EUOUWaterBasinInputSource::Rain:
		return RainInputSidePolicy;
	case EUOUWaterBasinInputSource::Script:
	case EUOUWaterBasinInputSource::Unknown:
	default:
		return ScriptInputSidePolicy;
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
