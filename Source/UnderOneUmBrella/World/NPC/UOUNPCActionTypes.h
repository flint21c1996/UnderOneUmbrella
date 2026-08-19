// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUNPCActionTypes.generated.h"

class AActor;
class UAnimMontage;

UENUM(BlueprintType)
enum class EUOUNPCActionType : uint8
{
	MoveToTarget UMETA(DisplayName = "Move To Target", ToolTip = "NPC를 설정된 타겟으로 이동시킵니다."),
	PlayAnimation UMETA(DisplayName = "Play Animation", ToolTip = "설정된 NPC 애니메이션 몽타주를 재생합니다."),
	JumpMoveToTarget UMETA(DisplayName = "Jump Move To Target", ToolTip = "NPC를 설정된 타겟을 향해 점프 이동시킵니다."),
	None = 255 UMETA(Hidden)
};

// 이동, 점프, 애니메이션 중 하나의 NPC 액션을 실행하기 위해 전달되는 데이터입니다.
USTRUCT(BlueprintType)
struct FUOUNPCActionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action", meta = (ToolTip = "NPC에 요청할 액션 타입입니다."))
	EUOUNPCActionType ActionType = EUOUNPCActionType::MoveToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Target", meta = (ToolTip = "액션 목적지로 Target Actor를 사용합니다. 끄면 Target Location을 사용합니다."))
	bool bUseTargetActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Target", meta = (EditCondition = "bUseTargetActor", EditConditionHides, ToolTip = "이동/점프 액션의 목적지로 사용할 액터입니다. Target Point나 빈 Actor를 두고 쓰기 좋습니다."))
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Target", meta = (EditCondition = "!bUseTargetActor", EditConditionHides, ToolTip = "Target Actor를 사용하지 않을 때 목적지로 사용할 월드 위치입니다."))
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "0.0", ToolTip = "타겟에 충분히 가까워졌다고 판단할 거리입니다. 이동 태스크는 NPC 캡슐 반지름도 함께 고려합니다."))
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "0.1", ToolTip = "점프 발사 속도를 계산할 때 사용할 이동 시간입니다. 값이 낮을수록 더 빠르게 점프합니다."))
	float JumpTravelTime = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ToolTip = "점프 착지 후 같은 타겟으로 짧은 이동을 요청해 최종 위치를 보정합니다."))
	bool bMoveToTargetAfterJumpLanding = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation", meta = (ToolTip = "Play Animation 액션에서 재생할 몽타주입니다. 비어 있으면 NPC의 기본 몽타주를 사용합니다."))
	TObjectPtr<UAnimMontage> AnimationMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation", meta = (ClampMin = "0.0", ToolTip = "선택한 애니메이션 몽타주의 재생 속도입니다."))
	float AnimationPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation", meta = (ToolTip = "재생을 시작할 몽타주 섹션 이름입니다. None이면 처음부터 재생합니다."))
	FName AnimationStartSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation", meta = (ToolTip = "켜져 있으면 몽타주가 끝나는 시간으로 액션을 완료하지 않고 Deactivate될 때까지 유지합니다. 몽타주의 해당 섹션은 자체 루프로 설정되어 있어야 합니다."))
	bool bLoopAnimationUntilDeactivated = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation|Temperature", meta = (ToolTip = "현재 NPC의 UOU Light Exposure Receiver 온도에 따라 재생 중인 몽타주의 속도를 갱신합니다."))
	bool bUseTemperatureDrivenPlayRate = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation|Temperature", meta = (EditCondition = "bUseTemperatureDrivenPlayRate", ClampMin = "-273.15", ToolTip = "최소 애니메이션 재생 속도를 적용할 온도입니다."))
	float MinPlayRateTemperature = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation|Temperature", meta = (EditCondition = "bUseTemperatureDrivenPlayRate", ClampMin = "-273.15", ToolTip = "최대 애니메이션 재생 속도를 적용할 온도입니다."))
	float MaxPlayRateTemperature = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation|Temperature", meta = (EditCondition = "bUseTemperatureDrivenPlayRate", ClampMin = "0.01", ToolTip = "온도가 Min Play Rate Temperature 이하일 때 사용할 재생 속도입니다."))
	float MinTemperatureAnimationPlayRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation|Temperature", meta = (EditCondition = "bUseTemperatureDrivenPlayRate", ClampMin = "0.01", ToolTip = "온도가 Max Play Rate Temperature 이상일 때 사용할 재생 속도입니다."))
	float MaxTemperatureAnimationPlayRate = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action", meta = (ToolTip = "액션이 끝나면 현재 요청을 비우고 요청한 오브젝트에 완료를 알립니다. 시퀀스 액터는 자동으로 켭니다."))
	bool bClearActionOnFinish = false;
};
