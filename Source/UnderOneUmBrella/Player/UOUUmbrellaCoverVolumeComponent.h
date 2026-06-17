// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "UOUUmbrellaCoverVolumeComponent.generated.h"

// 대화 판정 전용 우산 커버 박스입니다.
// 비 파티클 제거용 RainBlocker와 분리해서, NPC가 우산 아래에 들어왔는지만 판단합니다.
UCLASS(ClassGroup=(Player), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella Cover Volume"))
class UNDERONEUMBRELLA_API UUOUUmbrellaCoverVolumeComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UUOUUmbrellaCoverVolumeComponent();

	// 대화 커버 판정에 사용할 수 있는 상태인지 확인합니다.
	// 에디터에서 컴포넌트를 꺼두거나 충돌을 꺼두면 커버 판정에서 제외됩니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella|Dialogue Cover")
	bool CanUseForDialogueCover() const;
};
