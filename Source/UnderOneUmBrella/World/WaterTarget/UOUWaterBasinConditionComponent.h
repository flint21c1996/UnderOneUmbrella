// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/WaterTarget/UOUPuzzleConditionActivatable.h"
#include "UOUWaterBasinConditionComponent.generated.h"

class UUOUWaterBasinPlatformComponent;
class UUOUWaterBasinTargetComponent;

// WaterBasinConditionComponent가 조건 판정에 사용할 값을 정합니다.
// 수면 높이뿐 아니라 물 깊이, 채움 비율, 부피, 플랫폼 위치처럼 WaterBasin 상태에서 파생되는 값을 다룹니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinConditionValueSource : uint8
{
	WaterSurfaceWorldZ UMETA(DisplayName = "Water Surface World Z", ToolTip = "WaterTile의 현재 수면 월드 Z를 기준으로 조건을 판정합니다."),
	WaterDepth UMETA(DisplayName = "Water Depth", ToolTip = "WaterTile의 현재 물 깊이를 타일 단위 값으로 비교합니다."),
	WaterDepthWorld UMETA(DisplayName = "Water Depth World", ToolTip = "WaterTile의 현재 물 깊이를 월드 단위(cm)로 비교합니다."),
	WaterFillRatio UMETA(DisplayName = "Water Fill Ratio", ToolTip = "WaterTile의 현재 채움 비율을 비교합니다. 가득 참 조건은 Threshold를 1로 두면 됩니다."),
	WaterVolume UMETA(DisplayName = "Water Volume", ToolTip = "WaterTile의 현재 물 부피를 비교합니다."),
	PlatformWorldZ UMETA(DisplayName = "Platform World Z", ToolTip = "플랫폼의 현재 목표 월드 Z를 비교합니다.")
};

// 현재 값을 기준값과 어떤 방식으로 비교할지 정합니다.
UENUM(BlueprintType)
enum class EUOUWaterBasinConditionCompareMode : uint8
{
	GreaterOrEqual UMETA(DisplayName = "Greater Or Equal", ToolTip = "현재 값이 Threshold 이상이면 조건을 만족합니다."),
	Greater UMETA(DisplayName = "Greater", ToolTip = "현재 값이 Threshold보다 크면 조건을 만족합니다."),
	LessOrEqual UMETA(DisplayName = "Less Or Equal", ToolTip = "현재 값이 Threshold 이하이면 조건을 만족합니다."),
	Less UMETA(DisplayName = "Less", ToolTip = "현재 값이 Threshold보다 작으면 조건을 만족합니다."),
	Equal UMETA(DisplayName = "Equal", ToolTip = "현재 값이 Threshold와 Tolerance 범위 안에서 같으면 조건을 만족합니다."),
	NotEqual UMETA(DisplayName = "Not Equal", ToolTip = "현재 값이 Threshold와 Tolerance 범위 밖이면 조건을 만족합니다."),
	BetweenInclusive UMETA(DisplayName = "Between Inclusive", ToolTip = "현재 값이 Threshold와 Upper Threshold 사이에 있으면 조건을 만족합니다.")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUWaterBasinConditionEvent, const FUOUPuzzleConditionContext&, Context);

