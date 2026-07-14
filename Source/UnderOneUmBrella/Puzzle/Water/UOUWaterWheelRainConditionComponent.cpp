// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Water/UOUWaterWheelRainConditionComponent.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	float ClampSignedInput(float SignedInput)
	{
		return FMath::Clamp(SignedInput, -1.0f, 1.0f);
	}
}

UUOUWaterWheelRainConditionComponent::UUOUWaterWheelRainConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetAutoActivate(true);
}

void UUOUWaterWheelRainConditionComponent::OnRegister()
{
	Super::OnRegister();

	CacheBaseRotationIfNeeded();
}

void UUOUWaterWheelRainConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	Activate(true);
	SetComponentTickEnabled(true);
	SanitizeSettings();
	CacheBaseRotationIfNeeded();
	SetSatisfiedState(false, false);
}

void UUOUWaterWheelRainConditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	ConsumePendingRainInput();
	UpdateRotationSpeed(SafeDeltaTime);

	if (!FMath::IsNearlyZero(CurrentRotationSpeedDegreesPerSecond))
	{
		CurrentRotationAngleDegrees += CurrentRotationSpeedDegreesPerSecond * SafeDeltaTime;
		ApplyRotationAngle(CurrentRotationAngleDegrees);
	}

	if (FMath::Abs(CurrentRotationSpeedDegreesPerSecond) > StoppedSpeedThreshold)
	{
		bHasEverSpun = true;
	}

	RefreshStoppedCondition(SafeDeltaTime);
}

