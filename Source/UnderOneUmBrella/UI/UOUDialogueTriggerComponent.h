// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "TimerManager.h"
#include "UOUDialogueTriggerComponent.generated.h"

class UUserWidget;
class UWidgetComponent;
class UUOUDialogueSourceComponent;
class UUOUDialogueCoverTargetComponent;
class UUOUUmbrellaComponent;
class UUOUUmbrellaCoverVolumeComponent;
class UUOUCameraControllerComponent;
class UUOUPlayerInteractionExecutorComponent;
class UUOUUISubsystem;
class AActor;
class APlayerController;

// 플레이어가 가까이 왔을 때 대화 소스를 실행하는 트리거 컴포넌트입니다.
// 필요하면 우산을 펼친 채 일정 시간 동안 대상을 씌웠는지도 같이 검사합니다.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Trigger"))
class UNDERONEUMBRELLA_API UUOUDialogueTriggerComponent : public USphereComponent, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	UUOUDialogueTriggerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 지정한 액터를 대화 시작자로 보고 연결된 대화 소스를 실행합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Trigger")
	bool TryStartDialogue(AActor* InstigatorActor);

	// 현재 연결된 월드 힌트 위젯을 표시합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Hint")
	void ShowInteractionHint();

	// 현재 연결된 월드 힌트 위젯을 숨깁니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Hint")
	void HideInteractionHint();

	// 소유 액터에서 사용할 힌트 위젯 컴포넌트를 다시 찾아 캐시합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Hint")
	UWidgetComponent* ResolveHintWidgetComponent();

	// 한 번만 실행 옵션으로 막힌 상태를 풀어서 다시 대화가 시작될 수 있게 합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Trigger")
	void ResetTrigger();

	// 퍼즐 결과 등 외부 상태에 따라 새로운 대화 시작을 허용하거나 차단합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Trigger")
	void SetDialogueInteractionEnabled(bool bNewEnabled);

	// 이미 트리거 안에 있는 액터를 현재 대화 가능 상태로 다시 평가합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Trigger")
	void RefreshOverlappingInteraction();

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

	// 실행할 대화 소스입니다. 비어 있으면 이 컴포넌트가 붙은 액터에서 대화 소스를 찾습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Trigger")
	TObjectPtr<UUOUDialogueSourceComponent> DialogueSource = nullptr;

	// 켜져 있으면 Pawn 계열 액터만 대화를 시작할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bOnlyPawn = true;

	// false면 접촉은 추적하지만 힌트 표시와 새로운 대화 시작은 차단합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bDialogueInteractionEnabled = true;

	// 켜져 있으면 이 대화 트리거 범위에 들어왔을 때 물음표나 느낌표 같은 월드 힌트를 같이 표시합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint")
	bool bEnableInteractionHint = true;

	// 직접 지정할 힌트 위젯 컴포넌트입니다. 비어 있으면 이름 또는 첫 번째 WidgetComponent로 자동 탐색합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (EditCondition = "bEnableInteractionHint"))
	TObjectPtr<UWidgetComponent> HintWidgetComponent = nullptr;

	// 직접 지정하지 않았을 때 소유 액터 안에서 찾을 WidgetComponent 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (EditCondition = "bEnableInteractionHint"))
	FName HintWidgetComponentName = TEXT("SpeechBubbleWidget");

	// 이름으로 찾지 못했을 때 첫 번째 WidgetComponent를 힌트 위젯으로 사용할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (EditCondition = "bEnableInteractionHint"))
	bool bAutoFindFirstHintWidgetComponent = true;

	// 힌트 위젯에 전달할 짧은 텍스트입니다. 물음표, 느낌표, 조사 필요 문구 등에 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (EditCondition = "bEnableInteractionHint"))
	FText HintText;

	// 힌트 표시 시간입니다. 0 이하이면 기본 힌트 시간으로 처리합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (EditCondition = "bEnableInteractionHint", ClampMin = "-1.0"))
	double HintDuration = 3.0;

	// 대화 시작이 비활성화된 동안에도 퍼즐 힌트 Bubble은 보여줄지 명시적으로 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (EditCondition = "bEnableInteractionHint"))
	bool bShowHintWhenInteractionDisabled = true;

	// 힌트 위젯 블루프린트에서 표시를 담당하는 함수 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (AdvancedDisplay, EditCondition = "bEnableInteractionHint"))
	FName HintShowFunctionName = TEXT("ShowBubble");

	// 힌트 위젯 블루프린트에서 숨김을 담당하는 함수 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (AdvancedDisplay, EditCondition = "bEnableInteractionHint"))
	FName HintHideFunctionName = TEXT("HideBubble");

	// BeginPlay 때 힌트 위젯을 먼저 숨겨 처음부터 떠 있지 않게 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (EditCondition = "bEnableInteractionHint"))
	bool bHideHintOnBeginPlay = true;

	// true면 WidgetComponent 자체의 표시 상태도 함께 제어합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Hint", meta = (EditCondition = "bEnableInteractionHint"))
	bool bControlHintWidgetComponentVisibility = true;

	// 켜져 있으면 ResetTrigger가 호출되기 전까지 한 번만 실행됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bTriggerOnce = true;

	// 켜져 있으면 대화 시작자가 우산 컴포넌트를 가지고 있고 실제 우산도 소유해야 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bRequireUmbrella = true;

	// 켜져 있으면 우산이 펼쳐졌거나 뒤집힌 상태여야 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules", meta = (EditCondition = "bRequireUmbrella"))
	bool bRequireOpenUmbrella = true;

	// 켜져 있으면 대화 대상의 커버 타겟 기준점 반경이 플레이어의 대화용 우산 커버 박스와 일정 시간 닿아야 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Umbrella Cover", meta = (EditCondition = "bRequireUmbrella"))
	bool bRequireUmbrellaCoverHold = false;

	// 우산으로 씌워준 상태를 유지해야 하는 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Umbrella Cover", meta = (EditCondition = "bRequireUmbrellaCoverHold", ClampMin = "0.0"))
	float RequiredCoverDuration = 2.5f;

	// 커버 판정에 사용할 타겟 기준점입니다. 비어 있으면 이 액터에서 UOUDialogueCoverTargetComponent를 자동으로 찾습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Umbrella Cover", meta = (EditCondition = "bRequireUmbrellaCoverHold"))
	TObjectPtr<UUOUDialogueCoverTargetComponent> CoverTarget = nullptr;

	// 켜져 있으면 CoverTarget이 비어 있을 때 이 액터 안의 커버 타겟 기준점들을 자동으로 검사합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Umbrella Cover", meta = (EditCondition = "bRequireUmbrellaCoverHold"))
	bool bAutoFindCoverTargets = true;

	// PIE에서 우산 커버 판정과 대화 시작 상태를 화면에 띄웁니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Debug")
	bool bShowUmbrellaCoverDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Debug")
	bool bShowTriggerShapeInGame = false;

	// 켜져 있으면 대화 줌 카메라가 켜져 있는 동안 플레이어 이동 입력을 막습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Control")
	bool bLockMovementDuringDialogueFocus = true;

	// 켜져 있으면 대화 카메라가 켜져 있는 동안 카메라 가림 처리를 함께 켭니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Control")
	bool bEnableCameraOcclusionDuringDialogueFocus = true;

	// 대화 UI가 닫힌 뒤 카메라가 원래 시점으로 돌아가기 전 기다리는 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Control", meta = (ClampMin = "0.0"))
	float DialogueFocusEndDelay = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	bool bHasTriggered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	float CurrentCoverHoldTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	TObjectPtr<AActor> CurrentInstigatorActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	bool bIsCurrentlyCoveredByUmbrella = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	bool bHintVisible = false;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void HandleDialogueEnded();

	void HandleTrackedActorEnter(AActor* OtherActor, UPrimitiveComponent* OtherComp);
	UUOUDialogueSourceComponent* ResolveDialogueSource() const;
	bool PassesInstigatorRules(AActor* InstigatorActor) const;
	UUOUUmbrellaComponent* FindUmbrellaComponent(AActor* InstigatorActor) const;
	bool CanTrackOverlapActor(AActor* InstigatorActor) const;
	bool RegisterActorOverlap(AActor* InstigatorActor);
	bool UnregisterActorOverlap(AActor* InstigatorActor);
	bool IsOwnerCoveredByDialogueCover(AActor* InstigatorActor, FString* OutDebugDetails = nullptr) const;
	void ResolveCoverTargets(TArray<UUOUDialogueCoverTargetComponent*>& OutCoverTargets) const;
	void ResolveUmbrellaCoverVolumes(AActor* InstigatorActor, TArray<UUOUUmbrellaCoverVolumeComponent*>& OutCoverVolumes) const;
	UUOUUISubsystem* ResolveUISubsystem() const;
	UUOUCameraControllerComponent* FindCameraControllerComponent(AActor* InstigatorActor) const;
	void StartDialogueCameraFocus(AActor* InstigatorActor, AActor* SpeakerActor);
	void StopDialogueCameraFocus();
	void LockMovementForDialogue(AActor* InstigatorActor);
	void UnlockMovementForDialogue();
	void ClearCoverProgress();
	void SetHintWidgetComponentVisible(bool bNewVisible) const;
	bool CallHintWidgetShowFunction(
		UUserWidget* UserWidget,
		const FText& DisplayHintText,
		double DisplayDuration,
		FName PresentationStyle) const;
	bool CallHintWidgetHideFunction(UUserWidget* UserWidget) const;
	UUserWidget* GetHintUserWidget();
	void ShowCoverDebugMessage(const FString& Message, const FColor& Color, float Duration = 1.5f) const;
	void ShowCoverDebugStatus(const FString& Message, const FColor& Color) const;
	void ShowProximityDebugStatus(const FString& Message, const FColor& Color) const;

	UPROPERTY(Transient)
	TObjectPtr<UUOUCameraControllerComponent> ActiveDialogueCameraController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUUISubsystem> BoundUISubsystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUPlayerInteractionExecutorComponent> LockedInputExecutorComponent = nullptr;

	TMap<TWeakObjectPtr<AActor>, int32> ActiveOverlapCounts;

	FTimerHandle DialogueFocusEndDelayTimerHandle;

	FString LastHintDebugStatus = TEXT("None");

	bool bDialogueMovementLocked = false;
	bool bSavedCameraOcclusionEnabled = false;
	bool bHasSavedCameraOcclusionEnabled = false;
};
