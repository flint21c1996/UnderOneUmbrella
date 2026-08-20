// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUDialogueCompletedConditionComponent.generated.h"

class UUOUDialogueSourceComponent;
class UUOUUISubsystem;
class AActor;

// 지정한 NPC 대화가 마지막 대사까지 정상 완료됐는지를 퍼즐 조건으로 노출합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Completed Condition"))
class UNDERONEUMBRELLA_API UUOUDialogueCompletedConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUDialogueCompletedConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	// 완료 여부를 관찰할 대화 소스입니다. 비어 있으면 소유 액터에서 자동으로 찾습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue")
	TObjectPtr<UUOUDialogueSourceComponent> TargetDialogueSource = nullptr;

	// 게임 시작 시 조건 만족 여부입니다. 일반적인 대화 완료 조건은 false로 둡니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Dialogue")
	bool bInitialSatisfied = false;

	// 실제로 관찰 중인 대화 소스입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime")
	TObjectPtr<UUOUDialogueSourceComponent> ResolvedDialogueSource = nullptr;

	// 반복 사용이 필요할 때 완료 조건을 다시 불만족 상태로 되돌립니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Dialogue")
	void ResetCompletion();

private:
	UFUNCTION()
	void HandleDialogueCompleted(UUOUDialogueSourceComponent* CompletedDialogueSource);

	UUOUDialogueSourceComponent* ResolveTargetDialogueSource() const;
	UUOUUISubsystem* ResolveUISubsystem() const;
	void SubscribeDialogueCompletion();
	void UnsubscribeDialogueCompletion();

	UPROPERTY(Transient)
	TObjectPtr<UUOUUISubsystem> SubscribedUISubsystem = nullptr;
};
