// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUPushPullInteractorComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Debug/UOUDebugControllerComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUInteractionComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Puzzle/Crank/UOUCrankComponent.h"
#include "Puzzle/PushPull/UOUPushPullObjectComponent.h"
#include "World/Light/UOURotatableMirrorComponent.h"

namespace UOUPushPullInteractorPrivate
{
	constexpr float MoveInputThreshold = 0.0001f;
	constexpr float MinCardinalGrabDot = 0.82f;
}

UUOUPushPullInteractorComponent::UUOUPushPullInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUPushPullInteractorComponent::BeginPlay()
{
	Super::BeginPlay();

	CandidateSearchRadius = FMath::Max(0.0f, CandidateSearchRadius);
	ReleaseDistanceBuffer = FMath::Max(0.0f, ReleaseDistanceBuffer);
	PushPullWalkSpeed = FMath::Max(0.0f, PushPullWalkSpeed);
	GrabDistanceCorrectionStrength = FMath::Max(0.0f, GrabDistanceCorrectionStrength);
	MaxGrabDistanceCorrectionSpeed = FMath::Max(0.0f, MaxGrabDistanceCorrectionSpeed);
	GrabDistanceCorrectionDeadZone = FMath::Max(0.0f, GrabDistanceCorrectionDeadZone);
	ResolveOwnerReferences();
}

void UUOUPushPullInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateMovementInputFallback();
	RefreshCandidate();

	if (GrabbedObject != nullptr)
	{
		if (!bGrabInputHeld || !CanUseHands() || IsGrabbedObjectTooFar())
		{
			EndGrab();
		}
		else if (OwnerCharacter != nullptr)
		{
			const float MaxWalkSpeed = OwnerCharacter->GetCharacterMovement() != nullptr
				? OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed
				: 0.0f;
			GrabbedObject->SetHorizontalVelocity(BuildGrabbedObjectVelocity(MaxWalkSpeed));
			ApplyGrabbedRotation();
		}
	}
	else if (GrabbedCrank != nullptr)
	{
		if (!bGrabInputHeld || !CanUseHands() || IsGrabbedObjectTooFar())
		{
			EndGrab();
		}
		else
		{
			GrabbedCrank->ApplyCrankInput(CurrentAxisInput, DeltaTime);
			ApplyGrabbedRotation();
		}
	}
	else if (GrabbedMirror != nullptr)
	{
		if (!bGrabInputHeld || !CanUseHands() || IsGrabbedObjectTooFar())
		{
			EndGrab();
		}
		else
		{
			GrabbedMirror->ApplyMirrorPushInput(CurrentAxisInput, DeltaTime);
			GrabbedMoveAxis = GrabbedMirror->GetWorldInputAxisForInteractor(OwnerCharacter);
		}
	}

	UpdateScreenDebug();
	DrawWorldDebug();
}

void UUOUPushPullInteractorComponent::HandleGrabPressed()
{
	bGrabInputHeld = true;
	LastFailureReason = TEXT("Grab Pressed");
	if (GrabbedObject == nullptr && GrabbedCrank == nullptr && GrabbedMirror == nullptr)
	{
		TryBeginGrab();
	}
}

void UUOUPushPullInteractorComponent::HandleGrabReleased()
{
	bGrabInputHeld = false;
	EndGrab();
}

