// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUCameraControllerComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

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

	UpdateSnapCamera(DeltaTime);
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
	CameraBoom->TargetArmLength = TargetCameraDistance;
	CameraBoom->SetWorldRotation(FRotator(CameraPitchAngle, TargetCameraYaw, 0.0f));
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
}

void UUOUCameraControllerComponent::UpdateCameraOcclusion()
{
	if (FollowCamera == nullptr)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner == nullptr || World == nullptr)
	{
		return;
	}

	const FVector TraceStart = FollowCamera->GetComponentLocation();
	const FVector TraceEnd = Owner->GetActorLocation() + OcclusionTargetOffset;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CameraOcclusionTrace), false, Owner);
	QueryParams.AddIgnoredActor(Owner);

	TArray<FHitResult> HitResults;
	const bool bHitAnything = World->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(OcclusionProbeRadius),
		QueryParams);

	TSet<TObjectPtr<UMeshComponent>> DesiredOccludedMeshes;

	if (bHitAnything)
	{
		for (const FHitResult& HitResult : HitResults)
		{
			UMeshComponent* MeshComponent = Cast<UMeshComponent>(HitResult.GetComponent());
			if (MeshComponent == nullptr)
			{
				continue;
			}

			if (MeshComponent->GetOwner() == Owner)
			{
				continue;
			}

			DesiredOccludedMeshes.Add(MeshComponent);
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

void UUOUCameraControllerComponent::ApplyOcclusionToMesh(UMeshComponent* MeshComponent)
{
	if (MeshComponent == nullptr || OccludedMeshStates.Contains(MeshComponent))
	{
		return;
	}

	FOccludedMeshState NewState;
	const int32 MaterialCount = MeshComponent->GetNumMaterials();
	NewState.OriginalMaterials.Reserve(MaterialCount);

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
	if (MeshComponent == nullptr)
	{
		return;
	}

	FOccludedMeshState* State = OccludedMeshStates.Find(MeshComponent);
	if (State == nullptr)
	{
		return;
	}

	for (int32 MaterialIndex = 0; MaterialIndex < State->OriginalMaterials.Num(); ++MaterialIndex)
	{
		MeshComponent->SetMaterial(MaterialIndex, State->OriginalMaterials[MaterialIndex]);
	}

	MeshComponent->SetVisibility(true, true);
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
