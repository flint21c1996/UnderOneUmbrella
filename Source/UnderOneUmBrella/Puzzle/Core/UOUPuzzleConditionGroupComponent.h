// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "UOUPuzzleConditionGroupComponent.generated.h"

class UUOUPuzzleConditionSourceComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleGroupSatisfiedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleGroupUnsatisfiedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPuzzleGroupStateChangedSignature, bool, bIsSatisfied);

// 여러 조건 소스를 묶어서 하나의 만족 상태로 계산해 주는 중간 허브 컴포넌트다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUPuzzleConditionGroupComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPuzzleConditionGroupComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Events")
	FOnPuzzleGroupSatisfiedSignature OnSatisfied;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Events")
	FOnPuzzleGroupUnsatisfiedSignature OnUnsatisfied;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Events")
	FOnPuzzleGroupStateChangedSignature OnStateChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Conditions")
	TArray<FComponentReference> ConditionSourceReferences;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Conditions")
	bool bAutoCollectLocalConditionSources = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bIsSatisfied = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUOUPuzzleConditionSourceComponent>> ResolvedConditionSources;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUOUPuzzleConditionSourceComponent>> ExternalConditionSources;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Conditions")
	void RefreshNow();

	UFUNCTION(BlueprintPure, Category = "Puzzle|Conditions")
	bool IsSatisfied() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Conditions")
	int32 GetConditionCount() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Conditions")
	int32 GetSatisfiedCount() const;

	void SetExternalConditionSources(const TArray<UUOUPuzzleConditionSourceComponent*>& NewConditionSources);
	void ClearExternalConditionSources();

protected:
	UFUNCTION()
	void HandleConditionChanged(bool bNewSatisfied);

	void ResolveConditionSources();
	void SubscribeConditions();
	void UnsubscribeConditions();
	void RefreshSatisfiedState(bool bBroadcastEvents);
	bool AreAllConditionsSatisfied() const;
};
