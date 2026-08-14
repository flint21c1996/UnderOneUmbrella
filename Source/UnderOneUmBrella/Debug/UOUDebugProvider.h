// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDevelopmentToolsBuild.h"
#include "UObject/Interface.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugProvider.generated.h"

#if UOU_WITH_DEVELOPMENT_TOOLS
class IUOUDevelopmentDebugDrawContext;
#endif

// Interface for actors or components that provide data to the integrated debug system.
UINTERFACE(BlueprintType)
class UNDERONEUMBRELLA_API UUOUDebugProvider : public UInterface
{
	GENERATED_BODY()
};

class UNDERONEUMBRELLA_API IUOUDebugProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	EUOUDebugCategory GetDebugCategory() const;
	virtual EUOUDebugCategory GetDebugCategory_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	bool IsDebugProviderEnabled() const;
	virtual bool IsDebugProviderEnabled_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	FText GetDebugDisplayName() const;
	virtual FText GetDebugDisplayName_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	FText GetDebugSummaryText() const;
	virtual FText GetDebugSummaryText_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	FVector GetDebugWorldLocation() const;
	virtual FVector GetDebugWorldLocation_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	void GetDebugConnections(TArray<FUOUDebugConnection>& OutConnections) const;
	virtual void GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const;

#if UOU_WITH_DEVELOPMENT_TOOLS
	// Provider 공통 월드 라벨을 표시할지 결정합니다. 별도 정보 표시가 있는 Provider는 false를 반환할 수 있습니다.
	virtual bool ShouldDrawDevelopmentDebugLabel() const
	{
		return true;
	}

	// 선택된 네이티브 Provider가 자신에게 필요한 개발용 도형을 공통 Context에 전달합니다.
	virtual void GatherDevelopmentDebugDraw(IUOUDevelopmentDebugDrawContext& Context) const
	{
	}
#endif
};
