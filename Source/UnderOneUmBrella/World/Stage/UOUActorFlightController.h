// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUActorFlightController.generated.h"

class AUOUActorFlightController;
class UCurveFloat;
class UPrimitiveComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUActorFlightControllerEvent, AUOUActorFlightController*, Controller);

UENUM(BlueprintType)
enum class EUOUActorFlightOrder : uint8
{
	ArrayOrder UMETA(DisplayName = "배열 순서"),
	NearTargetFirst UMETA(DisplayName = "목표에 가까운 순서"),
	FarTargetFirst UMETA(DisplayName = "목표에서 먼 순서"),
	Random UMETA(DisplayName = "무작위")
};

UENUM(BlueprintType)
enum class EUOUActorFlightEasingMode : uint8
{
	Linear UMETA(DisplayName = "선형"),
	EaseIn UMETA(DisplayName = "가속"),
	EaseOut UMETA(DisplayName = "감속"),
	EaseInOut UMETA(DisplayName = "가속 후 감속")
};

// 여러 Actor를 순차적으로 곡선 이동시키는 시네마틱용 컨트롤러입니다.
// 격자 조각, 편지 조각처럼 레벨에 배치된 Actor 배열을 목표 지점으로 날려 보낼 때 사용합니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Actor Flight Controller"))
class UNDERONEUMBRELLA_API AUOUActorFlightController : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUActorFlightController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actor Flight")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 곡선을 따라 이동시킬 대상 Actor 목록입니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Actor Flight|Targets", meta = (ToolTip = "곡선을 따라 목표 지점으로 날려 보낼 Actor 목록입니다. StaticMeshActor, Blueprint Actor 등 일반 Actor를 넣을 수 있습니다."))
	TArray<TObjectPtr<AActor>> FlightActors;

	// 비워두면 이 컨트롤러 Actor의 위치를 목표 지점으로 사용합니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Actor Flight|Targets", meta = (ToolTip = "비워두면 이 컨트롤러 Actor의 위치를 목표 지점으로 사용합니다. 우체통 구멍에 빈 Target Actor를 두고 지정하면 편합니다."))
	TObjectPtr<AActor> TargetActor = nullptr;

	// TargetActor 또는 컨트롤러 기준 로컬 오프셋입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Targets", meta = (ToolTip = "목표 기준 로컬 오프셋입니다. 구멍 중심에서 살짝 안쪽으로 들어가게 만들 때 사용합니다."))
	FVector TargetLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Timing", meta = (ClampMin = "0.01", ToolTip = "각 Actor가 날아가는 데 걸리는 시간입니다."))
	float FlightDuration = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Timing", meta = (ClampMin = "0.0", ToolTip = "첫 Actor와 마지막 Actor의 출발 시간 차이입니다."))
	float TotalStaggerTime = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Timing", meta = (ClampMin = "0.0", ToolTip = "각 Actor 출발 시점에 추가할 무작위 지연 시간입니다."))
	float RandomStartDelayMax = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Timing")
	EUOUActorFlightOrder FlightOrder = EUOUActorFlightOrder::NearTargetFirst;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Timing", meta = (ToolTip = "지정하면 기본 Ease 설정 대신 이 커브의 0~1 값을 이동 알파로 사용합니다."))
	TObjectPtr<UCurveFloat> FlightCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Timing", meta = (EditCondition = "FlightCurve == nullptr", EditConditionHides))
	EUOUActorFlightEasingMode EasingMode = EUOUActorFlightEasingMode::EaseInOut;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Path", meta = (ClampMin = "0.0", ToolTip = "비행 곡선이 위로 솟는 기본 높이입니다."))
	float ArcHeight = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Path", meta = (ClampMin = "0.0", ToolTip = "Actor마다 ArcHeight에 더하거나 뺄 무작위 범위입니다."))
	float ArcHeightRandomRange = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Path", meta = (ClampMin = "0.0", ToolTip = "Actor마다 좌우로 휘어지는 최대 폭입니다."))
	float SideSpread = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Path", meta = (ClampMin = "0.0", ToolTip = "목표 앞에서 어느 정도 바깥쪽을 지나 목표로 들어갈지 정합니다."))
	float TargetApproachDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Path", meta = (ClampMin = "0.0", ToolTip = "Actor마다 최종 목표 위치에 더할 작은 무작위 반경입니다."))
	float TargetScatterRadius = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Rotation", meta = (ToolTip = "켜면 목표 Actor의 회전을 향해 보간합니다. 꺼두면 시작 회전을 유지한 채 회전 연출만 더합니다."))
	bool bMatchTargetRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Rotation", meta = (ClampMin = "0.0", ToolTip = "Actor마다 추가 회전할 최소 각도입니다."))
	float SpinMinDegrees = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Rotation", meta = (ClampMin = "0.0", ToolTip = "Actor마다 추가 회전할 최대 각도입니다."))
	float SpinMaxDegrees = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Scale", meta = (ClampMin = "0.0", ToolTip = "목표에 도착할 때 시작 스케일에 곱할 값입니다. 1이면 크기를 유지합니다."))
	float EndScaleMultiplier = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Random", meta = (ToolTip = "켜면 같은 Seed에서 매번 같은 곡선과 순서를 만듭니다."))
	bool bUseFixedRandomSeed = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|Random", meta = (EditCondition = "bUseFixedRandomSeed"))
	int32 RandomSeed = 2026;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (ToolTip = "시작할 때 대상 Actor의 SceneComponent를 Movable로 강제 변경합니다."))
	bool bForceMovableBeforeFlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (ToolTip = "시작할 때 물리 시뮬레이션을 끕니다. SetActorTransform과 물리가 충돌하지 않게 하기 위한 옵션입니다."))
	bool bDisablePhysicsDuringFlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (EditCondition = "bDisablePhysicsDuringFlight", ToolTip = "물리를 끄기 전에 선속도와 각속도를 0으로 만듭니다."))
	bool bClearPhysicsVelocityBeforeFlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (ToolTip = "시작할 때 중력을 끕니다."))
	bool bDisableGravityDuringFlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (ToolTip = "시작할 때 충돌 상태를 아래 값으로 덮어씁니다."))
	bool bOverrideCollisionDuringFlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (EditCondition = "bOverrideCollisionDuringFlight"))
	TEnumAsByte<ECollisionEnabled::Type> CollisionDuringFlight = ECollisionEnabled::NoCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (ToolTip = "시작할 때 숨겨져 있던 Actor도 비행 연출을 위해 보이게 합니다."))
	bool bUnhideActorsOnStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (ToolTip = "도착한 Actor를 숨깁니다. 우체통 안으로 사라지는 조각 연출에 적합합니다."))
	bool bHideActorsWhenFinished = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (ToolTip = "자연스럽게 완료된 뒤 시작 전에 캐싱한 물리, 중력, 충돌, Mobility 상태를 되돌립니다."))
	bool bRestorePreparedStateOnFinish = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Flight|State", meta = (ToolTip = "중간에 정지할 때 시작 전에 캐싱한 물리, 중력, 충돌, Mobility 상태를 되돌립니다."))
	bool bRestorePreparedStateOnCancel = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Actor Flight|State", meta = (ToolTip = "상태 복구 시 캐싱한 물리 속도도 되돌립니다."))
	bool bRestorePhysicsVelocityWhenRestoringState = false;

	UPROPERTY(BlueprintAssignable, Category = "Actor Flight|Events")
	FUOUActorFlightControllerEvent OnFlightStarted;

	UPROPERTY(BlueprintAssignable, Category = "Actor Flight|Events")
	FUOUActorFlightControllerEvent OnFlightFinished;

	UPROPERTY(BlueprintAssignable, Category = "Actor Flight|Events")
	FUOUActorFlightControllerEvent OnFlightStopped;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Actor Flight|Runtime")
	bool bIsFlying = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Actor Flight|Runtime")
	float FlightElapsedTime = 0.0f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Actor Flight|Actions")
	void StartFlight();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Actor Flight|Actions")
	void RestartFlight();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Actor Flight|Actions")
	void StopFlight();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Actor Flight|Actions")
	void ResetFlightActorsToStart();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Actor Flight|Actions")
	void RestorePreparedActorStates();

	UFUNCTION(BlueprintPure, Category = "Actor Flight|Runtime")
	bool IsFlying() const;

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

