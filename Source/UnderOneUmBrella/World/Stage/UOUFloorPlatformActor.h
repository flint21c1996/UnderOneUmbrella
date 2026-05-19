// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUFloorPlatformActor.generated.h"

class UCurveFloat;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class AUOUFloorPlatformActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUFloorPlatformMoveFinishedSignature, AUOUFloorPlatformActor*, Platform);

// 플랫폼에 임시로 붙인 물리 컴포넌트의 원래 물리 상태를 되돌리기 위한 기록입니다.
struct FUOUFloorPlatformCarriedPhysicsState
{
	TWeakObjectPtr<UPrimitiveComponent> Component;
	bool bWasSimulatingPhysics = false;
};

// 층 기반 스테이지에서 한 층의 플랫폼 이동을 담당하는 액터입니다.
// 퍼즐 결과를 받으면 시작 위치와 목표 위치 사이를 이동하고 완료 상태를 외부에 알립니다.
UCLASS(meta=(DisplayName="UOU Floor Platform Actor"))
class AUOUFloorPlatformActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUFloorPlatformActor();

	// 이동 중일 때 플랫폼 위치를 목표 위치까지 보간합니다.
	virtual void Tick(float DeltaSeconds) override;

	// 플랫폼 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 블루프린트에서 메쉬를 바로 지정해 테스트할 수 있는 기본 시각 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform")
	TObjectPtr<UStaticMeshComponent> PlatformMesh = nullptr;

	// 플랫폼 이동 시 같이 데려갈 액터를 찾는 감지 박스입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TObjectPtr<UBoxComponent> CarryDetectionBox = nullptr;

	// 이 플랫폼이 어떤 층에 속하는지 구분하기 위한 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform")
	int32 FloorIndex = 4;

	// 액터 기준으로 플랫폼이 이동할 목표 로컬 오프셋입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement")
	FVector TargetLocalOffset = FVector(0.0f, 1200.0f, 0.0f);

	// 시작 위치에서 목표 위치까지 이동하는 데 걸리는 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement", meta = (ClampMin = "0.0"))
	float MoveDuration = 2.0f;

	// 이동 알파 값을 보정할 때 사용하는 선택 곡선입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement")
	TObjectPtr<UCurveFloat> MoveCurve = nullptr;

	// 목표 위치에 도착한 뒤 플레이어와 다른 오브젝트가 이 플랫폼과 충돌하지 않게 할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Rules")
	bool bDisableCollisionAtTarget = false;

	// 게임 시작 시 이미 목표 위치에 있어야 하는 플랫폼인지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Rules")
	bool bStartAtTarget = false;

	// 플랫폼 이동 중 감지 박스 안의 액터를 함께 이동시킬지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	bool bCarryActorsOnMove = true;

	// 캐릭터 계열은 CharacterMovement가 움직이는 바닥을 따라가므로 기본 운반 대상에서 제외합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	bool bCarryPlayerCharacters = false;

	// 물리 시뮬레이션 중인 액터도 운반 대상에 포함할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	bool bCarryPhysicsSimulatingActors = true;

	// 물리 액터를 운반하는 동안 물리 시뮬레이션을 잠시 끄고 완료 후 원래대로 복구할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	bool bPauseCarriedPhysicsDuringMove = true;

	// 비어 있으면 클래스 필터를 쓰지 않고, 값이 있으면 이 클래스 계열 액터만 운반 후보로 봅니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TArray<TSubclassOf<AActor>> CarryActorClasses;

	// 비어 있으면 태그 필터를 쓰지 않고, 값이 있으면 이 태그를 가진 액터만 운반 후보로 봅니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TArray<FName> CarryActorTags;

	// 감지 박스 안에 있어도 운반하지 않을 액터 목록입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TArray<TObjectPtr<AActor>> IgnoredCarryActors;

	// 에디터에서 이동 기준점을 현재 액터 위치로 다시 잡을 때 사용합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Editor")
	void CaptureCurrentAsStart();

	// 플랫폼을 목표 위치로 이동시키기 시작합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Actions")
	void MoveToTarget();

	// 플랫폼을 시작 위치로 되돌립니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Actions")
	void ResetPlatform();

	// 플랫폼을 이동 없이 바로 목표 위치로 보냅니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Actions")
	void SnapToTarget();

	// 현재 플랫폼이 이동 중인지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Floor Platform|Runtime")
	bool IsMoving() const;

	// 현재 플랫폼이 목표 위치에 도착한 상태인지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Floor Platform|Runtime")
	bool IsAtTarget() const;

	// 플랫폼 목표 이동이 끝났을 때 호출되는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Floor Platform|Events")
	FUOUFloorPlatformMoveFinishedSignature OnMoveFinished;

	// 퍼즐 조건 그룹에서 받은 결과 액션을 플랫폼 이동 명령으로 변환합니다.
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	// CallInEditor로 이동을 시작했을 때 에디터 뷰포트에서도 이동이 갱신되게 합니다.
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

	// 시작 위치와 목표 위치를 현재 설정값 기준으로 계산합니다.
	void RefreshTargetTransforms();

	// 이동 완료 상태를 확정하고 이벤트를 전달합니다.
	void FinishMoveToTarget();

	// 목표 도착 상태에 따라 액터 전체 충돌을 켜거나 끕니다.
	void ApplyTargetCollisionState();

	// 감지 박스 안에서 플랫폼과 함께 이동할 액터를 찾아 임시로 붙입니다.
	void AttachCarriedActors();

	// 이전 이동에서 같이 움직였던 액터를 다시 운반 대상으로 붙입니다.
	void AttachLastMovedActors();

	// 오버랩 이벤트 상태와 상관없이 감지 박스 범위 안의 운반 후보 액터를 직접 찾습니다.
	void CollectCarryCandidateActors(TArray<AActor*>& OutCandidateActors) const;

	// 플랫폼 이동이 끝나거나 취소될 때 임시로 붙인 액터들을 월드 위치 유지 상태로 해제합니다.
	void DetachCarriedActors();

	// 감지된 액터가 이 플랫폼과 함께 이동할 수 있는 대상인지 검사합니다.
	bool CanCarryActor(AActor* CandidateActor) const;

	// 물리 액터가 부모 이동을 따라오도록 필요한 물리 상태를 잠시 멈춥니다.
	void PrepareCarriedActorForAttach(AActor* CandidateActor);

	// 플랫폼 이동 중 잠시 멈춘 물리 상태를 원래대로 되돌립니다.
	void RestoreCarriedPhysicsStates();

	// 액터가 물리 시뮬레이션 중인 컴포넌트를 가지고 있는지 확인합니다.
	bool HasSimulatingPhysicsComponent(AActor* CandidateActor) const;

	// 클래스나 태그 필터를 통과하는 운반 대상인지 확인합니다.
	bool MatchesCarryFilters(AActor* CandidateActor) const;

	// 이동 완료 후 다음 반대 이동에서도 같은 액터를 찾을 수 있도록 기록합니다.
	void CacheLastMovedActors();

	// 이동 곡선이 있으면 곡선 값을 사용하고 없으면 기본 알파를 그대로 사용합니다.
	float ResolveMoveAlpha(float RawAlpha) const;

