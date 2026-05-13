// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUNPCActionTypes.generated.h"

class AActor;
class UAnimMontage;

UENUM(BlueprintType)
enum class EUOUNPCActionType : uint8
{
	MoveToTarget,
	PlayAnimation,
	JumpMoveToTarget,
	None = 255 UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FUOUNPCActionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action")
	EUOUNPCActionType ActionType = EUOUNPCActionType::MoveToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Target")
	bool bUseTargetActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Target", meta = (EditCondition = "bUseTargetActor", EditConditionHides))
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Target", meta = (EditCondition = "!bUseTargetActor", EditConditionHides))
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "0.1"))
	float JumpTravelTime = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Movement")
	bool bMoveToTargetAfterJumpLanding = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation")
	TObjectPtr<UAnimMontage> AnimationMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation", meta = (ClampMin = "0.0"))
	float AnimationPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Animation")
	FName AnimationStartSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action")
	bool bClearActionOnFinish = false;
};
