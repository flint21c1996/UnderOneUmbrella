// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/UOUContextInteractionTypes.h"
#include "Puzzle/Interaction/UOUInteractionConditionComponent.h"
#include "UOUContextInteractionConditionComponent.generated.h"

class UUOUPlayerInteractionExecutorComponent;

// 플레이어 상호작용 연출을 거쳐 조건을 변경할 수 있는 확장용 상호작용 조건 컴포넌트입니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Context Interaction Condition"))
class UNDERONEUMBRELLA_API UUOUContextInteractionConditionComponent : public UUOUInteractionConditionComponent
{
	GENERATED_BODY()

public:
	UUOUContextInteractionConditionComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Interaction", meta = (ToolTip = "상호작용이 유효할 때 플레이어에게 요청할 공통 연출 정보입니다."))
	FUOUPlayerInteractionRequest PlayerInteractionRequest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Interaction", meta = (ToolTip = "켜져 있으면 플레이어 연출이 끝난 뒤 조건을 변경합니다. 꺼져 있으면 상호작용 즉시 조건을 변경합니다."))
	bool bWaitForPlayerInteraction = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Interaction", meta = (ToolTip = "켜져 있으면 조건이 이미 만족된 뒤에는 다시 상호작용을 시작하지 않습니다."))
	bool bBlockInteractionAfterSatisfied = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime", meta = (ToolTip = "현재 이 컴포넌트가 플레이어 연출 완료를 기다리는 중이면 true입니다."))
	bool bWaitingForPlayerInteraction = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime", meta = (ToolTip = "현재 완료를 기다리는 상호작용 플레이어입니다."))
	TObjectPtr<AActor> PendingInteractor = nullptr;

protected:
	virtual bool CanStartContextInteraction(AActor* Interactor) const;
	virtual void HandleAcceptedContextInteraction(AActor* Interactor);
	virtual void HandleRejectedContextInteraction(AActor* Interactor);

	void StartContextInteraction(AActor* Interactor);
	void FinishContextInteraction(bool bInterrupted);
	UUOUPlayerInteractionExecutorComponent* FindPlayerInteractionExecutor(AActor* Interactor) const;
	void ClearPlayerInteractionWait();

	UFUNCTION()
	void HandlePlayerInteractionFinished(UObject* InteractionSource, bool bInterrupted);

	UPROPERTY(Transient)
	TObjectPtr<UUOUPlayerInteractionExecutorComponent> PendingExecutor = nullptr;
};
