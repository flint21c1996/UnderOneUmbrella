// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugProvider.generated.h"

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

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	bool IsDebugProviderEnabled() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	FText GetDebugDisplayName() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	FText GetDebugSummaryText() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	FVector GetDebugWorldLocation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Debug")
	void GetDebugConnections(TArray<FUOUDebugConnection>& OutConnections) const;
};
