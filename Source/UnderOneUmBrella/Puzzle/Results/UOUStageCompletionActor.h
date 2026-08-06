// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUStageCompletionActor.generated.h"

class USceneComponent;

/** 퍼즐 결과를 명시적인 스테이지 클리어로 변환하여 현재 진행도를 SaveGame에 확정하는 Actor입니다. */
UCLASS(meta=(DisplayName="UOU Stage Completion Actor"))
class UNDERONEUMBRELLA_API AUOUStageCompletionActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUStageCompletionActor();

	/** Condition Group이 전달한 액션이 TriggerAction과 일치할 때 CompleteStage를 호출합니다. */
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

	/** 현재 활성 스테이지의 임시 Reward를 SaveGame에 확정합니다. 저장에 성공한 경우에만 true를 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Stage Completion")
	bool CompleteStage();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Completion")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	/** 이 액션을 받았을 때 스테이지 클리어를 확정합니다. Condition Group의 기본 만족 액션은 Activate입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Completion")
	EOUUPuzzleResultAction TriggerAction = EOUUPuzzleResultAction::Activate;

	/** 이 Actor가 현재 맵에서 이미 진행도 저장을 완료했는지를 나타내는 런타임 상태입니다. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stage Completion|Runtime")
	bool bHasCommitted = false;
};