bool UUOUPushPullInteractorComponent::HandleMoveInput(const FVector2D& MovementVector, float MovementYaw)
{
	if ((GrabbedObject == nullptr && GrabbedCrank == nullptr && GrabbedMirror == nullptr) || OwnerCharacter == nullptr)
	{
		return false;
	}

	const FRotator YawRotation(0.0f, MovementYaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector CameraRelativeMove = ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X;

	CurrentAxisInput = FVector::DotProduct(CameraRelativeMove, GrabbedMoveAxis);
	if (GrabbedObject != nullptr && FMath::Abs(CurrentAxisInput) > UOUPushPullInteractorPrivate::MoveInputThreshold)
	{
		OwnerCharacter->AddMovementInput(GrabbedMoveAxis, CurrentAxisInput);
	}
	else if (FMath::Abs(CurrentAxisInput) <= UOUPushPullInteractorPrivate::MoveInputThreshold)
	{
		CurrentAxisInput = 0.0f;
	}

	return true;
}

void UUOUPushPullInteractorComponent::RefreshCandidate()
{
	CurrentCandidateObject = nullptr;
	CurrentCandidateCrank = nullptr;
	CurrentCandidateMirror = nullptr;

	if (GrabbedObject != nullptr)
	{
		CurrentCandidateObject = GrabbedObject;
		return;
	}
	if (GrabbedCrank != nullptr)
	{
		CurrentCandidateCrank = GrabbedCrank;
		return;
	}
	if (GrabbedMirror != nullptr)
	{
		CurrentCandidateMirror = GrabbedMirror;
		return;
	}

	CurrentCandidateObject = FindBestCandidate();
	if (CurrentCandidateObject == nullptr)
	{
		CurrentCandidateCrank = FindBestCrankCandidate();
	}
	if (CurrentCandidateObject == nullptr && CurrentCandidateCrank == nullptr)
	{
		CurrentCandidateMirror = FindBestMirrorCandidate();
	}
}

bool UUOUPushPullInteractorComponent::CanUseHands() const
{
	if (OwnerCharacter == nullptr)
	{
		return false;
	}

	if (bRequireGrounded && OwnerCharacter->GetCharacterMovement() != nullptr && !OwnerCharacter->GetCharacterMovement()->IsMovingOnGround())
	{
		return false;
	}

	if (!bRequireClosedUmbrella || UmbrellaComponent == nullptr || !UmbrellaComponent->HasUmbrella())
	{
		return true;
	}

	return UmbrellaComponent->IsClosed();
}

bool UUOUPushPullInteractorComponent::IsGrabbedObjectTooFar() const
{
	if ((GrabbedObject == nullptr && GrabbedCrank == nullptr && GrabbedMirror == nullptr) ||
		InteractionComponent == nullptr || OwnerCharacter == nullptr)
	{
		return false;
	}

	const float AllowedDistance = InteractionComponent->InteractionRange + ReleaseDistanceBuffer;
	return FVector::Dist2D(OwnerCharacter->GetActorLocation(), GetGrabbedReferenceLocation()) > AllowedDistance;
}

void UUOUPushPullInteractorComponent::TryBeginGrab()
{
	if (OwnerCharacter == nullptr)
	{
		LastFailureReason = TEXT("No OwnerCharacter");
		return;
	}

	if (CurrentCandidateObject == nullptr && CurrentCandidateCrank == nullptr && CurrentCandidateMirror == nullptr)
	{
		LastFailureReason = TEXT("No Candidate");
		return;
	}

	if (!CanUseHands())
	{
		LastFailureReason = TEXT("Hands Blocked");
		return;
	}

	if (CurrentCandidateMirror != nullptr &&
		CurrentCandidateObject == nullptr && CurrentCandidateCrank == nullptr)
	{
		const FVector MoveAxis =
			CurrentCandidateMirror->GetWorldInputAxisForInteractor(OwnerCharacter);
		if (MoveAxis.IsNearlyZero())
		{
			LastFailureReason = TEXT("Invalid Mirror Axis");
			return;
		}

		if (!CurrentCandidateMirror->TryBeginMirrorPush(OwnerCharacter))
		{
			LastFailureReason = TEXT("Mirror Rejected Grab");
			return;
		}

		GrabbedMirror = CurrentCandidateMirror;
		GrabbedMoveAxis = GrabbedMirror->GetWorldInputAxisForInteractor(OwnerCharacter);
		CurrentAxisInput = 0.0f;
		LastFailureReason = TEXT("Mirror Grabbed");
		ClearGrabbedReferenceDistance();
		ApplyGrabbedCharacterMovementSettings(false);
		return;
	}

	if (CurrentCandidateCrank != nullptr && CurrentCandidateObject == nullptr)
	{
		const FVector MoveAxis = CurrentCandidateCrank->GetWorldInputAxisForInteractor(OwnerCharacter);
		if (MoveAxis.IsNearlyZero())
		{
			LastFailureReason = TEXT("Invalid Crank Axis");
			return;
		}

		if (!CurrentCandidateCrank->TryBeginGrab(OwnerCharacter))
		{
			LastFailureReason = TEXT("Crank Rejected Grab");
			return;
		}

		GrabbedCrank = CurrentCandidateCrank;
		GrabbedMoveAxis = MoveAxis;
		CurrentAxisInput = 0.0f;
		LastFailureReason = TEXT("Crank Grabbed");
		ClearGrabbedReferenceDistance();

		ApplyGrabbedCharacterMovementSettings(false);

		ApplyGrabbedRotation();
		return;
	}

	FVector MoveAxis = FVector::ZeroVector;
	if (!TryResolveGrabAxis(CurrentCandidateObject, MoveAxis))
	{
		LastFailureReason = TEXT("Invalid Grab Axis");
		return;
	}

	if (!CurrentCandidateObject->TryBeginGrab(OwnerCharacter))
	{
		LastFailureReason = TEXT("Object Rejected Grab");
		return;
	}

	GrabbedObject = CurrentCandidateObject;
	GrabbedMoveAxis = MoveAxis;
	CurrentAxisInput = 0.0f;
	LastFailureReason = TEXT("Grabbed");
	CacheGrabbedReferenceDistance();

	ApplyGrabbedCharacterMovementSettings(true);

	ApplyGrabbedRotation();
}

void UUOUPushPullInteractorComponent::EndGrab()
{
	RestoreGrabbedCharacterMovementSettings();

	if (GrabbedObject != nullptr)
	{
		GrabbedObject->EndGrab(OwnerCharacter);
	}
	if (GrabbedCrank != nullptr)
	{
		GrabbedCrank->EndGrab(OwnerCharacter);
	}
	if (GrabbedMirror != nullptr)
	{
		GrabbedMirror->EndMirrorPush(OwnerCharacter);
	}

	GrabbedObject = nullptr;
	GrabbedCrank = nullptr;
	GrabbedMirror = nullptr;
	GrabbedMoveAxis = FVector::ZeroVector;
	CurrentAxisInput = 0.0f;
	ClearGrabbedReferenceDistance();
	if (bGrabInputHeld)
	{
		LastFailureReason = TEXT("Grab Ended");
	}
}

void UUOUPushPullInteractorComponent::ApplyGrabbedCharacterMovementSettings(bool bLimitWalkSpeed)
{
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement();
	if (CharacterMovement == nullptr)
	{
		return;
	}

	if (!bHasCachedCharacterMovementSettings)
	{
		bCachedOrientRotationToMovement = CharacterMovement->bOrientRotationToMovement;
		CachedMaxWalkSpeed = CharacterMovement->MaxWalkSpeed;
		bHasCachedCharacterMovementSettings = true;
	}

	CharacterMovement->bOrientRotationToMovement = false;
	if (bLimitWalkSpeed)
	{
		CharacterMovement->MaxWalkSpeed = FMath::Min(CachedMaxWalkSpeed, PushPullWalkSpeed);
	}
	CharacterMovement->StopMovementImmediately();
}

void UUOUPushPullInteractorComponent::RestoreGrabbedCharacterMovementSettings()
{
	if (!bHasCachedCharacterMovementSettings)
	{
		return;
	}

	if (OwnerCharacter != nullptr)
	{
		if (UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement())
		{
			CharacterMovement->bOrientRotationToMovement = bCachedOrientRotationToMovement;
			CharacterMovement->MaxWalkSpeed = CachedMaxWalkSpeed;
		}
	}

	CachedMaxWalkSpeed = 0.0f;
	bHasCachedCharacterMovementSettings = false;
}

void UUOUPushPullInteractorComponent::CacheGrabbedReferenceDistance()
{
	bHasGrabbedReferenceDistance = false;
	GrabbedReferenceDistance2D = 0.0f;

	if (OwnerCharacter == nullptr || GrabbedObject == nullptr || GrabbedMoveAxis.IsNearlyZero())
	{
		return;
	}

	FVector HorizontalAxis = GrabbedMoveAxis;
	HorizontalAxis.Z = 0.0f;
	if (HorizontalAxis.IsNearlyZero())
	{
		return;
	}

	HorizontalAxis.Normalize();

	FVector ToPlayer = OwnerCharacter->GetActorLocation() - GrabbedObject->GetGrabReferenceLocation();
	ToPlayer.Z = 0.0f;

	GrabbedReferenceDistance2D = FVector::DotProduct(ToPlayer, HorizontalAxis);
	bHasGrabbedReferenceDistance = true;
}

void UUOUPushPullInteractorComponent::ClearGrabbedReferenceDistance()
{
	GrabbedReferenceDistance2D = 0.0f;
	bHasGrabbedReferenceDistance = false;
}

FVector UUOUPushPullInteractorComponent::BuildGrabbedObjectVelocity(float BaseMoveSpeed) const
{
	FVector HorizontalAxis = GrabbedMoveAxis;
	HorizontalAxis.Z = 0.0f;
	if (HorizontalAxis.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	HorizontalAxis.Normalize();
	FVector DesiredVelocity = HorizontalAxis * CurrentAxisInput * FMath::Max(0.0f, BaseMoveSpeed);

	if (!bUseGrabDistanceCorrection
		|| !bHasGrabbedReferenceDistance
		|| GrabDistanceCorrectionStrength <= 0.0f
		|| MaxGrabDistanceCorrectionSpeed <= 0.0f
		|| OwnerCharacter == nullptr
		|| GrabbedObject == nullptr)
	{
		return DesiredVelocity;
	}

	FVector ToPlayer = OwnerCharacter->GetActorLocation() - GrabbedObject->GetGrabReferenceLocation();
	ToPlayer.Z = 0.0f;

	const float CurrentDistance = FVector::DotProduct(ToPlayer, HorizontalAxis);
	const float DistanceError = CurrentDistance - GrabbedReferenceDistance2D;
	if (FMath::Abs(DistanceError) <= GrabDistanceCorrectionDeadZone)
	{
		return DesiredVelocity;
	}

	const float CorrectionError = DistanceError - FMath::Sign(DistanceError) * GrabDistanceCorrectionDeadZone;
	const float CorrectionSpeed = FMath::Clamp(
		CorrectionError * GrabDistanceCorrectionStrength,
		-MaxGrabDistanceCorrectionSpeed,
		MaxGrabDistanceCorrectionSpeed);

	return DesiredVelocity + HorizontalAxis * CorrectionSpeed;
}

void UUOUPushPullInteractorComponent::ApplyGrabbedRotation() const
{
	if (OwnerCharacter == nullptr || GrabbedMoveAxis.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRotation = FRotationMatrix::MakeFromX(-GrabbedMoveAxis).Rotator();
	OwnerCharacter->SetActorRotation(FRotator(0.0f, DesiredRotation.Yaw, 0.0f));
}

void UUOUPushPullInteractorComponent::ResolveOwnerReferences()
{
	OwnerCharacter = Cast<AUOUCharacter>(GetOwner());
	InteractionComponent = GetOwner() != nullptr ? GetOwner()->FindComponentByClass<UUOUInteractionComponent>() : nullptr;
	UmbrellaComponent = GetOwner() != nullptr ? GetOwner()->FindComponentByClass<UUOUUmbrellaComponent>() : nullptr;
}

void UUOUPushPullInteractorComponent::UpdateMovementInputFallback()
{
	if ((GrabbedObject == nullptr && GrabbedCrank == nullptr && GrabbedMirror == nullptr) ||
		OwnerCharacter == nullptr || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	if (PlayerController == nullptr)
	{
		return;
	}

	FVector2D MovementVector = FVector2D::ZeroVector;
	if (PlayerController->IsInputKeyDown(EKeys::W) || PlayerController->IsInputKeyDown(EKeys::Up))
	{
		MovementVector.Y += 1.0f;
	}
	if (PlayerController->IsInputKeyDown(EKeys::S) || PlayerController->IsInputKeyDown(EKeys::Down))
	{
		MovementVector.Y -= 1.0f;
	}
	if (PlayerController->IsInputKeyDown(EKeys::D) || PlayerController->IsInputKeyDown(EKeys::Right))
	{
		MovementVector.X += 1.0f;
	}
	if (PlayerController->IsInputKeyDown(EKeys::A) || PlayerController->IsInputKeyDown(EKeys::Left))
	{
		MovementVector.X -= 1.0f;
	}

	if (!MovementVector.IsNearlyZero())
	{
		MovementVector = MovementVector.GetSafeNormal();
	}

	float MovementYaw = 0.0f;
	if (const UUOUCameraControllerComponent* CameraController = OwnerCharacter->GetCameraControllerComponent())
	{
		MovementYaw = CameraController->GetMovementYaw();
	}

	const FRotator YawRotation(0.0f, MovementYaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector CameraRelativeMove = ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X;

	CurrentAxisInput = FVector::DotProduct(CameraRelativeMove, GrabbedMoveAxis);
	if (FMath::Abs(CurrentAxisInput) <= UOUPushPullInteractorPrivate::MoveInputThreshold)
	{
		CurrentAxisInput = 0.0f;
	}
}

void UUOUPushPullInteractorComponent::UpdateScreenDebug() const
{
	// 플레이어와 관련된 화면 디버그는 Debug Controller의 Player HUD에서 통합 표시합니다.
}

void UUOUPushPullInteractorComponent::DrawWorldDebug() const
{
	if (!UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Interaction))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>();
	const UUOUInteractionDebugControllerComponent* InteractionController = DebugSubsystem != nullptr
		? Cast<UUOUInteractionDebugControllerComponent>(DebugSubsystem->FindDebugControllerComponent(EUOUDebugCategory::Interaction))
		: nullptr;
	const bool bShowTrace = InteractionController == nullptr || InteractionController->bShowTrace;
	const bool bShowCandidate = InteractionController == nullptr || InteractionController->bShowCandidate;

	const FVector DetectionOrigin = GetDetectionOriginLocation();
	const FColor InteractionDebugColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::Interaction, FColor::Orange);
	const FColor SearchColor = UUOUDebugSubsystem::GetDebugCategoryColor(
		this,
		EUOUDebugCategory::Interaction,
		CurrentCandidateObject != nullptr || CurrentCandidateCrank != nullptr || CurrentCandidateMirror != nullptr
			? FColor::Green
			: FColor::Cyan);
	if (bShowTrace)
	{
		DrawDebugSphere(World, DetectionOrigin, CandidateSearchRadius, 24, SearchColor, false, 0.0f, 0, 1.5f);
	}

	if (bShowCandidate && CurrentCandidateObject != nullptr)
	{
		const FVector CandidateLocation = CurrentCandidateObject->GetGrabReferenceLocation();
		DrawDebugLine(World, DetectionOrigin, CandidateLocation, InteractionDebugColor, false, 0.0f, 0, 2.0f);
		DrawDebugSphere(World, CandidateLocation, 14.0f, 12, InteractionDebugColor, false, 0.0f, 0, 1.5f);
	}
	else if (bShowCandidate && CurrentCandidateCrank != nullptr)
	{
		const FVector CandidateLocation = CurrentCandidateCrank->GetGrabReferenceLocation();
		DrawDebugLine(World, DetectionOrigin, CandidateLocation, InteractionDebugColor, false, 0.0f, 0, 2.0f);
		DrawDebugSphere(World, CandidateLocation, 14.0f, 12, InteractionDebugColor, false, 0.0f, 0, 1.5f);
	}
	else if (bShowCandidate && CurrentCandidateMirror != nullptr)
	{
		const FVector CandidateLocation = CurrentCandidateMirror->GetGrabReferenceLocation();
		DrawDebugLine(World, DetectionOrigin, CandidateLocation, InteractionDebugColor, false, 0.0f, 0, 2.0f);
		DrawDebugSphere(World, CandidateLocation, 14.0f, 12, InteractionDebugColor, false, 0.0f, 0, 1.5f);
	}

	if (bShowCandidate && (GrabbedObject != nullptr || GrabbedCrank != nullptr || GrabbedMirror != nullptr))
	{
		const FVector GrabbedLocation = GetGrabbedReferenceLocation();
		DrawDebugDirectionalArrow(
			World,
			GrabbedLocation,
			GrabbedLocation + GrabbedMoveAxis * 100.0f,
			25.0f,
			InteractionDebugColor,
			false,
			0.0f,
			0,
			3.0f);
	}
}

bool UUOUPushPullInteractorComponent::TryResolveGrabAxis(UUOUPushPullObjectComponent* TargetObject, FVector& OutMoveAxis) const
{
	OutMoveAxis = FVector::ZeroVector;

	if (OwnerCharacter == nullptr || TargetObject == nullptr)
	{
		return false;
	}

	const UPrimitiveComponent* TargetPrimitive = TargetObject->GetTargetPrimitive();
	if (TargetPrimitive == nullptr)
	{
		return false;
	}

	FVector ToPlayer = OwnerCharacter->GetActorLocation() - TargetPrimitive->GetComponentLocation();
	ToPlayer.Z = 0.0f;
	if (ToPlayer.SizeSquared() <= UOUPushPullInteractorPrivate::MoveInputThreshold)
	{
		return false;
	}

	ToPlayer.Normalize();

	const FVector Right = GetHorizontalAxis(TargetPrimitive->GetRightVector(), FVector::RightVector);
	const FVector Forward = GetHorizontalAxis(TargetPrimitive->GetForwardVector(), FVector::ForwardVector);

	FVector BestAxis = Right;
	float BestDot = FVector::DotProduct(ToPlayer, Right);
	CheckBetterAxis(-Right, ToPlayer, BestAxis, BestDot);
	CheckBetterAxis(Forward, ToPlayer, BestAxis, BestDot);
	CheckBetterAxis(-Forward, ToPlayer, BestAxis, BestDot);

	if (BestDot < UOUPushPullInteractorPrivate::MinCardinalGrabDot)
	{
		return false;
	}

	OutMoveAxis = BestAxis;
	return true;
}

UUOUPushPullObjectComponent* UUOUPushPullInteractorComponent::FindBestCandidate() const
{
	if (OwnerCharacter == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	if (bDetectWorldDynamic)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	}

	if (bDetectPhysicsBody)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	}

	if (bDetectPuzzleWeight)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel1);
	}

	if (ObjectQueryParams.IsValid() == false)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PushPullCandidateOverlap), false, OwnerCharacter);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	const FVector DetectionOrigin = GetDetectionOriginLocation();
	const FCollisionShape SearchShape = FCollisionShape::MakeSphere(CandidateSearchRadius);
	if (!World->OverlapMultiByObjectType(OverlapResults, DetectionOrigin, FQuat::Identity, ObjectQueryParams, SearchShape, QueryParams))
	{
		return nullptr;
	}

	UUOUPushPullObjectComponent* BestCandidate = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* CandidateOwner = OverlapResult.GetActor();
		if (CandidateOwner == nullptr)
		{
			continue;
		}

		UUOUPushPullObjectComponent* CandidateObject = CandidateOwner->FindComponentByClass<UUOUPushPullObjectComponent>();
		if (CandidateObject == nullptr || !CandidateObject->CanGrab(OwnerCharacter))
		{
			continue;
		}

		FVector ResolvedAxis = FVector::ZeroVector;
		if (!TryResolveGrabAxis(CandidateObject, ResolvedAxis))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(DetectionOrigin, CandidateObject->GetGrabReferenceLocation());
		if (DistanceSquared >= BestDistanceSquared)
		{
			continue;
		}

		BestDistanceSquared = DistanceSquared;
		BestCandidate = CandidateObject;
	}

	return BestCandidate;
}

