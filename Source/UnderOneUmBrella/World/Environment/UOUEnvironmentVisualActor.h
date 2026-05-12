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

	UFUNCTION(BlueprintCallable, Category = "Environment Visual|Rain")
	void ConfigureRainVisual(
		const FVector& RainLocalPosition,
		const FVector& GroundSplashLocalPosition,
		const FRotator& EffectLocalRotation,
		const FVector2D& AreaSize);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetVisualIntensities(float PrimaryIntensity, float SecondaryIntensity);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetVisualsEnabled(bool bNewEnabled);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector CachedPrimaryLocalPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector CachedSecondaryLocalPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FRotator CachedEffectLocalRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector2D CachedAreaSize = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedPrimaryIntensity = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedSecondaryIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName PrimaryAreaSizeParameterName = TEXT("User.RainAreaSize");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName SecondaryAreaSizeParameterName = TEXT("User.GroundSplashAreaSize");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName PrimaryIntensityParameterName = TEXT("User.RainIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName SecondaryIntensityParameterName = TEXT("User.GroundSplashIntensity");

	void ApplyVisualEffectSettings();
	void ApplyVisualEffectTransforms();
	void ApplyNiagaraParameters();
	void RefreshNiagaraActivation();
};
