// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUPuzzleConditionSourceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPuzzleConditionChangedSignature, bool, bIsSatisfied);

// 개별 퍼즐 조건이 만족되었는지만 알려주는 공통 기반 컴포넌트다.
UCLASS(Abstract, BlueprintType, Blueprintable, ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUPuzzleConditionSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPuzzleConditionSourceComponent();

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Condition")
	FOnPuzzleConditionChangedSignature OnConditionChanged;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Condition")
	bool IsSatisfied() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Condition")
	bool bIsSatisfied = false;

	bool SetSatisfiedState(bool bNewSatisfied, bool bBroadcastChange = true);
};
