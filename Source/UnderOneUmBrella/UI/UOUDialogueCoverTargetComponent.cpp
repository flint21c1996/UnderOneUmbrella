// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueCoverTargetComponent.h"

#include "Player/UOUUmbrellaCoverVolumeComponent.h"

UUOUDialogueCoverTargetComponent::UUOUDialogueCoverTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UUOUDialogueCoverTargetComponent::GetScaledCoverRadius() const
{
	const FVector AbsoluteScale = GetComponentScale().GetAbs();
	const float MaxScale = FMath::Max3(AbsoluteScale.X, AbsoluteScale.Y, AbsoluteScale.Z);
	return FMath::Max(0.0f, CoverTargetRadius * MaxScale);
}

bool UUOUDialogueCoverTargetComponent::IsCoveredByUmbrellaVolume(const UUOUUmbrellaCoverVolumeComponent* CoverVolume) const
{
	if (CoverVolume == nullptr
		|| !CoverVolume->CanUseForDialogueCover()
		|| !IsRegistered()
		|| !IsActive())
	{
		return false;
	}

	// 대화 커버는 물리 Overlap 이벤트가 아니라 월드 바운드와 기준점 반경이 닿았는지를 직접 봅니다.
	// 정확한 회전 박스보다 조금 관대한 AABB 판정이라, 대화 조건처럼 느슨해야 하는 검사에 잘 맞습니다.
	const FVector TargetCenter = GetComponentLocation();
	const float RequiredTouchDistance = GetScaledCoverRadius() + FMath::Max(0.0f, CoverTouchTolerance);
	const FBox CoverWorldBox = CoverVolume->Bounds.GetBox();
	if (!CoverWorldBox.IsValid)
	{
		return false;
	}

	const FVector ClosestPoint = CoverWorldBox.GetClosestPointTo(TargetCenter);
	return FVector::DistSquared(ClosestPoint, TargetCenter) <= FMath::Square(RequiredTouchDistance);
}
