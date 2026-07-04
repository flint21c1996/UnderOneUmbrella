// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUCameraControllerComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CameraRotationCompleteToleranceDegrees = 1.0f;
}

UUOUCameraControllerComponent::UUOUCameraControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OccluderFadeMaterialFinder(
		TEXT("/Game/StarterContent/Materials/M_Glass.M_Glass"));
	if (OccluderFadeMaterialFinder.Succeeded())
	{
		OccluderFadeMaterial = OccluderFadeMaterialFinder.Object;
	}
}

void UUOUCameraControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheCameraComponents();
	ApplyCameraProjection();
	InitializeCameraRig();
}

void UUOUCameraControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreAllOccludedMeshes();
	Super::EndPlay(EndPlayReason);
}

void UUOUCameraControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDialogueFocusActive)
	{
		UpdateDialogueCamera(DeltaTime);
	}
	else
	{
		UpdateSnapCamera(DeltaTime);
	}
	UpdateCameraOcclusion();
}

void UUOUCameraControllerComponent::RotateCameraLeft()
{
	TargetCameraYaw -= CameraRotateStep;
}

void UUOUCameraControllerComponent::RotateCameraRight()
{
	TargetCameraYaw += CameraRotateStep;
}

void UUOUCameraControllerComponent::ZoomCameraIn()
{
	TargetCameraDistance = FMath::Clamp(TargetCameraDistance - CameraZoomStep, MinCameraDistance, MaxCameraDistance);
}

void UUOUCameraControllerComponent::ZoomCameraOut()
{
	TargetCameraDistance = FMath::Clamp(TargetCameraDistance + CameraZoomStep, MinCameraDistance, MaxCameraDistance);
}

float UUOUCameraControllerComponent::GetMovementYaw() const
{
	return CameraBoom != nullptr ? CameraBoom->GetComponentRotation().Yaw : 0.0f;
}

bool UUOUCameraControllerComponent::IsSnapCameraRotationInProgress() const
{
	if (CameraBoom == nullptr || bDialogueFocusActive)
	{
		return false;
	}

	const float CurrentYaw = CameraBoom->GetComponentRotation().Yaw;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetCameraYaw);
	return FMath::Abs(DeltaYaw) > CameraRotationCompleteToleranceDegrees;
}

float UUOUCameraControllerComponent::GetCurrentCameraDistance() const
{
	return CameraBoom != nullptr ? CameraBoom->TargetArmLength : 0.0f;
}

void UUOUCameraControllerComponent::SetCameraOcclusionEnabled(bool bNewEnabled)
{
	if (bCameraOcclusionEnabled == bNewEnabled)
	{
		return;
	}

	bCameraOcclusionEnabled = bNewEnabled;
	if (!bCameraOcclusionEnabled)
	{
		RestoreAllOccludedMeshes();
	}
}

void UUOUCameraControllerComponent::StartDialogueFocus(AActor* SpeakerActor)
{
	if (SpeakerActor == nullptr || CameraBoom == nullptr || FollowCamera == nullptr)
	{
		return;
	}

	if (!bDialogueFocusActive)
	{
		SavedTargetCameraYaw = TargetCameraYaw;
		SavedTargetCameraDistance = TargetCameraDistance;
		RegularCameraTargetOffset = CameraBoom->TargetOffset;
		TargetCameraOffset = RegularCameraTargetOffset;
	}

	DialogueSpeakerActor = SpeakerActor;
	bDialogueFocusActive = true;

	const FVector OwnerLocation = GetOwnerDialogueLocation();
	const FVector SpeakerLocation = GetSpeakerDialogueLocation();
	FVector PairDirection = SpeakerLocation - OwnerLocation;
	PairDirection.Z = 0.0f;
	if (!PairDirection.Normalize())
	{
		PairDirection = GetOwner() != nullptr ? GetOwner()->GetActorForwardVector() : FVector::ForwardVector;
		PairDirection.Z = 0.0f;
		PairDirection.Normalize();
	}

	const FVector PairRight = FVector::CrossProduct(FVector::UpVector, PairDirection).GetSafeNormal();
	const float CurrentSide = FVector::DotProduct(FollowCamera->GetComponentLocation() - OwnerLocation, PairRight);
	DialogueSideSign = bKeepCurrentDialogueCameraSide && CurrentSide < 0.0f ? -1.0f : 1.0f;
}

