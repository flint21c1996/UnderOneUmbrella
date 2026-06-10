// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/UOUInteractable.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUInteractionConditionComponent.generated.h"

UENUM(BlueprintType)
enum class EUOUInteractionConditionMode : uint8
{
	SetSatisfied UMETA(DisplayName = "Set Satisfied", ToolTip = "상호작용하면 조건을 만족 상태로 유지합니다."),
	SetUnsatisfied UMETA(DisplayName = "Set Unsatisfied", ToolTip = "상호작용하면 조건을 불만족 상태로 유지합니다."),
	Toggle UMETA(DisplayName = "Toggle", ToolTip = "상호작용할 때마다 만족/불만족 상태를 전환합니다."),
	Pulse UMETA(DisplayName = "Pulse", ToolTip = "상호작용 순간에만 짧게 만족 상태가 됩니다. ConditionGroup에서 이벤트처럼 쓰기 좋습니다.")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUInteractionConditionInteractedSignature, AActor*, Interactor);

// 플레이어 상호작용 입력을 ConditionGroup에서 읽을 수 있는 조건 상태로 변환합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Interaction Condition"))
class UNDERONEUMBRELLA_API UUOUInteractionConditionComponent
	: public UUOUPuzzleConditionSourceComponent
	, public IUOUInteractable
{
	GENERATED_BODY()

public:
	UUOUInteractionConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Interaction")
	FUOUInteractionConditionInteractedSignature OnInteracted;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Interaction", meta = (ToolTip = "게임 시작 시 조건 만족 여부입니다."))
	bool bInitialSatisfied = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Interaction", meta = (ToolTip = "상호작용 입력이 들어왔을 때 조건 상태를 바꾸는 방식입니다."))
	EUOUInteractionConditionMode InteractionMode = EUOUInteractionConditionMode::Pulse;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Interaction", meta = (ClampMin = "0.0", EditCondition = "InteractionMode == EUOUInteractionConditionMode::Pulse", EditConditionHides, ToolTip = "Pulse 모드에서 만족 상태를 유지할 시간입니다. ConditionGroup 결과의 UnsatisfiedAction은 보통 None으로 둡니다."))
	float PulseDuration = 0.15f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime")
	TObjectPtr<AActor> LastInteractor = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Interaction")
	void TriggerInteraction(AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Interaction")
	void SetInteractionSatisfied(bool bNewSatisfied);

protected:
	void FinishPulse();

private:
	FTimerHandle PulseTimerHandle;
};
