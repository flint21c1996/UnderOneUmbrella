// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUWaterBasinReactionComponentBase.generated.h"

class AActor;
class UUOUWaterBasinPlatformComponent;
class UUOUWaterBasinTargetComponent;

// 물 상태를 받은 ReactionComponent가 자기 조건을 판단할 때 사용하는 값 목록입니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinReactionValueSource : uint8
{
	WaterSurfaceWorldZ UMETA(DisplayName = "Water Surface World Z", ToolTip = "WaterTile의 현재 수면 월드 Z를 기준으로 조건을 판정합니다."),
	WaterDepth UMETA(DisplayName = "Water Depth", ToolTip = "WaterTile의 현재 물 깊이를 타일 단위 값으로 비교합니다."),
	WaterDepthWorld UMETA(DisplayName = "Water Depth World", ToolTip = "WaterTile의 현재 물 깊이를 월드 단위(cm)로 비교합니다."),
	WaterFillRatio UMETA(DisplayName = "Water Fill Ratio", ToolTip = "WaterTile의 현재 채움 비율을 비교합니다. 가득 참 조건은 Threshold를 1로 두면 됩니다."),
	WaterVolume UMETA(DisplayName = "Water Volume", ToolTip = "WaterTile의 현재 물 부피를 비교합니다."),
	PlatformWorldZ UMETA(DisplayName = "Platform World Z", ToolTip = "플랫폼의 현재 월드 Z를 비교합니다."),
	RotationAngleDegrees UMETA(DisplayName = "Rotation Angle Degrees", ToolTip = "RotationReactionComponent가 실제로 적용한 현재 회전 각도를 0~360도 기준으로 비교합니다."),
	SignedRotationAngleDegrees UMETA(DisplayName = "Signed Rotation Angle Degrees", ToolTip = "RotationReactionComponent가 실제로 적용한 현재 회전 각도를 -180~180도 기준으로 비교합니다. 회전 방향 부호가 필요한 조건에 사용합니다.")
};

// 현재 값을 기준값과 어떤 방식으로 비교할지 정합니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinReactionCompareMode : uint8
{
	GreaterOrEqual UMETA(DisplayName = "Greater Or Equal", ToolTip = "현재 값이 Threshold 이상이면 조건을 만족합니다."),
	Greater UMETA(DisplayName = "Greater", ToolTip = "현재 값이 Threshold보다 크면 조건을 만족합니다."),
	LessOrEqual UMETA(DisplayName = "Less Or Equal", ToolTip = "현재 값이 Threshold 이하이면 조건을 만족합니다."),
	Less UMETA(DisplayName = "Less", ToolTip = "현재 값이 Threshold보다 작으면 조건을 만족합니다."),
	Equal UMETA(DisplayName = "Equal", ToolTip = "현재 값이 Threshold와 Tolerance 범위 안에서 같으면 조건을 만족합니다."),
	NotEqual UMETA(DisplayName = "Not Equal", ToolTip = "현재 값이 Threshold와 Tolerance 범위 밖이면 조건을 만족합니다."),
	BetweenInclusive UMETA(DisplayName = "Between Inclusive", ToolTip = "현재 값이 Threshold와 Upper Threshold 사이에 있으면 조건을 만족합니다.")
};

// WaterTile의 현재 물 상태와 Platform 상태를 ReactionComponent에 전달하기 위한 데이터입니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUWaterBasinReactionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction")
	TObjectPtr<AActor> ReactionOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction")
	TObjectPtr<UActorComponent> ReactionComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Water")
	TObjectPtr<AActor> WaterTileActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Water")
	TObjectPtr<UUOUWaterBasinTargetComponent> WaterBasinTarget = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Water")
	float WaterSurfaceWorldZ = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Water")
	float WaterDepth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Water")
	float WaterDepthWorld = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Water")
	float WaterFillRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Water")
	float WaterVolume = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Platform")
	TObjectPtr<UUOUWaterBasinPlatformComponent> PlatformComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Platform")
	float PlatformWorldZ = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Rotation")
	float RotationAngleDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Rotation")
	float SignedRotationAngleDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Condition")
	bool bIsSatisfied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Condition")
	float CurrentValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Water Basin Reaction|Condition")
	float ThresholdValue = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUWaterBasinReactionEvent, const FUOUWaterBasinReactionContext&, Context);

