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

protected:
	virtual void OnRegister() override;

public:
	// 대화 커버 판정에 사용할 수 있는 상태인지 확인합니다.
	// 실제 충돌 이벤트가 아니라 컴포넌트 bounds를 직접 비교하므로 등록/활성 상태만 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella|Dialogue Cover")
	bool CanUseForDialogueCover() const;

private:
	void ApplyDialogueCoverCollisionSettings();
};
