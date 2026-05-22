// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UOUEnvironmentVisualComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

// RainArea처럼 주요 액터가 가진 영역과 상태를 Niagara 이펙트에 전달하는 시각 전용 컴포넌트입니다.
// 컴포넌트 자신의 Transform을 기준으로 Primary/Secondary Niagara 위치와 User Parameter를 갱신합니다.
UCLASS(ClassGroup=(Environment), meta=(BlueprintSpawnableComponent, DisplayName="UOU Environment Visual Component"))
class UUOUEnvironmentVisualComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUOUEnvironmentVisualComponent();

	// 이 컴포넌트가 제어할 Niagara 컴포넌트들을 지정합니다. 보통 소유 Actor 생성자에서 자식 컴포넌트를 연결합니다.
	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetEffectComponents(UNiagaraComponent* NewPrimaryEffect, UNiagaraComponent* NewSecondaryEffect);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetEffectSystems(UNiagaraSystem* NewPrimarySystem, UNiagaraSystem* NewSecondarySystem);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual|Rain")
	void ConfigureRainVisual(
		const FVector& RainLocalPosition,
		const FVector& GroundSplashLocalPosition,
		const FRotator& EffectLocalRotation,
		const FVector2D& AreaSize,
		const FVector& RainKillVolumeLocalCenter,
		const FVector& RainKillVolumeSize);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual|Rain")
	void SetRainBlockerData(
		bool bIsBlocking,
		const FVector& BlockerLocalPosition,
		const FVector& BlockerHalfExtent,
		float BlockerIntensity);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetVisualIntensities(float PrimaryIntensity, float SecondaryIntensity);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual|Rain")
	void SetRainSpawnRate(float NewRainSpawnRate);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual|Rain")
	void SetRainFallSpeed(float NewRainFallSpeed);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetVisualsEnabled(bool bNewEnabled);

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> PrimaryEffect = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SecondaryEffect = nullptr;

	UPROPERTY()
	bool bEnableVisuals = true;

	UPROPERTY()
	bool bEnableEditorPreview = true;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> PrimarySystem = nullptr;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> SecondarySystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> LastAppliedPrimarySystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> LastAppliedSecondarySystem = nullptr;

	UPROPERTY(Transient)
	FVector CachedPrimaryLocalPosition = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector CachedSecondaryLocalPosition = FVector::ZeroVector;

	UPROPERTY(Transient)
	FRotator CachedEffectLocalRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	FVector2D CachedAreaSize = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FVector CachedRainKillVolumeLocalCenter = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector CachedRainKillVolumeSize = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bCachedRainBlockerActive = false;

	UPROPERTY(Transient)
	FVector CachedRainBlockerLocalPosition = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector CachedRainBlockerHalfExtent = FVector::ZeroVector;

	UPROPERTY(Transient)
	float CachedRainBlockerIntensity = 0.0f;

	UPROPERTY(Transient)
	float CachedPrimaryIntensity = 1.0f;

	UPROPERTY(Transient)
	float CachedSecondaryIntensity = 1.0f;

	UPROPERTY(Transient)
	float CachedRainSpawnRate = 2400.0f;

	UPROPERTY(Transient)
	float CachedRainFallSpeed = -900.0f;

	UPROPERTY()
	FName PrimaryAreaSizeParameterName = TEXT("RainAreaSize");

	UPROPERTY()
	FName RainKillVolumeCenterParameterName = TEXT("RainKillVolumeCenter");

	UPROPERTY()
	FName RainKillVolumeSizeParameterName = TEXT("RainKillVolumeSize");

	UPROPERTY()
	FName SecondaryAreaSizeParameterName = TEXT("GroundSplashAreaSize");

	UPROPERTY()
	FName PrimaryIntensityParameterName = TEXT("RainIntensity");

	UPROPERTY()
	FName SecondaryIntensityParameterName = TEXT("GroundSplashIntensity");

	UPROPERTY()
	FName RainSpawnRateParameterName = TEXT("RainSpawnRate");

	UPROPERTY()
	FName RainFallSpeedParameterName = TEXT("RainFallSpeed");

	UPROPERTY()
	FName RainBlockerActiveParameterName = TEXT("RainBlockerActive");

	UPROPERTY()
	FName RainBlockerLocalPositionParameterName = TEXT("RainBlockerLocalPosition");

	UPROPERTY()
	FName RainBlockerHalfExtentParameterName = TEXT("RainBlockerHalfExtent");

	UPROPERTY()
	FName RainBlockerIntensityParameterName = TEXT("RainBlockerIntensity");

	UPROPERTY()
	bool bDrawRainBlockerNiagaraDebug = true;

	UPROPERTY()
	float RainBlockerNiagaraDebugThickness = 2.0f;

	void ApplyVisualEffectSettings(bool bForcePrimarySystem = false, bool bForceSecondarySystem = false);
	void ApplyVisualEffectTransforms();
	void ApplyNiagaraParameters();
	void RefreshNiagaraActivation();
	void DrawRainBlockerNiagaraDebug(const UNiagaraComponent* Effect, const FVector& BlockerWorldCenter, const FVector& BlockerHalfExtent) const;
	UNiagaraComponent* GetPrimaryEffectComponent() const;
	UNiagaraComponent* GetSecondaryEffectComponent() const;
};
