// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaCoverVolumeComponent.h"

UUOUUmbrellaCoverVolumeComponent::UUOUUmbrellaCoverVolumeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 생성자에서는 컴포넌트가 아직 월드에 등록되기 전이라 런타임 갱신 함수 대신 기본 BodyInstance 값만 잡습니다.
	InitBoxExtent(FVector(130.0f, 130.0f, 90.0f));
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyInstance.SetObjectType(ECC_WorldDynamic);
	BodyInstance.SetResponseToAllChannels(ECR_Ignore);
	SetGenerateOverlapEvents(false);
}

void UUOUUmbrellaCoverVolumeComponent::OnRegister()
{
	Super::OnRegister();

	ApplyDialogueCoverCollisionSettings();
}

bool UUOUUmbrellaCoverVolumeComponent::CanUseForDialogueCover() const
{
	return IsRegistered()
		&& IsActive();
}

void UUOUUmbrellaCoverVolumeComponent::ApplyDialogueCoverCollisionSettings()
{
	// 대화 커버 판정은 Overlap 이벤트가 아니라 Bounds 직접 비교로 처리합니다.
	// 물리 센서나 버튼이 플레이어 주변 우산 박스를 입력으로 잡지 않도록 충돌은 항상 꺼둡니다.
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetGenerateOverlapEvents(false);
}
