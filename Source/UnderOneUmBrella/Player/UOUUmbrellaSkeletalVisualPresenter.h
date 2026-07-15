// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UAnimationAsset;
class USceneComponent;
class USkeletalMeshComponent;

enum class EUOUUmbrellaDirectionState : uint8;
enum class EUOUUmbrellaState : uint8;
enum class EUOUUmbrellaVisualState : uint8;

struct FUOUUmbrellaSkeletalVisualVariant
{
	FName SocketName = NAME_None;
	FTransform RelativeTransform = FTransform::Identity;
	UAnimationAsset* Animation = nullptr;
};

struct FUOUUmbrellaSkeletalVisualVariants
{
	FUOUUmbrellaSkeletalVisualVariant Closed;
	FUOUUmbrellaSkeletalVisualVariant Open;
	FUOUUmbrellaSkeletalVisualVariant ClosedReversed;
	FUOUUmbrellaSkeletalVisualVariant OpenReversed;

	const FUOUUmbrellaSkeletalVisualVariant& Resolve(EUOUUmbrellaVisualState VisualState) const;
};

struct FUOUUmbrellaSkeletalVisualPlaybackState
{
	bool bHasAppliedAnimation = false;
	UAnimationAsset* LastAppliedAnimation = nullptr;
};

struct FUOUUmbrellaSkeletalVisualRequest
{
	USkeletalMeshComponent* Visual = nullptr;
	USceneComponent* HeldVisualAnchor = nullptr;
	USceneComponent* PickupAttachPoint = nullptr;
	AActor* Owner = nullptr;

	bool bHasUmbrella = false;
	bool bAttachToOwnerMeshSocket = true;
	bool bPlayAnimationDirectly = false;

	EUOUUmbrellaState State;
	EUOUUmbrellaDirectionState DirectionState;
	EUOUUmbrellaVisualState VisualState;
	FUOUUmbrellaSkeletalVisualVariant Variant;
};

// 스켈레탈 우산 비주얼의 부착, 표시, AnimInstance 전달과 직접 재생을 담당합니다.
class FUOUUmbrellaSkeletalVisualPresenter
{
public:
	static void Apply(
		const FUOUUmbrellaSkeletalVisualRequest& Request,
		FUOUUmbrellaSkeletalVisualPlaybackState& PlaybackState);
};
