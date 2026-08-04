// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "UOURewardPresentationWidget.generated.h"

class UUOURewardPresentationWidget;
class UWidgetAnimation;

UENUM()
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
	bool InitializePresentation(const FUOURewardPresentationData& InPresentationData);

	// 선택된 Intro 애니메이션으로 Presentation을 시작합니다.
	bool StartPresentation();

	// 버튼 입력이나 자동 종료 시 선택된 Outro 애니메이션을 시작합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward Presentation")
	bool RequestClose();

	// 미리 생성된 Widget을 다음 Reward Presentation에 재사용할 수 있도록 초기 상태로 되돌립니다.
	bool ResetPresentation();

	EUOURewardPresentationWidgetState GetPresentationState() const;

	// 현재 Presentation을 요청한 RewardActor를 식별할 ID입니다.
	FName GetPresentationRewardId() const { return PresentationData.RewardId; }

	FUOURewardPresentationWidgetFinishedSignature OnPresentationFinished;

protected:
	virtual void NativeOnInitialized() override;

#if WITH_EDITOR
	virtual void ValidateCompiledDefaults(
		class IWidgetCompilerLog& CompileLog) const override;
#endif

	// WBP가 가진 애니메이션 중 Presentation 시작에 사용할 항목입니다. None이면 즉시 Hold로 진입합니다.
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Reward Presentation|Animation",
		meta = (
			DisplayName = "Intro Animation",
			GetOptions = "GetAvailableAnimationNames"))
	FName IntroAnimationName = NAME_None;

	// WBP가 가진 애니메이션 중 Presentation 종료에 사용할 항목입니다. None이면 즉시 종료합니다.
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Reward Presentation|Animation",
		meta = (
			DisplayName = "Outro Animation",
			GetOptions = "GetAvailableAnimationNames"))
	FName OutroAnimationName = NAME_None;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Reward Presentation|Runtime")
	FUOURewardPresentationData PresentationData;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Reward Presentation|Runtime")
	EUOURewardPresentationWidgetState PresentationState =
		EUOURewardPresentationWidgetState::Uninitialized;

private:
	// Intro 종료 뒤 결과 표시 유지 구간을 시작합니다. DisplayDuration이 0이면 자동 종료하지 않습니다.
	bool BeginPresentationHold();

	// Outro가 없거나 종료된 시점에 Presentation 완료를 보고합니다.
	bool FinishPresentation();

	UFUNCTION()
	TArray<FName> GetAvailableAnimationNames() const;

	UWidgetAnimation* FindAnimationByName(FName AnimationName) const;

	UFUNCTION()
	void HandleIntroAnimationFinished();

	UFUNCTION()
	void HandleOutroAnimationFinished();

	void ClearAutoCloseTimer();
	void HandleAutoCloseTimerElapsed();

	UPROPERTY(Transient)
	TObjectPtr<UWidgetAnimation> ResolvedIntroAnimation = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetAnimation> ResolvedOutroAnimation = nullptr;

	FTimerHandle AutoCloseTimerHandle;
	bool bPresentationHoldStarted = false;
};
