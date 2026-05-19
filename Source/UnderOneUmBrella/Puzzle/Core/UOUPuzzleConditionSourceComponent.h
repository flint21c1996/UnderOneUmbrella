// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "UOUPuzzleConditionSourceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPuzzleConditionChangedSignature, bool, bIsSatisfied);

// 개별 퍼즐 조건의 만족 여부를 공통 방식으로 다루는 기반 컴포넌트입니다.
// 버튼, 물, 저울 같은 조건 소스는 이 클래스를 상속받아 상태를 노출합니다.
UCLASS(Abstract, BlueprintType, Blueprintable, ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUPuzzleConditionSourceComponent : public UActorComponent, public IUOUPuzzleDebugInfoProvider
{
	GENERATED_BODY()

public:
	UUOUPuzzleConditionSourceComponent();

	// 조건 만족 상태가 바뀌었을 때 그룹이나 외부 로직에 알리는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Condition")
	FOnPuzzleConditionChangedSignature OnConditionChanged;

	// 현재 조건이 만족 상태인지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle|Condition")
	bool IsSatisfied() const;

	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;

protected:
	// 현재 조건 만족 여부를 저장하는 공통 상태값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Condition")
	bool bIsSatisfied = false;

	// 만족 상태를 바꾸고 필요하면 변화 이벤트를 방송합니다.
	bool SetSatisfiedState(bool bNewSatisfied, bool bBroadcastChange = true);
};
