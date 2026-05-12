// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUEnvironmentVisualActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

// Environment zone actors can delegate their visual-only Niagara work to this child actor.
UCLASS(meta=(DisplayName="UOU Environment Visual"))
class AUOUEnvironmentVisualActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUEnvironmentVisualActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual")
	TObjectPtr<UNiagaraComponent> PrimaryEffect = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual")
	TObjectPtr<UNiagaraComponent> SecondaryEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual")
	bool bEnableVisuals = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual")
	TObjectPtr<UNiagaraSystem> PrimarySystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual")
	TObjectPtr<UNiagaraSystem> SecondarySystem = nullptr;
};
