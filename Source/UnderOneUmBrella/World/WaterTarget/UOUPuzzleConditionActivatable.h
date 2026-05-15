// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UOUPuzzleConditionActivatable.generated.h"

class AActor;
class UActorComponent;
class UUOUWaterBasinPlatformComponent;
class UUOUWaterBasinTargetComponent;

// PuzzleConditionComponent가 반응 대상에게 전달하는 조건 판정 정보입니다.
// ReactionComponent는 이 값을 보고 어떤 조건이 어떤 물 상태에서 만족/해제되었는지 판단할 수 있습니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUPuzzleConditionContext
{
	GENERATED_BODY()

	// 조건을 판정한 Actor입니다. 보통 Platform Actor가 됩니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition")
	TObjectPtr<AActor> ConditionOwner = nullptr;

	// 조건을 판정한 컴포넌트입니다. 추후 PuzzleConditionComponent가 들어갑니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition")
	TObjectPtr<UActorComponent> ConditionComponent = nullptr;

	// 여러 조건을 구분하기 위한 이름입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition")
	FName ConditionId = NAME_None;

	// 현재 조건 만족 여부입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition")
	bool bIsSatisfied = false;

	// 조건 판정에 사용된 현재 값입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition")
	float CurrentValue = 0.0f;

	// 조건 판정에 사용된 기준 값입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition")
	float ThresholdValue = 0.0f;

	// 조건과 연결된 WaterTile Actor입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition|Water")
	TObjectPtr<AActor> WaterTileActor = nullptr;

	// 조건과 연결된 WaterBasinTargetComponent입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition|Water")
	TObjectPtr<UUOUWaterBasinTargetComponent> WaterBasinTarget = nullptr;

	// WaterTile의 현재 수면 월드 Z입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition|Water")
	float WaterSurfaceWorldZ = 0.0f;

	// WaterTile의 현재 물 깊이입니다. WaterBasinTarget의 타일 단위 Depth 값입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition|Water")
	float WaterDepth = 0.0f;

	// WaterTile의 현재 물 깊이를 월드 단위(cm)로 변환한 값입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition|Water")
	float WaterDepthWorld = 0.0f;

	// WaterTile의 현재 물 채움 비율입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition|Water")
	float WaterFillRatio = 0.0f;

	// WaterTile의 현재 물 부피입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition|Water")
	float WaterVolume = 0.0f;

	// 조건과 연결된 PlatformComponent입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition|Platform")
	TObjectPtr<UUOUWaterBasinPlatformComponent> PlatformComponent = nullptr;

	// 플랫폼의 현재 월드 Z입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle Condition|Platform")
	float PlatformWorldZ = 0.0f;
};

// 조건을 만족했을 때 반응할 컴포넌트/액터가 구현하는 인터페이스입니다.
// 팀원은 UActorComponent에 이 인터페이스를 구현한 뒤 Platform Actor에 붙이면
// WaterBasinConditionComponent가 조건 변화 시 이 함수들을 호출할 수 있습니다.
UINTERFACE(Blueprintable)
class UNDERONEUMBRELLA_API UUOUPuzzleConditionActivatable : public UInterface
{
	GENERATED_BODY()
};

class UNDERONEUMBRELLA_API IUOUPuzzleConditionActivatable
{
	GENERATED_BODY()

public:
	// 조건이 만족되었을 때 이 대상이 실제로 실행 가능한지 확인합니다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Puzzle Condition")
	bool CanActivateByPuzzleCondition(const FUOUPuzzleConditionContext& Context) const;

	// 조건이 false에서 true로 바뀌는 순간 호출됩니다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Puzzle Condition")
	void ActivateByPuzzleCondition(const FUOUPuzzleConditionContext& Context);

	// 조건이 true에서 false로 바뀌는 순간 호출됩니다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Puzzle Condition")
	void DeactivateByPuzzleCondition(const FUOUPuzzleConditionContext& Context);

	// 조건의 만족 상태가 바뀔 때마다 호출됩니다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Puzzle Condition")
	void OnPuzzleConditionStateChanged(const FUOUPuzzleConditionContext& Context);
};
