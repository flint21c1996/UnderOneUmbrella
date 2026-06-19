// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Crank/UOUCrankComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

UUOUCrankComponent::UUOUCrankComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUCrankComponent::OnRegister()
{
	Super::OnRegister();

	CacheBaseRotationIfNeeded();
}

void UUOUCrankComponent::BeginPlay()
{
	Super::BeginPlay();

	DegreesPerInputSecond = FMath::Max(0.0f, DegreesPerInputSecond);
	CacheBaseRotationIfNeeded();
}

bool UUOUCrankComponent::CanGrab(AActor* Interactor) const
{
	return bCrankEnabled && Interactor != nullptr && (CurrentGrabber == nullptr || CurrentGrabber == Interactor);
}

bool UUOUCrankComponent::TryBeginGrab(AActor* Interactor)
{
	if (!CanGrab(Interactor))
	{
		return false;
	}

	CurrentGrabber = Interactor;
	bIsGrabbed = true;
	LastInputValue = 0.0f;
	return true;
}

void UUOUCrankComponent::EndGrab(AActor* Interactor)
{
	if (CurrentGrabber != Interactor)
	{
		return;
	}

	CurrentGrabber = nullptr;
	bIsGrabbed = false;
	LastInputValue = 0.0f;
}

float UUOUCrankComponent::ApplyCrankInput(float AxisInput, float DeltaSeconds)
{
	if (!bCrankEnabled || !bIsGrabbed)
	{
		LastInputValue = 0.0f;
		return 0.0f;
	}

	LastInputValue = FMath::Clamp(AxisInput, -1.0f, 1.0f);
	const float InputSign = bInvertInput ? -1.0f : 1.0f;
	const float AngleDelta = LastInputValue * InputSign * DegreesPerInputSecond * FMath::Max(DeltaSeconds, 0.0f);
	if (FMath::IsNearlyZero(AngleDelta))
	{
		return 0.0f;
	}

	CurrentRotationAngleDegrees = ClampRotationAngle(CurrentRotationAngleDegrees + AngleDelta);
	ApplyRotationAngle(CurrentRotationAngleDegrees);
	return AngleDelta;
}

void UUOUCrankComponent::ResetCrank(bool bApplyBaseTransform)
{
	bHasCachedBaseRotation = false;
	bHasCachedBaseMovementLocation = false;
	bHasCachedBaseDrivenRotation = false;
	CachedBaseMovementTargetComponent = nullptr;
	CachedBaseDrivenRotationTargetComponent = nullptr;
	CacheBaseRotationIfNeeded();
	CacheBaseDrivenRotationIfNeeded();
	CacheBaseMovementLocationIfNeeded();

	CurrentRotationAngleDegrees = 0.0f;
	CurrentDrivenRotationAngleDegrees = 0.0f;
	CurrentMovementDistance = 0.0f;
	LastInputValue = 0.0f;
	if (bApplyBaseTransform)
	{
		ApplyRotationAngle(CurrentRotationAngleDegrees);
	}
}

void UUOUCrankComponent::RefreshDrivenMovementBaseLocation()
{
	USceneComponent* TargetComponent = ResolveMovementTargetComponent();
	const FVector SafeAxis = MovementSpace == EUOUCrankTransformSpace::World
		? ResolveWorldAxis(MovementAxisMode, CustomMovementAxis, TargetComponent).GetSafeNormal()
		: ResolveLocalAxis(MovementAxisMode, CustomMovementAxis).GetSafeNormal();
	if (TargetComponent == nullptr || SafeAxis.IsNearlyZero())
	{
		bHasCachedBaseMovementLocation = false;
		CachedBaseMovementTargetComponent = nullptr;
		return;
	}

	BaseRelativeMovementLocation = TargetComponent->GetRelativeLocation() - SafeAxis * CurrentMovementDistance;
	BaseWorldMovementLocation = TargetComponent->GetComponentLocation() - SafeAxis * CurrentMovementDistance;
	CachedBaseMovementTargetComponent = TargetComponent;
	bHasCachedBaseMovementLocation = true;
}

FVector UUOUCrankComponent::GetGrabReferenceLocation() const
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

