// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Interaction/UOUInteractable.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "World/Light/UOULightExposureTypes.h"
#include "UOUHeatWireComponent.generated.h"

class USplineComponent;
class UUOULightExposureReceiverComponent;

UENUM(BlueprintType)
enum class EUOUHeatWireState : uint8
{
	Unlit UMETA(DisplayName = "Cold", ToolTip = "아직 가열되지 않은 상태입니다."),
	Burning UMETA(DisplayName = "Heating", ToolTip = "열선의 열이 진행 중인 상태입니다."),
	Paused UMETA(DisplayName = "Paused", ToolTip = "열 전달이 일시 정지된 상태입니다."),
	Extinguished UMETA(DisplayName = "Stopped", ToolTip = "열 전달이 중지된 상태입니다."),
	BurnedOut UMETA(DisplayName = "Complete", ToolTip = "열이 끝점까지 도달한 상태입니다.")
};

USTRUCT(BlueprintType)
struct FUOUHeatWireWetSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Heat Wire|Wetness", meta = (ToolTip = "디버그와 블루프린트에서 구간을 구분하기 위한 이름입니다."))
	FName SectionName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Heat Wire|Wetness", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "열선 전체 길이 기준 이 젖음 구간의 시작 진행률입니다."))
	float StartProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Heat Wire|Wetness", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "열선 전체 길이 기준 이 젖음 구간의 끝 진행률입니다."))
	float EndProgress = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Heat Wire|Wetness", meta = (ClampMin = "0.0", ToolTip = "현재 젖어 있는 정도입니다. Blocking Wetness 이상이면 열이 이 구간을 지나가지 못합니다."))
	float Wetness = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Heat Wire|Wetness", meta = (ClampMin = "0.0", ToolTip = "이 구간에 누적될 수 있는 최대 젖음 정도입니다."))
	float MaxWetness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Heat Wire|Wetness", meta = (ClampMin = "0.0", ToolTip = "RainArea가 이 구간 중심에 비가 닿는지 검사할 때 참고하는 반경입니다."))
	float RainCoverageRadius = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Heat Wire|Wetness", meta = (ToolTip = "켜져 있으면 RainArea와 블루프린트 비 입력으로 젖을 수 있습니다."))
	bool bReceivesRain = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Heat Wire|Wetness", meta = (DisplayName = "Blocks Heat", ToolTip = "켜져 있으면 충분히 젖었을 때 열 전달을 막습니다."))
	bool bBlocksFire = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Runtime", meta = (ToolTip = "마지막으로 이 구간이 비 입력을 받은 월드 시간입니다."))
	float LastRainWorldTime = -BIG_NUMBER;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUHeatWireIgnitedSignature, AActor*, Igniter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOUHeatWireSimpleSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUHeatWireStateChangedSignature, EUOUHeatWireState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUOUHeatWireProgressChangedSignature, float, NewProgress, float, RemainingTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUOUHeatWireWetSectionChangedSignature, int32, SectionIndex, float, NewWetness);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUHeatWireBlockedSectionChangedSignature, int32, BlockedSectionIndex);