void UUOUCameraControllerComponent::EndDialogueFocus()
{
	if (!bDialogueFocusActive)
	{
		return;
	}

	bDialogueFocusActive = false;
	DialogueSpeakerActor = nullptr;
	TargetCameraYaw = SavedTargetCameraYaw;
	TargetCameraDistance = SavedTargetCameraDistance;
	TargetCameraOffset = RegularCameraTargetOffset;
}

void UUOUCameraControllerComponent::CacheCameraComponents()
{
	if (!bAutoFindCameraComponents)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (CameraBoom == nullptr)
	{
		CameraBoom = Owner->FindComponentByClass<USpringArmComponent>();
	}

	if (FollowCamera == nullptr)
	{
		FollowCamera = Owner->FindComponentByClass<UCameraComponent>();
	}
}

void UUOUCameraControllerComponent::InitializeCameraRig()
{
	if (CameraBoom == nullptr)
	{
		return;
	}

	MinCameraDistance = FMath::Max(0.0f, MinCameraDistance);
	MaxCameraDistance = FMath::Max(MinCameraDistance, MaxCameraDistance);
	CameraZoomStep = FMath::Max(0.0f, CameraZoomStep);
	CameraZoomInterpSpeed = FMath::Max(0.0f, CameraZoomInterpSpeed);
	CameraRotationInterpSpeed = FMath::Max(0.0f, CameraRotationInterpSpeed);

	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;

	TargetCameraYaw = CameraBoom->GetComponentRotation().Yaw;
	TargetCameraDistance = FMath::Clamp(CameraBoom->TargetArmLength, MinCameraDistance, MaxCameraDistance);
	RegularCameraTargetOffset = CameraBoom->TargetOffset;
	TargetCameraOffset = RegularCameraTargetOffset;
	CameraBoom->TargetArmLength = TargetCameraDistance;
	CameraBoom->SetWorldRotation(FRotator(CameraPitchAngle, TargetCameraYaw, 0.0f));
}

void UUOUCameraControllerComponent::ApplyCameraProjection()
{
	if (FollowCamera == nullptr)
	{
		return;
	}

	FollowCamera->SetProjectionMode(bUseOrthographicProjection
		? ECameraProjectionMode::Orthographic
		: ECameraProjectionMode::Perspective);
	FollowCamera->SetOrthoWidth(FMath::Max(1.0f, OrthographicWidth));
}

void UUOUCameraControllerComponent::UpdateSnapCamera(float DeltaSeconds)
{
	if (CameraBoom == nullptr)
	{
		return;
	}

	const FRotator CurrentRotation = CameraBoom->GetComponentRotation();
	const FRotator TargetRotation(CameraPitchAngle, TargetCameraYaw, 0.0f);
	const FRotator NextRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, CameraRotationInterpSpeed);
	CameraBoom->SetWorldRotation(NextRotation);

	const float NextDistance = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetCameraDistance, DeltaSeconds, CameraZoomInterpSpeed);
	CameraBoom->TargetArmLength = NextDistance;

	if (FollowCamera != nullptr && FollowCamera->ProjectionMode == ECameraProjectionMode::Orthographic)
	{
		const float NextOrthoWidth = FMath::FInterpTo(FollowCamera->OrthoWidth, OrthographicWidth, DeltaSeconds, CameraZoomInterpSpeed);
		FollowCamera->SetOrthoWidth(FMath::Max(1.0f, NextOrthoWidth));
	}

	CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, TargetCameraOffset, DeltaSeconds, CameraRotationInterpSpeed);
}

