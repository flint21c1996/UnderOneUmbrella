// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/UOUContextInteractionTypes.h"
#include "UOUPlayerInteractionExecutorComponent.generated.h"

class UAnimInstance;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnUOUPlayerInteractionFinishedSignature,
	UObject*, InteractionSource,
	bool, bInterrupted);

// 플레이어에게 요청된 상호작용 연출을 실행하고 완료 시점을 알려주는 컴포넌트입니다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent, DisplayName="UOU Player Interaction Executor"))
class UNDERONEUMBRELLA_API UUOUPlayerInteractionExecutorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPlayerInteractionExecutorComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Player")
	FOnUOUPlayerInteractionFinishedSignature OnInteractionFinished;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Interaction|Runtime", meta = (ToolTip = "플레이어 상호작용 연출이 진행 중이면 true입니다."))
	bool bInteractionActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Interaction|Runtime", meta = (ToolTip = "현재 플레이어 상호작용을 요청한 오브젝트입니다."))
	TObjectPtr<UObject> ActiveInteractionSource = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Interaction|Runtime", meta = (ToolTip = "현재 상호작용 연출이 플레이어 입력을 막고 있으면 true입니다."))
	bool bBlockInputWhileActive = false;

	UFUNCTION(BlueprintCallable, Category = "Interaction|Player")
	bool TryStartInteraction(UObject* InteractionSource, const FUOUPlayerInteractionRequest& InteractionRequest);

	UFUNCTION(BlueprintCallable, Category = "Interaction|Player")
	void CancelActiveInteraction();

	UFUNCTION(BlueprintPure, Category = "Interaction|Player")
	bool IsInteractionActiveFor(UObject* InteractionSource) const;

	UFUNCTION(BlueprintPure, Category = "Interaction|Player")
	bool ShouldBlockPlayerInput() const;

	UFUNCTION(BlueprintCallable, Category = "Interaction|Player")
	void RequestPlayerInputBlock(UObject* BlockSource, bool bStopMovementImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "Interaction|Player")
	void ReleasePlayerInputBlock(UObject* BlockSource);

	UFUNCTION(BlueprintPure, Category = "Interaction|Player")
	bool IsPlayerInputBlockedBy(UObject* BlockSource) const;

	UFUNCTION(BlueprintCallable, Category = "Interaction|Player", meta = (WorldContext = "WorldContextObject"))
	static UUOUPlayerInteractionExecutorComponent* FindLocalPlayerExecutor(
		const UObject* WorldContextObject,
		int32 PlayerIndex = 0);

protected:
	void FinishActiveInteraction(bool bInterrupted);
	void ClearMontageDelegate();
	void StopOwnerMovementImmediately() const;
	bool HasExternalPlayerInputBlock() const;

	void HandleInteractionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveMontage = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAnimInstance> ActiveAnimInstance;

	TMap<TWeakObjectPtr<UObject>, int32> InputBlockRequestCounts;
};
