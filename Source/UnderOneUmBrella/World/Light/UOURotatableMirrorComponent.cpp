// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOURotatableMirrorComponent.h"

#include "Animation/AnimMontage.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "Debug/UOUDevelopmentToolsBuild.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
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

void UUOURotatableMirrorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndMirrorPush(CurrentPusher.Get());
	Super::EndPlay(EndPlayReason);
}

void UUOURotatableMirrorComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TArray<AActor*> OverlappingPushers;
	if (PushVolume != nullptr)
	{
		PushVolume->GetOverlappingActors(OverlappingPushers, APawn::StaticClass());
	}

	if (CurrentPusher != nullptr)
	{
		DrawDebugState(OverlappingPushers);
		return;
	}

	if (!bAllowProximityPush || !bRotationEnabled || DeltaTime <= 0.0f ||
		RotatingComponent == nullptr || PushVolume == nullptr)
	{
		DrawDebugState(OverlappingPushers);
		return;
	}

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

bool UUOURotatableMirrorComponent::CanBeginMirrorPush(const APawn* Pusher) const
{
	if (!bEnableGrabPush || !bRotationEnabled || Pusher == nullptr ||
		RotatingComponent == nullptr ||
		(bPlayerControlledOnly && !Pusher->IsPlayerControlled()) ||
		(CurrentPusher != nullptr && CurrentPusher != Pusher))
	{
		return false;
	}

	const USceneComponent* PushHandle = FindNearestPushHandle(Pusher);
	return PushHandle != nullptr &&
		FVector::DistSquared2D(Pusher->GetActorLocation(), PushHandle->GetComponentLocation()) <=
		FMath::Square(FMath::Max(0.0f, MaximumGrabDistance));
}

bool UUOURotatableMirrorComponent::TryBeginMirrorPush(APawn* Pusher)
{
	if (CurrentPusher == Pusher)
	{
		return true;
	}
	if (!CanBeginMirrorPush(Pusher))
	{
		return false;
	}

	ActivePushHandle = FindNearestPushHandle(Pusher);
	if (ActivePushHandle == nullptr)
	{
		return false;
	}

	const FVector HandleLocation = ActivePushHandle->GetComponentLocation();
	FVector HandleToPlayer = Pusher->GetActorLocation() - HandleLocation;
	HandleToPlayer.Z = 0.0f;
	if (HandleToPlayer.IsNearlyZero())
	{
		HandleToPlayer = RotatingComponent->GetForwardVector();
		HandleToPlayer.Z = 0.0f;
	}
	HandleToPlayer.Normalize();

	FVector AttachLocation = HandleLocation + HandleToPlayer * FMath::Max(0.0f, PlayerAttachDistance);
	AttachLocation.Z = Pusher->GetActorLocation().Z;
	AttachedPlayerLocalLocation =
		RotatingComponent->GetComponentTransform().InverseTransformPosition(AttachLocation);
	CurrentPusher = Pusher;

	if (bSnapPlayerOnGrab && !UpdateAttachedPlayerTransform(true))
	{
		CurrentPusher = nullptr;
		ActivePushHandle = nullptr;
		return false;
	}

	ApplyPusherFacing();
	if (ACharacter* Character = Cast<ACharacter>(Pusher); Character != nullptr && PushMontage != nullptr)
	{
		Character->PlayAnimMontage(PushMontage, FMath::Max(0.01f, PushMontagePlayRate));
	}

	OnMirrorPushStarted.Broadcast(Pusher, ActivePushHandle.Get());
	return true;
}

void UUOURotatableMirrorComponent::EndMirrorPush(APawn* Pusher)
{
	if (CurrentPusher == nullptr || (Pusher != nullptr && CurrentPusher != Pusher))
	{
		return;
	}

	APawn* ReleasedPusher = CurrentPusher.Get();
	if (ACharacter* Character = Cast<ACharacter>(ReleasedPusher);
		Character != nullptr && PushMontage != nullptr)
	{
		Character->StopAnimMontage(PushMontage);
	}

	CurrentPusher = nullptr;
	ActivePushHandle = nullptr;
	OnMirrorPushEnded.Broadcast(ReleasedPusher);
}