FVector UUOUCrankComponent::GetWorldInputAxisForInteractor(AActor* Interactor) const
{
	const USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	FVector Axis = ResolveWorldAxis(InputAxisMode, CustomInputAxis, TargetComponent);
	Axis.Z = 0.0f;
	if (Axis.IsNearlyZero())
	{
		Axis = FVector::ForwardVector;
	}
	Axis.Normalize();

	if (Interactor != nullptr)
	{
		FVector ToInteractor = Interactor->GetActorLocation() - GetGrabReferenceLocation();
		ToInteractor.Z = 0.0f;
		if (!ToInteractor.IsNearlyZero() && FVector::DotProduct(ToInteractor.GetSafeNormal(), Axis) < 0.0f)
		{
			Axis *= -1.0f;
		}
	}

	return Axis;
}

USceneComponent* UUOUCrankComponent::ResolveRotationTargetComponent() const
{
	if (IsValid(RotationTargetComponent))
	{
		return RotationTargetComponent.Get();
	}

	if (USceneComponent* FoundComponent = FindSceneComponentByNameOrTag(RotationTargetComponentName))
	{
		return FoundComponent;
	}

	return bUseOwnerRootWhenTargetMissing && GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
}

USceneComponent* UUOUCrankComponent::ResolveDrivenRotationTargetComponent() const
{
	if (IsValid(DrivenRotationTargetComponent))
	{
		return DrivenRotationTargetComponent.Get();
	}

	if (IsValid(DrivenRotationTargetActor))
	{
		return DrivenRotationTargetActor->GetRootComponent();
	}

	if (USceneComponent* FoundComponent = FindSceneComponentByNameOrTag(DrivenRotationTargetComponentName))
	{
		return FoundComponent;
	}

	return bUseOwnerRootWhenDrivenRotationTargetMissing && GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
}

USceneComponent* UUOUCrankComponent::ResolveMovementTargetComponent() const
{
	if (IsValid(MovementTargetComponent))
	{
		return MovementTargetComponent.Get();
	}

	if (IsValid(MovementTargetActor))
	{
		return MovementTargetActor->GetRootComponent();
	}

	if (USceneComponent* FoundComponent = FindSceneComponentByNameOrTag(MovementTargetComponentName))
	{
		return FoundComponent;
	}

	return bUseOwnerRootWhenMovementTargetMissing && GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
}

USceneComponent* UUOUCrankComponent::FindSceneComponentByNameOrTag(FName ComponentName) const
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

void UUOUCrankComponent::CacheBaseRotationIfNeeded()
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

void UUOUCrankComponent::CacheBaseDrivenRotationIfNeeded()
{
	if (!bDriveRotationFromCrank)
	{
		return;
	}

	USceneComponent* TargetComponent = ResolveDrivenRotationTargetComponent();
	if (TargetComponent == nullptr)
	{
		return;
	}

	if (bHasCachedBaseDrivenRotation && CachedBaseDrivenRotationTargetComponent == TargetComponent)
	{
		return;
	}

	BaseRelativeDrivenRotation = TargetComponent->GetRelativeRotation().Quaternion();
	BaseWorldDrivenRotation = TargetComponent->GetComponentQuat();
	CachedBaseDrivenRotationTargetComponent = TargetComponent;
	bHasCachedBaseDrivenRotation = true;
}

void UUOUCrankComponent::CacheBaseMovementLocationIfNeeded()
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

	if (bHasCachedBaseMovementLocation && CachedBaseMovementTargetComponent == TargetComponent)
	{
		return;
	}

	BaseRelativeMovementLocation = TargetComponent->GetRelativeLocation();
	BaseWorldMovementLocation = TargetComponent->GetComponentLocation();
	CachedBaseMovementTargetComponent = TargetComponent;
	bHasCachedBaseMovementLocation = true;
}

