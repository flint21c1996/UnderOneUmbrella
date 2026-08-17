// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOURotatableMirrorComponent.h"

#include "Animation/AnimMontage.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Debug/UOUDevelopmentDebugDrawContext.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"

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
		return;
	}

	if (!bAllowProximityPush || !bRotationEnabled || DeltaTime <= 0.0f ||
		RotatingComponent == nullptr || PushVolume == nullptr)
	{
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
}

FText UUOURotatableMirrorComponent::GetDebugSummaryText_Implementation() const
{
	const TArray<FString> DebugLines = {
		FString::Printf(TEXT("Rotatable Mirror: %s"), bRotationEnabled ? TEXT("Enabled") : TEXT("Disabled")),
		FString::Printf(TEXT("Angle: %.1f / %.1f ~ %.1f"), CurrentAngle, MinimumAngle, MaximumAngle),
		FString::Printf(TEXT("Rotating Component: %s"), *GetNameSafe(RotatingComponent.Get())),
		FString::Printf(TEXT("Push Volume: %s"), *GetNameSafe(PushVolume.Get()))
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

EUOUDebugCategory UUOURotatableMirrorComponent::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

#if UOU_WITH_DEVELOPMENT_TOOLS
void UUOURotatableMirrorComponent::GatherDevelopmentDebugDraw(
	IUOUDevelopmentDebugDrawContext& Context) const
{
	const USceneComponent* RotatingComponentPtr = GetRotatingComponent();
	if (!IsValid(RotatingComponentPtr))
	{
		return;
	}

	const FTransform& RotatingTransform = RotatingComponentPtr->GetComponentTransform();
	const FVector PivotLocation = RotatingTransform.TransformPosition(LocalPivotOffset);
	FVector RotationAxisWorld = RotatingTransform
		.TransformVectorNoScale(LocalRotationAxis)
		.GetSafeNormal();
	if (RotationAxisWorld.IsNearlyZero())
	{
		RotationAxisWorld = FVector::UpVector;
	}

	Context.DrawLine(
		PivotLocation - RotationAxisWorld * 75.0f,
		PivotLocation + RotationAxisWorld * 75.0f,
		FColor::Magenta,
		3.0f);
	Context.DrawString(
		PivotLocation + RotationAxisWorld * 85.0f,
		FString::Printf(TEXT("Mirror %.1f deg"), CurrentAngle),
		FColor::Magenta);

	const UPrimitiveComponent* PushVolumePtr = GetPushVolume();
	if (!IsValid(PushVolumePtr))
	{
		return;
	}

	TArray<AActor*> OverlappingPushers;
	PushVolumePtr->GetOverlappingActors(OverlappingPushers, APawn::StaticClass());
	for (const AActor* Pusher : OverlappingPushers)
	{
		if (!IsValid(Pusher))
		{
			continue;
		}

		Context.DrawLine(
			PivotLocation,
			Pusher->GetActorLocation(),
			FColor::Cyan,
			1.5f);
		Context.DrawArrow(
			Pusher->GetActorLocation(),
			Pusher->GetActorLocation() + Pusher->GetVelocity() * 0.15f,
			20.0f,
			FColor::Green,
			1.5f);
	}
}
#endif

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
	if (PushHandle == nullptr)
	{
		return false;
	}

	const float HandleDistance = FVector::Dist2D(
		Pusher->GetActorLocation(),
		PushHandle->GetComponentLocation());
	FVector ClosestSurfacePoint;
	float SurfaceGap = TNumericLimits<float>::Max();
	if (!FindClosestGrabSurfacePoint(Pusher, ClosestSurfacePoint, SurfaceGap))
	{
		return false;
	}

	FVector ToSurface = ClosestSurfacePoint - Pusher->GetActorLocation();
	ToSurface.Z = 0.0f;
	if (ToSurface.IsNearlyZero())
	{
		return false;
	}
	const float FacingCorrectionAngle = FMath::Abs(FMath::FindDeltaAngleDegrees(
		Pusher->GetActorRotation().Yaw,
		ToSurface.Rotation().Yaw));
	return HandleDistance <= FMath::Max(0.0f, MaximumGrabDistance) &&
		SurfaceGap <= FMath::Max(0.0f, MaximumGrabSurfaceGap) &&
		(!bFacePushHandle ||
			FacingCorrectionAngle <= FMath::Clamp(MaximumGrabFacingCorrectionAngle, 0.0f, 180.0f));
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

	FVector ClosestSurfacePoint;
	float SurfaceGap = 0.0f;
	if (!FindClosestGrabSurfacePoint(Pusher, ClosestSurfacePoint, SurfaceGap))
	{
		ActivePushHandle = nullptr;
		return false;
	}
	FVector ToSurface = ClosestSurfacePoint - Pusher->GetActorLocation();
	ToSurface.Z = 0.0f;
	if (bFacePushHandle && !ToSurface.IsNearlyZero())
	{
		Pusher->SetActorRotation(FRotator(0.0f, ToSurface.Rotation().Yaw, 0.0f));
	}

	// 잡기 시작 시 캐릭터를 정해진 위치와 방향으로 순간 정렬하지 않습니다.
	// 현재 Transform을 거울의 로컬 공간에 저장해 이후 거울 회전만 자연스럽게 따라가게 합니다.
	AttachedPlayerLocalLocation =
		RotatingComponent->GetComponentTransform().InverseTransformPosition(Pusher->GetActorLocation());
	AttachedPlayerLocalRotation =
		RotatingComponent->GetComponentQuat().Inverse() * Pusher->GetActorQuat();
	CurrentPusher = Pusher;
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
	MaximumGrabSurfaceGap = FMath::Max(0.0f, MaximumGrabSurfaceGap);
	MaximumGrabFacingCorrectionAngle = FMath::Clamp(MaximumGrabFacingCorrectionAngle, 0.0f, 180.0f);
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

bool UUOURotatableMirrorComponent::FindClosestGrabSurfacePoint(
	const APawn* Pusher,
	FVector& OutClosestPoint,
	float& OutSurfaceGap) const
{
	OutClosestPoint = FVector::ZeroVector;
	OutSurfaceGap = TNumericLimits<float>::Max();
	if (Pusher == nullptr || GetOwner() == nullptr || RotatingComponent == nullptr)
	{
		return false;
	}

	float PusherRadius = 0.0f;
	if (const ACharacter* Character = Cast<ACharacter>(Pusher))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			PusherRadius = Capsule->GetScaledCapsuleRadius();
		}
	}

	float ClosestCenterDistance = TNumericLimits<float>::Max();
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(GetOwner());
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (Primitive == nullptr || Primitive == PushVolume ||
			(Primitive != RotatingComponent && !Primitive->IsAttachedTo(RotatingComponent)))
		{
			continue;
		}

		FVector ClosestPoint;
		const float CollisionDistance = Primitive->GetClosestPointOnCollision(
			Pusher->GetActorLocation(),
			ClosestPoint);
		const bool bHasCollisionPoint = CollisionDistance >= 0.0f;
		const FVector CandidatePoint = bHasCollisionPoint
			? ClosestPoint
			: Primitive->Bounds.GetBox().GetClosestPointTo(Pusher->GetActorLocation());
		const float CenterDistance = FVector::Dist(Pusher->GetActorLocation(), CandidatePoint);
		if (CenterDistance < ClosestCenterDistance)
		{
			ClosestCenterDistance = CenterDistance;
			OutClosestPoint = CandidatePoint;
		}
	}

	if (!FMath::IsFinite(ClosestCenterDistance))
	{
		return false;
	}
	OutSurfaceGap = FMath::Max(0.0f, ClosestCenterDistance - PusherRadius);
	return true;
}

float UUOURotatableMirrorComponent::CalculateGrabSurfaceGap(const APawn* Pusher) const
{
	FVector ClosestPoint;
	float SurfaceGap = TNumericLimits<float>::Max();
	FindClosestGrabSurfacePoint(Pusher, ClosestPoint, SurfaceGap);
	return SurfaceGap;
}

bool UUOURotatableMirrorComponent::UpdateAttachedPlayerTransform(bool bSweepMovement)
{
	if (CurrentPusher == nullptr || RotatingComponent == nullptr)
	{
		return false;
	}

	const FVector DesiredLocation =
		RotatingComponent->GetComponentTransform().TransformPosition(AttachedPlayerLocalLocation);
	const FQuat DesiredRotation =
		RotatingComponent->GetComponentQuat() * AttachedPlayerLocalRotation;
	FHitResult MoveHit;
	CurrentPusher->SetActorLocationAndRotation(
		DesiredLocation,
		DesiredRotation,
		bSweepMovement,
		&MoveHit,
		ETeleportType::None);
	const bool bReachedDesiredLocation = FVector::DistSquared(
		CurrentPusher->GetActorLocation(),
		DesiredLocation) <= FMath::Square(2.0f);
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
