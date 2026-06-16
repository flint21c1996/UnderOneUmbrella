// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUCrankActor.generated.h"

class UStaticMeshComponent;
class UUOUCrankComponent;

UCLASS()
class UNDERONEUMBRELLA_API AUOUCrankActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUCrankActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank")
	TObjectPtr<UStaticMeshComponent> CrankVisual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank")
	TObjectPtr<UUOUCrankComponent> CrankComponent = nullptr;
};