void UUOUCameraControllerComponent::UpdateDialogueCamera(float DeltaSeconds)
{
	if (CameraBoom == nullptr || DialogueSpeakerActor == nullptr)
	{
		EndDialogueFocus();
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		EndDialogueFocus();
		return;
	}

	const FVector OwnerLocation = GetOwnerDialogueLocation();
	const FVector SpeakerLocation = GetSpeakerDialogueLocation();
	const FVector FocusGroundLocation = FMath::Lerp(OwnerLocation, SpeakerLocation, DialogueFocusBiasToSpeaker);
	const FVector FocusLocation = FocusGroundLocation + FVector(0.0f, 0.0f, DialogueLookAtHeight);

	FVector PairDirection = SpeakerLocation - OwnerLocation;
	PairDirection.Z = 0.0f;
	if (!PairDirection.Normalize())
	{
		PairDirection = Owner->GetActorForwardVector();
		PairDirection.Z = 0.0f;
		PairDirection.Normalize();
	}

	const FVector PairRight = FVector::CrossProduct(FVector::UpVector, PairDirection).GetSafeNormal();
	const FVector CameraBackDirection = (-PairDirection).RotateAngleAxis(DialogueOrbitAngleDegrees * DialogueSideSign, FVector::UpVector).GetSafeNormal();
	const FVector DesiredCameraLocation = FocusLocation
		+ CameraBackDirection * DialogueCameraDistance
		+ PairRight * DialogueShoulderOffset * DialogueSideSign
		+ FVector(0.0f, 0.0f, DialogueCameraHeightOffset);
	const FRotator DesiredRotation = (FocusLocation - DesiredCameraLocation).Rotation();
	const float DesiredDistance = FVector::Distance(FocusLocation, DesiredCameraLocation);

	TargetCameraOffset = FocusLocation - Owner->GetActorLocation();
	CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, TargetCameraOffset, DeltaSeconds, DialogueCameraInterpSpeed);
	CameraBoom->SetWorldRotation(FMath::RInterpTo(CameraBoom->GetComponentRotation(), DesiredRotation, DeltaSeconds, DialogueCameraInterpSpeed));
	if (bAdjustDistanceDuringDialogue)
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DesiredDistance, DeltaSeconds, DialogueCameraInterpSpeed);
	}

	if (bAdjustOrthoWidthDuringDialogue && FollowCamera != nullptr && FollowCamera->ProjectionMode == ECameraProjectionMode::Orthographic)
	{
		const float NextOrthoWidth = FMath::FInterpTo(FollowCamera->OrthoWidth, DialogueOrthographicWidth, DeltaSeconds, DialogueCameraInterpSpeed);
		FollowCamera->SetOrthoWidth(FMath::Max(1.0f, NextOrthoWidth));
	}
}

void UUOUCameraControllerComponent::UpdateCameraOcclusion()
{
	if (!bCameraOcclusionEnabled)
	{
		if (OccludedMeshStates.Num() > 0)
		{
			RestoreAllOccludedMeshes();
		}
		return;
	}

	if (FollowCamera == nullptr)
	{
		RestoreAllOccludedMeshes();
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner == nullptr || World == nullptr)
	{
		RestoreAllOccludedMeshes();
		return;
	}

	const FVector TraceStart = FollowCamera->GetComponentLocation();
	const FVector TraceEnd = GetCameraOcclusionTraceEnd(Owner);
	const FVector CameraRight = FollowCamera->GetRightVector();
	const FVector CameraUp = FollowCamera->GetUpVector();

	TArray<FVector2D> ProbeOffsets;
	BuildCameraOcclusionProbeOffsets(ProbeOffsets);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CameraOcclusionTrace), false, Owner);
	QueryParams.AddIgnoredActor(Owner);
	if (bDialogueFocusActive && DialogueSpeakerActor != nullptr)
	{
		QueryParams.AddIgnoredActor(DialogueSpeakerActor);
	}

	TSet<TObjectPtr<UMeshComponent>> DesiredOccludedMeshes;
	const int32 OccluderLimit = FMath::Max(1, MaxOccludedMeshCount);

	for (int32 SweepPassIndex = 0; SweepPassIndex < OccluderLimit; ++SweepPassIndex)
	{
		bool bAddedMeshThisPass = false;

		for (const FVector2D& ProbeOffset : ProbeOffsets)
		{
			if (DesiredOccludedMeshes.Num() >= OccluderLimit)
			{
				break;
			}

			const FVector WorldProbeOffset = CameraRight * ProbeOffset.X + CameraUp * ProbeOffset.Y;

			TArray<FHitResult> HitResults;
			const bool bHitAnything = World->SweepMultiByChannel(
				HitResults,
				TraceStart + WorldProbeOffset,
				TraceEnd + WorldProbeOffset,
				FQuat::Identity,
				ECC_Visibility,
				FCollisionShape::MakeSphere(OcclusionProbeRadius),
				QueryParams);

			if (!bHitAnything)
			{
				continue;
			}

			HitResults.Sort([](const FHitResult& Left, const FHitResult& Right)
			{
				return Left.Distance < Right.Distance;
			});

			for (const FHitResult& HitResult : HitResults)
			{
				UMeshComponent* MeshComponent = Cast<UMeshComponent>(HitResult.GetComponent());
				if (MeshComponent == nullptr)
				{
					continue;
				}

				if (DesiredOccludedMeshes.Contains(MeshComponent))
				{
					continue;
				}

				if (!IsCameraOcclusionCandidate(MeshComponent, Owner, TraceEnd))
				{
					QueryParams.AddIgnoredComponent(MeshComponent);
					continue;
				}

				DesiredOccludedMeshes.Add(MeshComponent);
				QueryParams.AddIgnoredComponent(MeshComponent);
				bAddedMeshThisPass = true;

				if (DesiredOccludedMeshes.Num() >= OccluderLimit)
				{
					break;
				}
			}
		}

		if (!bAddedMeshThisPass || DesiredOccludedMeshes.Num() >= OccluderLimit)
		{
			break;
		}
	}

	TArray<TObjectPtr<UMeshComponent>> MeshesToRestore;
	for (const TPair<TObjectPtr<UMeshComponent>, FOccludedMeshState>& Pair : OccludedMeshStates)
	{
		if (!DesiredOccludedMeshes.Contains(Pair.Key))
		{
			MeshesToRestore.Add(Pair.Key);
		}
	}

	for (UMeshComponent* MeshComponent : MeshesToRestore)
	{
		RestoreOcclusionFromMesh(MeshComponent);
	}

	for (UMeshComponent* MeshComponent : DesiredOccludedMeshes)
	{
		ApplyOcclusionToMesh(MeshComponent);
	}
}

