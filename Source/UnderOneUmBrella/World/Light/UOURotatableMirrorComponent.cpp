// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOURotatableMirrorComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UUOURotatableMirrorComponent::UUOURotatableMirrorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UUOURotatableMirrorComponent::BeginPlay()
{
	Super::BeginPlay();

	ValidateSettings();
	ResolveComponents();
	ConfigurePushVolume();
	CaptureInitialTransform();
}

void UUOURotatableMirrorComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bRotationEnabled || DeltaTime <= 0.0f || RotatingComponent == nullptr || PushVolume == nullptr)
	{
		return;
	}

	TArray<AActor*> OverlappingPushers;
	PushVolume->GetOverlappingActors(OverlappingPushers, APawn::StaticClass());

	float CombinedPushInput = 0.0f;
	for (AActor* OverlappingActor : OverlappingPushers)
	{
		CombinedPushInput += CalculatePushInput(Cast<APawn>(OverlappingActor));
	}
	CombinedPushInput = FMath::Clamp(CombinedPushInput, -1.0f, 1.0f);

	if (!FMath::IsNearlyZero(CombinedPushInput))
	{
		SetMirrorAngle(CurrentAngle + CombinedPushInput * MaximumRotationSpeed * DeltaTime);
	}

	DrawDebugState(OverlappingPushers);
}

TArray<FString> UUOURotatableMirrorComponent::GetPuzzleDebugInfo_Implementation() const
{
	return {
		FString::Printf(TEXT("Rotatable Mirror: %s"), bRotationEnabled ? TEXT("Enabled") : TEXT("Disabled")),
		FString::Printf(TEXT("Angle: %.1f / %.1f ~ %.1f"), CurrentAngle, MinimumAngle, MaximumAngle),
		FString::Printf(TEXT("Rotating Component: %s"), *GetNameSafe(RotatingComponent.Get())),
		FString::Printf(TEXT("Push Volume: %s"), *GetNameSafe(PushVolume.Get()))
	};
}

#if WITH_EDITOR
void UUOURotatableMirrorComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ValidateSettings();
}
#endif

void UUOURotatableMirrorComponent::SetMirrorAngle(float NewAngle)
{
	const float ClampedAngle = FMath::Clamp(NewAngle, MinimumAngle, MaximumAngle);
	if (FMath::IsNearlyEqual(CurrentAngle, ClampedAngle, KINDA_SMALL_NUMBER))
	{
		return;
	}

	CurrentAngle = ClampedAngle;
	ApplyCurrentRotation();
	OnMirrorRotationChanged.Broadcast(CurrentAngle);
}

void UUOURotatableMirrorComponent::ResetMirrorAngle()
{
	SetMirrorAngle(0.0f);
}

void UUOURotatableMirrorComponent::ValidateSettings()
{
	LocalRotationAxis = LocalRotationAxis.GetSafeNormal();
	if (LocalRotationAxis.IsNearlyZero())
	{
		LocalRotationAxis = FVector::UpVector;
	}

	MinimumAngle = FMath::Min(MinimumAngle, 0.0f);
	MaximumAngle = FMath::Max(MaximumAngle, 0.0f);
	if (MinimumAngle > MaximumAngle)
	{
		Swap(MinimumAngle, MaximumAngle);
	}

	MaximumRotationSpeed = FMath::Max(0.0f, MaximumRotationSpeed);
	MinimumPushSpeed = FMath::Max(0.0f, MinimumPushSpeed);
	MinimumLeverArm = FMath::Max(0.0f, MinimumLeverArm);
	FullTorqueLeverArm = FMath::Max(MinimumLeverArm + 1.0f, FullTorqueLeverArm);
	CurrentAngle = FMath::Clamp(CurrentAngle, MinimumAngle, MaximumAngle);
}

void UUOURotatableMirrorComponent::ResolveComponents()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	RotatingComponent = Cast<USceneComponent>(RotatingComponentReference.GetComponent(Owner));
	if (RotatingComponent == nullptr)
	{
		RotatingComponent = Owner->GetRootComponent();
	}

	PushVolume = Cast<UPrimitiveComponent>(PushVolumeReference.GetComponent(Owner));
	if (PushVolume != nullptr || !bAutoFindPushVolume)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent->GetFName() == PreferredPushVolumeName)
		{
			PushVolume = PrimitiveComponent;
			return;
		}
	}
}

void UUOURotatableMirrorComponent::ConfigurePushVolume()
{
	if (!bConfigurePushVolumeCollision || PushVolume == nullptr)
	{
		return;
	}

	PushVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PushVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	PushVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PushVolume->SetGenerateOverlapEvents(true);
}

void UUOURotatableMirrorComponent::CaptureInitialTransform()
{
	if (RotatingComponent == nullptr)
	{
		return;
	}

	InitialRelativeRotation = RotatingComponent->GetRelativeRotation().Quaternion();
	InitialRelativeLocation = RotatingComponent->GetRelativeLocation();
	CurrentAngle = 0.0f;
	bInitialTransformCaptured = true;
}