protected:
#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	struct FUOUActorFlightComponentRuntimeState
	{
		TWeakObjectPtr<USceneComponent> Component;
		EComponentMobility::Type Mobility = EComponentMobility::Static;
		bool bWasPrimitive = false;
		ECollisionEnabled::Type CollisionEnabled = ECollisionEnabled::NoCollision;
		bool bWasSimulatingPhysics = false;
		bool bGravityEnabled = false;
		FVector LinearVelocity = FVector::ZeroVector;
		FVector AngularVelocityInDegrees = FVector::ZeroVector;
	};

	struct FUOUActorFlightRuntimeItem
	{
		TWeakObjectPtr<AActor> Actor;
		FTransform StartTransform = FTransform::Identity;
		FTransform EndTransform = FTransform::Identity;
		FVector ControlPointA = FVector::ZeroVector;
		FVector ControlPointB = FVector::ZeroVector;
		FVector SpinAxis = FVector::UpVector;
		float SpinDegrees = 0.0f;
		float StartDelay = 0.0f;
		bool bWasHidden = false;
		bool bFinished = false;
		TArray<FUOUActorFlightComponentRuntimeState> ComponentStates;
	};

	TArray<FUOUActorFlightRuntimeItem> FlightItems;

	void BuildFlightItems();
	void SortFlightActors(TArray<AActor*>& Actors, const FVector& TargetLocation, FRandomStream& RandomStream) const;
	void PrepareActorForFlight(FUOUActorFlightRuntimeItem& Item);
	void RestoreActorState(FUOUActorFlightRuntimeItem& Item) const;
	void ApplyFlightTransform(FUOUActorFlightRuntimeItem& Item, float RawAlpha) const;
	void MarkActorFinished(FUOUActorFlightRuntimeItem& Item) const;
	void FinishFlight();

	FTransform ResolveTargetTransform() const;
	float ResolveFlightAlpha(float RawAlpha) const;
	static FVector EvaluateCubicBezier(const FVector& Point0, const FVector& Point1, const FVector& Point2, const FVector& Point3, float Alpha);
};