UUOUCrankComponent* UUOUPushPullInteractorComponent::FindBestCrankCandidate() const
{
	if (OwnerCharacter == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	if (bDetectWorldDynamic)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	}

	if (bDetectPhysicsBody)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	}

	if (bDetectPuzzleWeight)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel1);
	}

	if (ObjectQueryParams.IsValid() == false)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CrankCandidateOverlap), false, OwnerCharacter);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	const FVector DetectionOrigin = GetDetectionOriginLocation();
	const FCollisionShape SearchShape = FCollisionShape::MakeSphere(CandidateSearchRadius);
	if (!World->OverlapMultiByObjectType(OverlapResults, DetectionOrigin, FQuat::Identity, ObjectQueryParams, SearchShape, QueryParams))
	{
		return nullptr;
	}

	UUOUCrankComponent* BestCandidate = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* CandidateOwner = OverlapResult.GetActor();
		if (CandidateOwner == nullptr)
		{
			continue;
		}

		UUOUCrankComponent* CandidateCrank = CandidateOwner->FindComponentByClass<UUOUCrankComponent>();
		if (CandidateCrank == nullptr || !CandidateCrank->CanGrab(OwnerCharacter))
		{
			continue;
		}

		const FVector InputAxis = CandidateCrank->GetWorldInputAxisForInteractor(OwnerCharacter);
		if (InputAxis.IsNearlyZero())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(DetectionOrigin, CandidateCrank->GetGrabReferenceLocation());
		if (DistanceSquared >= BestDistanceSquared)
		{
			continue;
		}

		BestDistanceSquared = DistanceSquared;
		BestCandidate = CandidateCrank;
	}

	return BestCandidate;
}