private:
	// 이동을 시작한 순간부터 누적된 시간입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Runtime", meta = (AllowPrivateAccess = "true"))
	float MoveElapsedTime = 0.0f;

	// 현재 플랫폼이 이동 중인지 저장합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Runtime", meta = (AllowPrivateAccess = "true"))
	bool bIsMoving = false;

	// 현재 플랫폼이 목표 위치에 도착한 상태인지 저장합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Runtime", meta = (AllowPrivateAccess = "true"))
	bool bIsAtTarget = false;

	// 플랫폼이 이동을 시작하는 기준 트랜스폼입니다.
	UPROPERTY(Transient)
	FTransform StartTransform = FTransform::Identity;

	// 플랫폼이 목표 상태에서 도착해야 하는 트랜스폼입니다.
	UPROPERTY(Transient)
	FTransform TargetTransform = FTransform::Identity;

	// 시작 위치가 한 번이라도 기록되었는지 확인하는 값입니다.
	UPROPERTY(Transient)
	bool bHasCapturedStartTransform = false;

	// 이동 중 플랫폼에 임시로 붙여둔 액터 목록입니다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> CarriedActors;

	// 직전 이동에서 함께 움직인 액터 목록입니다.
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> LastMovedActors;

	// 운반 중 잠시 물리를 꺼둔 컴포넌트들의 원래 상태입니다.
	TArray<FUOUFloorPlatformCarriedPhysicsState> CarriedPhysicsStates;
};
