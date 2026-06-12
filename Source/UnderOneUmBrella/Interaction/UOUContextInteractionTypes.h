// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUContextInteractionTypes.generated.h"

class UAnimMontage;

// 상호작용 대상이 플레이어에게 요청할 공통 실행 정보입니다.
USTRUCT(BlueprintType)
struct FUOUPlayerInteractionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Player", meta = (ToolTip = "상호작용 중 플레이어가 재생할 몽타주입니다. 비워두면 즉시 완료됩니다."))
	TObjectPtr<UAnimMontage> PlayerMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Player", meta = (ClampMin = "0.01", ToolTip = "플레이어 몽타주 재생 속도입니다."))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Player", meta = (ToolTip = "플레이어 몽타주를 시작할 섹션 이름입니다. None이면 기본 시작 섹션을 사용합니다."))
	FName MontageStartSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Player", meta = (ToolTip = "상호작용 연출이 진행되는 동안 플레이어 조작 입력을 무시합니다."))
	bool bBlockPlayerInputDuringInteraction = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Player", meta = (ToolTip = "상호작용 연출을 시작할 때 현재 플레이어 이동을 즉시 멈춥니다."))
	bool bStopMovementOnStart = true;

	bool HasPlayerMontage() const
	{
		return PlayerMontage != nullptr;
	}
};