TArray<FString> UUOUWaterWheelRainConditionComponent::GetPuzzleDebugInfo_Implementation() const
{
	return {
		FString::Printf(
			TEXT("Water Wheel Stop: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(
			TEXT("Speed: %.1f / %.1f deg/s"),
			CurrentRotationSpeedDegreesPerSecond,
			TargetRotationSpeedDegreesPerSecond),
		FString::Printf(
			TEXT("Rain Input: %.2f, Strength: %.2f"),
			LastSignedRainInput,
			LastRainInputStrength),
		FString::Printf(
			TEXT("Pour Input: %.2f, Strength: %.2f"),
			LastSignedPouredWaterInput,
			LastPouredWaterInputStrength),
		FString::Printf(
			TEXT("Water Recent: %s / Ever Spun: %s"),
			bHasWaterInputRecently ? TEXT("Y") : TEXT("N"),
			bHasEverSpun ? TEXT("Y") : TEXT("N"))
	};
}

void UUOUWaterWheelRainConditionComponent::ReceiveRainInput(const FUOUWaterWheelRainInputContext& InputContext)
{
	if (!CanReceiveRainInput())
	{
		return;
	}

	const float SafeStrength = FMath::Max(0.0f, InputContext.Strength);
	if (SafeStrength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		LastRainInputTimestamp = World->GetTimeSeconds();
	}

	const float SignedContribution = CalculateSignedRainContribution(InputContext);
	PendingSignedRainInput += SignedContribution * SafeStrength;
	PendingRainInputWeight += SafeStrength;
	bHasPendingRainInput = true;
}

void UUOUWaterWheelRainConditionComponent::ReceivePouredWaterInput(const FUOUWaterWheelRainInputContext& InputContext)
{
	if (!CanReceivePouredWaterInput())
	{
		return;
	}

	const float SafeStrength = FMath::Max(0.0f, InputContext.Strength);
	if (SafeStrength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		LastPouredWaterInputTimestamp = World->GetTimeSeconds();
	}

	const float SignedContribution = CalculateSignedRainContribution(InputContext);
	PendingSignedPouredWaterInput += SignedContribution * SafeStrength;
	PendingPouredWaterInputWeight += SafeStrength;
	bHasPendingPouredWaterInput = true;
}

void UUOUWaterWheelRainConditionComponent::GetRainCatchSamples(TArray<FUOUWaterWheelRainCatchSample>& OutSamples) const
{
	OutSamples.Reset();

	const AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	for (const FUOUWaterWheelRainCatchPoint& CatchPoint : RainCatchPoints)
	{
		if (CatchPoint.Weight <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const UActorComponent* ReferencedComponent = CatchPoint.CatchComponentReference.GetComponent(const_cast<AActor*>(Owner));
		const USceneComponent* CatchSceneComponent = Cast<USceneComponent>(ReferencedComponent);
		if (CatchSceneComponent == nullptr)
		{
			CatchSceneComponent = ResolveRotationTargetComponent();
		}

		FUOUWaterWheelRainCatchSample Sample;
		Sample.WorldLocation = CatchSceneComponent != nullptr
			? CatchSceneComponent->GetComponentTransform().TransformPosition(CatchPoint.LocalOffset)
			: Owner->GetActorTransform().TransformPosition(CatchPoint.LocalOffset);
		Sample.Weight = FMath::Max(0.0f, CatchPoint.Weight);
		Sample.CoverageRadius = FMath::Max(0.0f, CatchPoint.CoverageRadius);
		OutSamples.Add(Sample);
	}

	if (OutSamples.IsEmpty() && bUseOwnerLocationWhenNoCatchPoints)
	{
		FUOUWaterWheelRainCatchSample Sample;
		Sample.WorldLocation = ResolveFallbackCatchWorldLocation();
		Sample.Weight = 1.0f;
		Sample.CoverageRadius = 75.0f;
		OutSamples.Add(Sample);
	}
}

bool UUOUWaterWheelRainConditionComponent::CanReceiveRainInput() const
{
	return bRainInputEnabled && IsActive() && GetOwner() != nullptr;
}

bool UUOUWaterWheelRainConditionComponent::CanReceivePouredWaterInput() const
{
	return bPouredWaterInputEnabled && IsActive() && GetOwner() != nullptr;
}

void UUOUWaterWheelRainConditionComponent::ResetWaterWheelState(bool bClearEverSpun, bool bApplyBaseRotation)
{
	CurrentRotationSpeedDegreesPerSecond = 0.0f;
	TargetRotationSpeedDegreesPerSecond = 0.0f;
	CurrentRotationAngleDegrees = 0.0f;
	LastSignedRainInput = 0.0f;
	LastRainInputStrength = 0.0f;
	LastSignedPouredWaterInput = 0.0f;
	LastPouredWaterInputStrength = 0.0f;
	bHasRainInputRecently = false;
	bHasPouredWaterInputRecently = false;
	bHasWaterInputRecently = false;
	StoppedConditionElapsedTime = 0.0f;
	LastRainInputTimestamp = -BIG_NUMBER;
	LastPouredWaterInputTimestamp = -BIG_NUMBER;
	bHasPendingRainInput = false;
	PendingSignedRainInput = 0.0f;
	PendingRainInputWeight = 0.0f;
	bHasPendingPouredWaterInput = false;
	PendingSignedPouredWaterInput = 0.0f;
	PendingPouredWaterInputWeight = 0.0f;

	if (bClearEverSpun)
	{
		bHasEverSpun = false;
	}

	if (bApplyBaseRotation)
	{
		CacheBaseRotationIfNeeded();
		ApplyRotationAngle(CurrentRotationAngleDegrees);
	}

	SetSatisfiedState(false, true);
}

void UUOUWaterWheelRainConditionComponent::SanitizeSettings()
{
	MaxRotationSpeedDegreesPerSecond = FMath::Max(0.0f, MaxRotationSpeedDegreesPerSecond);
	RainInputStrengthForMaxSpeed = FMath::Max(KINDA_SMALL_NUMBER, RainInputStrengthForMaxSpeed);
	PouredWaterInputStrengthForMaxSpeed = FMath::Max(KINDA_SMALL_NUMBER, PouredWaterInputStrengthForMaxSpeed);
	PouredWaterSpeedMultiplier = FMath::Max(0.0f, PouredWaterSpeedMultiplier);
	MaxBoostedRotationSpeedDegreesPerSecond = FMath::Max(MaxRotationSpeedDegreesPerSecond, MaxBoostedRotationSpeedDegreesPerSecond);
	AccelerationDegreesPerSecond = FMath::Max(0.0f, AccelerationDegreesPerSecond);
	DecelerationDegreesPerSecond = FMath::Max(0.0f, DecelerationDegreesPerSecond);
	TorqueDeadZone = FMath::Clamp(TorqueDeadZone, 0.0f, 1.0f);
	RainInputGraceTime = FMath::Max(0.0f, RainInputGraceTime);
	StoppedSpeedThreshold = FMath::Max(0.0f, StoppedSpeedThreshold);
	StoppedConfirmTime = FMath::Max(0.0f, StoppedConfirmTime);
}

void UUOUWaterWheelRainConditionComponent::CacheBaseRotationIfNeeded()
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

void UUOUWaterWheelRainConditionComponent::ConsumePendingRainInput()
{
	bHasRainInputRecently = HasRecentRainInput();
	bHasPouredWaterInputRecently = HasRecentPouredWaterInput();
	bHasWaterInputRecently = bHasRainInputRecently || bHasPouredWaterInputRecently;

	if (bHasPendingRainInput)
	{
		const float SignedInput = bNormalizeInputByTotalWeight
			? (PendingRainInputWeight > KINDA_SMALL_NUMBER ? PendingSignedRainInput / PendingRainInputWeight : 0.0f)
			: PendingSignedRainInput / RainInputStrengthForMaxSpeed;

		LastSignedRainInput = ClampSignedInput(SignedInput);
		LastRainInputStrength = PendingRainInputWeight;
	}
	else if (!bHasRainInputRecently)
	{
		LastSignedRainInput = 0.0f;
		LastRainInputStrength = 0.0f;
	}

	if (bHasPendingPouredWaterInput)
	{
		const float SignedInput = PendingSignedPouredWaterInput / PouredWaterInputStrengthForMaxSpeed;
		LastSignedPouredWaterInput = ClampSignedInput(SignedInput);
		LastPouredWaterInputStrength = PendingPouredWaterInputWeight;
	}
	else if (!bHasPouredWaterInputRecently)
	{
		LastSignedPouredWaterInput = 0.0f;
		LastPouredWaterInputStrength = 0.0f;
	}

	const float RainTargetSpeed = MaxRotationSpeedDegreesPerSecond * LastSignedRainInput;
	const float PouredWaterTargetSpeed =
		MaxRotationSpeedDegreesPerSecond * PouredWaterSpeedMultiplier * LastSignedPouredWaterInput;
	const float SafeMaxBoostedSpeed =
		FMath::Max(MaxRotationSpeedDegreesPerSecond, MaxBoostedRotationSpeedDegreesPerSecond);
	TargetRotationSpeedDegreesPerSecond = FMath::Clamp(
		RainTargetSpeed + PouredWaterTargetSpeed,
		-SafeMaxBoostedSpeed,
		SafeMaxBoostedSpeed);

	PendingSignedRainInput = 0.0f;
	PendingRainInputWeight = 0.0f;
	bHasPendingRainInput = false;
	PendingSignedPouredWaterInput = 0.0f;
	PendingPouredWaterInputWeight = 0.0f;
	bHasPendingPouredWaterInput = false;
}

void UUOUWaterWheelRainConditionComponent::UpdateRotationSpeed(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	const bool bTargetIsNearlyStopped = FMath::Abs(TargetRotationSpeedDegreesPerSecond) <= StoppedSpeedThreshold;
	const bool bDirectionWillFlip =
		!FMath::IsNearlyZero(CurrentRotationSpeedDegreesPerSecond)
		&& !FMath::IsNearlyZero(TargetRotationSpeedDegreesPerSecond)
		&& FMath::Sign(CurrentRotationSpeedDegreesPerSecond) != FMath::Sign(TargetRotationSpeedDegreesPerSecond);
	const bool bSpeedingUpSameDirection =
		!bTargetIsNearlyStopped
		&& !bDirectionWillFlip
		&& FMath::Abs(TargetRotationSpeedDegreesPerSecond) > FMath::Abs(CurrentRotationSpeedDegreesPerSecond);
	const float InterpSpeed = bSpeedingUpSameDirection
		? AccelerationDegreesPerSecond
		: DecelerationDegreesPerSecond;

	if (InterpSpeed <= KINDA_SMALL_NUMBER)
	{
		CurrentRotationSpeedDegreesPerSecond = TargetRotationSpeedDegreesPerSecond;
		return;
	}

	CurrentRotationSpeedDegreesPerSecond = FMath::FInterpConstantTo(
		CurrentRotationSpeedDegreesPerSecond,
		TargetRotationSpeedDegreesPerSecond,
		DeltaTime,
		InterpSpeed);
}

void UUOUWaterWheelRainConditionComponent::ApplyRotationAngle(float AngleDegrees)
{
	USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	if (TargetComponent == nullptr)
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
	if (RotationSpace == EUOUWaterWheelRotationSpace::World)
	{
		TargetComponent->SetWorldRotation((RotationDelta * BaseWorldRotation).Rotator());
		return;
	}

	TargetComponent->SetRelativeRotation((BaseRelativeRotation * RotationDelta).Rotator());
}

void UUOUWaterWheelRainConditionComponent::RefreshStoppedCondition(float DeltaTime)
{
	const bool bSpeedStopped = FMath::Abs(CurrentRotationSpeedDegreesPerSecond) <= StoppedSpeedThreshold;
	const bool bNoWaterInput = !bRequireNoRainInputForStoppedCondition || !bHasWaterInputRecently;
	const bool bEverSpunSatisfied = !bRequireEverSpunBeforeSatisfied || bHasEverSpun;
	const bool bCanSatisfy = bSpeedStopped && bNoWaterInput && bEverSpunSatisfied;

	if (!bCanSatisfy)
	{
		StoppedConditionElapsedTime = 0.0f;
		SetSatisfiedState(false, true);
		return;
	}

	StoppedConditionElapsedTime += DeltaTime;
	SetSatisfiedState(StoppedConditionElapsedTime >= StoppedConfirmTime, true);
}

USceneComponent* UUOUWaterWheelRainConditionComponent::ResolveRotationTargetComponent() const
{
	if (IsValid(RotationTargetComponent))
	{
		return RotationTargetComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindSceneComponentByNameOrTag(RotationTargetComponentName))
	{
		return FoundComponent;
	}

	return bUseOwnerRootWhenTargetMissing && GetOwner() != nullptr ? GetOwner()->GetRootComponent() : nullptr;
}

USceneComponent* UUOUWaterWheelRainConditionComponent::ResolveRotationCenterComponent() const
{
	if (IsValid(RotationCenterComponent))
	{
		return RotationCenterComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindSceneComponentByNameOrTag(RotationCenterComponentName))
	{
		return FoundComponent;
	}

	return ResolveRotationTargetComponent();
}

USceneComponent* UUOUWaterWheelRainConditionComponent::FindSceneComponentByNameOrTag(FName ComponentName) const
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

FVector UUOUWaterWheelRainConditionComponent::ResolveWheelCenterWorldLocation() const
{
	if (const USceneComponent* CenterComponent = ResolveRotationCenterComponent())
	{
		return CenterComponent->GetComponentLocation();
	}

	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation();
	}

	return FVector::ZeroVector;
}

FVector UUOUWaterWheelRainConditionComponent::ResolveWorldRotationAxis(const USceneComponent* TargetComponent) const
{
	const FVector SafeAxis = RotationAxis.GetSafeNormal();
	if (SafeAxis.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	if (RotationSpace == EUOUWaterWheelRotationSpace::World || TargetComponent == nullptr)
	{
		return SafeAxis;
	}

	return TargetComponent->GetComponentTransform().TransformVectorNoScale(SafeAxis).GetSafeNormal();
}

FVector UUOUWaterWheelRainConditionComponent::ResolveFallbackCatchWorldLocation() const
{
	if (const USceneComponent* TargetComponent = ResolveRotationTargetComponent())
	{
		return TargetComponent->GetComponentLocation();
	}

	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation();
	}

	return FVector::ZeroVector;
}

float UUOUWaterWheelRainConditionComponent::CalculateSignedRainContribution(const FUOUWaterWheelRainInputContext& InputContext) const
{
	const FVector RainDirection = InputContext.WorldDirection.GetSafeNormal();
	if (RainDirection.IsNearlyZero())
	{
		return 0.0f;
	}

	const FVector InputLocation = InputContext.bHasValidWorldLocation
		? InputContext.WorldLocation
		: ResolveFallbackCatchWorldLocation();
	const USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	const FVector AxisWorld = ResolveWorldRotationAxis(TargetComponent);
	if (AxisWorld.IsNearlyZero())
	{
		return 0.0f;
	}

	const FVector Lever = (InputLocation - ResolveWheelCenterWorldLocation()).GetSafeNormal();
	if (Lever.IsNearlyZero())
	{
		return 0.0f;
	}

	float SignedTorque = FVector::DotProduct(AxisWorld, FVector::CrossProduct(Lever, RainDirection));
	if (FMath::Abs(SignedTorque) <= TorqueDeadZone)
	{
		return 0.0f;
	}

	if (bInvertRotationDirection)
	{
		SignedTorque *= -1.0f;
	}

	return ClampSignedInput(SignedTorque);
}

bool UUOUWaterWheelRainConditionComponent::HasRecentRainInput() const
{
	if (LastRainInputTimestamp <= -BIG_NUMBER * 0.5f)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	return World->GetTimeSeconds() - LastRainInputTimestamp <= RainInputGraceTime;
}

bool UUOUWaterWheelRainConditionComponent::HasRecentPouredWaterInput() const
{
	if (LastPouredWaterInputTimestamp <= -BIG_NUMBER * 0.5f)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	return World->GetTimeSeconds() - LastPouredWaterInputTimestamp <= RainInputGraceTime;
}

bool UUOUWaterWheelRainConditionComponent::CanAcceptPour_Implementation(const FUOUPourInputContext& Context) const
{
	return Context.Volume > 0.0f && CanReceivePouredWaterInput();
}

FUOUPourReceiveResult UUOUWaterWheelRainConditionComponent::TryReceivePour_Implementation(const FUOUPourInputContext& Context)
{
	FUOUPourReceiveResult Result;
	if (!CanAcceptPour_Implementation(Context))
	{
		return Result;
	}

	FUOUWaterWheelRainInputContext WaterWheelContext;
	WaterWheelContext.Strength = Context.Duration > KINDA_SMALL_NUMBER
		? Context.Volume / Context.Duration
		: Context.Volume;
	WaterWheelContext.Duration = Context.Duration;
	WaterWheelContext.WorldDirection = Context.WorldDirection;
	WaterWheelContext.WorldLocation = Context.WorldLocation;
	WaterWheelContext.bHasValidWorldLocation = Context.bHasValidWorldLocation;
	WaterWheelContext.InstigatorActor = Context.InstigatorActor;
	ReceivePouredWaterInput(WaterWheelContext);

	Result.bAccepted = true;
	Result.AcceptedVolume = Context.Volume;
	Result.ReceiverId = TEXT("WaterWheel");
	Result.ReceiverType = EUOUPourDropReceiverType::WaterWheel;
	Result.ReceiverObject = this;
	Result.ReceiverActor = GetOwner();
	return Result;
}

int32 UUOUWaterWheelRainConditionComponent::GetPourReceivePriority_Implementation() const
{
	// 기존 TryDeliverWater의 수신 타입 검사 순서를 유지합니다.
	return 500;
}

bool UUOUWaterWheelRainConditionComponent::CanAcceptPourAtLocation_Implementation(const FUOUPourInputContext& Context) const
{
	(void)Context;
	return false;
}
