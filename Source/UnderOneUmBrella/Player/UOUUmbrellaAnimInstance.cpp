// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaAnimInstance.h"

#include "Components/SkeletalMeshComponent.h"

void UUOUUmbrellaAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	TargetCloseAlpha = bHasUmbrella && (bIsOpen || bIsOpenReversed) ? 0.0f : 1.0f;
	if (CloseInterpSpeed <= 0.0f || DeltaSeconds <= 0.0f)
	{
		CloseAlpha = TargetCloseAlpha;
	}
	else
	{
		CloseAlpha = FMath::Clamp(
			FMath::FInterpTo(CloseAlpha, TargetCloseAlpha, DeltaSeconds, CloseInterpSpeed),
			0.0f,
			1.0f);
	}

	if (bApplyCloseMorphTargetDirectly && !CloseMorphTargetName.IsNone())
	{
		if (USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent())
		{
			// TODO: ABP Modify Curve 구성이 정리되면 이 임시 직접 적용 경로는 끄고 AnimGraph 제어로 전환합니다.
			MeshComponent->SetMorphTarget(CloseMorphTargetName, CloseAlpha, false);
		}
	}
}

void UUOUUmbrellaAnimInstance::SetUmbrellaState(
	bool bNewHasUmbrella,
	EUOUUmbrellaState NewUmbrellaState,
	EUOUUmbrellaDirectionState NewDirectionState,
	EUOUUmbrellaVisualState NewVisualState)
{
	bHasUmbrella = bNewHasUmbrella;
	UmbrellaState = NewUmbrellaState;
	DirectionState = NewDirectionState;
	VisualState = NewVisualState;

	bIsClosed = VisualState == EUOUUmbrellaVisualState::Closed;
	bIsOpen = VisualState == EUOUUmbrellaVisualState::Open;
	bIsClosedReversed = VisualState == EUOUUmbrellaVisualState::ClosedReversed;
	bIsOpenReversed = VisualState == EUOUUmbrellaVisualState::OpenReversed;
	bIsPouring = UmbrellaState == EUOUUmbrellaState::Pouring;
	TargetCloseAlpha = bHasUmbrella && (bIsOpen || bIsOpenReversed) ? 0.0f : 1.0f;
}
