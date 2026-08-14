// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugProvider.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUPuzzleMoverActor.generated.h"

class USceneComponent;

// 조건 그룹에서 전달한 결과 액션을 받아 지정된 위치 사이를 이동하는 퍼즐 결과 액터입니다.
// 문이나 플랫폼처럼 상태에 따라 위치가 바뀌는 기믹의 공통 베이스로 사용합니다.
UCLASS(meta=(DisplayName="UOU Puzzle Mover Actor"))
class AUOUPuzzleMoverActor
	: public AActor
	, public IUOUPuzzleResultReceiver
	, public IUOUPuzzleDebugInfoProvider
	, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	AUOUPuzzleMoverActor();

	// 현재 활성 상태와 일시정지 상태를 보고 목표 위치로 이동을 갱신합니다.
	virtual void Tick(float DeltaSeconds) override;

	// 결과 액터 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 실제로 이동할 대상 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> MovingTarget = nullptr;

	// 비활성 상태일 때 도달해야 하는 기준 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> InactivePoint = nullptr;

	// 활성 상태일 때 도달해야 하는 기준 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> ActivePoint = nullptr;

	// 목표 위치로 이동할 때 사용하는 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Movement")
	float MoveSpeed = 200.0f;

	// 켜져 있으면 MovingTarget이 ActivePoint, InactivePoint의 스케일도 따라갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Movement")
	bool bApplyPointScale = true;

	// MovingTarget이 목표 포인트 스케일로 변하는 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Movement", meta = (ClampMin = "0.0"))
	float ScaleSpeed = 4.0f;

	// 켜져 있으면 MovingTarget이 ActivePoint, InactivePoint의 회전도 따라갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Movement")
	bool bApplyPointRotation = true;

	// MovingTarget이 목표 포인트 회전으로 변하는 속도입니다. 초당 회전 각도로 처리됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Movement", meta = (ClampMin = "0.0"))
	float RotationSpeed = 180.0f;

	// 현재 활성 상태인지 저장하는 런타임 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bActivated = false;

	// 현재 이동이 일시정지 중인지 저장하는 런타임 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bPaused = false;

	// 결과 액터를 활성 상태로 전환합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Activate();

	// 결과 액터를 비활성 상태로 전환합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Deactivate();

	// 현재 이동을 일시정지합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Pause();

	// 일시정지된 이동을 다시 재개합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Resume();

	// 현재 활성 상태를 반전합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Toggle();

	// 활성 상태를 직접 지정하면서 일시정지도 함께 해제합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void SetActivated(bool bNewActivated);

	// 인터페이스로 받은 결과 액션을 내부 이동 상태 함수로 연결합니다.
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;
	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;

protected:
	// 현재 활성 상태에 따라 목표 위치로 부드럽게 이동합니다.
	void MoveTarget(float DeltaSeconds);
};
