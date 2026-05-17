// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugProvider.generated.h"

// 액터나 컴포넌트가 통합 디버그 시스템에 자신이 제공할 정보를 알려주는 인터페이스입니다.
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

