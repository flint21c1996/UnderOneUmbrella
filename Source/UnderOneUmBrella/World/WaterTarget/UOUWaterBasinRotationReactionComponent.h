// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/WaterTarget/UOUWaterBasinReactionComponentBase.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"
#include "UOUWaterBasinRotationReactionComponent.generated.h"

class USceneComponent;
class UUOUWaterBasinTargetComponent;

UENUM(BlueprintType)
enum class EUOUWaterBasinRotationReactionMode : uint8
{
	IncrementalOnIncrease UMETA(DisplayName = "증가 시 누적 회전", ToolTip = "물 값이 증가한 만큼 회전을 누적합니다. 이미 가득 찬 상태의 물 입력은 선택적으로 증가 시도로 처리할 수 있습니다."),
	AbsoluteByValue UMETA(DisplayName = "값 기준 절대 회전", ToolTip = "현재 물 값을 회전 각도에 직접 매핑합니다. 물이 배수되면 회전도 되돌아갑니다."),
	IncrementalOnWaterInput UMETA(DisplayName = "물 입력 시 누적 회전", ToolTip = "실제 수위가 변하지 않아도 물 입력량만큼 회전을 누적합니다."),
	IncrementalOnWaterChange UMETA(DisplayName = "물 증감 시 누적 회전", ToolTip = "물 값이 증가하거나 감소할 때 회전을 누적합니다. 배수 방향은 별도로 설정할 수 있습니다.")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinRotationReactionSpace : uint8
{
	Relative UMETA(DisplayName = "상대 회전", ToolTip = "대상 컴포넌트의 로컬 회전을 기준으로 회전합니다."),
	World UMETA(DisplayName = "월드 회전", ToolTip = "월드 회전 공간을 기준으로 회전합니다.")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinRotationInputSidePolicy : uint8
{
	Ignore UMETA(DisplayName = "무시"),
	FixedPositive UMETA(DisplayName = "항상 양수"),
	FixedNegative UMETA(DisplayName = "항상 음수"),
	ByInputSide UMETA(DisplayName = "입력 좌우 기준")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinDrainRotationDirection : uint8
{
	SameAsIncrease UMETA(DisplayName = "증가와 같은 방향", ToolTip = "물이 배수될 때 물이 찰 때와 같은 방향으로 회전합니다."),
	OppositeToIncrease UMETA(DisplayName = "증가와 반대 방향", ToolTip = "물이 배수될 때 물이 찰 때와 반대 방향으로 회전합니다.")
};

// WaterBasin의 값 변화나 물 입력 이벤트를 기준으로 대상 컴포넌트를 회전시킵니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU 물통 회전 반응"))
class UNDERONEUMBRELLA_API UUOUWaterBasinRotationReactionComponent : public UUOUWaterBasinReactionComponentBase
{
	GENERATED_BODY()

public:
	UUOUWaterBasinRotationReactionComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Target", meta = (ToolTip = "회전시킬 씬 컴포넌트입니다. 비어 있으면 회전 대상 이름으로 찾고, 허용된 경우 소유자의 루트 컴포넌트를 사용합니다."))
	TObjectPtr<USceneComponent> RotationTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Target", meta = (ToolTip = "회전 대상 컴포넌트를 직접 지정하지 않았을 때 사용할 컴포넌트 이름 또는 태그입니다."))
	FName RotationTargetComponentName = TEXT("Platform");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Target", meta = (ToolTip = "명시적인 회전 대상을 찾지 못했을 때 소유자의 루트 컴포넌트를 사용할지 정합니다."))
	bool bUseOwnerRootWhenTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "물 값 변화 또는 물 입력이 회전을 구동하는 방식을 정합니다."))
	EUOUWaterBasinRotationReactionMode RotationMode = EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "회전 공간입니다. 상대 회전은 대상의 로컬 회전을, 월드 회전은 월드 회전을 기준으로 사용합니다."))
	EUOUWaterBasinRotationReactionSpace RotationSpace = EUOUWaterBasinRotationReactionSpace::Relative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "회전축입니다. 상대 회전에서는 대상의 로컬 축으로, 월드 회전에서는 월드 축으로 해석합니다."))
	FVector RotationAxis = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode != EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "물 값 1 단위당 회전할 각도입니다. 채움 비율 기준에서는 1.0이 가득 찬 상태를 의미합니다."))
	float DegreesPerValueUnit = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnIncrease || RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterChange", EditConditionHides, ToolTip = "켜져 있으면 대상 또는 연결 그룹이 이미 가득 찬 상태에서 들어온 물 입력도 값 증가 시도로 보고 회전합니다. 꺼두면 실제 물 값이 변할 때만 회전합니다."))
	bool bRotateOnFullWaterInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterChange", EditConditionHides, ToolTip = "물 값이 감소할 때 회전할 방향입니다."))
	EUOUWaterBasinDrainRotationDirection DrainRotationDirection = EUOUWaterBasinDrainRotationDirection::SameAsIncrease;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "물 입력 부피 1 단위당 회전할 각도입니다."))
	float DegreesPerInputVolume = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "플레이어 물 붓기 입력의 회전 부호 결정 방식입니다."))
	EUOUWaterBasinRotationInputSidePolicy PlayerPourInputSidePolicy = EUOUWaterBasinRotationInputSidePolicy::ByInputSide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "비 입력의 회전 부호 결정 방식입니다."))
	EUOUWaterBasinRotationInputSidePolicy RainInputSidePolicy = EUOUWaterBasinRotationInputSidePolicy::FixedPositive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "스크립트 또는 알 수 없는 입력의 회전 부호 결정 방식입니다."))
	EUOUWaterBasinRotationInputSidePolicy ScriptInputSidePolicy = EUOUWaterBasinRotationInputSidePolicy::FixedPositive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "물 입력 위치가 왼쪽인지 오른쪽인지 판단할 중심입니다. 비어 있으면 이름으로 찾고, 그래도 없으면 회전 대상을 사용합니다."))
	TObjectPtr<USceneComponent> InputSideCenterComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "직접 지정한 중심 컴포넌트가 비어 있을 때 왼쪽/오른쪽 판단 중심으로 사용할 컴포넌트 이름 또는 태그입니다."))
	FName InputSideCenterComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "왼쪽과 오른쪽을 판단할 전방 기준입니다. 비어 있으면 이름으로 찾고, 그래도 없으면 입력 좌우 중심을 사용합니다."))
	TObjectPtr<USceneComponent> InputSideForwardReferenceComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "직접 지정한 전방 기준 컴포넌트가 비어 있을 때 사용할 컴포넌트 이름 또는 태그입니다."))
	FName InputSideForwardReferenceComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (ClampMin = "0.0", EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "왼쪽/오른쪽 판정값이 이 값보다 작으면 중심선에 가까운 애매한 입력으로 보고 회전하지 않습니다."))
	float InputSideDeadZone = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "켜져 있으면 기준 오른쪽의 물 입력이 양수 방향으로 회전합니다. 꺼져 있으면 오른쪽 입력이 음수 방향으로 회전합니다."))
	bool bRightSideInputRotatesPositive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "켜져 있으면 목표 각도를 향해 고정 속도로 회전합니다. 꺼져 있으면 즉시 적용합니다."))
	bool bUseRotationInterpolation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ClampMin = "0.0", EditCondition = "bUseRotationInterpolation", EditConditionHides, ToolTip = "보간 회전이 켜져 있을 때 초당 회전할 각도입니다."))
	float RotationSpeedDegreesPerSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Condition", meta = (ToolTip = "켜져 있으면 기본 반응 조건이 만족될 때만 회전을 적용합니다."))
	bool bRequireConditionSatisfied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Clamp", meta = (ToolTip = "켜져 있으면 누적 회전 각도를 최소/최대 각도 사이로 제한합니다."))
	bool bClampRotationAngle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides, ToolTip = "누적 회전 각도의 최솟값입니다."))
	float MinRotationAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides, ToolTip = "누적 회전 각도의 최댓값입니다."))
	float MaxRotationAngleDegrees = 90.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Rotation Reaction|Runtime")
	float LastObservedValue = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Rotation Reaction|Runtime")
	float TargetAngleDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Rotation Reaction|Runtime")
	float CurrentAppliedAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ToolTip = "왼쪽/오른쪽 판단 중심, 전방 기준, 회전축, 마지막 물 입력, 계산된 회전 부호를 그립니다."))
	bool bDrawInputSideDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawInputSideDebug", EditConditionHides, ToolTip = "디버그 화살표 길이입니다."))
	float InputSideDebugDrawScale = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawInputSideDebug", EditConditionHides, ToolTip = "중심과 입력 위치를 표시하는 디버그 구체 반지름입니다."))
	float InputSideDebugSphereRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawInputSideDebug", EditConditionHides, ToolTip = "디버그 선과 화살표의 두께입니다."))
	float InputSideDebugThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (EditCondition = "bDrawInputSideDebug", EditConditionHides, ToolTip = "켜져 있으면 디버그 라벨을 함께 그립니다."))
	bool bDrawInputSideDebugLabel = true;

	UFUNCTION(BlueprintCallable, Category = "Water Basin Rotation Reaction")
	void ResetRotationReaction(bool bResetObservedValue = true, bool bApplyBaseRotation = true);

