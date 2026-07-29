// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "UOURewardPresentationWidget.generated.h"

class UUOURewardPresentationWidget;

UENUM(BlueprintType)
enum class EUOURewardPresentationWidgetState : uint8
{
	Uninitialized,
	Ready,
	Presenting,
	Closing,
	Finished
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOURewardPresentationWidgetFinishedSignature,
	UUOURewardPresentationWidget*,
	PresentationWidget);

// 서로 다른 Reward 결과 Widget을 동일한 생명주기로 실행하기 위한 공통 기반입니다.
UCLASS(Abstract, Blueprintable)
class UNDERONEUMBRELLA_API UUOURewardPresentationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeDestruct() override;

	// 새 결과 데이터를 한 번 전달하고 Widget을 시작 가능한 상태로 만듭니다.
	UFUNCTION(BlueprintCallable, Category = "Reward Presentation")
	bool InitializePresentation(const FUOURewardPresentationData& InPresentationData);

	// 파생 Widget이 자신의 Intro 또는 전체 결과 애니메이션을 시작하도록 요청합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward Presentation")
	bool StartPresentation();

	// 현재 활성 Widget에 의미 기반 Cue를 전달합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward Presentation")
	bool HandlePresentationCue(const FUOURewardPresentationCue& Cue);

	// Intro가 끝난 뒤 결과를 읽거나 입력을 기다리는 구간을 시작합니다. DisplayDuration이 0이면 자동 종료하지 않습니다.
	UFUNCTION(BlueprintCallable, Category = "Reward Presentation")
	bool BeginPresentationHold();

	// 파생 Widget이 자신의 Outro 애니메이션을 시작하도록 요청합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward Presentation")
	bool RequestClose();

	// 파생 Widget이 모든 애니메이션을 끝낸 뒤 호출하는 완료 보고 함수입니다.
	UFUNCTION(BlueprintCallable, Category = "Reward Presentation")
	bool FinishPresentation();

	// 미리 생성된 Widget을 다음 Reward Presentation에 재사용할 수 있도록 초기 상태로 되돌립니다.
	UFUNCTION(BlueprintCallable, Category = "Reward Presentation")
	bool ResetPresentation();

	UFUNCTION(BlueprintPure, Category = "Reward Presentation")
	EUOURewardPresentationWidgetState GetPresentationState() const;

	UFUNCTION(BlueprintPure, Category = "Reward Presentation")
	bool IsPresentationActive() const;

	// 현재 Presentation을 요청한 RewardActor를 식별할 ID입니다.
	UFUNCTION(BlueprintPure, Category = "Reward Presentation")
	FName GetPresentationRewardId() const { return PresentationData.RewardId; }

	UPROPERTY(BlueprintAssignable, Category = "Reward Presentation|Events")
	FUOURewardPresentationWidgetFinishedSignature OnPresentationFinished;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Reward Presentation|Runtime")
	FUOURewardPresentationData PresentationData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Reward Presentation|Runtime")
	EUOURewardPresentationWidgetState PresentationState =
		EUOURewardPresentationWidgetState::Uninitialized;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Reward Presentation|Events")
	void ReceivePresentationInitialized(const FUOURewardPresentationData& InPresentationData);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Reward Presentation|Events")
	void ReceivePresentationStarted();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Reward Presentation|Events")
	void ReceivePresentationCue(const FUOURewardPresentationCue& Cue);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Reward Presentation|Events")
	void ReceivePresentationCloseRequested();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Reward Presentation|Events")
	void ReceivePresentationReset();

private:
	void ClearAutoCloseTimer();
	void HandleAutoCloseTimerElapsed();

	FTimerHandle AutoCloseTimerHandle;
	bool bPresentationHoldStarted = false;
};