void UUOURotatableMirrorComponent::ApplyCurrentRotation()
{
	if (RotatingComponent == nullptr || !bInitialTransformCaptured)
	{
		return;
	}

	const FQuat LocalDeltaRotation(
		LocalRotationAxis,
		FMath::DegreesToRadians(CurrentAngle));
	const FQuat NewRelativeRotation = InitialRelativeRotation * LocalDeltaRotation;
	const FVector InitialPivotLocation =
		InitialRelativeLocation + InitialRelativeRotation.RotateVector(LocalPivotOffset);
	const FVector NewRelativeLocation =
		InitialPivotLocation - NewRelativeRotation.RotateVector(LocalPivotOffset);

	RotatingComponent->SetRelativeLocationAndRotation(
		NewRelativeLocation,
		NewRelativeRotation,
		false,
		nullptr,
		ETeleportType::None);
}

float UUOURotatableMirrorComponent::CalculatePushInput(const APawn* Pusher) const
{
	if (Pusher == nullptr ||
		RotatingComponent == nullptr ||
		(bPlayerControlledOnly && !Pusher->IsPlayerControlled()))
	{
		return 0.0f;
	}

	const FVector RotationAxisWorld = GetRotationAxisWorld();
	const FVector PusherVelocity = Pusher->GetVelocity();
	FVector PlanarPushMotion =
		PusherVelocity - RotationAxisWorld * FVector::DotProduct(PusherVelocity, RotationAxisWorld);
	float PushMotionLength = PlanarPushMotion.Size();
	float PushStrength = 1.0f;
	if (PushMotionLength < MinimumPushSpeed)
	{
		const FVector MovementInput = Pusher->GetLastMovementInputVector();
		PlanarPushMotion =
			MovementInput - RotationAxisWorld * FVector::DotProduct(MovementInput, RotationAxisWorld);
		PushMotionLength = PlanarPushMotion.Size();
		PushStrength = FMath::Clamp(PushMotionLength, 0.0f, 1.0f);
	}
	if (PushMotionLength <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const FVector PivotToPusher = Pusher->GetActorLocation() - GetPivotWorldLocation();
	const FVector PlanarLeverArm =
		PivotToPusher - RotationAxisWorld * FVector::DotProduct(PivotToPusher, RotationAxisWorld);
	const float LeverArmLength = PlanarLeverArm.Size();
	if (LeverArmLength < MinimumLeverArm)
	{
		return 0.0f;
	}

	const float RotationDirection = FVector::DotProduct(
		FVector::CrossProduct(PlanarLeverArm / LeverArmLength, PlanarPushMotion / PushMotionLength),
		RotationAxisWorld);
	const float LeverArmFactor = FMath::Clamp(
		(LeverArmLength - MinimumLeverArm) /
		FMath::Max(1.0f, FullTorqueLeverArm - MinimumLeverArm),
		0.0f,
		1.0f);
	return RotationDirection * LeverArmFactor * PushStrength;
}

FVector UUOURotatableMirrorComponent::GetPivotWorldLocation() const
{
	return RotatingComponent != nullptr
		? RotatingComponent->GetComponentTransform().TransformPosition(LocalPivotOffset)
		: FVector::ZeroVector;
}

FVector UUOURotatableMirrorComponent::GetRotationAxisWorld() const
{
	return RotatingComponent != nullptr
		? RotatingComponent->GetComponentTransform().TransformVectorNoScale(LocalRotationAxis).GetSafeNormal()
		: FVector::UpVector;
}

void UUOURotatableMirrorComponent::DrawDebugState(const TArray<AActor*>& OverlappingPushers) const
{
	if (!bDrawDebug ||
		!UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Puzzle) ||
		GetWorld() == nullptr ||
		RotatingComponent == nullptr)
	{
		return;
	}

	const FVector PivotLocation = GetPivotWorldLocation();
	const FVector RotationAxisWorld = GetRotationAxisWorld();
	DrawDebugLine(
		GetWorld(),
		PivotLocation - RotationAxisWorld * 75.0f,
		PivotLocation + RotationAxisWorld * 75.0f,
		FColor::Magenta,
		false,
		0.0f,
		0,
		3.0f);
	DrawDebugString(
		GetWorld(),
		PivotLocation + RotationAxisWorld * 85.0f,
		FString::Printf(TEXT("Mirror %.1f deg"), CurrentAngle),
		nullptr,
		FColor::Magenta,
		0.0f,
		false);

	for (const AActor* Pusher : OverlappingPushers)
	{
		if (Pusher == nullptr)
		{
			continue;
		}

		DrawDebugLine(
			GetWorld(),
			PivotLocation,
			Pusher->GetActorLocation(),
			FColor::Cyan,
			false,
			0.0f,
			0,
			1.5f);
		DrawDebugDirectionalArrow(
			GetWorld(),
			Pusher->GetActorLocation(),
			Pusher->GetActorLocation() + Pusher->GetVelocity() * 0.15f,
			20.0f,
			FColor::Green,
			false,
			0.0f,
			0,
			1.5f);
	}
}
