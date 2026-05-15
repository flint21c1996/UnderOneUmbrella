// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UOUPuzzleWeightSource.generated.h"

// 무게 계산 인터페이스를 엔진이 인식할 수 있게 하는 리플렉션용 껍데기입니다.
UINTERFACE(BlueprintType)
class UUOUPuzzleWeightSource : public UInterface
{
	GENERATED_BODY()
};

// 퍼즐에서 사용할 무게 값을 제공하는 C++ 인터페이스 본체입니다.
// 버튼이나 저울 센서는 이 인터페이스를 통해 공통 방식으로 무게를 읽습니다.
class IUOUPuzzleWeightSource
{
	GENERATED_BODY()

public:
	// 현재 퍼즐 판정에 사용할 무게 값을 반환합니다.
	virtual float GetPuzzleWeight() const = 0;
};
