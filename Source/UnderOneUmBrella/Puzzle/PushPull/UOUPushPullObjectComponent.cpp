// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUPushPullObjectComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace UOUPushPullObjectComponentPrivate
{
	constexpr float MinStepAssistHeight = 2.0f;
	constexpr float StepAssistTracePadding = 2.0f;
}

UUOUPushPullObjectComponent::UUOUPushPullObjectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUPushPullObjectComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxHorizontalSpeed = FMath::Max(0.0f, MaxHorizontalSpeed);
	MaxStepAssistHeight = FMath::Max(0.0f, MaxStepAssistHeight);
	StepAssistProbeDistance = FMath::Max(0.0f, StepAssistProbeDistance);
	StepAssistLiftSpeed = FMath::Max(0.0f, StepAssistLiftSpeed);
	StepAssistMinHorizontalSpeed = FMath::Max(0.0f, StepAssistMinHorizontalSpeed);
	GrabbedAngularDamping = FMath::Max(0.0f, GrabbedAngularDamping);
	MaxGrabbedAngularSpeedDegrees = FMath::Max(0.0f, MaxGrabbedAngularSpeedDegrees);
	ResolveTargetPrimitive();
	CacheBasePhysicsLocks();
	ApplyGrabStateConstraints(false);
}

bool UUOUPushPullObjectComponent::CanGrab(AActor* Interactor) const
{
	if (!bEnablePushPullMovement || Interactor == nullptr || TargetPrimitive == nullptr || !TargetPrimitive->IsSimulatingPhysics())
	{
		return false;
	}

	return CurrentGrabber == nullptr || CurrentGrabber == Interactor;
}

bool UUOUPushPullObjectComponent::TryBeginGrab(AActor* Interactor)
{
	if (!CanGrab(Interactor))
	{
		return false;
	}

	CurrentGrabber = Interactor;
	bIsGrabbed = true;
	ApplyGrabStateConstraints(true);
	return true;
}

void UUOUPushPullObjectComponent::EndGrab(AActor* Interactor)
{
	if (CurrentGrabber != Interactor)
	{
		return;
	}

	CurrentGrabber = nullptr;
	bIsGrabbed = false;
	StopHorizontalMotion();
	ApplyGrabStateConstraints(false);
}

void UUOUPushPullObjectComponent::SetPushPullMovementEnabled(bool bEnabled)
{
	if (bEnablePushPullMovement == bEnabled)
	{
		return;
	}

	bEnablePushPullMovement = bEnabled;

	if (!bEnablePushPullMovement)
	{
		CurrentGrabber = nullptr;
		bIsGrabbed = false;
		StopHorizontalMotion();
		ApplyGrabStateConstraints(false);
	}
}

FVector UUOUPushPullObjectComponent::SetHorizontalVelocity(FVector HorizontalVelocity)
{
	if (!bEnablePushPullMovement || !bIsGrabbed || TargetPrimitive == nullptr)
	{
		return FVector::ZeroVector;
	}

	HorizontalVelocity.Z = 0.0f;
	if (MaxHorizontalSpeed > 0.0f && HorizontalVelocity.Size() > MaxHorizontalSpeed)
	{
		HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * MaxHorizontalSpeed;
	}
	if (!HorizontalVelocity.IsNearlyZero())
	{
		TargetPrimitive->WakeAllRigidBodies();
	}

	const FVector CurrentVelocity = TargetPrimitive->GetPhysicsLinearVelocity();
	const bool bAppliedStepAssist = ApplyLowStepAssist(HorizontalVelocity);
	const float VerticalVelocity = bAppliedStepAssist ? FMath::Min(CurrentVelocity.Z, 0.0f) : CurrentVelocity.Z;
	TargetPrimitive->SetPhysicsLinearVelocity(FVector(HorizontalVelocity.X, HorizontalVelocity.Y, VerticalVelocity));
	if (bLockRotationWhileGrabbed)
	{
		StabilizeGrabbedRotation();
	}
	else
	{
		ClampGrabbedAngularVelocity();
	}
	return HorizontalVelocity;
}

FVector UUOUPushPullObjectComponent::GetGrabReferenceLocation() const
{
	if (TargetPrimitive != nullptr)
	{
		return TargetPrimitive->GetComponentLocation();
	}

	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation();
	}

	return FVector::ZeroVector;
}

