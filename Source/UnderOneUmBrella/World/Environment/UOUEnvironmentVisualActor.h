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

	UFUNCTION(BlueprintCallable, Category = "Environment Visual|Rain")
	void SetRainBlockerData(
		bool bIsBlocking,
		const FVector& BlockerLocalPosition,
		float BlockerRadius,
		float BlockerIntensity);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetVisualIntensities(float PrimaryIntensity, float SecondaryIntensity);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetVisualsEnabled(bool bNewEnabled);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

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

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> LastAppliedPrimarySystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> LastAppliedSecondarySystem = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector CachedPrimaryLocalPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector CachedSecondaryLocalPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FRotator CachedEffectLocalRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector2D CachedAreaSize = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	bool bCachedRainBlockerActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector CachedRainBlockerLocalPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedRainBlockerRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedRainBlockerIntensity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedPrimaryIntensity = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedSecondaryIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName PrimaryAreaSizeParameterName = TEXT("RainAreaSize");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName SecondaryAreaSizeParameterName = TEXT("GroundSplashAreaSize");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName PrimaryIntensityParameterName = TEXT("RainIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName SecondaryIntensityParameterName = TEXT("GroundSplashIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산이 현재 비를 막고 있는지 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerActiveParameterName = TEXT("RainBlockerActive");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산이 비를 막는 중심 위치를 Niagara에 전달할 User Parameter 이름입니다. Visual Actor 기준 로컬 좌표입니다."))
	FName RainBlockerLocalPositionParameterName = TEXT("RainBlockerLocalPosition");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산이 비를 막는 반지름을 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerRadiusParameterName = TEXT("RainBlockerRadius");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산이 비를 막는 표현 강도를 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerIntensityParameterName = TEXT("RainBlockerIntensity");

	void ApplyVisualEffectSettings(bool bForcePrimarySystem = false, bool bForceSecondarySystem = false);
	void ApplyVisualEffectTransforms();
	void ApplyNiagaraParameters();
	void RefreshNiagaraActivation();
};
