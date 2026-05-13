// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "World/NPC/UOUNPCActionTypes.h"
#include "UOUNPCController.generated.h"

class UBehaviorTree;

// NPC 액션 요청을 Behavior Tree 블랙보드에 연결하는 AI Controller입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU NPC Controller", ToolTip = "NPC 기본 Behavior Tree를 실행하고 액션 요청을 블랙보드에 기록합니다."))
class AUOUNPCController : public AAIController
{
	GENERATED_BODY()

public:
	AUOUNPCController();

	virtual void OnPossess(APawn* InPawn) override;

	static const FName ActivatedKeyName;
	static const FName TargetActorKeyName;
	static const FName TargetLocationKeyName;
	static const FName ActionTypeKeyName;
	static const FName SelfActorKeyName;

	UFUNCTION(BlueprintCallable, Category = "NPC|Behavior", meta = (ToolTip = "소유 중인 NPC에 할당된 Behavior Tree를 실행합니다."))
	bool RunAssignedBehaviorTree(UBehaviorTree* BehaviorTreeAsset);

	UFUNCTION(BlueprintCallable, Category = "NPC|Behavior", meta = (ToolTip = "기존 단일 액션 NPC 설정에서 사용하는 활성화 블랙보드 값 설정 함수입니다."))
	bool SetActivationBlackboard(bool bNewActivated, AActor* TargetActor, const FVector& TargetLocation, uint8 ActionTypeValue);

	UFUNCTION(BlueprintCallable, Category = "NPC|Behavior", meta = (ToolTip = "현재 NPC 액션 요청을 블랙보드에 기록합니다."))
	bool SetActionBlackboard(bool bHasAction, const FUOUNPCActionRequest& ActionRequest);

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement", meta = (ToolTip = "목표 액터로 AI 이동을 요청합니다."))
	bool MoveToGoalActor(AActor* GoalActor, float AcceptanceRadius);

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement", meta = (ToolTip = "월드 위치로 AI 이동을 요청합니다."))
	bool MoveToGoalLocation(const FVector& GoalLocation, float AcceptanceRadius);
};
