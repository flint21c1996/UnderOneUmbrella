// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueCoverTargetComponent.h"

#include "Player/UOUUmbrellaCoverVolumeComponent.h"

UUOUDialogueCoverTargetComponent::UUOUDialogueCoverTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 이 컴포넌트는 실제 충돌 이벤트를 발생시키기보다 에디터에서 커버 반경을 보여주는 기준 스피어로 사용합니다.
	InitSphereRadius(80.0f);
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyInstance.SetObjectType(ECC_WorldDynamic);
	BodyInstance.SetResponseToAllChannels(ECR_Ignore);
	SetGenerateOverlapEvents(false);
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

	// 대화 커버는 물리 Overlap 이벤트가 아니라 월드 바운드와 스피어 반경이 닿았는지를 직접 봅니다.
	// 정확한 회전 박스보다 조금 관대한 AABB 판정이라, 대화 조건처럼 느슨해야 하는 검사에 잘 맞습니다.
	const FVector TargetCenter = GetComponentLocation();
	const float RequiredTouchDistance = FMath::Max(0.0f, GetScaledSphereRadius() + CoverTouchTolerance);
	const FBox CoverWorldBox = CoverVolume->Bounds.GetBox();
	if (!CoverWorldBox.IsValid)
	{
		return false;
	}

	const FVector ClosestPoint = CoverWorldBox.GetClosestPointTo(TargetCenter);
	return FVector::DistSquared(ClosestPoint, TargetCenter) <= FMath::Square(RequiredTouchDistance);
}
