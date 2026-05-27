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
	IncrementalOnIncrease UMETA(DisplayName = "Incremental On Increase", ToolTip = "Accumulates rotation by increased water value. Full water input can optionally be treated as an attempted increase."),
	AbsoluteByValue UMETA(DisplayName = "Absolute By Value", ToolTip = "Maps the current water value directly to a rotation angle. Draining water rewinds rotation."),
	IncrementalOnWaterInput UMETA(DisplayName = "Incremental On Water Input", ToolTip = "Accumulates rotation by water input volume even if the actual water level does not change."),
	IncrementalOnWaterChange UMETA(DisplayName = "Incremental On Water Change", ToolTip = "Accumulates rotation when the water value increases or decreases. Drain direction can be configured separately.")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinRotationReactionSpace : uint8
{
	Relative UMETA(DisplayName = "Relative", ToolTip = "Rotates relative to the target component rotation."),
	World UMETA(DisplayName = "World", ToolTip = "Rotates in world rotation space.")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinRotationInputSidePolicy : uint8
{
	Ignore UMETA(DisplayName = "Ignore"),
	FixedPositive UMETA(DisplayName = "Fixed Positive"),
	FixedNegative UMETA(DisplayName = "Fixed Negative"),
	ByInputSide UMETA(DisplayName = "By Input Side")
};

UENUM(BlueprintType)
enum class EUOUWaterBasinDrainRotationDirection : uint8
{
	SameAsIncrease UMETA(DisplayName = "Same As Increase", ToolTip = "Draining water rotates in the same direction as filling water."),
	OppositeToIncrease UMETA(DisplayName = "Opposite To Increase", ToolTip = "Draining water rotates opposite to filling water.")
};

// Rotates a target component from WaterBasin reaction values or water input events.
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Target", meta = (ToolTip = "SceneComponent to rotate. If empty, the component is resolved by RotationTargetComponentName, then owner root if allowed."))
	TObjectPtr<USceneComponent> RotationTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Target", meta = (ToolTip = "Component name or tag used when RotationTargetComponent is empty."))
	FName RotationTargetComponentName = TEXT("Platform");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Target", meta = (ToolTip = "If true, use the owner root component when no explicit rotation target can be resolved."))
	bool bUseOwnerRootWhenTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "How water changes or water input should drive rotation."))
	EUOUWaterBasinRotationReactionMode RotationMode = EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "Rotation space. Relative uses target local rotation, World uses world rotation."))
	EUOUWaterBasinRotationReactionSpace RotationSpace = EUOUWaterBasinRotationReactionSpace::Relative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "Rotation axis. Relative mode treats this as target local axis; World mode treats it as world axis."))
	FVector RotationAxis = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode != EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Degrees to rotate per 1 water value unit. With FillRatio, 1.0 means full."))
	float DegreesPerValueUnit = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnIncrease || RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterChange", EditConditionHides, ToolTip = "If true, water input received while the target or connected group is already full still rotates as an attempted value increase. Leave off to rotate only when the water value actually changes."))
	bool bRotateOnFullWaterInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterChange", EditConditionHides, ToolTip = "Direction to rotate when the water value decreases."))
	EUOUWaterBasinDrainRotationDirection DrainRotationDirection = EUOUWaterBasinDrainRotationDirection::SameAsIncrease;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Degrees to rotate per 1 water input volume unit."))
	float DegreesPerInputVolume = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Sign policy for player pour input."))
	EUOUWaterBasinRotationInputSidePolicy PlayerPourInputSidePolicy = EUOUWaterBasinRotationInputSidePolicy::ByInputSide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Sign policy for rain input."))
	EUOUWaterBasinRotationInputSidePolicy RainInputSidePolicy = EUOUWaterBasinRotationInputSidePolicy::FixedPositive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput", EditConditionHides, ToolTip = "Sign policy for script or unknown input."))
	EUOUWaterBasinRotationInputSidePolicy ScriptInputSidePolicy = EUOUWaterBasinRotationInputSidePolicy::FixedPositive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "Center used to judge whether water input location is on the left or right side. Empty uses name lookup, then rotation target."))
	TObjectPtr<USceneComponent> InputSideCenterComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "Component name or tag used as the left/right center when direct center component is empty."))
	FName InputSideCenterComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "Forward basis used to judge left and right. Empty uses name lookup, then input side center."))
	TObjectPtr<USceneComponent> InputSideForwardReferenceComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "Component name or tag used as the forward reference when direct reference component is empty."))
	FName InputSideForwardReferenceComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (ClampMin = "0.0", EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "If the left/right side result is smaller than this value, the input location is treated as center-line ambiguous and does not rotate."))
	float InputSideDeadZone = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Input Side", meta = (EditCondition = "RotationMode == EUOUWaterBasinRotationReactionMode::IncrementalOnWaterInput && (PlayerPourInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || RainInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide || ScriptInputSidePolicy == EUOUWaterBasinRotationInputSidePolicy::ByInputSide)", EditConditionHides, ToolTip = "If true, water input on the reference right side rotates positive. If false, right side rotates negative."))
	bool bRightSideInputRotatesPositive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ToolTip = "If true, rotate toward the target angle at a fixed speed. If false, apply instantly."))
	bool bUseRotationInterpolation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Rotation", meta = (ClampMin = "0.0", EditCondition = "bUseRotationInterpolation", EditConditionHides, ToolTip = "Rotation speed in degrees per second when interpolation is enabled."))
	float RotationSpeedDegreesPerSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Condition", meta = (ToolTip = "If true, apply rotation only when the base reaction condition is satisfied."))
	bool bRequireConditionSatisfied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Clamp", meta = (ToolTip = "If true, clamp the accumulated rotation angle between Min and Max."))
	bool bClampRotationAngle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides, ToolTip = "Minimum accumulated rotation angle."))
	float MinRotationAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides, ToolTip = "Maximum accumulated rotation angle."))
	float MaxRotationAngleDegrees = 90.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Rotation Reaction|Runtime")
	float LastObservedValue = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Rotation Reaction|Runtime")
	float TargetAngleDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Basin Rotation Reaction|Runtime")
	float CurrentAppliedAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ToolTip = "Draws the left/right center, forward reference, axis, last water input, and resolved rotation sign."))
	bool bDrawInputSideDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawInputSideDebug", EditConditionHides, ToolTip = "Debug arrow length."))
	float InputSideDebugDrawScale = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawInputSideDebug", EditConditionHides, ToolTip = "Debug sphere radius for center and input location."))
	float InputSideDebugSphereRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawInputSideDebug", EditConditionHides, ToolTip = "Debug line and arrow thickness."))
	float InputSideDebugThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Debug", meta = (EditCondition = "bDrawInputSideDebug", EditConditionHides, ToolTip = "If true, draw debug labels."))
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
