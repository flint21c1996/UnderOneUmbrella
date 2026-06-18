// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "UOUPlayerAnimInstance.generated.h"

class AUOUCharacter;
class UUOUUmbrellaComponent;

// 플레이어 애니메이션 블루프린트에서 사용하는 런타임 상태를 갱신합니다.
UCLASS(BlueprintType, Blueprintable)
class UUOUPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement", meta = (ToolTip = "수평 이동 속도입니다."))
	float Speed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement", meta = (ToolTip = "캐릭터의 Z축 속도입니다."))
	float VerticalSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement", meta = (ToolTip = "캐릭터가 공중에 있는지 여부입니다."))
	bool IsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Umbrella", meta = (ToolTip = "우산을 보유하고 있는지 여부입니다."))
	bool HasUmbrella = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Umbrella", meta = (ToolTip = "우산이 펼쳐져 있는지 여부입니다."))
	bool IsUmbrellaOpen = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Umbrella", meta = (ToolTip = "우산이 뒤집혀 있는지 여부입니다."))
	bool IsUmbrellaUpsideDown = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Umbrella", meta = (ToolTip = "우산으로 물을 따르고 있는지 여부입니다."))
	bool IsPouring = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Umbrella", meta = (ToolTip = "일반 우산 보유 애니메이션 세트를 사용할지 여부입니다."))
	bool UseUmbrellaAnim = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Umbrella", meta = (ToolTip = "뒤집힌 우산 애니메이션 세트를 사용할지 여부입니다."))
	bool UseFlippedUmbrellaAnim = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Animation|References", meta = (ToolTip = "현재 애니메이션을 소유한 플레이어 캐릭터입니다."))
	TObjectPtr<AUOUCharacter> OwnerCharacter = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Animation|References", meta = (ToolTip = "소유 캐릭터에 붙어 있는 우산 컴포넌트입니다."))
	TObjectPtr<UUOUUmbrellaComponent> UmbrellaComponent = nullptr;

private:
	void CacheOwnerIfNeeded();
	void UpdateMovementVariables();
	void UpdateUmbrellaVariables();
	void UpdateDerivedAnimationVariables();
	void ResetMovementVariables();
	void ResetUmbrellaVariables();
};
