// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUPushPullObjectComponent.generated.h"

class AActor;
class UPrimitiveComponent;

// 밀고 당기기 대상 오브젝트의 물리 제약과 잡힘 상태를 관리하는 컴포넌트입니다.
// 플레이어가 블럭을 잡았을 때만 수평 이동을 허용하고 놓으면 다시 고정합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUPushPullObjectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPushPullObjectComponent();

	// 게임 시작 시 실제로 제어할 물리 프리미티브를 찾고 기본 잠금 상태를 저장합니다.
	virtual void BeginPlay() override;

	// 대상 프리미티브를 자동으로 찾을지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull")
	bool bAutoFindTargetPrimitive = true;

	// 실제 물리 이동과 제약을 적용할 프리미티브 컴포넌트입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull")
	TObjectPtr<UPrimitiveComponent> TargetPrimitive = nullptr;

	// 수평 방향 최대 이동 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull", meta = (ClampMin = "0.0"))
	float MaxHorizontalSpeed = 300.0f;

	// 낮은 단차에 닿았을 때 수직 속도를 보조해 물리 상자가 턱을 넘을 수 있게 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull|Step", meta = (ToolTip = "켜져 있으면 Push/Pull 이동 중 낮은 단차를 감지해 상자가 살짝 올라가도록 수직 속도를 보조합니다."))
	bool bEnableLowStepAssist = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull|Step", meta = (ClampMin = "0.0", ToolTip = "이 높이 이하의 단차만 이동 가능한 낮은 단차로 봅니다."))
	float MaxStepAssistHeight = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull|Step", meta = (ClampMin = "0.0", ToolTip = "상자 전방에서 단차를 찾기 위해 추가로 확인하는 거리입니다."))
	float StepAssistProbeDistance = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull|Step", meta = (ClampMin = "0.0", ToolTip = "낮은 단차를 넘을 때 적용할 최대 수직 보조 속도입니다."))
	float StepAssistLiftSpeed = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull|Step", meta = (ClampMin = "0.0", ToolTip = "이 수평 속도보다 느리면 단차 보조를 적용하지 않습니다."))
	float StepAssistMinHorizontalSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull|Rotation", meta = (ToolTip = "켜져 있으면 Push/Pull 중 박스의 Pitch/Roll/Yaw 회전을 잠그고 Pitch/Roll을 0으로 안정화합니다. 낮은 단차는 Step Assist로 넘습니다."))
	bool bLockRotationWhileGrabbed = true;

	// 잡고 이동하는 동안 회전이 과하게 튀지 않도록 각속도를 제한합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull|Rotation", meta = (ToolTip = "켜져 있으면 Push/Pull 중 X/Y 회전 각속도를 제한하고 Z 회전 각속도는 제거합니다."))
	bool bClampGrabbedAngularVelocity = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull|Rotation", meta = (ClampMin = "0.0", ToolTip = "Push/Pull 중 적용할 각감쇠입니다. 값이 클수록 회전이 빠르게 안정됩니다."))
	float GrabbedAngularDamping = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull|Rotation", meta = (ClampMin = "0.0", ToolTip = "Push/Pull 중 허용할 최대 X/Y 각속도입니다. 0이면 각속도 제한을 끕니다."))
	float MaxGrabbedAngularSpeedDegrees = 90.0f;

	// 끄면 물담기/무게 기능은 유지하되 Push/Pull 잡기와 이동만 비활성화합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull")
	bool bEnablePushPullMovement = true;

	// 현재 이 오브젝트가 잡혀 있는지 저장합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bIsGrabbed = false;

	// 현재 상호작용자가 이 오브젝트를 잡을 수 있는 상태인지 검사합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|PushPull")
	bool CanGrab(AActor* Interactor) const;

	// 잡기 시도를 수행하고 성공하면 잡힘 상태를 시작합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|PushPull")
	bool TryBeginGrab(AActor* Interactor);

	// 현재 잡고 있는 상태를 종료하고 제약을 원래대로 돌립니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|PushPull")
	void EndGrab(AActor* Interactor);

	// 런타임에서 Push/Pull 이동 허용 여부를 바꿉니다. 꺼지는 순간 잡기 상태도 정리합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|PushPull")
	void SetPushPullMovementEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Puzzle|PushPull")
	bool IsPushPullMovementEnabled() const { return bEnablePushPullMovement; }

	// 수평 속도를 적용하면서 최대 속도를 넘지 않도록 정리합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|PushPull")
	FVector SetHorizontalVelocity(FVector HorizontalVelocity);

	// 현재 제어 중인 물리 프리미티브를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle|PushPull")
	UPrimitiveComponent* GetTargetPrimitive() const { return TargetPrimitive; }

	// 잡기 기준 위치 계산에 사용할 대표 위치를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle|PushPull")
	FVector GetGrabReferenceLocation() const;

protected:
	// 현재 이 오브젝트를 잡고 있는 액터입니다.
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentGrabber = nullptr;

	// 원래 물리 설정에서 X축 이동 잠금 여부를 저장합니다.
	UPROPERTY(Transient)
	bool bBaseLockXTranslation = false;

	// 원래 물리 설정에서 Y축 이동 잠금 여부를 저장합니다.
	UPROPERTY(Transient)
	bool bBaseLockYTranslation = false;

	UPROPERTY(Transient)
	bool bBaseLockXRotation = false;

	UPROPERTY(Transient)
	bool bBaseLockYRotation = false;

	UPROPERTY(Transient)
	bool bBaseLockZRotation = false;

	UPROPERTY(Transient)
	float BaseLinearDamping = 0.0f;

	UPROPERTY(Transient)
	float BaseAngularDamping = 0.0f;

	// 제어할 프리미티브를 자동 탐색하거나 직접 참조를 확인합니다.
	void ResolveTargetPrimitive();

	// 원래 물리 잠금 상태를 저장해 두었다가 해제 시 복원합니다.
	void CacheBasePhysicsLocks();

	// 잡힘 상태에 맞춰 수평 이동 가능 여부를 바꿉니다.
	void ApplyGrabStateConstraints(bool bCanMoveHorizontally);

	bool ApplyLowStepAssist(const FVector& HorizontalVelocity) const;
	bool TryFindLowStepAhead(const FVector& MoveDirection, float& OutStepHeight) const;
	bool TraceLowStepAtProbeLocation(const FVector& ProbeLocation, float CurrentBottomZ, float& OutStepHeight) const;
	void StabilizeGrabbedRotation() const;
	void ClampGrabbedAngularVelocity() const;

	// 놓을 때 남아 있는 수평 속도를 정리합니다.
	void StopHorizontalMotion();
};
