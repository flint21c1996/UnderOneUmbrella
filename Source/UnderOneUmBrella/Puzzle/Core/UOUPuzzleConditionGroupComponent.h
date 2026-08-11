// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDevelopmentCheatBuild.h"
#include "Engine/EngineTypes.h"
#include "UOUPuzzleConditionGroupComponent.generated.h"

class UUOUPuzzleConditionSourceComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleGroupSatisfiedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleGroupUnsatisfiedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPuzzleGroupStateChangedSignature, bool, bIsSatisfied);

// 여러 조건 소스를 하나로 묶어서 하나의 만족 상태로 계산하는 그룹 컴포넌트입니다.
// 버튼, 물, 저울 같은 원인들을 조합해서 퍼즐 완료 여부를 판정할 때 사용합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUPuzzleConditionGroupComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPuzzleConditionGroupComponent();

	// 게임 시작 시 조건 구독과 초기 상태 계산을 준비합니다.
	virtual void BeginPlay() override;

	// 종료 시 조건 소스 구독을 정리합니다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 그룹이 만족되었을 때 방송하는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Events")
	FOnPuzzleGroupSatisfiedSignature OnSatisfied;

	// 그룹이 불만족 상태가 되었을 때 방송하는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Events")
	FOnPuzzleGroupUnsatisfiedSignature OnUnsatisfied;

	// 그룹 만족 상태가 바뀔 때마다 방송하는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Events")
	FOnPuzzleGroupStateChangedSignature OnStateChanged;

	// 직접 연결할 조건 소스 컴포넌트 참조 목록입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Conditions")
	TArray<FComponentReference> ConditionSourceReferences;

	// 같은 액터 안의 조건 소스를 자동으로 수집할지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Conditions")
	bool bAutoCollectLocalConditionSources = false;

	// 현재 그룹이 만족 상태인지 저장하는 런타임 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bIsSatisfied = false;

	// 현재 이 컴포넌트가 직접 해석한 조건 소스 목록입니다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUOUPuzzleConditionSourceComponent>> ResolvedConditionSources;

	// 외부 액터나 그룹 액터가 주입한 조건 소스 목록입니다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUOUPuzzleConditionSourceComponent>> ExternalConditionSources;

	// 조건 목록을 다시 읽고 만족 상태를 즉시 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Conditions")
	void RefreshNow();

	// 현재 만족 상태를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle|Conditions")
	bool IsSatisfied() const;

	// 현재 그룹에 연결된 조건 수를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle|Conditions")
	int32 GetConditionCount() const;

	// 현재 만족 중인 조건 수를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle|Conditions")
	int32 GetSatisfiedCount() const;

#if UOU_WITH_PUZZLE_CHEATS
	// 실제 조건 소스를 변경하지 않고 기존 상태 변경 이벤트 경로를 통해 그룹을 만족시킵니다.
	// 개발 도구 모듈에서만 호출하는 일반 C++ 진입점입니다.
	bool ForceSatisfiedForCheat();
#endif

	// 외부에서 수집한 조건 소스 목록을 교체합니다.
	void SetExternalConditionSources(const TArray<UUOUPuzzleConditionSourceComponent*>& NewConditionSources);

	// 외부 조건 소스 목록을 비웁니다.
	void ClearExternalConditionSources();

protected:
#if UOU_WITH_PUZZLE_CHEATS
	// 개발 치트가 실제 조건 계산을 대신해 그룹을 만족 상태로 유지하는 비직렬화 런타임 값입니다.
	bool bHasCheatSatisfiedOverride = false;
#endif

	// 개별 조건 소스 상태 변화에 반응해서 그룹 상태를 다시 계산합니다.
	UFUNCTION()
	void HandleConditionChanged(bool bNewSatisfied);

	// 로컬 참조와 외부 참조를 합쳐 실제 조건 소스 목록을 구성합니다.
	void ResolveConditionSources();

	// 현재 조건 소스 목록에 상태 변화 이벤트를 구독합니다.
	void SubscribeConditions();

	// 현재 조건 소스 목록의 이벤트 구독을 해제합니다.
	void UnsubscribeConditions();

	// 전체 만족 상태를 계산하고 필요하면 이벤트를 방송합니다.
	void RefreshSatisfiedState(bool bBroadcastEvents);

#if UOU_WITH_PUZZLE_CHEATS
	// 치트 오버라이드와 실제 조건 상태를 합쳐 이번 갱신의 최종 만족 상태를 반환합니다.
	bool ResolveSatisfiedState() const;
#endif

	// 모든 조건이 만족되었는지 검사합니다.
	bool AreAllConditionsSatisfied() const;
};