float UUOURotatableMirrorComponent::ApplyMirrorPushInput(float AxisInput, float DeltaTime)
{
	if (CurrentPusher == nullptr || DeltaTime <= 0.0f)
	{
		return 0.0f;
	}

	const float SafeInput = FMath::Clamp(AxisInput, -1.0f, 1.0f);
	if (FMath::IsNearlyZero(SafeInput))
	{
		return 0.0f;
	}

	const float PreviousAngle = CurrentAngle;
	const FVector PreviousPusherLocation = CurrentPusher->GetActorLocation();
	const FRotator PreviousPusherRotation = CurrentPusher->GetActorRotation();
	SetMirrorAngle(CurrentAngle + SafeInput * MaximumRotationSpeed * DeltaTime);
	const float AppliedAngleDelta = CurrentAngle - PreviousAngle;
	if (FMath::IsNearlyZero(AppliedAngleDelta))
	{
		return 0.0f;
	}

	if (!UpdateAttachedPlayerTransform(true))
	{
		SetMirrorAngle(PreviousAngle);
		CurrentPusher->SetActorLocationAndRotation(
			PreviousPusherLocation,
			PreviousPusherRotation,
			false,
			nullptr,
			ETeleportType::None);
		return 0.0f;
	}

	return AppliedAngleDelta;
}

FVector UUOURotatableMirrorComponent::GetGrabReferenceLocation() const
{
	if (ActivePushHandle != nullptr)
	{
		return ActivePushHandle->GetComponentLocation();
	}
	if (RotatingComponent != nullptr)
	{
		return RotatingComponent->GetComponentLocation();
	}
	return GetOwner() != nullptr ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

FVector UUOURotatableMirrorComponent::GetWorldInputAxisForInteractor(const AActor* Interactor) const
{
	const USceneComponent* PushHandle = ActivePushHandle != nullptr
		? ActivePushHandle.Get()
		: FindNearestPushHandle(Interactor);
	if (RotatingComponent == nullptr || PushHandle == nullptr)
	{
		return FVector::ZeroVector;
	}

	const FVector RotationAxis = GetRotationAxisWorld();
	FVector RadialDirection = PushHandle->GetComponentLocation() - GetPivotWorldLocation();
	RadialDirection -= RotationAxis * FVector::DotProduct(RadialDirection, RotationAxis);
	if (RadialDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	return FVector::CrossProduct(RotationAxis, RadialDirection.GetSafeNormal()).GetSafeNormal();
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
	MaximumGrabDistance = FMath::Max(0.0f, MaximumGrabDistance);
	PlayerAttachDistance = FMath::Max(0.0f, PlayerAttachDistance);
	PushMontagePlayRate = FMath::Max(0.01f, PushMontagePlayRate);
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

USceneComponent* UUOURotatableMirrorComponent::FindNearestPushHandle(const AActor* Interactor) const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Interactor == nullptr)
	{
		return nullptr;
	}

	USceneComponent* NearestHandle = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	TInlineComponentArray<USceneComponent*> SceneComponents(Owner);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent == nullptr || !SceneComponent->ComponentTags.Contains(PushHandleTag))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			Interactor->GetActorLocation(),
			SceneComponent->GetComponentLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestHandle = SceneComponent;
		}
	}

	return NearestHandle;
}

bool UUOURotatableMirrorComponent::UpdateAttachedPlayerTransform(bool bSweepMovement)
{
	if (CurrentPusher == nullptr || RotatingComponent == nullptr)
	{
		return false;
	}

	const FVector DesiredLocation =
		RotatingComponent->GetComponentTransform().TransformPosition(AttachedPlayerLocalLocation);
	FHitResult MoveHit;
	CurrentPusher->SetActorLocation(
		DesiredLocation,
		bSweepMovement,
		&MoveHit,
		ETeleportType::None);
	const bool bReachedDesiredLocation = FVector::DistSquared(
		CurrentPusher->GetActorLocation(),
		DesiredLocation) <= FMath::Square(2.0f);
	if (bReachedDesiredLocation)
	{
		ApplyPusherFacing();
	}
	return bReachedDesiredLocation;
}

void UUOURotatableMirrorComponent::ApplyPusherFacing() const
{
	if (!bFacePushHandle || CurrentPusher == nullptr || ActivePushHandle == nullptr)
	{
		return;
	}

	FVector ToHandle = ActivePushHandle->GetComponentLocation() - CurrentPusher->GetActorLocation();
	ToHandle.Z = 0.0f;
	if (!ToHandle.IsNearlyZero())
	{
		CurrentPusher->SetActorRotation(FRotator(0.0f, ToHandle.Rotation().Yaw, 0.0f));
	}
}

void UUOURotatableMirrorComponent::DrawDebugState(const TArray<AActor*>& OverlappingPushers) const
{
#if UOU_WITH_DEVELOPMENT_TOOLS
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
#endif
}
