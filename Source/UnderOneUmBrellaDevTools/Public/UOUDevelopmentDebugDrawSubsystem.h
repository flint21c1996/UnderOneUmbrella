// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UOUDevelopmentDebugDrawSubsystem.generated.h"

class UUOUDevelopmentDebugControlSubsystem;
class AActor;

// HUD 액터 목록에 표시할 액터와 대표 디버그 카테고리입니다.
struct FUOUDevelopmentDebugActorEntry
{
	TWeakObjectPtr<AActor> Actor;
	EUOUDebugCategory Category = EUOUDebugCategory::System;
};

// 개발 도구 설정을 읽고 디버그 정보와 월드 표시를 렌더링할 실행부입니다.
UCLASS()
class UNDERONEUMBRELLADEVTOOLS_API UUOUDevelopmentDebugDrawSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// 현재 월드의 액터와 컴포넌트에서 Puzzle 카테고리 Provider를 다시 수집합니다.
	UFUNCTION(BlueprintCallable, Category = "Development Debug|Puzzle")
	void RefreshPuzzleDebugProviders();

	// 현재 캐시에 남아 있는 유효한 Puzzle Provider 개수를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Development Debug|Puzzle")
	int32 GetPuzzleDebugProviderCount() const;

	// 이 Subsystem의 Tick에서 갱신한 최신 플레이어 정보를 HUD에 제공합니다.
	const FString& GetPlayerDebugText() const { return PlayerDebugText; }

	// 이 Subsystem에서 주기적으로 갱신한 최신 성능 정보를 HUD에 제공합니다.
	const FString& GetPerformanceDebugText() const { return PerformanceDebugText; }

	// 이 Subsystem에서 주기적으로 갱신한 최신 VFX 개수 정보를 HUD에 제공합니다.
	const FString& GetVFXDebugText() const { return VFXDebugText; }

	// 현재 이관된 액터 단위 디버그 표시가 지원하는 월드 액터 목록을 반환합니다.
	void GetSelectableDebugActors(TArray<FUOUDevelopmentDebugActorEntry>& OutActors) const;

private:
	void RefreshPlayerDebugText();
	void DrawPlayerUmbrellaRainBlockerDebug() const;
	void DrawPlayerUmbrellaPourTraceDebug() const;
	void DrawPlayerUmbrellaPourPlacementDebug() const;
	void DrawInteractionDebug() const;
	void DrawNPCDebug() const;
	void RefreshPerformanceDebugText(float DeltaTime);
	void ResetPerformanceDebugState();
	void RefreshVFXDebugData(float DeltaTime);
	void DrawVFXOwnerLabels() const;
	void DrawRainAreaVFXDebug() const;
	void DrawEnvironmentVisualDebug() const;
	void ResetVFXDebugState();
	void TryAddPuzzleDebugProvider(UObject* ProviderObject);
	void DrawWaterBasinDebug() const;
	void DrawRotatableMirrorDebug() const;
	void DrawLightExposureReceiverDebug() const;
	void DrawUmbrellaLightReflectorDebug() const;
	void DrawLightExposureSourceDebug() const;
	void DrawSelectedPuzzleInfo() const;
	void DrawPuzzleProviderCustomDebug() const;
	void DrawPuzzleProviderConnections() const;
	void DrawPuzzleProviderLabels() const;
	bool ShouldDrawActor(const AActor* Actor) const;

	// 같은 월드의 전체 및 카테고리 디버그 설정을 읽기 위한 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentDebugControlSubsystem> DebugControlSubsystem;

	// 현재 월드에서 발견한 Puzzle 카테고리 Provider의 약한 참조 캐시입니다.
	TArray<TWeakObjectPtr<UObject>> PuzzleDebugProviders;

	// 다음 Puzzle Provider 월드 스캔까지 남은 시간입니다.
	float PuzzleProviderRefreshTimeRemaining = 0.0f;

	// 개발 HUD가 읽는 현재 월드 플레이어 정보의 런타임 캐시입니다.
	FString PlayerDebugText;

	// 개발 HUD가 읽는 현재 월드 성능 정보의 런타임 캐시입니다.
	FString PerformanceDebugText;

	// 다음 성능 정보 갱신까지 남은 런타임 시간입니다.
	float PerformanceUpdateTimeRemaining = 0.0f;

	// 성능 정보 갱신 구간에 누적한 프레임 시간입니다.
	float PerformanceAccumulatedDeltaTime = 0.0f;

	// 평균 프레임 시간을 계산하기 위해 누적한 샘플 수입니다.
	int32 PerformanceSampleCount = 0;

	// 개발 HUD가 읽는 현재 월드 VFX 개수 정보의 런타임 캐시입니다.
	FString VFXDebugText;

	// 활성 VFX를 가진 Owner별 월드 라벨 문자열의 런타임 캐시입니다.
	TArray<FString> VFXOwnerLabelTexts;

	// Owner별 월드 라벨을 표시할 위치의 런타임 캐시입니다.
	TArray<FVector> VFXOwnerLabelLocations;

	// 다음 VFX 컴포넌트 스캔까지 남은 런타임 시간입니다.
	float VFXUpdateTimeRemaining = 0.0f;

	// VFX 캐시를 만든 시점의 복수 선택 액터 목록입니다. 선택 변경 시 캐시를 즉시 갱신합니다.
	TArray<TWeakObjectPtr<AActor>> VFXCachedSelectedActors;
};
