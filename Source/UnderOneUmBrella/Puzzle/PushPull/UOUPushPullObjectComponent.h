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
	float MaxHorizontalSpeed = 250.0f;

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

	// 제어할 프리미티브를 자동 탐색하거나 직접 참조를 확인합니다.
	void ResolveTargetPrimitive();

	// 원래 물리 잠금 상태를 저장해 두었다가 해제 시 복원합니다.
	void CacheBasePhysicsLocks();

	// 잡힘 상태에 맞춰 수평 이동 가능 여부를 바꿉니다.
	void ApplyGrabStateConstraints(bool bCanMoveHorizontally);

	// 놓을 때 남아 있는 수평 속도를 정리합니다.
	void StopHorizontalMotion();
};