protected:
	virtual void OnWaterBasinReactionStateUpdated_Implementation(const FUOUWaterBasinReactionContext& Context) override;

private:
	bool bHasObservedValue = false;
	bool bHasCachedBaseRotation = false;
	FQuat BaseRelativeRotation = FQuat::Identity;
	FQuat BaseWorldRotation = FQuat::Identity;

	UPROPERTY(Transient)
	TObjectPtr<UUOUWaterBasinTargetComponent> BoundInputWaterBasinTarget = nullptr;

	bool bHasLastInputSideDebug = false;
	FVector LastInputSideDebugWorldLocation = FVector::ZeroVector;
	float LastInputSideDebugRotationSign = 0.0f;
	float LastInputSideDebugVolume = 0.0f;
	EUOUWaterBasinInputSource LastInputSideDebugSource = EUOUWaterBasinInputSource::Unknown;

	UFUNCTION()
	void HandleWaterInputReceived(UUOUWaterBasinTargetComponent* Target, const FUOUWaterBasinInputContext& InputContext);

	USceneComponent* ResolveRotationTargetComponent() const;
	USceneComponent* FindRotationTargetComponent() const;
	const USceneComponent* ResolveInputSideCenterComponent(const USceneComponent* TargetComponent) const;
	const USceneComponent* ResolveInputSideForwardReferenceComponent(const USceneComponent* CenterComponent) const;
	USceneComponent* FindSceneComponentByNameOrTag(FName ComponentName) const;
	void BindToWaterInputTarget();
	void UnbindFromWaterInputTarget();
	void CacheBaseRotationIfNeeded();
	void SetTargetRotationAngle(float NewTargetAngleDegrees);
	void UpdateInterpolatedRotation(float DeltaTime);
	void ApplyRotationAngle(float AngleDegrees);
	void CacheInputSideDebug(const FUOUWaterBasinInputContext& InputContext, float RotationSign);
	void DrawInputSideDebug() const;
	float ResolveInputRotationSign(const FUOUWaterBasinInputContext& InputContext) const;
	float ResolveInputSideSign(const FUOUWaterBasinInputContext& InputContext) const;
	FVector ResolveInputSideForwardWorldDirection(const USceneComponent* ReferenceComponent) const;
	FVector ResolveInputSideRightWorldDirection(const USceneComponent* ReferenceComponent, const USceneComponent* TargetComponent) const;
	EUOUWaterBasinRotationInputSidePolicy GetInputSidePolicy(EUOUWaterBasinInputSource Source) const;
	FVector ResolveWorldRotationAxis(const USceneComponent* TargetComponent) const;
	float ClampRotationAngle(float AngleDegrees) const;
};
