// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "UOURewardCollectionMotionComponent.generated.h"

class USceneComponent;
class USplineComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOURewardCollectionMotionFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOURewardCollectionMotionCueSignature,
	const FUOURewardPresentationCue&,
	Cue);

// 수집 순간 Reward의 시각 컴포넌트에 짧은 이동·회전·크기 변화를 적용합니다.
UCLASS(ClassGroup=(Reward), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOURewardCollectionMotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOURewardCollectionMotionComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// 현재 Transform을 시작점으로 저장하고 Spline 경로를 따라 수집 움직임을 재생합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Motion")
	bool StartCollectionMotion(
		USceneComponent* TargetComponent,
		USplineComponent* MotionPath,
		const TArray<FUOURewardPresentationCue>& CueRequests);

#if WITH_EDITOR
	float GetMotionDurationForEditor() const { return MotionDuration; }
	const TArray<FUOURewardMotionCueTiming>& GetCueTimelineForEditor() const
	{
		return CueTimeline;
	}
	void SetCueTriggerTimeForEditor(const FGuid& RequestId, float TriggerTime);
#endif

	UPROPERTY(BlueprintAssignable, Category = "Reward|Motion|Events")
	FUOURewardCollectionMotionFinishedSignature OnCollectionMotionFinished;

	UPROPERTY(BlueprintAssignable, Category = "Reward|Motion|Events")
	FUOURewardCollectionMotionCueSignature OnCollectionMotionCue;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion")
	bool bMotionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion", meta = (ClampMin = "0.0"))
	float MotionDuration = 0.8f;

	// 활성화하면 Spline Point에 설정한 회전을 시작점 기준 상대 회전으로 적용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion")
	bool bFollowSplineRotation = false;

	// Spline 회전과 별개로 움직임 전체에 걸쳐 추가되는 회전량입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion")
	FRotator AdditionalRotation = FRotator(0.0f, 360.0f, 0.0f);

	// 시작 크기에 곱해지는 종료 배율입니다. 0이면 움직임 끝에서 완전히 축소됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion", meta = (ClampMin = "0.0"))
	float EndScaleMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion", meta = (ClampMin = "1.0"))
	float EaseExponent = 2.0f;

	// CueRequest별 실행 시간입니다. 값 편집은 MotionComponent의 전용 타임라인 UI가 담당합니다.
	UPROPERTY()
	TArray<FUOURewardMotionCueTiming> CueTimeline;

private:
	struct FActiveCueTiming
	{
		int32 CueIndex = INDEX_NONE;
		float TriggerTime = 0.0f;
	};

	void ApplyMotion(float NormalizedTime);
	void BuildCueSchedule();
	void BroadcastPassedCues(float CurrentTime);
	void FinishCollectionMotion();

	TWeakObjectPtr<USceneComponent> MotionTarget;
	TWeakObjectPtr<USplineComponent> ActiveMotionPath;
	FTransform StartRelativeTransform = FTransform::Identity;
	FVector StartPathRelativeLocation = FVector::ZeroVector;
	FQuat StartPathRelativeRotation = FQuat::Identity;
	// FeedbackComponent에서 전달받아 현재 재생 중에만 사용하는 Cue 복사본입니다.
	TArray<FUOURewardPresentationCue> ActiveCueRequests;
	TArray<FActiveCueTiming> ActiveCueTimeline;
	int32 NextCueIndex = 0;
	float ElapsedTime = 0.0f;
	bool bMotionPlaying = false;
};
