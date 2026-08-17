// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Puzzle/Interaction/UOUContextInteractionConditionComponent.h"
#include "Puzzle/Umbrella/UOUUmbrellaConditionTypes.h"
#include "UOUUmbrellaInteractionConditionComponent.generated.h"

// 상호작용한 플레이어의 우산 상태가 맞을 때만 퍼즐 조건을 변경하는 컴포넌트입니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella Interaction Condition"))
class UNDERONEUMBRELLA_API UUOUUmbrellaInteractionConditionComponent : public UUOUContextInteractionConditionComponent
{
	GENERATED_BODY()

public:
	UUOUUmbrellaInteractionConditionComponent();

	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Umbrella", meta = (ToolTip = "상호작용이 유효하려면 플레이어 우산이 이 상태여야 합니다."))
	EUOUUmbrellaState RequiredUmbrellaState = EUOUUmbrellaState::Closed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Umbrella", meta = (ToolTip = "Any가 아니면 플레이어 우산 손잡이 방향까지 함께 검사합니다."))
	EUOUUmbrellaDirectionRequirement RequiredUmbrellaDirection = EUOUUmbrellaDirectionRequirement::Any;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime", meta = (ToolTip = "마지막으로 우산 상태 검사를 통과하지 못한 상호작용 액터입니다."))
	TObjectPtr<AActor> LastRejectedInteractor = nullptr;

protected:
	virtual bool CanStartContextInteraction(AActor* Interactor) const override;
	virtual void HandleAcceptedContextInteraction(AActor* Interactor) override;
	virtual void HandleRejectedContextInteraction(AActor* Interactor) override;

	bool DoesInteractorUmbrellaStateMatch(AActor* Interactor) const;
	bool DoesUmbrellaDirectionMatch(const UUOUUmbrellaComponent& UmbrellaComponent) const;
};