void UUOUPushPullObjectComponent::ResolveTargetPrimitive()
{
	if (!bAutoFindTargetPrimitive || TargetPrimitive != nullptr)
	{
		return;
	}

	if (AActor* Owner = GetOwner())
	{
		TargetPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (TargetPrimitive != nullptr && TargetPrimitive->IsSimulatingPhysics())
		{
			return;
		}

		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (PrimitiveComponent != nullptr && PrimitiveComponent->IsSimulatingPhysics())
			{
				TargetPrimitive = PrimitiveComponent;
				return;
			}
		}
	}
}

void UUOUPushPullObjectComponent::CacheBasePhysicsLocks()
{
	if (TargetPrimitive == nullptr)
	{
		return;
	}

	bBaseLockXTranslation = TargetPrimitive->BodyInstance.bLockXTranslation;
	bBaseLockYTranslation = TargetPrimitive->BodyInstance.bLockYTranslation;
	bBaseLockXRotation = TargetPrimitive->BodyInstance.bLockXRotation;
	bBaseLockYRotation = TargetPrimitive->BodyInstance.bLockYRotation;
	bBaseLockZRotation = TargetPrimitive->BodyInstance.bLockZRotation;
	BaseLinearDamping = TargetPrimitive->GetLinearDamping();
	BaseAngularDamping = TargetPrimitive->GetAngularDamping();
}

void UUOUPushPullObjectComponent::ApplyGrabStateConstraints(bool bCanMoveHorizontally)
{
	if (TargetPrimitive == nullptr)
	{
		return;
	}

	TargetPrimitive->SetConstraintMode(EDOFMode::SixDOF);
	TargetPrimitive->BodyInstance.bLockXTranslation = bCanMoveHorizontally ? bBaseLockXTranslation : true;
	TargetPrimitive->BodyInstance.bLockYTranslation = bCanMoveHorizontally ? bBaseLockYTranslation : true;
	const bool bShouldLockRotation = bCanMoveHorizontally && bLockRotationWhileGrabbed;
	TargetPrimitive->BodyInstance.bLockXRotation = bShouldLockRotation ? true : bBaseLockXRotation;
	TargetPrimitive->BodyInstance.bLockYRotation = bShouldLockRotation ? true : bBaseLockYRotation;
	TargetPrimitive->BodyInstance.bLockZRotation = bShouldLockRotation ? true : bBaseLockZRotation;
	TargetPrimitive->SetLinearDamping(bCanMoveHorizontally ? 0.0f : BaseLinearDamping);
	TargetPrimitive->SetAngularDamping((bCanMoveHorizontally && !bShouldLockRotation) ? GrabbedAngularDamping : BaseAngularDamping);
	TargetPrimitive->RecreatePhysicsState();

	if (bShouldLockRotation)
	{
		StabilizeGrabbedRotation();
	}
}

bool UUOUPushPullObjectComponent::ApplyLowStepAssist(const FVector& HorizontalVelocity) const
{
	if (!bEnableLowStepAssist
		|| TargetPrimitive == nullptr
		|| MaxStepAssistHeight <= 0.0f
		|| StepAssistLiftSpeed <= 0.0f
		|| HorizontalVelocity.Size2D() < StepAssistMinHorizontalSpeed)
	{
		return false;
	}

	FVector MoveDirection = HorizontalVelocity;
	MoveDirection.Z = 0.0f;
	if (!MoveDirection.Normalize())
	{
		return false;
	}

	float StepHeight = 0.0f;
	if (!TryFindLowStepAhead(MoveDirection, StepHeight))
	{
		return false;
	}

	const UWorld* World = TargetPrimitive->GetWorld();
	const float DeltaSeconds = World != nullptr && World->GetDeltaSeconds() > 0.0f
		? FMath::Min(World->GetDeltaSeconds(), 1.0f / 30.0f)
		: 1.0f / 60.0f;
	const float LiftAmount = FMath::Min(
		StepHeight + UOUPushPullObjectComponentPrivate::StepAssistTracePadding,
		StepAssistLiftSpeed * DeltaSeconds);
	if (LiftAmount <= 0.0f)
	{
		return false;
	}

	FHitResult SweepHit;
	const FVector TargetLocation = TargetPrimitive->GetComponentLocation() + FVector(0.0f, 0.0f, LiftAmount);
	TargetPrimitive->SetWorldLocation(TargetLocation, true, &SweepHit, ETeleportType::TeleportPhysics);
	return !SweepHit.bStartPenetrating;
}

