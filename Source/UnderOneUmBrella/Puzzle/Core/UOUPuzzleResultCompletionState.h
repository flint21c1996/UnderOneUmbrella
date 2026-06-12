// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UObject/Interface.h"
#include "UOUPuzzleResultCompletionState.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnUOUPuzzleResultCompletionStateChangedNativeSignature,
	EOUUPuzzleResultAction,
	bool);

// 결과 액터가 특정 결과 액션의 완료 여부를 알려줄 때 선택적으로 구현하는 인터페이스입니다.
UINTERFACE(BlueprintType)
class UUOUPuzzleResultCompletionState : public UInterface
{
	GENERATED_BODY()
};

class IUOUPuzzleResultCompletionState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Puzzle|Result")
	bool IsPuzzleResultCompleted(EOUUPuzzleResultAction Action) const;

	// 완료 상태가 바뀌는 순간을 C++ 조건 컴포넌트가 Tick 없이 구독할 때 사용합니다.
	virtual FOnUOUPuzzleResultCompletionStateChangedNativeSignature* GetPuzzleResultCompletionStateChangedEvent()
	{
		return nullptr;
	}
};