UUOURotatableMirrorComponent* UUOUPushPullInteractorComponent::FindBestMirrorCandidate() const
{
	if (OwnerCharacter == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RotatableMirrorCandidateOverlap), false, OwnerCharacter);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	TArray<FOverlapResult> OverlapResults;
	const FVector DetectionOrigin = GetDetectionOriginLocation();
	if (!World->OverlapMultiByObjectType(
		OverlapResults,
		DetectionOrigin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(CandidateSearchRadius),
		QueryParams))
	{
		return nullptr;
	}

	UUOURotatableMirrorComponent* BestCandidate = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	TSet<AActor*> ProcessedActors;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* CandidateOwner = OverlapResult.GetActor();
		if (CandidateOwner == nullptr || ProcessedActors.Contains(CandidateOwner))
		{
			continue;
		}
		ProcessedActors.Add(CandidateOwner);

		UUOURotatableMirrorComponent* CandidateMirror =
			CandidateOwner->FindComponentByClass<UUOURotatableMirrorComponent>();
		if (CandidateMirror == nullptr || !CandidateMirror->CanBeginMirrorPush(OwnerCharacter))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			DetectionOrigin,
			CandidateMirror->GetGrabReferenceLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestCandidate = CandidateMirror;
		}
	}

	return BestCandidate;
}

