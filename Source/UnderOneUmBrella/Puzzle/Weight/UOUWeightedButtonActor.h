// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUWeightedButtonActor.generated.h"

class AActor;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOUWeightedButtonComponent;
class UUOUWeightSensorComponent;

// 무게 버튼 퍼즐을 바로 월드에 배치할 수 있게 센서와 버튼 비주얼을 묶어 둔 액터입니다.
// 버튼 조건 판정은 제자리에 두고, 결과 연출이나 표면 추적 같은 배치용 동작을 선택적으로 더할 수 있습니다.
UCLASS(meta=(DisplayName="UOU Weighted Button Actor"))
class AUOUWeightedButtonActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUWeightedButtonActor();

	// 퍼즐 조건 그룹에서 전달한 결과 액션을 받아 버튼 비주얼 침하 연출을 켜고 끕니다.
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

	// 에디터에서 현재 버튼 위치를 표면 대상 기준 로컬 오프셋으로 저장합니다.
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Puzzle|Surface Follow")
	void CaptureCurrentSurfaceOffset();

	// Result Sink 기능 자체를 켜거나 끕니다. 끄면 진행 중인 침하 상태도 해제합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Result Sink")
	void SetResultSinkEnabled(bool bNewEnabled);

	// Result Sink 침하 상태를 직접 켜거나 끕니다. 기능이 꺼져 있으면 켜지지 않습니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Result Sink")
	void SetResultSinkActive(bool bNewActive);

	// 에디터와 블루프린트에서 Result Sink 침하 상태를 켭니다.
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Puzzle|Result Sink")
	void ActivateResultSink();

	// 에디터와 블루프린트에서 Result Sink 침하 상태를 끕니다.
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Puzzle|Result Sink")
	void DeactivateResultSink();

	// 에디터와 블루프린트에서 Result Sink 침하 상태를 반전합니다.
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Puzzle|Result Sink")
	void ToggleResultSink();

protected:
	// 에디터와 런타임에서 버튼 충돌 배치를 최신 설정으로 맞춥니다.
	virtual void OnConstruction(const FTransform& Transform) override;

	// 버튼 누름 컴포넌트가 실제로 움직일 루트를 참조하도록 연결합니다.
	virtual void PostInitializeComponents() override;

	// 시작 시 표면 추적 기준 위치를 잡고 버튼 이동을 준비합니다.
	virtual void BeginPlay() override;

	// 버튼 비주얼 침하와 표면 추적 이동을 매 프레임 갱신합니다.
	// 버튼 비주얼이 침하 위치로 이동하는 연출을 매 프레임 갱신합니다.
	virtual void Tick(float DeltaSeconds) override;

	// 버튼 액터 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 플레이어나 무게 오브젝트가 올라왔는지 감지하는 판정 볼륨입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UBoxComponent> WeightSensorVolume = nullptr;

	// 실제 화면에 보이는 버튼 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UStaticMeshComponent> ButtonVisual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> ButtonMotionRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UBoxComponent> ButtonSurfaceCollision = nullptr;

	// 버튼이 눌리지 않았을 때 ButtonVisual이 돌아갈 기준 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> ReleasedPoint = nullptr;

	// 버튼이 눌렸을 때 ButtonVisual이 내려갈 기준 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> PressedPoint = nullptr;

	// 퍼즐이 완전히 해결된 뒤 ButtonVisual이 추가로 내려갈 기준 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Result Sink")
	TObjectPtr<USceneComponent> ResultSinkPoint = nullptr;

	// WeightSensorVolume의 겹침 액터를 보고 현재 무게를 계산합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUWeightSensorComponent> WeightSensorComponent = nullptr;

	// WeightSensorComponent의 값을 버튼 눌림 조건으로 바꾸고 ButtonVisual 눌림 이동을 담당합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUWeightedButtonComponent> WeightedButtonComponent = nullptr;

	// 켜져 있으면 Activate 결과를 받았을 때 센서는 그대로 두고 ButtonVisual만 ResultSinkPoint로 내려갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Result Sink")
	bool bEnableResultSinkMotion = false;

	// 침하 연출 중 일반 버튼 눌림 애니메이션이 ButtonVisual을 다시 끌어올리지 못하게 막습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Result Sink")
	bool bPausePressMotionWhileSinking = true;

	// ButtonVisual이 ResultSinkPoint로 이동하는 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Result Sink", meta = (ClampMin = "0.0"))
	float ResultSinkMoveSpeed = 120.0f;

	// 현재 퍼즐 결과 침하 연출이 켜져 있는지 확인하는 런타임 상태입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bResultSinkActive = false;

	// 켜져 있으면 버튼 액터가 지정한 표면 대상 컴포넌트의 로컬 위치를 따라갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Surface Follow")
	bool bFollowSurfaceTarget = false;

	// 버튼이 올라가 있는 움직이는 대상 액터입니다. 보통 BP_UOU_MoverActor를 지정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Surface Follow")
	TObjectPtr<AActor> SurfaceTargetActor = nullptr;

	// 대상 액터 안에서 실제로 움직이는 컴포넌트 이름입니다. UOUPuzzleMoverActor라면 보통 MovingTarget입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Surface Follow")
	FName SurfaceTargetComponentName = TEXT("MovingTarget");

	// 시작할 때 현재 버튼 위치를 표면 대상 기준 로컬 오프셋으로 자동 저장할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Surface Follow")
	bool bCaptureSurfaceOffsetOnBeginPlay = true;

	// SurfaceTargetComponent 기준으로 버튼이 붙어 있을 로컬 위치입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Surface Follow")
	FVector SurfaceLocalOffset = FVector::ZeroVector;

	// 표면을 따라갈 때 이동하는 속도입니다. 0이면 즉시 붙습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Surface Follow", meta = (ClampMin = "0.0"))
	float SurfaceFollowSpeed = 0.0f;

	// 현재 위치가 표면 대상 기준 로컬 위치로 저장되었는지 확인하는 런타임 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bSurfaceFollowOffsetCaptured = false;

private:
	void ConfigureWeightedButtonMotionReferences() const;
	void ConfigureButtonCollisionLayout() const;
	void ConfigureButtonCollision() const;
	void ConfigureBlockingCollision(UBoxComponent* CollisionComponent) const;
	void SyncButtonCollisionToVisual() const;

	// 침하 상태에 따라 ButtonVisual을 ResultSinkPoint 쪽으로 보간 이동합니다.
	void MoveResultSinkVisual(float DeltaSeconds);

	// 침하 상태에 맞춰 기존 버튼 눌림 이동 틱을 켜거나 끕니다.
	void RefreshButtonMotionTickState() const;

	// SurfaceTargetActor에서 실제로 따라갈 씬 컴포넌트를 찾습니다.
	USceneComponent* ResolveSurfaceTargetComponent() const;

	// 현재 버튼 위치를 표면 대상 기준 로컬 오프셋으로 저장합니다.
	void CaptureSurfaceOffsetFromCurrentLocation();

	// 저장된 로컬 오프셋을 월드 위치로 변환해 버튼 액터 위치를 갱신합니다.
	void UpdateSurfaceFollow(float DeltaSeconds);
};
