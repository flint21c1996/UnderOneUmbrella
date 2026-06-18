// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaCoverVolumeComponent.h"

UUOUUmbrellaCoverVolumeComponent::UUOUUmbrellaCoverVolumeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 생성자에서는 컴포넌트가 아직 월드에 등록되기 전이라 런타임 갱신 함수 대신 기본 BodyInstance 값만 잡습니다.
	InitBoxExtent(FVector(130.0f, 130.0f, 90.0f));
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BodyInstance.SetObjectType(ECC_WorldDynamic);
	BodyInstance.SetResponseToAllChannels(ECR_Overlap);
	SetGenerateOverlapEvents(true);
}

bool UUOUUmbrellaCoverVolumeComponent::CanUseForDialogueCover() const
{
	return IsRegistered()
		&& IsActive();
}
