// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

enum class EUOUUmbrellaVisualState : uint8;

struct FUOUUmbrellaRuntimeVisualAssets
{
	UStaticMesh* Mesh = nullptr;
	TArray<UMaterialInterface*> Materials;
	FVector SourceRelativeScale = FVector::OneVector;
};

// 월드 픽업에서 복사한 런타임 우산 메시의 생성과 표시 적용을 담당합니다.
class FUOUUmbrellaRuntimeVisualPresenter
{
public:
	static UStaticMeshComponent* EnsureVisual(
		AActor* Owner,
		USceneComponent* AttachParent,
		UStaticMeshComponent* ExistingVisual,
		const FTransform& InitialRelativeTransform);

	static FUOUUmbrellaRuntimeVisualAssets CaptureAssets(
		const UStaticMeshComponent* SourceVisual);

	static void ApplyAssets(
		UStaticMeshComponent* Visual,
		const FUOUUmbrellaRuntimeVisualAssets& Assets,
		UStaticMesh* DefaultMesh);

	static FTransform CalculateBaseRelativeTransform(
		const FTransform& AnchorRelativeTransform,
		const FVector& HeldVisualRelativeScale,
		const FVector& SourceRelativeScale,
		bool bUseSourceRelativeScale);

	static FTransform CalculateStateRelativeTransform(
		const FTransform& BaseRelativeTransform,
		bool bFlipWhenReversed,
		EUOUUmbrellaVisualState VisualState,
		const FRotator& ReversedRotationOffset,
		const FVector& ReversedLocationOffset);

	static void ApplyStateTransform(
		UStaticMeshComponent* Visual,
		const FTransform& BaseRelativeTransform,
		bool bFlipWhenReversed,
		EUOUUmbrellaVisualState VisualState,
		const FRotator& ReversedRotationOffset,
		const FVector& ReversedLocationOffset);
};
