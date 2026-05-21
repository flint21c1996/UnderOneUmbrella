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
		const FRotator& BlockerLocalRotation,
		const FVector& BlockerHalfExtent,
		float BlockerIntensity);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetVisualIntensities(float PrimaryIntensity, float SecondaryIntensity);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual|Rain")
	void SetRainSpawnRate(float NewRainSpawnRate);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual|Rain")
	void SetRainBlockerKillRadiusPadding(float NewKillRadiusPadding);

	UFUNCTION(BlueprintCallable, Category = "Environment Visual")
	void SetVisualsEnabled(bool bNewEnabled);

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Effects", meta = (ToolTip = "비 내림 표현을 담당하는 Niagara 컴포넌트입니다."))
	TObjectPtr<UNiagaraComponent> PrimaryEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Effects", meta = (ToolTip = "바닥 물 튐처럼 보조 표현을 담당하는 Niagara 컴포넌트입니다."))
	TObjectPtr<UNiagaraComponent> SecondaryEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Effects", meta = (ToolTip = "연결된 NiagaraComponent가 없을 때 내부 재생용 NiagaraComponent를 자동으로 생성할지 정합니다."))
	bool bAutoCreateMissingEffectComponents = true;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> InternalPrimaryEffect = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> InternalSecondaryEffect = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual")
	bool bEnableVisuals = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Editor Preview", meta = (ToolTip = "에디터 배치 상태에서도 Niagara 프리뷰를 재생할지 정합니다. 실제 게임 실행 중에는 bEnableVisuals와 강도 값이 활성 상태를 결정합니다."))
	bool bEnableEditorPreview = true;

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
	FVector CachedRainKillVolumeLocalCenter = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector CachedRainKillVolumeSize = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	bool bCachedRainBlockerActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector CachedRainBlockerLocalPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FRotator CachedRainBlockerLocalRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	FVector CachedRainBlockerHalfExtent = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedRainBlockerIntensity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedPrimaryIntensity = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedSecondaryIntensity = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Runtime")
	float CachedRainSpawnRate = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName PrimaryAreaSizeParameterName = TEXT("RainAreaSize");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "비 파티클을 유지할 RainArea 박스 중심을 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainKillVolumeCenterParameterName = TEXT("RainKillVolumeCenter");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "비 파티클을 유지할 RainArea 박스 크기를 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainKillVolumeSizeParameterName = TEXT("RainKillVolumeSize");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName SecondaryAreaSizeParameterName = TEXT("GroundSplashAreaSize");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName PrimaryIntensityParameterName = TEXT("RainIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters")
	FName SecondaryIntensityParameterName = TEXT("GroundSplashIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "비 Niagara의 Spawn Rate에 전달할 User Parameter 이름입니다."))
	FName RainSpawnRateParameterName = TEXT("RainSpawnRate");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산이 현재 비를 막고 있는지 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerActiveParameterName = TEXT("RainBlockerActive");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산이 비를 막는 중심 위치를 Niagara에 전달할 User Parameter 이름입니다. Visual Component 기준 로컬 좌표입니다."))
	FName RainBlockerLocalPositionParameterName = TEXT("RainBlockerLocalPosition");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산이 비를 막는 반지름을 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerRadiusParameterName = TEXT("RainBlockerRadius");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산 Kill Volume Box의 절반 크기를 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerHalfExtentParameterName = TEXT("RainBlockerHalfExtent");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산 Kill Volume Box의 로컬 Right 축을 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerRightVectorParameterName = TEXT("RainBlockerRightVector");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산 Kill Volume Box의 로컬 Forward 축을 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerForwardVectorParameterName = TEXT("RainBlockerForwardVector");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산 Kill Volume Box의 로컬 Up 축을 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerUpVectorParameterName = TEXT("RainBlockerUpVector");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Parameters", meta = (ToolTip = "우산이 비를 막는 표현 강도를 Niagara에 전달할 User Parameter 이름입니다."))
	FName RainBlockerIntensityParameterName = TEXT("RainBlockerIntensity");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Visual|Rain Block", meta = (ToolTip = "RainArea에서 전달된 Niagara kill 반지름 여유 값입니다. 직접 수정하지 않고 RainArea에서 관리합니다."))
	float RainBlockerKillRadiusPadding = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Debug", meta = (ToolTip = "Niagara에 전달된 우산 비 차단 영역을 월드에 표시합니다."))
	bool bDrawRainBlockerNiagaraDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment Visual|Debug", meta = (ClampMin = "0.0", ToolTip = "Niagara 비 차단 디버그 구체의 선 두께입니다."))
	float RainBlockerNiagaraDebugThickness = 2.0f;

	void ApplyVisualEffectSettings(bool bForcePrimarySystem = false, bool bForceSecondarySystem = false);
	void ApplyVisualEffectTransforms();
	void ApplyNiagaraParameters();
	void RefreshNiagaraActivation();
	void DrawRainBlockerNiagaraDebug(const UNiagaraComponent* Effect, const FVector& BlockerWorldCenter, const FQuat& BlockerWorldRotation, const FVector& BlockerHalfExtent) const;
	void EnsureInternalEffectComponents();
	UNiagaraComponent* GetPrimaryEffectComponent() const;
	UNiagaraComponent* GetSecondaryEffectComponent() const;
};