bool UUOUPushPullObjectComponent::TryFindLowStepAhead(const FVector& MoveDirection, float& OutStepHeight) const
{
	OutStepHeight = 0.0f;

	if (TargetPrimitive == nullptr || TargetPrimitive->GetWorld() == nullptr)
	{
		return false;
	}

	const FBoxSphereBounds Bounds = TargetPrimitive->Bounds;
	const FVector HorizontalDirection(MoveDirection.X, MoveDirection.Y, 0.0f);
	if (HorizontalDirection.IsNearlyZero())
	{
		return false;
	}

	const float DirectionExtent = FMath::Abs(HorizontalDirection.X) * Bounds.BoxExtent.X
		+ FMath::Abs(HorizontalDirection.Y) * Bounds.BoxExtent.Y;
	const FVector SideDirection(-HorizontalDirection.Y, HorizontalDirection.X, 0.0f);
	const float SideExtent = FMath::Abs(SideDirection.X) * Bounds.BoxExtent.X
		+ FMath::Abs(SideDirection.Y) * Bounds.BoxExtent.Y;
	const FVector ProbeCenter = Bounds.Origin + HorizontalDirection * (DirectionExtent + StepAssistProbeDistance);
	const float CurrentBottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;

	const float ProbeSideOffsets[] = { 0.0f, -SideExtent * 0.75f, SideExtent * 0.75f };
	bool bFoundStep = false;
	for (const float ProbeSideOffset : ProbeSideOffsets)
	{
		float ProbeStepHeight = 0.0f;
		if (TraceLowStepAtProbeLocation(ProbeCenter + SideDirection * ProbeSideOffset, CurrentBottomZ, ProbeStepHeight))
		{
			OutStepHeight = FMath::Max(OutStepHeight, ProbeStepHeight);
			bFoundStep = true;
		}
	}

	return bFoundStep;
}

bool UUOUPushPullObjectComponent::TraceLowStepAtProbeLocation(const FVector& ProbeLocation, float CurrentBottomZ, float& OutStepHeight) const
{
	OutStepHeight = 0.0f;

	if (TargetPrimitive == nullptr || TargetPrimitive->GetWorld() == nullptr)
	{
		return false;
	}

	const FVector TraceStart(
		ProbeLocation.X,
		ProbeLocation.Y,
		CurrentBottomZ + MaxStepAssistHeight + UOUPushPullObjectComponentPrivate::StepAssistTracePadding);
	const FVector TraceEnd(
		ProbeLocation.X,
		ProbeLocation.Y,
		CurrentBottomZ - UOUPushPullObjectComponentPrivate::StepAssistTracePadding);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOUPushPullLowStepAssist), false, GetOwner());
	QueryParams.AddIgnoredComponent(TargetPrimitive.Get());
	if (AActor* Owner = GetOwner())
	{
		QueryParams.AddIgnoredActor(Owner);
	}

	FHitResult Hit;
	if (!TargetPrimitive->GetWorld()->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		TargetPrimitive->GetCollisionObjectType(),
		QueryParams))
	{
		return false;
	}

	OutStepHeight = Hit.Location.Z - CurrentBottomZ;
	return OutStepHeight >= UOUPushPullObjectComponentPrivate::MinStepAssistHeight
		&& OutStepHeight <= MaxStepAssistHeight;
}

void UUOUPushPullObjectComponent::StabilizeGrabbedRotation() const
{
	if (TargetPrimitive == nullptr)
	{
		return;
	}

	const FRotator CurrentRotation = TargetPrimitive->GetComponentRotation();
	TargetPrimitive->SetWorldRotation(
		FRotator(0.0f, CurrentRotation.Yaw, 0.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	TargetPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

void UUOUPushPullObjectComponent::ClampGrabbedAngularVelocity() const
{
	if (!bClampGrabbedAngularVelocity || TargetPrimitive == nullptr)
	{
		return;
	}

	const FVector CurrentAngularVelocity = TargetPrimitive->GetPhysicsAngularVelocityInDegrees();
	FVector ClampedAngularVelocity(CurrentAngularVelocity.X, CurrentAngularVelocity.Y, 0.0f);
	if (MaxGrabbedAngularSpeedDegrees > 0.0f && ClampedAngularVelocity.Size2D() > MaxGrabbedAngularSpeedDegrees)
	{
		ClampedAngularVelocity = ClampedAngularVelocity.GetSafeNormal2D() * MaxGrabbedAngularSpeedDegrees;
	}

	TargetPrimitive->SetPhysicsAngularVelocityInDegrees(ClampedAngularVelocity);
}

void UUOUPushPullObjectComponent::StopHorizontalMotion()
{
	if (TargetPrimitive == nullptr)
	{
		return;
	}

	const FVector CurrentVelocity = TargetPrimitive->GetPhysicsLinearVelocity();
	TargetPrimitive->SetPhysicsLinearVelocity(FVector(0.0f, 0.0f, CurrentVelocity.Z));
	TargetPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}