// 물 상태에 반응하는 플랫폼 기능 컴포넌트의 공통 부모입니다.
// WaterTile 자동 탐색, 이벤트 바인딩, Context 생성, 기본 조건 판정을 제공하고
// 파생 ReactionComponent는 조건 만족/해제 시 실제 행동만 구현하면 됩니다.
UCLASS(Abstract, ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Water Basin Reaction Base"))
class UNDERONEUMBRELLA_API UUOUWaterBasinReactionComponentBase : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUWaterBasinReactionComponentBase();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// 물 상태를 읽을 WaterTile Actor입니다. 직접 지정하면 수동 override로 사용하고, 비워두면 자동 탐색으로 채웁니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Target", meta = (ToolTip = "물 상태를 읽을 WaterTile Actor입니다. 직접 지정하면 수동 override로 사용하고, 비워두면 자동 탐색으로 채웁니다."))
	TObjectPtr<AActor> TargetWaterTileActor = nullptr;

	// TargetWaterTileActor가 비어 있을 때 Attach Parent 체인 또는 PlatformComponent에서 WaterTile을 자동으로 찾아 설정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Target", meta = (ToolTip = "TargetWaterTileActor가 비어 있으면 Attach Parent 체인 또는 같은 Actor의 WaterBasinPlatformComponent에서 WaterTile을 자동으로 찾아 설정합니다."))
	bool bAutoResolveTargetWaterTileActor = true;

	// BeginPlay 시점에 현재 물 상태로 한 번 조건을 평가합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Trigger", meta = (ToolTip = "게임 시작 시 현재 물 상태로 조건을 한 번 평가합니다."))
	bool bEvaluateOnBeginPlay = true;

	// 매 프레임 조건을 다시 평가합니다. PlatformWorldZ처럼 물 이벤트 없이 값이 변하는 조건에 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Trigger", meta = (ToolTip = "매 프레임 조건을 다시 평가합니다. 보간 이동 중인 플랫폼 위치를 기준으로 조건을 잡을 때 사용합니다."))
	bool bEvaluateEveryTick = false;

	// true면 조건이 처음 만족되는 순간에만 OnWaterBasinReactionSatisfied를 호출합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Trigger", meta = (ToolTip = "켜져 있으면 조건이 처음 만족되는 순간에만 만족 반응을 호출합니다."))
	bool bTriggerOnce = false;

	// Reaction 조건 결과를 PuzzleConditionGroup에 노출할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Trigger", meta = (ToolTip = "켜져 있으면 Reaction 조건 평가 결과를 PuzzleConditionSource 상태로 동기화합니다. PuzzleConditionGroup에서 이 컴포넌트를 조건으로 사용할 때 켭니다."))
	bool bExposeAsPuzzleCondition = true;

	// 조건 판정에 사용할 값을 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Condition", meta = (ToolTip = "이 ReactionComponent가 조건 판정에 사용할 값을 정합니다."))
	EUOUWaterBasinReactionValueSource ValueSource = EUOUWaterBasinReactionValueSource::WaterFillRatio;

	// 현재 값을 기준값과 어떻게 비교할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Condition", meta = (ToolTip = "현재 값을 Threshold와 비교하는 방식입니다."))
	EUOUWaterBasinReactionCompareMode CompareMode = EUOUWaterBasinReactionCompareMode::GreaterOrEqual;

	// 조건 판정 기준 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Condition", meta = (ToolTip = "조건 판정 기준 값입니다. 가득 참 조건은 ValueSource를 WaterFillRatio로 두고 Threshold를 1로 설정하면 됩니다."))
	float ThresholdValue = 1.0f;

	// BetweenInclusive 비교에서 사용하는 상한 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Condition", meta = (ToolTip = "BetweenInclusive 비교에서 사용하는 상한 값입니다."))
	float UpperThresholdValue = 1.0f;

	// Equal, NotEqual, GreaterOrEqual, LessOrEqual, BetweenInclusive처럼 경계에 걸리는 판정에서 허용할 오차입니다.
	// 수면과 플랫폼 위치는 Tick 보간과 float 계산을 거치므로 기본값 0.001은 눈에 보이지 않는 작은 흔들림을 무시하기 위한 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Condition", meta = (ClampMin = "0.0", ToolTip = "경계 조건 판정에서 허용할 오차입니다. Tick 보간/float 계산으로 생기는 아주 작은 흔들림을 무시하기 위해 사용합니다."))
	float Tolerance = 0.001f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Reaction|Runtime")
	bool bIsConditionSatisfied = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Reaction|Runtime")
	bool bHasEvaluated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Reaction|Runtime")
	FUOUWaterBasinReactionContext LastContext;

	// Debug Draw 텍스트 표시 여부입니다. 플랫폼이 많을 때 화면이 복잡해지지 않도록 기본값은 꺼져 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Debug", meta = (ToolTip = "Reaction 조건 상태를 Debug Draw 텍스트로 표시합니다. 기본값은 꺼져 있으며, 테스트할 컴포넌트에서만 켜서 사용합니다."))
	bool bDrawDebugText = false;

	// Owner Actor 위치에서 어느 정도 떨어진 곳에 텍스트를 표시할지 정합니다.
	// 기본 Z 160은 플랫폼/캐릭터 메시와 겹치지 않도록 머리 위쪽에 띄우기 위한 디버그 표시용 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Debug", meta = (ToolTip = "Owner Actor 위치 기준 Debug Draw 텍스트 표시 오프셋입니다. 기본 Z 160은 메시와 겹치지 않도록 위쪽에 띄우기 위한 값입니다."))
	FVector DrawDebugOffset = FVector(0.0f, 0.0f, 160.0f);

	// 아직 조건 평가가 한 번도 없을 때 사용할 텍스트 색상입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Debug", meta = (ToolTip = "아직 조건 평가가 한 번도 없을 때 사용할 텍스트 색상입니다."))
	FColor DebugWaitingColor = FColor::Yellow;

	// 조건이 만족된 상태일 때 사용할 텍스트 색상입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Debug", meta = (ToolTip = "조건이 만족된 상태일 때 사용할 텍스트 색상입니다."))
	FColor DebugSatisfiedColor = FColor::Green;

	// 조건이 만족되지 않은 상태일 때 사용할 텍스트 색상입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Reaction|Debug", meta = (ToolTip = "조건이 만족되지 않은 상태일 때 사용할 텍스트 색상입니다."))
	FColor DebugUnsatisfiedColor = FColor::Red;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Reaction|Runtime")
	int32 SatisfiedEventCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Reaction|Runtime")
	int32 UnsatisfiedEventCount = 0;

	UPROPERTY(BlueprintAssignable, Category = "Water Basin Reaction|Event")
	FUOUWaterBasinReactionEvent OnReactionConditionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Water Basin Reaction|Event")
	FUOUWaterBasinReactionEvent OnReactionSatisfied;

	UPROPERTY(BlueprintAssignable, Category = "Water Basin Reaction|Event")
	FUOUWaterBasinReactionEvent OnReactionUnsatisfied;

	UFUNCTION(BlueprintCallable, Category = "Water Basin Reaction")
	void EvaluateReaction(bool bForceNotify = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin Reaction")
	bool IsReactionConditionSatisfied() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin Reaction")
	FUOUWaterBasinReactionContext GetLastReactionContext() const;

	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;

protected:
	// 물 상태가 갱신될 때마다 호출됩니다. 조건 만족 여부와 상관없이 디버깅/캐싱에 사용할 수 있습니다.
	UFUNCTION(BlueprintNativeEvent, Category = "Water Basin Reaction")
	void OnWaterBasinReactionStateUpdated(const FUOUWaterBasinReactionContext& Context);
	virtual void OnWaterBasinReactionStateUpdated_Implementation(const FUOUWaterBasinReactionContext& Context);

	// 조건이 false에서 true로 바뀌는 순간 호출됩니다.
	UFUNCTION(BlueprintNativeEvent, Category = "Water Basin Reaction")
	void OnWaterBasinReactionSatisfied(const FUOUWaterBasinReactionContext& Context);
	virtual void OnWaterBasinReactionSatisfied_Implementation(const FUOUWaterBasinReactionContext& Context);

	// 조건이 true에서 false로 바뀌는 순간 호출됩니다.
	UFUNCTION(BlueprintNativeEvent, Category = "Water Basin Reaction")
	void OnWaterBasinReactionUnsatisfied(const FUOUWaterBasinReactionContext& Context);
	virtual void OnWaterBasinReactionUnsatisfied_Implementation(const FUOUWaterBasinReactionContext& Context);

	UUOUWaterBasinTargetComponent* ResolveWaterBasinTarget();
	UUOUWaterBasinPlatformComponent* ResolvePlatformComponent() const;
	FUOUWaterBasinReactionContext BuildReactionContext(UUOUWaterBasinTargetComponent* WaterBasinTarget);

	// 현재 Context가 이 Reaction의 조건을 만족하는지 판정합니다.
	// 기본 구현은 ValueSource, CompareMode, ThresholdValue를 사용합니다.
	// 특수 퍼즐 조건이 필요하면 파생 컴포넌트에서 이 함수만 오버라이드하면 됩니다.
	virtual bool EvaluateReactionCondition(FUOUWaterBasinReactionContext& Context);

	float ResolveCurrentValue(const FUOUWaterBasinReactionContext& Context) const;
	bool DoesValueSatisfyCondition(float CurrentValue) const;
	void DrawReactionDebugText();

private:
	UPROPERTY(Transient)
	TObjectPtr<UUOUWaterBasinTargetComponent> BoundWaterBasinTarget = nullptr;

	UPROPERTY(Transient)
	bool bTargetWaterTileActorAutoResolved = false;

	UPROPERTY(Transient)
	bool bHasTriggeredOnce = false;

	UFUNCTION()
	void HandleWaterStateChanged(UUOUWaterBasinTargetComponent* ChangedTarget);

	void BindToWaterBasinTarget();
	void UnbindFromWaterBasinTarget();
	void NotifyReactionResult(const FUOUWaterBasinReactionContext& Context, bool bWasSatisfied, bool bForceNotify);
};
