// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaRuntimeVisualPresenter.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Player/UOUUmbrellaVisualPolicy.h"

UStaticMeshComponent* FUOUUmbrellaRuntimeVisualPresenter::EnsureVisual(
	AActor* Owner,
	USceneComponent* AttachParent,
	UStaticMeshComponent* ExistingVisual,
	const FTransform& InitialRelativeTransform)
{
	if (ExistingVisual != nullptr || Owner == nullptr)
	{
		return ExistingVisual;
	}

	UStaticMeshComponent* Visual = NewObject<UStaticMeshComponent>(Owner, TEXT("RuntimeHeldUmbrellaVisual"));
	if (Visual == nullptr)
	{
		return nullptr;
	}

	Owner->AddInstanceComponent(Visual);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetCastShadow(true);
	Visual->SetVisibility(false, true);
	Visual->SetupAttachment(AttachParent != nullptr ? AttachParent : Owner->GetRootComponent());
	Visual->RegisterComponent();
	Visual->SetRelativeTransform(InitialRelativeTransform);
	return Visual;
}

FUOUUmbrellaRuntimeVisualAssets FUOUUmbrellaRuntimeVisualPresenter::CaptureAssets(
	const UStaticMeshComponent* SourceVisual)
{
	FUOUUmbrellaRuntimeVisualAssets Assets;
	if (SourceVisual == nullptr)
	{
		return Assets;
	}

	Assets.Mesh = SourceVisual->GetStaticMesh();
	Assets.SourceRelativeScale = SourceVisual->GetRelativeScale3D();

	const int32 MaterialCount = SourceVisual->GetNumMaterials();
	Assets.Materials.Reserve(MaterialCount);
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		Assets.Materials.Add(SourceVisual->GetMaterial(MaterialIndex));
	}
	return Assets;
}

void FUOUUmbrellaRuntimeVisualPresenter::ApplyAssets(
	UStaticMeshComponent* Visual,
	const FUOUUmbrellaRuntimeVisualAssets& Assets,
	UStaticMesh* DefaultMesh)
{
	if (Visual == nullptr)
	{
		return;
	}

	Visual->SetStaticMesh(Assets.Mesh != nullptr ? Assets.Mesh : DefaultMesh);
	for (int32 MaterialIndex = 0; MaterialIndex < Assets.Materials.Num(); ++MaterialIndex)
	{
		Visual->SetMaterial(MaterialIndex, Assets.Materials[MaterialIndex]);
	}
}

FTransform FUOUUmbrellaRuntimeVisualPresenter::CalculateBaseRelativeTransform(
	const FTransform& AnchorRelativeTransform,
	const FVector& HeldVisualRelativeScale,
	const FVector& SourceRelativeScale,
	bool bUseSourceRelativeScale)
{
	const FVector EffectiveSourceScale = bUseSourceRelativeScale
		? SourceRelativeScale
		: FVector::OneVector;
	const FVector RelativeScale(
		HeldVisualRelativeScale.X * EffectiveSourceScale.X,
		HeldVisualRelativeScale.Y * EffectiveSourceScale.Y,
		HeldVisualRelativeScale.Z * EffectiveSourceScale.Z);

	return FTransform(
		AnchorRelativeTransform.GetRotation(),
		AnchorRelativeTransform.GetLocation(),
		RelativeScale);
}

FTransform FUOUUmbrellaRuntimeVisualPresenter::CalculateStateRelativeTransform(
	const FTransform& BaseRelativeTransform,
	bool bFlipWhenReversed,
	EUOUUmbrellaVisualState VisualState,
	const FRotator& ReversedRotationOffset,
	const FVector& ReversedLocationOffset)
{
	FTransform Result = BaseRelativeTransform;
	if (FUOUUmbrellaVisualPolicy::ShouldFlipRuntimeVisual(bFlipWhenReversed, VisualState))
	{
		const FQuat FlippedRotation = Result.GetRotation() * ReversedRotationOffset.Quaternion();
		Result.SetRotation(FlippedRotation.GetNormalized());
		Result.AddToTranslation(ReversedLocationOffset);
	}
	return Result;
}

void FUOUUmbrellaRuntimeVisualPresenter::ApplyStateTransform(
	UStaticMeshComponent* Visual,
	const FTransform& BaseRelativeTransform,
	bool bFlipWhenReversed,
	EUOUUmbrellaVisualState VisualState,
	const FRotator& ReversedRotationOffset,
	const FVector& ReversedLocationOffset)
{
	if (Visual == nullptr)
	{
		return;
	}

	Visual->SetRelativeTransform(CalculateStateRelativeTransform(
		BaseRelativeTransform,
		bFlipWhenReversed,
		VisualState,
		ReversedRotationOffset,
		ReversedLocationOffset));
}