FVector UUOUCameraControllerComponent::GetCameraOcclusionTraceEnd(const AActor* Owner) const
{
	if (Owner == nullptr)
	{
		return FVector::ZeroVector;
	}

	if (bDialogueFocusActive)
	{
		return Owner->GetActorLocation() + TargetCameraOffset;
	}

	return Owner->GetActorLocation() + OcclusionTargetOffset;
}

void UUOUCameraControllerComponent::BuildCameraOcclusionProbeOffsets(TArray<FVector2D>& OutProbeOffsets) const
{
	OutProbeOffsets.Reset();
	OutProbeOffsets.Add(FVector2D::ZeroVector);

	const float LateralExtent = FMath::Max(0.0f, OcclusionProbeLateralExtent);
	const float VerticalExtent = FMath::Max(0.0f, OcclusionProbeVerticalExtent);

	if (LateralExtent > KINDA_SMALL_NUMBER)
	{
		OutProbeOffsets.Add(FVector2D(LateralExtent, 0.0f));
		OutProbeOffsets.Add(FVector2D(-LateralExtent, 0.0f));
	}

	if (VerticalExtent > KINDA_SMALL_NUMBER)
	{
		OutProbeOffsets.Add(FVector2D(0.0f, VerticalExtent));
		OutProbeOffsets.Add(FVector2D(0.0f, -VerticalExtent));
	}

	if (LateralExtent > KINDA_SMALL_NUMBER && VerticalExtent > KINDA_SMALL_NUMBER)
	{
		OutProbeOffsets.Add(FVector2D(LateralExtent, VerticalExtent));
		OutProbeOffsets.Add(FVector2D(LateralExtent, -VerticalExtent));
		OutProbeOffsets.Add(FVector2D(-LateralExtent, VerticalExtent));
		OutProbeOffsets.Add(FVector2D(-LateralExtent, -VerticalExtent));
	}
}

bool UUOUCameraControllerComponent::IsCameraOcclusionCandidate(const UMeshComponent* MeshComponent, const AActor* Owner, const FVector& TraceEnd) const
{
	if (MeshComponent == nullptr)
	{
		return false;
	}

	if (MeshComponent->GetOwner() == Owner)
	{
		return false;
	}

	if (IsOwnerSupportMesh(MeshComponent, Owner))
	{
		return false;
	}

	const FBoxSphereBounds& MeshBounds = MeshComponent->Bounds;
	const float MeshTopZ = MeshBounds.Origin.Z + MeshBounds.BoxExtent.Z;
	const float RequiredTopZ = TraceEnd.Z - FMath::Max(0.0f, OcclusionLowObjectIgnoreBelowTarget);
	if (MeshTopZ < RequiredTopZ)
	{
		return false;
	}

	return true;
}

