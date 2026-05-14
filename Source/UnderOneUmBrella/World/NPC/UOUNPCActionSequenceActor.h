// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/NPC/UOUNPCActionTypes.h"
#include "UOUNPCActionSequenceActor.generated.h"

class AUOUNPCCharacter;
class USceneComponent;

// 활성화 또는 비활성화될 때 NPC 액션 요청 목록을 순서대로 실행하는 액터입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU NPC Action Sequence", ToolTip = "설정된 NPC 액션 목록을 순서대로 실행합니다."))
class AUOUNPCActionSequenceActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUNPCActionSequenceActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Sequence")
	TObjectPtr<USceneComponent> RootScene = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Sequence", meta = (ToolTip = "액션 시퀀스를 받을 NPC입니다."))
	TObjectPtr<AUOUNPCCharacter> TargetNPC = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Sequence", meta = (ToolTip = "Activate 호출 시 순서대로 실행할 액션 목록입니다. 예: B로 이동, C로 점프, D로 점프."))
	TArray<FUOUNPCActionRequest> ActivateActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Sequence", meta = (ToolTip = "Deactivate 호출 시 순서대로 실행할 액션 목록입니다. 복귀 경로나 취소 동작에 사용할 수 있습니다."))
	TArray<FUOUNPCActionRequest> DeactivateActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Sequence", meta = (ToolTip = "플레이 시작 시 Activate 시퀀스를 자동으로 실행합니다."))
	bool bActivateOnBeginPlay = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "이 시퀀스 액터의 현재 활성화 상태입니다."))
	bool bActivated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "현재 액션 목록을 실행 중이면 true입니다."))
	bool bRunningSequence = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "현재 실행 중인 시퀀스가 Deactivate에서 시작된 경우 true입니다."))
	bool bRunningDeactivateSequence = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "현재 실행 중인 액션의 인덱스입니다."))
	int32 CurrentActionIndex = INDEX_NONE;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Activate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Toggle();

	UFUNCTION(BlueprintCallable, Category = "NPC|Sequence")
	void StopSequence();

protected:
	UPROPERTY()
	TArray<FUOUNPCActionRequest> CurrentSequenceActions;

	void StartSequence(const TArray<FUOUNPCActionRequest>& Actions, bool bDeactivateSequence);
	void RunCurrentAction();
	void FinishSequence();
	void BindToTargetNPC();
	void UnbindFromTargetNPC();

	UFUNCTION()
	void HandleNPCActionCompleted(AUOUNPCCharacter* NPC, UObject* ActionSource);
};
