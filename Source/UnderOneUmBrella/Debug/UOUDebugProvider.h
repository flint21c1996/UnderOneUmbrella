// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugProvider.generated.h"

// 액터나 컴포넌트가 통합 디버그 시스템에 자신이 제공할 정보를 알려주는 인터페이스입니다.
// 일반적인 액터 디버그 확장은 UUOUDebugProviderComponent 또는 파생 컴포넌트를 붙이는 방식을 우선합니다.
// 직접 인터페이스 구현은 컴포넌트로 표현하기 어려운 특수한 대상에만 사용합니다.
UINTERFACE(BlueprintType)
class UUOUDebugProvider : public UInterface
{
	GENERATED_BODY()
};

class IUOUDebugProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "디버그")
	EUOUDebugCategory GetDebugCategory() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "디버그")
	bool IsDebugProviderEnabled() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "디버그")
	FText GetDebugDisplayName() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "디버그")
	FText GetDebugSummaryText() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "디버그")
	FVector GetDebugWorldLocation() const;
};