bool UUOUCameraControllerComponent::IsOwnerSupportMesh(const UMeshComponent* MeshComponent, const AActor* Owner) const
{
	if (MeshComponent == nullptr || Owner == nullptr)
	{
		return false;
	}

	const ACharacter* CharacterOwner = Cast<ACharacter>(Owner);
	if (CharacterOwner == nullptr)
	{
		return false;
	}

	if (const UPrimitiveComponent* MovementBaseComponent = CharacterOwner->GetMovementBase())
	{
		if (MeshComponent == MovementBaseComponent)
		{
			return true;
		}
	}

	const UCharacterMovementComponent* CharacterMovement = CharacterOwner->GetCharacterMovement();
	const UPrimitiveComponent* FloorComponent = CharacterMovement != nullptr
		? CharacterMovement->CurrentFloor.HitResult.GetComponent()
		: nullptr;

	return MeshComponent == FloorComponent;
}

void UUOUCameraControllerComponent::ApplyOcclusionToMesh(UMeshComponent* MeshComponent)
{
	if (!IsValid(MeshComponent) || OccludedMeshStates.Contains(MeshComponent))
	{
		return;
	}

	FOccludedMeshState NewState;
	const int32 MaterialCount = MeshComponent->GetNumMaterials();
	NewState.OriginalMaterials.Reserve(MaterialCount);
	NewState.bWasVisible = MeshComponent->IsVisible();

	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		NewState.OriginalMaterials.Add(MeshComponent->GetMaterial(MaterialIndex));

		if (OccluderFadeMaterial != nullptr)
		{
			MeshComponent->SetMaterial(MaterialIndex, OccluderFadeMaterial);
		}
	}

	if (OccluderFadeMaterial == nullptr)
	{
		MeshComponent->SetVisibility(false, true);
	}

	OccludedMeshStates.Add(MeshComponent, MoveTemp(NewState));
}

void UUOUCameraControllerComponent::RestoreOcclusionFromMesh(UMeshComponent* MeshComponent)
{
	if (!IsValid(MeshComponent))
	{
		OccludedMeshStates.Remove(MeshComponent);
		return;
	}

	FOccludedMeshState* State = OccludedMeshStates.Find(MeshComponent);
	if (State == nullptr)
	{
		return;
	}

	const int32 RestoreMaterialCount = FMath::Min(MeshComponent->GetNumMaterials(), State->OriginalMaterials.Num());
	for (int32 MaterialIndex = 0; MaterialIndex < RestoreMaterialCount; ++MaterialIndex)
	{
		UMaterialInterface* OriginalMaterial = State->OriginalMaterials[MaterialIndex].Get();
		if (OriginalMaterial == nullptr || IsValid(OriginalMaterial))
		{
			MeshComponent->SetMaterial(MaterialIndex, OriginalMaterial);
		}
	}

	MeshComponent->SetVisibility(State->bWasVisible, true);
	OccludedMeshStates.Remove(MeshComponent);
}

void UUOUCameraControllerComponent::RestoreAllOccludedMeshes()
{
	TArray<TObjectPtr<UMeshComponent>> MeshesToRestore;
	OccludedMeshStates.GenerateKeyArray(MeshesToRestore);

	for (UMeshComponent* MeshComponent : MeshesToRestore)
	{
		RestoreOcclusionFromMesh(MeshComponent);
	}
}

FVector UUOUCameraControllerComponent::GetOwnerDialogueLocation() const
{
	const AActor* Owner = GetOwner();
	return Owner != nullptr ? Owner->GetActorLocation() : FVector::ZeroVector;
}

FVector UUOUCameraControllerComponent::GetSpeakerDialogueLocation() const
{
	return DialogueSpeakerActor != nullptr ? DialogueSpeakerActor->GetActorLocation() : GetOwnerDialogueLocation();
}
