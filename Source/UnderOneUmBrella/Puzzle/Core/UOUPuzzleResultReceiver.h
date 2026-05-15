// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UOUPuzzleResultReceiver.generated.h"

// 조건 그룹이 결과 액터에게 전달할 공통 액션 종류입니다.
// 조건 만족과 해제에 따라 어떤 반응을 할지 이 값으로 구분합니다.
UENUM(BlueprintType)
enum class EOUUPuzzleResultAction : uint8
{
	None,
	Activate,
	Deactivate,
	Pause,
	Resume,
	Toggle
};

// 이 타입을 인터페이스로 인식하게 하기 위한 리플렉션용 껍데기 클래스입니다.
// 엔진과 블루프린트가 인터페이스 타입을 식별할 때 사용합니다.
UINTERFACE(BlueprintType)
class UUOUPuzzleResultReceiver : public UInterface
{
	GENERATED_BODY()
};

// 실제 결과 수신 함수를 선언하는 C++ 인터페이스 본체입니다.
// 결과 액터는 이 함수를 재정의해서 자기 방식대로 액션을 처리합니다.
class IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	// 조건 그룹이 전달한 결과 액션을 받아 처리하는 공통 진입점입니다.
	// 외부에서는 이 함수 하나만 호출하고 내부 구현은 각 액터가 맡습니다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Puzzle|Result")
	void ApplyPuzzleResult(EOUUPuzzleResultAction Action);
};