void UUOUCrankComponent::ApplyRotationAngle(float AngleDegrees)
{
	USceneComponent* TargetComponent = ResolveRotationTargetComponent();
	const FVector SafeAxis = ResolveLocalAxis(RotationAxisMode, CustomRotationAxis).GetSafeNormal();
	if (TargetComponent != nullptr && !SafeAxis.IsNearlyZero())
	{
		CacheBaseRotationIfNeeded();

		const FQuat RotationDelta(SafeAxis, FMath::DegreesToRadians(AngleDegrees));
		if (RotationSpace == EUOUCrankTransformSpace::World)
		{
			TargetComponent->SetWorldRotation((RotationDelta * BaseWorldRotation).Rotator());
		}
		else
		{
			TargetComponent->SetRelativeRotation((BaseRelativeRotation * RotationDelta).Rotator());
		}
	}

	ApplyDrivenMovement(AngleDegrees);
	ApplyDrivenRotation(AngleDegrees);
}

void UUOUCrankComponent::ApplyDrivenRotation(float CrankAngleDegrees)
{
	bLastDrivenRotationApplied = false;

	if (!bDriveRotationFromCrank)
	{
		LastDrivenRotationFailureReason = TEXT("Disabled");
		return;
	}

	USceneComponent* TargetComponent = ResolveDrivenRotationTargetComponent();
	if (TargetComponent == nullptr)
	{
		LastDrivenRotationFailureReason = TEXT("NoTarget");
		return;
	}

	if (TargetComponent->Mobility != EComponentMobility::Movable)
	{
		if (!bForceDrivenRotationTargetMovable)
		{
			LastDrivenRotationFailureReason = TEXT("TargetNotMovable");
			return;
		}

		TargetComponent->SetMobility(EComponentMobility::Movable);
	}

	const FVector SafeAxis = DrivenRotationSpace == EUOUCrankTransformSpace::World
		? ResolveWorldAxis(DrivenRotationAxisMode, CustomDrivenRotationAxis, TargetComponent).GetSafeNormal()
		: ResolveLocalAxis(DrivenRotationAxisMode, CustomDrivenRotationAxis).GetSafeNormal();
	if (SafeAxis.IsNearlyZero())
	{
		LastDrivenRotationFailureReason = TEXT("InvalidAxis");
		return;
	}

	CacheBaseDrivenRotationIfNeeded();

	const float DrivenAngle = ClampDrivenRotationAngle(CrankAngleDegrees * DrivenRotationDegreesPerCrankDegree);
	CurrentDrivenRotationAngleDegrees = DrivenAngle;
	const FQuat RotationDelta(SafeAxis, FMath::DegreesToRadians(DrivenAngle));

	if (DrivenRotationSpace == EUOUCrankTransformSpace::World)
	{
		const FRotator NextRotation = (RotationDelta * BaseWorldDrivenRotation).Rotator();
		if (DrivenRotationTargetActor != nullptr && DrivenRotationTargetActor->GetRootComponent() == TargetComponent)
		{
			DrivenRotationTargetActor->SetActorRotation(NextRotation, ETeleportType::TeleportPhysics);
		}
		else
		{
			TargetComponent->SetWorldRotation(NextRotation, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
	else
	{
		TargetComponent->SetRelativeRotation((BaseRelativeDrivenRotation * RotationDelta).Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	bLastDrivenRotationApplied = true;
	LastDrivenRotationFailureReason = TEXT("None");
}

void UUOUCrankComponent::ApplyDrivenMovement(float AngleDegrees)
{
	bLastDrivenMovementApplied = false;
	LastDrivenMovementAxis = FVector::ZeroVector;
	LastRawMovementDistance = 0.0f;

	if (!bDriveMovementFromRotation)
	{
		LastDrivenMovementFailureReason = TEXT("Disabled");
		return;
	}

	USceneComponent* TargetComponent = ResolveMovementTargetComponent();
	if (TargetComponent == nullptr)
	{
		LastDrivenMovementFailureReason = TEXT("NoTarget");
		return;
	}

	if (TargetComponent->Mobility != EComponentMobility::Movable)
	{
		if (!bForceMovementTargetMovable)
		{
			LastDrivenMovementFailureReason = TEXT("TargetNotMovable");
			return;
		}

		TargetComponent->SetMobility(EComponentMobility::Movable);
	}

	const FVector SafeAxis = MovementSpace == EUOUCrankTransformSpace::World
		? ResolveWorldAxis(MovementAxisMode, CustomMovementAxis, TargetComponent).GetSafeNormal()
		: ResolveLocalAxis(MovementAxisMode, CustomMovementAxis).GetSafeNormal();
	if (SafeAxis.IsNearlyZero())
	{
		LastDrivenMovementFailureReason = TEXT("InvalidAxis");
		return;
	}
	LastDrivenMovementAxis = SafeAxis;

	CacheBaseMovementLocationIfNeeded();

	float MovementDistance = AngleDegrees * DistancePerRotationDegree;
	LastRawMovementDistance = MovementDistance;
	if (bClampMovementDistance)
	{
		const float MinDistance = FMath::Min(MinMovementDistance, MaxMovementDistance);
		const float MaxDistance = FMath::Max(MinMovementDistance, MaxMovementDistance);
		MovementDistance = FMath::Clamp(MovementDistance, MinDistance, MaxDistance);
	}

	CurrentMovementDistance = MovementDistance;

	if (MovementSpace == EUOUCrankTransformSpace::World)
	{
		LastDesiredMovementLocation = BaseWorldMovementLocation + SafeAxis * MovementDistance;
		if (MovementTargetActor != nullptr && MovementTargetActor->GetRootComponent() == TargetComponent)
		{
			MovementTargetActor->SetActorLocation(LastDesiredMovementLocation, false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			TargetComponent->SetWorldLocation(LastDesiredMovementLocation, false, nullptr, ETeleportType::TeleportPhysics);
		}
		LastActualMovementLocation = TargetComponent->GetComponentLocation();
		bLastDrivenMovementApplied = true;
		LastDrivenMovementFailureReason = TEXT("None");
		return;
	}

	LastDesiredMovementLocation = BaseRelativeMovementLocation + SafeAxis * MovementDistance;
	if (UPrimitiveComponent* PrimitiveTarget = Cast<UPrimitiveComponent>(TargetComponent))
	{
		PrimitiveTarget->SetRelativeLocation(LastDesiredMovementLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		TargetComponent->SetRelativeLocation(LastDesiredMovementLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	LastActualMovementLocation = TargetComponent->GetRelativeLocation();
	bLastDrivenMovementApplied = true;
	LastDrivenMovementFailureReason = TEXT("None");
}

float UUOUCrankComponent::ClampRotationAngle(float AngleDegrees) const
{
	if (!bClampRotationAngle)
	{
		return AngleDegrees;
	}

	const float MinAngle = FMath::Min(MinRotationAngleDegrees, MaxRotationAngleDegrees);
	const float MaxAngle = FMath::Max(MinRotationAngleDegrees, MaxRotationAngleDegrees);
	return FMath::Clamp(AngleDegrees, MinAngle, MaxAngle);
}

float UUOUCrankComponent::ClampDrivenRotationAngle(float AngleDegrees) const
{
	if (!bClampDrivenRotationAngle)
	{
		return AngleDegrees;
	}

	const float MinAngle = FMath::Min(MinDrivenRotationAngleDegrees, MaxDrivenRotationAngleDegrees);
	const float MaxAngle = FMath::Max(MinDrivenRotationAngleDegrees, MaxDrivenRotationAngleDegrees);
	return FMath::Clamp(AngleDegrees, MinAngle, MaxAngle);
}

FVector UUOUCrankComponent::ResolveLocalAxis(EUOUCrankAxisMode AxisMode, const FVector& CustomAxis) const
{
	switch (AxisMode)
	{
	case EUOUCrankAxisMode::X:
		return FVector::XAxisVector;
	case EUOUCrankAxisMode::Y:
		return FVector::YAxisVector;
	case EUOUCrankAxisMode::Custom:
		return CustomAxis;
	case EUOUCrankAxisMode::Z:
	default:
		return FVector::ZAxisVector;
	}
}

FVector UUOUCrankComponent::ResolveWorldAxis(EUOUCrankAxisMode AxisMode, const FVector& CustomAxis, const USceneComponent* BasisComponent) const
{
	const FVector LocalAxis = ResolveLocalAxis(AxisMode, CustomAxis).GetSafeNormal();
	if (LocalAxis.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	if (BasisComponent == nullptr)
	{
		return LocalAxis;
	}

	return BasisComponent->GetComponentTransform().TransformVectorNoScale(LocalAxis).GetSafeNormal();
}
