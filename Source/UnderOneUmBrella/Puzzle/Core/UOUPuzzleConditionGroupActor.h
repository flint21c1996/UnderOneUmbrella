// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleConditionGroupComponent.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUPuzzleConditionGroupActor.generated.h"

class USceneComponent;
class UUOUPuzzleConditionSourceComponent;
class UUOUPuzzleDebugProviderComponent;

// 조건 그룹이 만족되거나 해제될 때 어떤 액터에 어떤 결과를 보낼지 묶어두는 설정입니다.
// 하나의 그룹 액터가 여러 결과 액터를 동시에 제어할 수 있게 도와줍니다.
USTRUCT(BlueprintType)
struct FOUUPuzzleResultBinding
{
	GENERATED_BODY()

	// 결과 액션을 전달할 대상 액터입니다.
	// 문이나 플랫폼 같은 결과 기믹 액터를 여기에 연결합니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 조건이 만족되었을 때 대상 액터에 전달할 액션입니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle")
	EOUUPuzzleResultAction SatisfiedAction = EOUUPuzzleResultAction::Activate;

	// 조건이 해제되었을 때 대상 액터에 전달할 액션입니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle")
	EOUUPuzzleResultAction UnsatisfiedAction = EOUUPuzzleResultAction::Deactivate;
};

// 여러 퍼즐 원인과 결과를 씬에서 직접 이어주는 조건 그룹 액터입니다.
// 내부 조건 그룹 컴포넌트를 감싸고 결과 액터로 액션을 전달하는 허브 역할을 합니다.
UCLASS(meta=(DisplayName="UOU Puzzle Condition Group Actor"))
class AUOUPuzzleConditionGroupActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUPuzzleConditionGroupActor();

	// 에디터에서 값이 바뀔 때 조건과 결과 연결을 다시 정리합니다.
	virtual void OnConstruction(const FTransform& Transform) override;

	// 게임 시작 시 내부 이벤트 구독과 조건 초기 갱신을 마칩니다.
	virtual void BeginPlay() override;

	// 조건 그룹이 만족되었을 때 외부에 알리는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Group Actor|Events")
	FOnPuzzleGroupSatisfiedSignature OnSatisfied;

	// 조건 그룹이 불만족 상태로 돌아왔을 때 외부에 알리는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Group Actor|Events")
	FOnPuzzleGroupUnsatisfiedSignature OnUnsatisfied;

	// 조건 만족 상태가 바뀔 때마다 외부에 알리는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Group Actor|Events")
	FOnPuzzleGroupStateChangedSignature OnStateChanged;

	// 이 그룹이 수집할 조건 액터 목록입니다.
	// 각 액터 안의 조건 소스 컴포넌트를 모아서 내부 그룹 컴포넌트에 전달합니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Group Actor|Conditions")
	TArray<TObjectPtr<AActor>> ConditionActors;

	// 조건 액터들 안의 조건 소스를 자동으로 수집할지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Group Actor|Conditions")
	bool bCollectConditionSourcesFromConditionActors = true;

	// 조건 만족과 해제에 반응할 결과 액터 바인딩 목록입니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Group Actor|Results")
	TArray<FOUUPuzzleResultBinding> ResultBindings;

	// 조건 액터에서 실제로 찾은 조건 소스 목록입니다.
	// 런타임에 내부 그룹 컴포넌트로 전달할 캐시 데이터입니다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUOUPuzzleConditionSourceComponent>> ResolvedConditionSources;

	// 그룹 액터의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Group Actor")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 실제 조건 만족 계산을 담당하는 내부 그룹 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Group Actor")
	TObjectPtr<UUOUPuzzleConditionGroupComponent> PuzzleConditionGroupComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Group Actor|Debug")
	TObjectPtr<UUOUPuzzleDebugProviderComponent> PuzzleDebugProviderComponent = nullptr;

	// 현재 연결된 조건 액터 목록을 다시 해석해서 그룹 상태를 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Group Actor|Conditions")
	void RefreshGroupSetup();

	// 현재 그룹이 만족 상태인지 바로 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Puzzle|Group Actor|Conditions")
	bool IsSatisfied() const;

	// 블루프린트에서 만족 시점의 추가 연출을 붙일 수 있는 이벤트입니다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Puzzle|Group Actor|Events")
	void ReceiveGroupSatisfied();

	// 블루프린트에서 불만족 시점의 추가 연출을 붙일 수 있는 이벤트입니다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Puzzle|Group Actor|Events")
	void ReceiveGroupUnsatisfied();

	// 블루프린트에서 상태 변화에 반응할 수 있는 공통 이벤트입니다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Puzzle|Group Actor|Events")
	void ReceiveGroupStateChanged(bool bNewSatisfied);

protected:
	// 내부 그룹 컴포넌트의 만족 이벤트를 받아 외부 결과로 확장합니다.
	UFUNCTION()
	void HandleGroupSatisfied();

	// 내부 그룹 컴포넌트의 불만족 이벤트를 받아 외부 결과로 확장합니다.
	UFUNCTION()
	void HandleGroupUnsatisfied();

	// 내부 그룹 컴포넌트의 상태 변화 이벤트를 외부로 중계합니다.
	UFUNCTION()
	void HandleGroupStateChanged(bool bNewSatisfied);

	// ConditionActors 안에서 조건 소스 컴포넌트를 다시 수집합니다.
	void ResolveConditionSourcesFromActors();

	// 현재 만족 여부에 맞는 결과 액션을 모든 바인딩에 전달합니다.
	void DispatchResultBindings(bool bSatisfied) const;

	// 개별 결과 액터 하나에 액션을 전달합니다.
	void ExecuteResultAction(AActor* TargetActor, EOUUPuzzleResultAction Action) const;
};