// WaterTile의 물 상태 또는 수면을 따라 움직이는 Platform 상태를 감시하는 조건 컴포넌트입니다.
// 조건 판정만 담당하고, 실제 퍼즐 반응은 IUOUPuzzleConditionActivatable을 구현한 Actor/Component가 담당합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Water Basin Condition"))
class UNDERONEUMBRELLA_API UUOUWaterBasinConditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUWaterBasinConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 여러 조건 컴포넌트를 구분하기 위한 이름입니다. Context에도 그대로 전달됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition", meta = (ToolTip = "여러 조건 컴포넌트를 구분하기 위한 이름입니다. ReactionComponent는 이 값을 보고 어떤 조건에서 호출됐는지 구분할 수 있습니다."))
	FName ConditionId = TEXT("WaterCondition");

	// 조건을 읽을 WaterTile Actor입니다. 비워두면 PlatformComponent의 TargetActor를 우선 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Target", meta = (ToolTip = "조건을 읽을 WaterTile Actor입니다. 해당 Actor에는 UOUWaterBasinTargetComponent가 있어야 합니다."))
	TObjectPtr<AActor> TargetWaterTileActor = nullptr;

	// TargetWaterTileActor가 비어 있을 때 같은 Actor의 WaterBasinPlatformComponent에서 WaterTile을 자동으로 찾습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Target", meta = (ToolTip = "TargetWaterTileActor가 비어 있으면 같은 Actor의 WaterBasinPlatformComponent가 참조하는 WaterTile을 자동으로 사용합니다."))
	bool bAutoResolveWaterTileFromPlatform = true;

	// 조건 값으로 어떤 물/플랫폼 값을 사용할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Rule", meta = (ToolTip = "조건 판정에 사용할 값을 정합니다. 예를 들어 수면 월드 Z, 채움 비율, 플랫폼 높이 등을 선택할 수 있습니다."))
	EUOUWaterBasinConditionValueSource ValueSource = EUOUWaterBasinConditionValueSource::WaterFillRatio;

	// 현재 값을 기준값과 어떻게 비교할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Rule", meta = (ToolTip = "현재 값을 Threshold와 비교하는 방식입니다."))
	EUOUWaterBasinConditionCompareMode CompareMode = EUOUWaterBasinConditionCompareMode::GreaterOrEqual;

	// 조건 판정 기준 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Rule", meta = (ToolTip = "조건 판정 기준 값입니다. 가득 참 조건은 ValueSource를 WaterFillRatio로 두고 Threshold를 1로 설정하면 됩니다."))
	float ThresholdValue = 1.0f;

	// BetweenInclusive 비교에서 사용하는 상한 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Rule", meta = (ToolTip = "BetweenInclusive 비교에서 사용하는 상한 값입니다."))
	float UpperThresholdValue = 1.0f;

	// Equal, NotEqual 판정에서 허용할 오차입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Rule", meta = (ClampMin = "0.0", ToolTip = "Equal, NotEqual 판정에서 허용할 오차입니다."))
	float Tolerance = 0.001f;

	// BeginPlay 시점에 조건을 한 번 평가합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Trigger", meta = (ToolTip = "게임 시작 시 현재 물 상태로 조건을 한 번 평가합니다."))
	bool bEvaluateOnBeginPlay = true;

	// 매 프레임 조건을 다시 평가합니다. PlatformWorldZ처럼 물 이벤트 없이 값이 변하는 조건에 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Trigger", meta = (ToolTip = "매 프레임 조건을 다시 평가합니다. 보간 이동 중인 플랫폼 위치를 기준으로 조건을 잡을 때 사용합니다."))
	bool bEvaluateEveryTick = false;

	// true면 조건이 처음 만족되는 순간에만 Activate를 호출합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Trigger", meta = (ToolTip = "켜져 있으면 조건이 처음 만족되는 순간에만 Activate를 호출합니다."))
	bool bTriggerOnce = false;

	// 같은 Actor에 붙은 컴포넌트 중 IUOUPuzzleConditionActivatable 구현체를 자동으로 호출합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Reaction", meta = (ToolTip = "같은 Actor에 붙은 ReactionComponent들을 자동으로 찾아 호출합니다."))
	bool bNotifyOwnerComponents = true;

	// Owner Actor가 IUOUPuzzleConditionActivatable을 직접 구현했을 때 Actor에도 알림을 보냅니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Reaction", meta = (ToolTip = "Owner Actor가 인터페이스를 직접 구현한 경우 Actor에도 조건 알림을 보냅니다."))
	bool bNotifyOwnerActor = false;

	// 조건 변화 시 추가로 알림을 보낼 외부 Actor 목록입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Reaction", meta = (ToolTip = "조건 변화 시 추가로 알림을 보낼 외부 Actor 목록입니다. Actor가 인터페이스를 구현했거나, Actor 내부 컴포넌트가 인터페이스를 구현하면 호출됩니다."))
	TArray<TObjectPtr<AActor>> ExplicitReactionActors;

	// ExplicitReactionActors 내부의 컴포넌트까지 인터페이스 구현 여부를 검사합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Reaction", meta = (ToolTip = "ExplicitReactionActors 안에 붙은 컴포넌트까지 Reaction 인터페이스 구현 여부를 검사합니다."))
	bool bNotifyComponentsOnExplicitActors = true;

	// 조건 변화 시 추가로 알림을 보낼 외부 컴포넌트 목록입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Condition|Reaction", meta = (ToolTip = "조건 변화 시 직접 호출할 외부 ReactionComponent 목록입니다."))
	TArray<TObjectPtr<UActorComponent>> ExplicitReactionComponents;

	// 조건 만족 상태가 바뀔 때마다 호출되는 블루프린트 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Water Basin Condition|Event")
	FUOUWaterBasinConditionEvent OnConditionChanged;

	// 조건이 false에서 true로 바뀌는 순간 호출되는 블루프린트 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Water Basin Condition|Event")
	FUOUWaterBasinConditionEvent OnConditionSatisfied;

	// 조건이 true에서 false로 바뀌는 순간 호출되는 블루프린트 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Water Basin Condition|Event")
	FUOUWaterBasinConditionEvent OnConditionUnsatisfied;

	// 현재 상태를 즉시 다시 평가합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Basin Condition")
	void EvaluateCondition(bool bForceNotify = false);

	// 현재 조건 만족 여부를 반환합니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin Condition")
	bool IsConditionSatisfied() const;

	// 마지막으로 계산된 조건 Context를 반환합니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Basin Condition")
	FUOUPuzzleConditionContext GetLastContext() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UUOUWaterBasinTargetComponent> BoundWaterBasinTarget = nullptr;

	UPROPERTY(Transient)
	FUOUPuzzleConditionContext LastContext;

	UPROPERTY(Transient)
	bool bIsSatisfied = false;

	UPROPERTY(Transient)
	bool bHasEvaluated = false;

	UPROPERTY(Transient)
	bool bHasTriggeredOnce = false;

	UFUNCTION()
	void HandleWaterStateChanged(UUOUWaterBasinTargetComponent* ChangedTarget);

	void BindToWaterBasinTarget();
	void UnbindFromWaterBasinTarget();
	UUOUWaterBasinTargetComponent* ResolveWaterBasinTarget() const;
	UUOUWaterBasinPlatformComponent* ResolvePlatformComponent() const;
	FUOUPuzzleConditionContext BuildConditionContext(UUOUWaterBasinTargetComponent* WaterBasinTarget) const;
	float ResolveCurrentValue(const FUOUPuzzleConditionContext& Context) const;
	bool DoesValueSatisfyCondition(float CurrentValue) const;
	void BroadcastConditionResult(const FUOUPuzzleConditionContext& Context, bool bWasSatisfied);
	void NotifyReactionTargets(const FUOUPuzzleConditionContext& Context, bool bWasSatisfied);
	void NotifyActivatableObject(UObject* Object, const FUOUPuzzleConditionContext& Context, bool bWasSatisfied, TSet<UObject*>& NotifiedObjects);
};
