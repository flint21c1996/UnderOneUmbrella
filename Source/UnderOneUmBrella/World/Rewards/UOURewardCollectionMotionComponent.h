// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Rewards/UOURewardPresentationTypes.h"
#include "UOURewardCollectionMotionComponent.generated.h"

class USceneComponent;
class USplineComponent;

UENUM(BlueprintType)
enum class EUOURewardMotionPhase : uint8
{
	Appearance,
	Collection
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOURewardAppearanceMotionFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOURewardAppearanceMotionCueSignature,
	const FUOURewardPresentationCue&,
	Cue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOURewardCollectionMotionFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOURewardCollectionMotionCueSignature,
	const FUOURewardPresentationCue&,
	Cue);

// Reward의 등장과 수집 단계에서 Spline 기반 이동·회전·크기 변화를 재생합니다.
// 기존 에셋 호환성을 위해 C++ 클래스명은 CollectionMotionComponent를 유지합니다.
UCLASS(
	ClassGroup=(Reward),
	meta=(BlueprintSpawnableComponent, DisplayName="UOU Reward Motion"))
class UNDERONEUMBRELLA_API UUOURewardCollectionMotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOURewardCollectionMotionComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 현재 Transform을 최종 상태로 저장하고 Spline 시작점부터 등장 움직임을 재생합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Motion|Appearance")
	bool StartAppearanceMotion(
		USceneComponent* TargetComponent,
		USplineComponent* MotionPath,
		const TArray<FUOURewardPresentationCue>& CueRequests);

	// 진행 중인 등장 움직임을 중단하고 필요하면 Target을 연출 전 최종 Transform으로 되돌립니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Motion|Appearance")
	void StopAppearanceMotion(bool bRestoreFinalTransform = true);

	UFUNCTION(BlueprintPure, Category = "Reward|Motion|Appearance")
	bool IsAppearanceMotionPlaying() const;

	// 현재 Transform을 시작점으로 저장하고 Spline 경로를 따라 수집 움직임을 재생합니다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Motion|Collection")
	bool StartCollectionMotion(
		USceneComponent* TargetComponent,
		USplineComponent* MotionPath,
		const TArray<FUOURewardPresentationCue>& CueRequests);

#if WITH_EDITOR
	float GetMotionDurationForEditor(EUOURewardMotionPhase Phase) const;
	const TArray<FUOURewardMotionCueTiming>& GetCueTimelineForEditor(
		EUOURewardMotionPhase Phase) const;
	bool SynchronizeCueTimelineForEditor(
		EUOURewardMotionPhase Phase,
		const TArray<FUOURewardPresentationCue>& CueRequests);
	void SetCueTriggerTimeForEditor(
		EUOURewardMotionPhase Phase,
		const FGuid& RequestId,
		float TriggerTime);
	void SetPresentationCloseTimeForEditor(
		EUOURewardMotionPhase Phase,
		const FGuid& RequestId,
		float CloseTime);
#endif

	UPROPERTY(BlueprintAssignable, Category = "Reward|Motion|Appearance|Events")
	FUOURewardAppearanceMotionFinishedSignature OnAppearanceMotionFinished;

	UPROPERTY(BlueprintAssignable, Category = "Reward|Motion|Appearance|Events")
	FUOURewardAppearanceMotionCueSignature OnAppearanceMotionCue;

	UPROPERTY(BlueprintAssignable, Category = "Reward|Motion|Collection|Events")
	FUOURewardCollectionMotionFinishedSignature OnCollectionMotionFinished;

	UPROPERTY(BlueprintAssignable, Category = "Reward|Motion|Collection|Events")
	FUOURewardCollectionMotionCueSignature OnCollectionMotionCue;

protected:
	// 기존 Reward는 즉시 활성화되도록 기본값을 끄고, 등장 연출이 필요한 인스턴스에서만 켭니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Appearance")
	bool bAppearanceMotionEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Appearance", meta = (ClampMin = "0.0"))
	float AppearanceMotionDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Appearance")
	bool bFollowAppearanceSplineRotation = false;

	// 등장 시작 시 최종 크기에 곱할 값입니다. 0이면 작은 점에서 원래 크기로 커집니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Appearance", meta = (ClampMin = "0.0"))
	float AppearanceStartScaleMultiplier = 1.0f;

	// 등장 시작점에서 적용하고 도착할수록 0으로 줄어드는 추가 회전입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Appearance")
	FRotator AppearanceStartRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Appearance", meta = (ClampMin = "1.0"))
	float AppearanceEaseExponent = 2.0f;

	// Appearance Feedback의 CueRequest별 실행 시간입니다. 전용 타임라인 UI가 편집합니다.
	UPROPERTY()
	TArray<FUOURewardMotionCueTiming> AppearanceCueTimeline;

	// 아래 Collection 프로퍼티 이름과 기본값은 기존 에셋 직렬화 호환성을 위해 유지합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Collection")
	bool bMotionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Collection", meta = (ClampMin = "0.0"))
	float MotionDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Collection")
	bool bFollowSplineRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Collection")
	FRotator AdditionalRotation = FRotator(0.0f, 360.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Collection", meta = (ClampMin = "0.0"))
	float EndScaleMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Motion|Collection", meta = (ClampMin = "1.0"))
	float EaseExponent = 2.0f;

	// Collection Feedback의 CueRequest별 실행 시간입니다. 전용 타임라인 UI가 편집합니다.
	UPROPERTY()
	TArray<FUOURewardMotionCueTiming> CueTimeline;

private:
	struct FActiveCueTiming
	{
		int32 CueIndex = INDEX_NONE;
		float TriggerTime = 0.0f;
		bool bPresentationClose = false;
	};

	bool StartMotion(
		EUOURewardMotionPhase Phase,
		USceneComponent* TargetComponent,
		USplineComponent* MotionPath,
		const TArray<FUOURewardPresentationCue>& CueRequests);
	bool IsMotionEnabled(EUOURewardMotionPhase Phase) const;
	float GetMotionDuration(EUOURewardMotionPhase Phase) const;
	float GetEaseExponent(EUOURewardMotionPhase Phase) const;
	bool ShouldFollowSplineRotation(EUOURewardMotionPhase Phase) const;
	const TArray<FUOURewardMotionCueTiming>& GetCueTimeline(EUOURewardMotionPhase Phase) const;
	TArray<FUOURewardMotionCueTiming>& GetMutableCueTimeline(EUOURewardMotionPhase Phase);
	void ApplyMotion(float NormalizedTime);
	void BuildCueSchedule();
	void BroadcastPassedCues(float CurrentTime);
	void FinishMotion();
	void ResetRuntimeState();

	TWeakObjectPtr<USceneComponent> MotionTarget;
	TWeakObjectPtr<USplineComponent> ActiveMotionPath;
	// Appearance에서는 최종 Transform, Collection에서는 시작 Transform입니다.
	FTransform ReferenceRelativeTransform = FTransform::Identity;
	// ReferenceRelativeTransform에 대응하는 Spline 끝점(Appearance) 또는 시작점(Collection)입니다.
	FVector AnchorPathRelativeLocation = FVector::ZeroVector;
	FQuat AnchorPathRelativeRotation = FQuat::Identity;
	TArray<FUOURewardPresentationCue> ActiveCueRequests;
	TArray<FActiveCueTiming> ActiveCueTimeline;
	EUOURewardMotionPhase ActiveMotionPhase = EUOURewardMotionPhase::Collection;
	int32 NextCueIndex = 0;
	float ElapsedTime = 0.0f;
	bool bMotionPlaying = false;
};
