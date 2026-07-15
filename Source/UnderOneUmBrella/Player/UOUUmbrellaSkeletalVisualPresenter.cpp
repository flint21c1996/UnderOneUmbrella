// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaSkeletalVisualPresenter.h"

#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Player/UOUUmbrellaAnimInstance.h"
#include "Player/UOUUmbrellaComponent.h"

const FUOUUmbrellaSkeletalVisualVariant& FUOUUmbrellaSkeletalVisualVariants::Resolve(
	EUOUUmbrellaVisualState VisualState) const
{
	switch (VisualState)
	{
	case EUOUUmbrellaVisualState::Open:
		return Open;
	case EUOUUmbrellaVisualState::ClosedReversed:
		return ClosedReversed;
	case EUOUUmbrellaVisualState::OpenReversed:
		return OpenReversed;
	case EUOUUmbrellaVisualState::Closed:
	default:
		return Closed;
	}
}

void FUOUUmbrellaSkeletalVisualPresenter::Apply(
	const FUOUUmbrellaSkeletalVisualRequest& Request,
	FUOUUmbrellaSkeletalVisualPlaybackState& PlaybackState)
{
	USkeletalMeshComponent* Visual = Request.Visual;
	if (Visual == nullptr)
	{
		return;
	}

	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetCastShadow(true);

	UUOUUmbrellaAnimInstance* UmbrellaAnimInstance = Cast<UUOUUmbrellaAnimInstance>(Visual->GetAnimInstance());
	if (!Request.bHasUmbrella)
	{
		Visual->SetVisibility(false, true);
		PlaybackState = FUOUUmbrellaSkeletalVisualPlaybackState();

		if (UmbrellaAnimInstance != nullptr)
		{
			UmbrellaAnimInstance->SetUmbrellaState(
				false,
				Request.State,
				Request.DirectionState,
				Request.VisualState);
		}
		return;
	}

	USceneComponent* AttachParent = nullptr;
	FName AttachSocketName = NAME_None;
	if (Request.bAttachToOwnerMeshSocket)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(Request.Owner))
		{
			AttachParent = OwnerCharacter->GetMesh();
			AttachSocketName = Request.Variant.SocketName;
		}
	}

	if (AttachParent == nullptr)
	{
		AttachParent = Request.HeldVisualAnchor != nullptr
			? Request.HeldVisualAnchor
			: (Request.PickupAttachPoint != nullptr
				? Request.PickupAttachPoint
				: (Request.Owner != nullptr ? Request.Owner->GetRootComponent() : nullptr));
	}

	if (AttachParent != nullptr)
	{
		Visual->AttachToComponent(
			AttachParent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);
	}

	Visual->SetRelativeTransform(Request.Variant.RelativeTransform);
	Visual->SetVisibility(true, true);

	if (UmbrellaAnimInstance != nullptr)
	{
		UmbrellaAnimInstance->SetUmbrellaState(
			true,
			Request.State,
			Request.DirectionState,
			Request.VisualState);
	}

	if (!Request.bPlayAnimationDirectly)
	{
		PlaybackState = FUOUUmbrellaSkeletalVisualPlaybackState();
		return;
	}

	UAnimationAsset* AnimationToPlay = Request.Variant.Animation;
	if (AnimationToPlay != nullptr
		&& (!PlaybackState.bHasAppliedAnimation || PlaybackState.LastAppliedAnimation != AnimationToPlay))
	{
		Visual->PlayAnimation(AnimationToPlay, true);
		PlaybackState.LastAppliedAnimation = AnimationToPlay;
		PlaybackState.bHasAppliedAnimation = true;
	}
}
