// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUFloorPlatformStepComponent.generated.h"

class AUOUFloorPlatformTargetActor;

// 플랫폼의 여러 목표 마커 중 다음 이동 지점을 고르는 순차 이동 상태를 담당합니다.
// 실제 목표 배열은 기존 액터에 남겨 맵에 저장된 배치 데이터를 유지합니다.
UCLASS(ClassGroup=(UOU), meta=(BlueprintSpawnableComponent, DisplayName="UOU Floor Platform Step Component"))
class UUOUFloorPlatformStepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUFloorPlatformStepComponent();

	// 순차 이동 중 기록한 런타임 인덱스와 시작점 복귀 예약을 초기화합니다.
	void ResetRuntimeState();

	// 다음 이동이 시작 위치로 돌아가는 특수 구간이 아니도록 초기화합니다.
	void ClearReturnToStartRequest();

	// 현재 이동 중인 목표 인덱스를 기록합니다.
	void SetActiveTargetIndex(int32 TargetIndex);

	// 순차 목표 마커를 실제 이동 목표로 사용할 수 있는지 확인합니다.
	bool ShouldUseMoveSteps(
		bool bUseMoveSteps,
		const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
		int32 CurrentTargetIndex) const;

	// 순차 목표 배열에서 사용할 수 있는 목표 마커를 찾아 반환합니다.
	AUOUFloorPlatformTargetActor* GetTargetMarkerAt(
		const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
		int32 TargetIndex) const;

	// 현재 순차 목표 인덱스에서 사용할 수 있는 목표 마커를 반환합니다.
	AUOUFloorPlatformTargetActor* GetCurrentTargetMarker(
		bool bUseMoveSteps,
		const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
		int32 CurrentTargetIndex) const;

	// 이동과 스냅이 같은 기준으로 다음 순차 목표를 고를 수 있게 목표 트랜스폼과 인덱스를 계산합니다.
	bool ResolveNextTargetTransform(
		const FTransform& StartTransform,
		bool bUseMoveSteps,
		const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
		int32 CurrentTargetIndex,
		FTransform& OutTargetTransform,
		int32& OutTargetIndex) const;

	// 이동 완료 후 다음 Activate가 사용할 순차 목표 인덱스를 갱신합니다.
	void AdvanceTargetIndex(
		bool bUseMoveSteps,
		const TArray<TObjectPtr<AUOUFloorPlatformTargetActor>>& TargetMarkers,
		bool bLoopMoveSteps,
		bool bLoopThroughStart,
		int32& InOutCurrentTargetIndex);

private:
	// 현재 이동 중인 순차 목표 인덱스입니다.
	UPROPERTY(Transient)
	int32 ActiveTargetIndex = INDEX_NONE;

	// 반복 이동에서 다음 이동이 시작 위치로 돌아가는 구간인지 저장합니다.
	UPROPERTY(Transient)
	bool bNextMoveReturnsToStart = false;
};
