// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUDialoguePuzzleStateComponent.generated.h"

class UUOUDialogueSourceComponent;
class UUOUDialogueTriggerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUOUDialoguePuzzleStateAppliedSignature,
	bool,
	bPuzzleSolved,
	FName,
	DialogueState);

// ConditionGroup의 결과 액션을 DialogueSource의 Before/After 상태로 변환합니다.
// ConditionGroup ResultBinding에서는 가능하면 이 컴포넌트를 Specific Component로 지정하세요.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Puzzle State"))
class UNDERONEUMBRELLA_API UUOUDialoguePuzzleStateComponent
	: public UActorComponent
	, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	UUOUDialoguePuzzleStateComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Dialogue State")
	void SetPuzzleSolved(bool bNewPuzzleSolved);

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Dialogue State")
	void ApplyCurrentDialogueState();

	UFUNCTION(BlueprintPure, Category = "Puzzle|Dialogue State")
	bool IsPuzzleSolved() const { return bPuzzleSolved; }

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Dialogue State")
	UUOUDialogueSourceComponent* ResolveDialogueSource() const;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Dialogue State")
	UUOUDialogueTriggerComponent* ResolveDialogueTrigger() const;

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Dialogue State")
	FUOUDialoguePuzzleStateAppliedSignature OnDialoguePuzzleStateApplied;

	// 직접 지정할 대화 소스입니다. 비어 있으면 Target Dialogue Actor 또는 소유 액터에서 찾습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State|Target")
	TObjectPtr<UUOUDialogueSourceComponent> TargetDialogueSource = nullptr;

	// 상태 변경 시 TriggerOnce를 초기화하고 현재 접근 중인 플레이어에게 새 힌트를 갱신할 Trigger입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State|Target")
	TObjectPtr<UUOUDialogueTriggerComponent> TargetDialogueTrigger = nullptr;

	// 어댑터와 대화 컴포넌트가 서로 다른 액터에 있을 때 지정합니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Dialogue State|Target")
	TObjectPtr<AActor> TargetDialogueActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State")
	FName UnsolvedDialogueState = TEXT("BeforePuzzle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State")
	FName SolvedDialogueState = TEXT("AfterPuzzle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State")
	bool bStartSolved = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State")
	bool bApplyInitialStateOnBeginPlay = true;

	// 상태가 달라질 때 bCanReplay=false로 막힌 재생 기록을 초기화합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State|Refresh")
	bool bResetDialoguePlaybackOnStateChange = true;

	// 상태가 달라질 때 DialogueTrigger의 TriggerOnce 기록을 초기화합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State|Refresh")
	bool bResetTriggerOnStateChange = true;

	// 플레이어가 이미 범위 안에 있으면 바뀐 상태의 말풍선을 즉시 다시 표시합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State|Refresh")
	bool bRefreshOverlappingInteraction = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue State|Runtime")
	bool bPuzzleSolved = false;

private:
	AActor* ResolveDialogueActor() const;
};
