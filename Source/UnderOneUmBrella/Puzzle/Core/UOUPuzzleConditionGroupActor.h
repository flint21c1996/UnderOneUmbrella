// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleConditionGroupComponent.h"
#include "UOUPuzzleConditionGroupActor.generated.h"

class USceneComponent;
class UUOUPuzzleConditionSourceComponent;

UENUM(BlueprintType)
enum class EOUUPuzzleResultAction : uint8
{
	None,
	Activate,
	Deactivate,
	Pause,
	Resume,
	Toggle
};

USTRUCT(BlueprintType)
struct FOUUPuzzleResultBinding
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle")
	EOUUPuzzleResultAction SatisfiedAction = EOUUPuzzleResultAction::Activate;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle")
	EOUUPuzzleResultAction UnsatisfiedAction = EOUUPuzzleResultAction::Deactivate;
};

// 여러 퍼즐 원인과 결과를 씬에서 직접 이어 주는 조건 그룹 허브 액터다.
UCLASS(meta=(DisplayName="UOU Puzzle Condition Group Actor"))
class AUOUPuzzleConditionGroupActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUPuzzleConditionGroupActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Group Actor|Events")
	FOnPuzzleGroupSatisfiedSignature OnSatisfied;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Group Actor|Events")
	FOnPuzzleGroupUnsatisfiedSignature OnUnsatisfied;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Group Actor|Events")
	FOnPuzzleGroupStateChangedSignature OnStateChanged;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Group Actor|Conditions")
	TArray<TObjectPtr<AActor>> ConditionActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Group Actor|Conditions")
	bool bCollectConditionSourcesFromConditionActors = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Group Actor|Results")
	TArray<FOUUPuzzleResultBinding> ResultBindings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUOUPuzzleConditionSourceComponent>> ResolvedConditionSources;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Group Actor")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Group Actor")
	TObjectPtr<UUOUPuzzleConditionGroupComponent> PuzzleConditionGroupComponent = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Group Actor|Conditions")
	void RefreshGroupSetup();

	UFUNCTION(BlueprintPure, Category = "Puzzle|Group Actor|Conditions")
	bool IsSatisfied() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Puzzle|Group Actor|Events")
	void ReceiveGroupSatisfied();

	UFUNCTION(BlueprintImplementableEvent, Category = "Puzzle|Group Actor|Events")
	void ReceiveGroupUnsatisfied();

	UFUNCTION(BlueprintImplementableEvent, Category = "Puzzle|Group Actor|Events")
	void ReceiveGroupStateChanged(bool bNewSatisfied);

protected:
	UFUNCTION()
	void HandleGroupSatisfied();

	UFUNCTION()
	void HandleGroupUnsatisfied();

	UFUNCTION()
	void HandleGroupStateChanged(bool bNewSatisfied);

	void ResolveConditionSourcesFromActors();
	void DispatchResultBindings(bool bSatisfied) const;
	void ExecuteResultAction(AActor* TargetActor, EOUUPuzzleResultAction Action) const;
	static FName GetActionFunctionName(EOUUPuzzleResultAction Action);
};
