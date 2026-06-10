// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "UOUFloorPlatformCarryComponent.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;

// 플랫폼에 임시로 붙인 물리 컴포넌트의 원래 물리 상태를 되돌리기 위한 기록입니다.
struct FUOUFloorPlatformCarriedPhysicsState
{
	TWeakObjectPtr<UPrimitiveComponent> Component;
	bool bWasSimulatingPhysics = false;
	bool bLockXTranslation = false;
	bool bLockYTranslation = false;
	bool bLockZTranslation = false;
	bool bLockXRotation = false;
	bool bLockYRotation = false;
	bool bLockZRotation = false;
};

struct FUOUFloorPlatformCarriedMobilityState
{
	TWeakObjectPtr<USceneComponent> Component;
	EComponentMobility::Type Mobility = EComponentMobility::Static;
};

// 이동 플랫폼 위에 놓인 액터를 함께 이동시키는 운반 처리를 담당하는 컴포넌트입니다.
// 감지 박스 안의 후보를 찾고, 이동 중 Attach한 뒤 완료 시 물리 상태를 복구합니다.
UCLASS(ClassGroup=(UOU), meta=(BlueprintSpawnableComponent, DisplayName="UOU Floor Platform Carry Component"))
class UUOUFloorPlatformCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUFloorPlatformCarryComponent();

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry", meta = (ToolTip = "켜져 있으면 Static Mobility 컴포넌트를 가진 액터는 이동 발판 운반 대상에서 제외합니다. Movable 부모에 Static 자식을 붙일 수 없어서 기본값은 켜져 있습니다."))
	bool bIgnoreStaticMobilityActors = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry", meta = (ToolTip = "켜져 있으면 Static Mobility 액터를 발판에 붙이기 전에 Movable로 임시 전환하고, 분리할 때 원래 Mobility로 복구합니다. 의도적으로 발판과 함께 움직일 정적 프롭에만 사용하세요."))
	bool bForceMovableBeforeAttach = false;

	// 비어 있으면 클래스 필터를 쓰지 않고, 값이 있으면 이 클래스 계열 액터만 운반 후보로 봅니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TArray<TSubclassOf<AActor>> CarryActorClasses;

	// 비어 있으면 태그 필터를 쓰지 않고, 값이 있으면 이 태그를 가진 액터만 운반 후보로 봅니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TArray<FName> CarryActorTags;

	// 기본 물리 채널 외에 운반 감지에 포함할 오브젝트 채널입니다.
	// PuzzleWeight처럼 커스텀 채널을 쓰는 상자도 플랫폼과 함께 움직이게 할 때 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TArray<TEnumAsByte<ECollisionChannel>> AdditionalCarryObjectChannels = { ECC_GameTraceChannel1 };

	// 감지 박스 안에 있어도 운반하지 않을 액터 목록입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TArray<TObjectPtr<AActor>> IgnoredCarryActors;

	// 여기에 넣은 BP 클래스와 그 자식 클래스는 감지 박스 안에 있어도 같이 이동하지 않습니다.
	// 특정 장치나 투명벽처럼 플랫폼 이동에 딸려오면 안 되는 액터 타입을 제외할 때 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TArray<TSubclassOf<AActor>> IgnoredCarryActorClasses;

	// 운반 후보를 찾는 박스 컴포넌트를 연결합니다.
	void SetDetectionBox(UBoxComponent* InDetectionBox);

	// 감지 박스 안에서 플랫폼과 함께 이동할 액터를 찾아 임시로 붙입니다.
	void AttachCarriedActors();

	// 이전 이동에서 같이 움직였던 액터를 다시 운반 대상으로 붙입니다.
	void AttachLastMovedActors();

	// 플랫폼 이동이 끝나거나 취소될 때 임시로 붙인 액터들을 월드 위치 유지 상태로 해제합니다.
	void DetachCarriedActors();

	// 이동 완료 후 다음 반대 이동에서도 같은 액터를 찾을 수 있도록 기록합니다.
	void CacheLastMovedActors();

	// 현재 플랫폼에 임시로 붙어 있는 운반 대상이 있는지 반환합니다.
	bool HasCarriedActors() const;

private:
	// 오버랩 이벤트 상태와 상관없이 감지 박스 범위 안의 운반 후보 액터를 직접 찾습니다.
	void CollectCarryCandidateActors(TArray<AActor*>& OutCandidateActors) const;

	// 감지된 액터가 이 플랫폼과 함께 이동할 수 있는 대상인지 검사합니다.
	bool CanCarryActor(AActor* CandidateActor) const;

	// 물리 액터가 부모 이동을 따라오도록 필요한 물리 상태를 잠시 멈춥니다.
	void PrepareCarriedActorForAttach(AActor* CandidateActor);
	void PrepareCarriedActorMobilityForAttach(AActor* CandidateActor);

	// 플랫폼 이동 중 잠시 멈춘 물리 상태를 원래대로 되돌립니다.
	void RestoreCarriedPhysicsStates();
	void RestoreCarriedMobilityStates();

	// 액터가 물리 시뮬레이션 중인 컴포넌트를 가지고 있는지 확인합니다.
	bool HasSimulatingPhysicsComponent(AActor* CandidateActor) const;
	bool HasStaticMobilityComponent(AActor* CandidateActor) const;

	// 클래스나 태그 필터를 통과하는 운반 대상인지 확인합니다.
	bool MatchesCarryFilters(AActor* CandidateActor) const;

	// 운반 후보를 검사할 감지 박스입니다.
	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> DetectionBox = nullptr;

	// 이동 중 플랫폼에 임시로 붙여둔 액터 목록입니다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> CarriedActors;

	// 직전 이동에서 함께 움직인 액터 목록입니다.
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> LastMovedActors;

	// 운반 중 잠시 물리를 꺼둔 컴포넌트들의 원래 상태입니다.
	TArray<FUOUFloorPlatformCarriedPhysicsState> CarriedPhysicsStates;
	TArray<FUOUFloorPlatformCarriedMobilityState> CarriedMobilityStates;
};
