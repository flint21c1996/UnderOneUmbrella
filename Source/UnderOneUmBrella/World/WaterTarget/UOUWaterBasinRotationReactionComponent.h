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
	IncrementalOnIncrease UMETA(DisplayName = "Incremental On Increase", ToolTip = "Water value가 증가한 양만큼 누적 회전합니다. 물이 빠져도 회전은 되돌아가지 않습니다."),
	AbsoluteByValue UMETA(DisplayName = "Absolute By Value", ToolTip = "현재 Water value에 대응하는 각도로 회전합니다. 물이 빠지면 회전도 되돌아갑니다."),
	IncrementalOnWaterInput UMETA(DisplayName = "Incremental On Water Input", ToolTip = "실제 수위가 변하지 않아도 WaterBasinTarget에 들어온 물 입력량만큼 누적 회전합니다.")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinRotationReactionSpace : uint8
{
	Relative UMETA(DisplayName = "Relative", ToolTip = "대상 컴포넌트의 상대 회전을 기준으로 회전합니다."),
	World UMETA(DisplayName = "World", ToolTip = "월드 회전을 기준으로 회전합니다.")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinRotationInputDirectionPolicy : uint8
{
	Ignore UMETA(DisplayName = "Ignore"),
	FixedPositive UMETA(DisplayName = "Fixed Positive"),
	FixedNegative UMETA(DisplayName = "Fixed Negative"),
	ByInputDirection UMETA(DisplayName = "By Input Direction")
};

// WaterBasinReactionComponentBase의 물 상태 감지를 이용해 플랫폼을 지정 축으로 회전시키는 ReactionComponent입니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Water Basin Rotation Reaction"))
class UNDERONEUMBRELLA_API UUOUWaterBasinRotationReactionComponent : public UUOUWaterBasinReactionComponentBase
{
	GENERATED_BODY()

public:
	UUOUWaterBasinRotationReactionComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Target", meta = (ToolTip = "회전시킬 SceneComponent입니다. 비워두면 RotationTargetComponentName으로 찾고, 그래도 없으면 옵션에 따라 Owner RootComponent를 사용합니다."))
	TObjectPtr<USceneComponent> RotationTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Target", meta = (ToolTip = "RotationTargetComponent가 비어 있을 때 Owner에서 찾을 Component 이름 또는 Component Tag입니다."))
	FName RotationTargetComponentName = TEXT("Platform");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Target", meta = (ToolTip = "RotationTargetComponent를 찾지 못했을 때 Owner RootComponent를 회전 대상으로 사용할지 정합니다."))
	bool bUseOwnerRootWhenTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Direction", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || RainDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || ScriptDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection)", EditConditionHides, ToolTip = "By Input Direction uses this component's forward vector as the front basis, then checks whether the water input location is on the left or right side."))
	TObjectPtr<USceneComponent> InputDirectionReferenceComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Direction", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || RainDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || ScriptDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection)", EditConditionHides, ToolTip = "Component name or tag to use as the By Input Direction forward reference when InputDirectionReferenceComponent is empty."))
	FName InputDirectionReferenceComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Direction", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || RainDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || ScriptDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection)", EditConditionHides, ToolTip = "If no input direction reference is found, use RotationTargetComponent's forward vector."))
	bool bUseRotationTargetAsInputDirectionReferenceWhenMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "물 값 변화를 회전으로 해석하는 방식입니다."))
	EUOUWaterBasinRotationReactionMode RotationMode = EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "회전을 적용할 공간입니다. Relative는 상대 회전, World는 월드 회전을 기준으로 합니다."))
	EUOUWaterBasinRotationReactionSpace RotationSpace = EUOUWaterBasinRotationReactionSpace::Relative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "회전 축입니다. Relative 모드에서는 대상 컴포넌트의 로컬 축, World 모드에서는 월드 축으로 해석합니다. 음수 축이나 Degrees Per Value Unit 음수 값으로 반대 방향을 만들 수 있습니다."))
	FVector RotationAxis = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode != EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Water value 1 단위 변화당 회전할 각도입니다. 기본 ValueSource인 FillRatio에서는 1.0이 가득 참을 의미하므로 90이면 0~100% 변화에 90도 회전합니다."))
	float DegreesPerValueUnit = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Rotation Mode가 Incremental On Water Input일 때 물 입력 부피 1 단위당 회전할 각도입니다."))
	float DegreesPerInputVolume = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Direction", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Player Pour 입력이 들어왔을 때 회전 방향을 정하는 방식입니다."))
	EUOUWaterBasinRotationInputDirectionPolicy PlayerPourDirectionPolicy = EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Direction", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Rain 입력이 들어왔을 때 회전 방향을 정하는 방식입니다. 기본값은 기존 입력량 기반 회전과 같은 정방향입니다."))
	EUOUWaterBasinRotationInputDirectionPolicy RainDirectionPolicy = EUOUWaterBasinRotationInputDirectionPolicy::FixedPositive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Direction", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Script 또는 Unknown 입력이 들어왔을 때 회전 방향을 정하는 방식입니다."))
	EUOUWaterBasinRotationInputDirectionPolicy ScriptDirectionPolicy = EUOUWaterBasinRotationInputDirectionPolicy::FixedPositive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Direction", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || RainDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || ScriptDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection)", EditConditionHides, ToolTip = "Fallback world forward direction used when By Input Direction has no reference component."))
	FVector InputDirectionReferenceVector = FVector(1.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Direction", meta = (ClampMin = "0.0", EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || RainDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || ScriptDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection)", EditConditionHides, ToolTip = "If the left/right side result is smaller than this value, the input location is treated as center-line ambiguous and does not rotate."))
	float InputDirectionDeadZone = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Direction", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || RainDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection || ScriptDirectionPolicy == EUOUWaterBasinRotationInputDirectionPolicy::ByInputDirection)", EditConditionHides, ToolTip = "By Input Direction으로 계산한 회전 부호를 반전합니다. 피벗이나 메시 방향 때문에 의도와 반대로 보일 때 켭니다."))
	bool bInvertInputDirectionRotationSign = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "켜져 있으면 목표 각도까지 지정한 초당 각도 속도로 회전합니다. 꺼져 있으면 물 상태 변화 시 즉시 목표 각도로 이동합니다."))
	bool bUseRotationInterpolation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ClampMin = "0.0", EditCondition = "bUseRotationInterpolation", EditConditionHides, ToolTip = "회전 보간을 사용할 때 초당 회전할 각도입니다. 90이면 1초에 90도 이동합니다."))
	float RotationSpeedDegreesPerSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Condition", meta = (ToolTip = "켜져 있으면 ReactionBase의 조건이 만족될 때만 회전을 적용합니다. 꺼져 있으면 조건은 디버그/이벤트용으로만 평가됩니다."))
	bool bRequireConditionSatisfied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Clamp", meta = (ToolTip = "회전 각도를 Min/Max 범위로 제한할지 정합니다."))
	bool bClampRotationAngle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides, ToolTip = "허용할 최소 누적 회전 각도입니다."))
	float MinRotationAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides, ToolTip = "허용할 최대 누적 회전 각도입니다."))
	float MaxRotationAngleDegrees = 90.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Rotation Reaction|Runtime")
	float LastObservedValue = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Rotation Reaction|Runtime")
	float TargetAngleDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Rotation Reaction|Runtime")
	float CurrentAppliedAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ToolTip = "회전 중심, 회전축, 마지막 물 입력 방향, 최종 회전 반응 방향을 월드 디버그로 표시합니다."))
	bool bDrawInputDirectionDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawInputDirectionDebug", EditConditionHides, ToolTip = "디버그 화살표 길이입니다."))
	float InputDirectionDebugDrawScale = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawInputDirectionDebug", EditConditionHides, ToolTip = "디버그 중심/입력 위치 구체 반지름입니다."))
	float InputDirectionDebugSphereRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawInputDirectionDebug", EditConditionHides, ToolTip = "디버그 선과 화살표 두께입니다."))
	float InputDirectionDebugThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (EditCondition = "bDrawInputDirectionDebug", EditConditionHides, ToolTip = "디버그 라벨을 함께 표시할지 정합니다."))
	bool bDrawInputDirectionDebugLabel = true;

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

	bool bHasLastInputDirectionDebug = false;
	FVector LastInputDebugWorldLocation = FVector::ZeroVector;
	FVector LastInputDebugWorldDirection = FVector::ZeroVector;
	float LastInputDebugRotationSign = 0.0f;
	float LastInputDebugVolume = 0.0f;
	EUOUWaterBasinInputSource LastInputDebugSource = EUOUWaterBasinInputSource::Unknown;

	UFUNCTION()
	void HandleWaterInputReceived(UUOUWaterBasinTargetComponent* Target, const FUOUWaterBasinInputContext& InputContext);

	USceneComponent* ResolveRotationTargetComponent() const;
	USceneComponent* FindRotationTargetComponent() const;
	const USceneComponent* ResolveInputDirectionReferenceComponent(const USceneComponent* TargetComponent) const;
	USceneComponent* FindSceneComponentByNameOrTag(FName ComponentName) const;
	void BindToWaterInputTarget();
	void UnbindFromWaterInputTarget();
	void CacheBaseRotationIfNeeded();
	void SetTargetRotationAngle(float NewTargetAngleDegrees);
	void UpdateInterpolatedRotation(float DeltaTime);
	void ApplyRotationAngle(float AngleDegrees);
	void CacheInputDirectionDebug(const FUOUWaterBasinInputContext& InputContext, float DirectionSign);
	void DrawInputDirectionDebug() const;
	float ResolveInputRotationSign(const FUOUWaterBasinInputContext& InputContext) const;
	float ResolveInputDirectionSign(const FUOUWaterBasinInputContext& InputContext) const;
	float ApplyInputDirectionSignInversion(float DirectionSign) const;
	FVector ResolveInputDirectionReferenceWorldDirection(const USceneComponent* ReferenceComponent) const;
	FVector ResolveInputDirectionSideWorldDirection(const USceneComponent* ReferenceComponent, const USceneComponent* TargetComponent) const;
	EUOUWaterBasinRotationInputDirectionPolicy GetInputDirectionPolicy(EUOUWaterBasinInputSource Source) const;
	FVector ResolveWorldRotationAxis(const USceneComponent* TargetComponent) const;
	float ClampRotationAngle(float AngleDegrees) const;
};