// 열선의 가열, 진행, 완료 상태를 퍼즐 조건으로 노출하는 컴포넌트입니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Heat Wire", ToolTip = "가열되면 일정 시간 동안 진행되고 완료 시 퍼즐 조건을 만족시키는 열선 컴포넌트입니다."))
class UNDERONEUMBRELLA_API UUOUHeatWireComponent
	: public UUOUPuzzleConditionSourceComponent
	, public IUOUPuzzleResultReceiver
	, public IUOUInteractable
{
	GENERATED_BODY()

public:
	UUOUHeatWireComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Heat Wire")
	FUOUHeatWireIgnitedSignature OnHeatWireIgnited;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Heat Wire")
	FUOUHeatWireSimpleSignature OnHeatWireExtinguished;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Heat Wire")
	FUOUHeatWireSimpleSignature OnHeatWireBurnedOut;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Heat Wire")
	FUOUHeatWireSimpleSignature OnHeatWireReset;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Heat Wire")
	FUOUHeatWireStateChangedSignature OnHeatWireStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Heat Wire")
	FUOUHeatWireProgressChangedSignature OnHeatWireProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Heat Wire|Wetness")
	FUOUHeatWireWetSectionChangedSignature OnWetSectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Heat Wire|Wetness")
	FUOUHeatWireBlockedSectionChangedSignature OnBlockedSectionChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Heat Travel Duration", ClampMin = "0.0", UIMin = "0.0", ToolTip = "열이 열선 시작점부터 끝점까지 전달되는 데 걸리는 시간입니다. 0이면 가열 즉시 완료됩니다."))
	float BurnDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Heat Travel Rate Multiplier", ClampMin = "0.0", ToolTip = "열 전달 속도 배율입니다. 0이면 진행하지 않습니다."))
	float BurnRateMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Initial Heat Progress", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "게임 시작 또는 Reset 시 적용할 초기 진행률입니다. 1이면 이미 완료된 상태로 시작합니다."))
	float InitialProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Auto Heat On Begin Play", ToolTip = "게임 시작 시 자동으로 열선을 가열합니다."))
	bool bAutoIgniteOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Allow Interaction Heat", ToolTip = "상호작용 입력으로 열선을 가열할 수 있으면 true입니다."))
	bool bAllowInteractionIgnite = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Path", meta = (ToolTip = "열 진행 위치와 비 샘플 위치 계산에 사용할 SplineComponent입니다. 비어 있으면 소유 액터의 SplineComponent를 자동으로 찾습니다."))
	TObjectPtr<USplineComponent> HeatWirePathComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Path", meta = (ToolTip = "Heat Wire Path가 비어 있으면 소유 액터의 SplineComponent를 자동으로 찾습니다."))
	bool bAutoFindHeatWirePath = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Path", meta = (ClampMin = "1.0", ToolTip = "Spline이 없을 때 액터 로컬 X축으로 가정하는 열선 길이입니다."))
	float FallbackHeatWireWorldLength = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Path", meta = (ClampMin = "4", ToolTip = "월드 위치를 Spline 진행률로 변환할 때 사용할 샘플 수입니다. 높을수록 정확하지만 계산 비용이 늘어납니다."))
	int32 SplineWorldSampleCount = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Wetness", meta = (ToolTip = "비어 있는 Wet Sections를 BeginPlay 때 균등 구간으로 자동 생성합니다."))
	bool bBuildDefaultWetSectionsOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Wetness", meta = (ClampMin = "1", ToolTip = "자동 생성할 기본 젖음 구간 수입니다."))
	int32 DefaultWetSectionCount = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Wetness", meta = (ClampMin = "0.0", ToolTip = "구간 젖음이 이 값 이상이면 열이 통과하지 못합니다."))
	float BlockingWetness = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Wetness", meta = (ToolTip = "켜져 있으면 비 입력을 받은 차단 구간은 즉시 Blocking Wetness 이상으로 젖어 열 진행을 막습니다."))
	bool bRainImmediatelyBlocksWetSections = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Wetness", meta = (ClampMin = "0.0", ToolTip = "비가 멈춘 뒤 바로 마르지 않도록 주는 유예 시간입니다."))
	float DryingGraceTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Heat Wire|Wetness", meta = (ClampMin = "0.0", ToolTip = "비를 맞지 않는 구간의 젖음이 초당 줄어드는 양입니다."))
	float DryRate = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Heat Wire|Wetness", meta = (ToolTip = "열선의 젖음 판정을 나누는 구간입니다. 여러 구간이 젖어 있으면 열은 가장 먼저 만나는 젖은 구간 앞에서 멈춥니다."))
	TArray<FUOUHeatWireWetSection> WetSections;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light", meta = (DisplayName = "Heat From Light Exposure", ToolTip = "빛 노출을 받으면 자동으로 열선을 가열합니다."))
	bool bIgniteFromLightExposure = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light", meta = (ToolTip = "명시적인 참조가 없으면 소유 액터의 UOU Light Exposure Receiver를 자동으로 찾습니다."))
	bool bAutoFindLightReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light", meta = (UseComponentPicker, AllowedClasses = "/Script/UnderOneUmBrella.UOULightExposureReceiverComponent", DisplayName = "Light Receiver", ToolTip = "열선을 가열할 때 사용할 빛 수신 컴포넌트입니다."))
	FComponentReference LightReceiverReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light", meta = (ClampMin = "0.0", ToolTip = "열선을 가열하기 위해 필요한 최소 빛 세기입니다."))
	float LightIgnitionIntensityThreshold = 0.1f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (ToolTip = "현재 열선 상태입니다."))
	EUOUHeatWireState HeatWireState = EUOUHeatWireState::Unlit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (DisplayName = "Heat Progress", ToolTip = "0부터 1까지의 현재 열 전달 진행률입니다."))
	float BurnProgress = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (ToolTip = "현재 열 전달을 막고 있는 Wet Sections 인덱스입니다. 없으면 -1입니다."))
	int32 BlockedSectionIndex = INDEX_NONE;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (DisplayName = "Last Heat Source", ToolTip = "마지막으로 열선을 가열한 액터입니다."))
	TObjectPtr<AActor> LastIgniter = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (ToolTip = "현재 연결된 빛 수신 컴포넌트입니다."))
	TObjectPtr<UUOULightExposureReceiverComponent> LightReceiver = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Start Heat Wire", ToolTip = "열선을 가열합니다."))
	bool Ignite(AActor* Igniter);

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Stop Heat Wire", ToolTip = "진행 중인 열 전달을 중지합니다. 진행률은 유지됩니다."))
	bool Extinguish();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire", meta = (ToolTip = "열 전달을 일시 정지합니다."))
	bool PauseHeatWire();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire", meta = (ToolTip = "일시 정지된 열 전달을 다시 진행합니다."))
	bool ResumeHeatWire();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire", meta = (ToolTip = "열선을 초기 진행률로 되돌립니다."))
	void ResetHeatWire();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire", meta = (ToolTip = "열선을 즉시 완료 상태로 만듭니다."))
	void FinishHeatWireImmediately();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Set Heat Progress", ToolTip = "열 전달 진행률을 직접 설정합니다. 1 이상이면 완료됩니다."))
	void SetBurnProgress(float NewProgress);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Puzzle|Heat Wire|Wetness", meta = (ToolTip = "Default Wet Section Count 값에 맞춰 Wet Sections를 균등 구간으로 다시 만듭니다."))
	void RebuildDefaultWetSections();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire|Wetness", meta = (ToolTip = "모든 구간의 젖음 정도를 0으로 만듭니다."))
	void ClearWetness();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire|Rain", meta = (ToolTip = "열선 진행률 위치에 비 입력을 적용합니다."))
	void ApplyRainAtProgress(float Progress, float WetnessAmount, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire|Rain", meta = (ToolTip = "열선 진행률 범위에 비 입력을 적용합니다."))
	void ApplyRainToProgressRange(float StartProgress, float EndProgress, float WetnessAmount, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire|Rain", meta = (ToolTip = "월드 위치와 가장 가까운 열선 구간에 비 입력을 적용합니다."))
	void ApplyRainAtWorldLocation(FVector WorldLocation, float WetnessAmount, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Heat Wire|Rain", meta = (ToolTip = "지정한 Wet Sections 인덱스에 비 입력을 적용합니다. RainArea에서 호출합니다."))
	void ApplyRainToWetSection(int32 SectionIndex, float WetnessAmount, AActor* InstigatorActor);

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Is Heat Moving", ToolTip = "열선의 열이 현재 진행 중이면 true를 반환합니다."))
	bool IsBurning() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Has Heat Reached End", ToolTip = "열이 끝점까지 도달했으면 true를 반환합니다."))
	bool IsBurnedOut() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire", meta = (DisplayName = "Get Remaining Heat Time", ToolTip = "현재 속도 기준 남은 시간을 반환합니다."))
	float GetRemainingBurnTime() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire", meta = (ToolTip = "젖은 구간 앞에서 열 전달이 대기 중이면 true를 반환합니다."))
	bool IsBlockedByWetness() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire|Path", meta = (DisplayName = "Get Heat Front World Location", ToolTip = "현재 열 진행 지점의 월드 위치를 반환합니다."))
	FVector GetFireWorldLocation() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire|Path", meta = (ToolTip = "진행률에 해당하는 열선 월드 위치를 반환합니다."))
	FVector GetWorldLocationAtProgress(float Progress) const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire|Path", meta = (ToolTip = "월드 위치에 가장 가까운 열선 진행률을 반환합니다."))
	float GetProgressAtWorldLocation(FVector WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire|Rain", meta = (ToolTip = "RainArea에서 이 열선에 비 입력을 줄 수 있으면 true를 반환합니다."))
	bool CanReceiveRainInput() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire|Rain", meta = (ToolTip = "Wet Sections 개수를 반환합니다."))
	int32 GetWetSectionCount() const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire|Rain", meta = (ToolTip = "해당 Wet Sections 인덱스가 비를 받을 수 있으면 true를 반환합니다."))
	bool CanWetSectionReceiveRain(int32 SectionIndex) const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire|Rain", meta = (ToolTip = "해당 Wet Sections 인덱스의 월드 샘플 위치를 반환합니다."))
	bool GetWetSectionWorldLocation(int32 SectionIndex, FVector& OutWorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Heat Wire|Rain", meta = (ToolTip = "해당 Wet Sections 인덱스의 비 샘플 반경을 반환합니다."))
	float GetWetSectionRainCoverageRadius(int32 SectionIndex) const;

protected:
	UFUNCTION()
	void HandleLightExposureReceived(const FUOULightExposureData& ExposureData);

	void ValidateSettings();
	void ValidateWetSections();
	void BuildDefaultWetSections();
	void ResolveHeatWirePath();
	void ResolveLightReceiver();
	void SubscribeLightReceiver();
	void UnsubscribeLightReceiver();
	bool HasLightReceiverReference() const;
	AActor* ResolveExposureSourceActor(const FUOULightExposureData& ExposureData) const;
	void UpdateWetSectionDrying(float DeltaTime);
	void UpdateBlockedState();
	int32 FindBlockingSectionAtProgress(float Progress) const;
	int32 FindFirstBlockingSectionInRange(float StartProgress, float EndProgress, float& OutBlockProgress) const;
	bool IsWetSectionBlocking(const FUOUHeatWireWetSection& Section) const;
	void SetHeatWireState(EUOUHeatWireState NewState);
	void SetBlockedSectionIndex(int32 NewBlockedSectionIndex);
	void SetWetSectionWetness(int32 SectionIndex, float NewWetness, bool bBroadcastChange);
	void SetBurnProgressInternal(float NewProgress, bool bBroadcastChange);
	void AdvanceBurn(float DeltaTime);
	void CompleteBurn();
	void RefreshTickState();
};
