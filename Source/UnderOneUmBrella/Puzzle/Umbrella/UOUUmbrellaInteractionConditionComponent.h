// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Puzzle/Interaction/UOUInteractionConditionComponent.h"
#include "UOUUmbrellaInteractionConditionComponent.generated.h"

// 상호작용한 플레이어의 우산 상태가 맞을 때만 퍼즐 조건을 변경하는 컴포넌트입니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella Interaction Condition"))
class UNDERONEUMBRELLA_API UUOUUmbrellaInteractionConditionComponent : public UUOUInteractionConditionComponent
{
	GENERATED_BODY()

public:
	UUOUUmbrellaInteractionConditionComponent();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Umbrella", meta = (ToolTip = "상호작용이 유효하려면 플레이어 우산이 이 상태여야 합니다."))
	EUOUUmbrellaState RequiredUmbrellaState = EUOUUmbrellaState::Closed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime", meta = (ToolTip = "마지막으로 우산 상태 검사를 통과하지 못한 상호작용 액터입니다."))
	TObjectPtr<AActor> LastRejectedInteractor = nullptr;

protected:
	bool DoesInteractorUmbrellaStateMatch(AActor* Interactor) const;
};