FVector UUOUPushPullInteractorComponent::GetDetectionOriginLocation() const
{
	if (InteractionComponent != nullptr)
	{
		if (InteractionComponent->DetectionOrigin != nullptr)
		{
			return InteractionComponent->DetectionOrigin->GetComponentLocation();
		}

		if (OwnerCharacter != nullptr)
		{
			return OwnerCharacter->GetActorLocation() + InteractionComponent->DetectionOffset;
		}
	}

	if (OwnerCharacter != nullptr)
	{
		return OwnerCharacter->GetActorLocation() + FVector(50.0f, 0.0f, 40.0f);
	}

	return FVector::ZeroVector;
}

FVector UUOUPushPullInteractorComponent::GetGrabbedReferenceLocation() const
{
	if (GrabbedObject != nullptr)
	{
		return GrabbedObject->GetGrabReferenceLocation();
	}

	if (GrabbedCrank != nullptr)
	{
		return GrabbedCrank->GetGrabReferenceLocation();
	}

	if (GrabbedMirror != nullptr)
	{
		return GrabbedMirror->GetGrabReferenceLocation();
	}

	return FVector::ZeroVector;
}

void UUOUPushPullInteractorComponent::CheckBetterAxis(const FVector& Axis, const FVector& ToPlayer, FVector& BestAxis, float& BestDot)
{
	const float Dot = FVector::DotProduct(ToPlayer, Axis);
	if (Dot > BestDot)
	{
		BestAxis = Axis;
		BestDot = Dot;
	}
}

FVector UUOUPushPullInteractorComponent::GetHorizontalAxis(FVector Axis, const FVector& Fallback)
{
	Axis.Z = 0.0f;
	if (Axis.SizeSquared() <= UOUPushPullInteractorPrivate::MoveInputThreshold)
	{
		return Fallback;
	}

	return Axis.GetSafeNormal();
}
