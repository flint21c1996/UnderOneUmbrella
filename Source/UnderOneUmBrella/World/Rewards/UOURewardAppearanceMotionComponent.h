// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "UOURewardAppearanceMotionComponent.generated.h"

class USceneComponent;
class USplineComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOURewardAppearanceMotionFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOURewardAppearanceMotionCueSignature,
	const FUOURewardPresentationCue&,
	Cue);

// Reward가 활성화될 때 Spline 시작점에서 에디터에 배치한 최종 Transform까지 이동시키는 등장 연출입니다.
UCLASS(ClassGroup=(Reward), meta=(BlueprintSpawnableComponent, DisplayName="UOU Reward Appearance Motion"))
class UNDERONEUMBRELLA_API UUOURewardAppearanceMotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOURewardAppearanceMotionComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Target의 현재 Transform을 최종 상태로 저장한 뒤 MotionPath의 시작점부터 등장 이동을 재생합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Appearance Motion")
	bool StartAppearanceMotion(
		USceneComponent* TargetComponent,
		USplineComponent* MotionPath,
		const TArray<FUOURewardPresentationCue>& CueRequests);

	// 진행 중인 등장 이동을 중단하고 필요하면 Target을 연출 전 최종 Transform으로 되돌립니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Appearance Motion")
	void StopAppearanceMotion(bool bRestoreFinalTransform = true);

	UFUNCTION(BlueprintPure, Category = "Reward|Appearance Motion")
	bool IsAppearanceMotionPlaying() const;

#if WITH_EDITOR
	float GetMotionDurationForEditor() const { return MotionDuration; }
	const TArray<FUOURewardMotionCueTiming>& GetCueTimelineForEditor() const
	{
		return CueTimeline;
	}
	bool SynchronizeCueTimelineForEditor(
		const TArray<FUOURewardPresentationCue>& CueRequests);
	void SetCueTriggerTimeForEditor(const FGuid& RequestId, float TriggerTime);
	void SetPresentationCloseTimeForEditor(const FGuid& RequestId, float CloseTime);
#endif

	UPROPERTY(BlueprintAssignable, Category = "Reward|Appearance Motion|Events")
	FUOURewardAppearanceMotionFinishedSignature OnAppearanceMotionFinished;

	UPROPERTY(BlueprintAssignable, Category = "Reward|Appearance Motion|Events")
	FUOURewardAppearanceMotionCueSignature OnAppearanceMotionCue;

protected:
	// 기존 Reward는 즉시 활성화되도록 기본값을 끄고, 등장 연출이 필요한 인스턴스에서만 켭니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Appearance Motion")
	bool bMotionEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Appearance Motion", meta = (ClampMin = "0.0"))
	float MotionDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Appearance Motion")
	bool bFollowSplineRotation = false;

	// 등장 시작 시 최종 크기에 곱할 값입니다. 0이면 작은 점에서 원래 크기로 커집니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Appearance Motion", meta = (ClampMin = "0.0"))
	float StartScaleMultiplier = 1.0f;

	// 등장 시작점에서 적용하고 도착할수록 0으로 줄어드는 추가 회전입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Appearance Motion")
	FRotator StartRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Appearance Motion", meta = (ClampMin = "1.0"))
	float EaseExponent = 2.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reward|Appearance Motion|Runtime")
	bool bMotionPlaying = false;

	// Appearance Feedback의 CueRequest별 실행 시간입니다. 전용 타임라인 UI가 편집합니다.
	UPROPERTY()
	TArray<FUOURewardMotionCueTiming> CueTimeline;

private:
	struct FActiveCueTiming
	{
		int32 CueIndex = INDEX_NONE;
		float TriggerTime = 0.0f;
		bool bPresentationClose = false;
	};

	void ApplyMotion(float NormalizedTime);
	void BuildCueSchedule();
	void BroadcastPassedCues(float CurrentTime);
	void FinishAppearanceMotion();
	void ResetRuntimeState();

	TWeakObjectPtr<USceneComponent> MotionTarget;
	TWeakObjectPtr<USplineComponent> ActiveMotionPath;
	FTransform FinalRelativeTransform = FTransform::Identity;
	FVector EndPathRelativeLocation = FVector::ZeroVector;
	FQuat EndPathRelativeRotation = FQuat::Identity;
	TArray<FUOURewardPresentationCue> ActiveCueRequests;
	TArray<FActiveCueTiming> ActiveCueTimeline;
	int32 NextCueIndex = 0;
	float ElapsedTime = 0.0f;
};
