// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Pour/UOUPourReceiverComponent.h"
#include "UOUPourRotationReactionComponent.generated.h"

class USceneComponent;

UENUM(BlueprintType)
enum class EUOUPourRotationReactionSpace : uint8
{
	Relative UMETA(DisplayName = "Relative"),
	World UMETA(DisplayName = "World")
};

UENUM(BlueprintType)
enum class EUOUPourRotationAmountMode : uint8
{
	Duration UMETA(DisplayName = "Duration"),
	Event UMETA(DisplayName = "Event")
};

UENUM(BlueprintType)
enum class EUOUPourRotationDirectionMode : uint8
{
	FixedPositive UMETA(DisplayName = "Fixed Positive"),
	FixedNegative UMETA(DisplayName = "Fixed Negative"),
	ByPourTorque UMETA(DisplayName = "By Pour Torque")
};

UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Pour Rotation Reaction"))
class UNDERONEUMBRELLA_API UUOUPourRotationReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPourRotationReactionComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Activation")
	bool bRotationReactionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Input")
	TObjectPtr<UUOUPourReceiverComponent> PourReceiverComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Input")
	bool bAutoFindPourReceiverComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Target")
	TObjectPtr<USceneComponent> RotationTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Target")
	FName RotationTargetComponentName = TEXT("Platform");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Target")
	bool bUseOwnerRootWhenTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation")
	EUOUPourRotationReactionSpace RotationSpace = EUOUPourRotationReactionSpace::Relative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation")
	FVector RotationAxis = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation")
	EUOUPourRotationDirectionMode RotationDirectionMode = EUOUPourRotationDirectionMode::FixedPositive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation")
	EUOUPourRotationAmountMode RotationAmountMode = EUOUPourRotationAmountMode::Duration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (EditCondition = "RotationAmountMode == EUOUPourRotationAmountMode::Duration", EditConditionHides))
	float DegreesPerPourSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (EditCondition = "RotationAmountMode == EUOUPourRotationAmountMode::Event", EditConditionHides))
	float DegreesPerPourEvent = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Torque", meta = (EditCondition = "RotationDirectionMode == EUOUPourRotationDirectionMode::ByPourTorque", EditConditionHides))
	TObjectPtr<USceneComponent> TorqueCenterComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Torque", meta = (EditCondition = "RotationDirectionMode == EUOUPourRotationDirectionMode::ByPourTorque", EditConditionHides))
	FName TorqueCenterComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Torque", meta = (ClampMin = "0.0", EditCondition = "RotationDirectionMode == EUOUPourRotationDirectionMode::ByPourTorque", EditConditionHides))
	float TorqueDeadZone = 0.001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Torque", meta = (EditCondition = "RotationDirectionMode == EUOUPourRotationDirectionMode::ByPourTorque", EditConditionHides))
	bool bInvertPourTorqueDirection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation")
	bool bUseRotationInterpolation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (ClampMin = "0.0", EditCondition = "bUseRotationInterpolation", EditConditionHides))
	float RotationSpeedDegreesPerSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Clamp")
	bool bClampRotationAngle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides))
	float MinRotationAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides))
	float MaxRotationAngleDegrees = 90.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Pour Rotation|Runtime")
	float TargetAngleDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Pour Rotation|Runtime")
	float CurrentAppliedAngleDegrees = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Pour Rotation")
	void ResetRotationReaction(bool bApplyBaseRotation = true);

	UFUNCTION(BlueprintCallable, Category = "Pour Rotation")
	void SetRotationReactionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Pour Rotation")
	bool IsRotationReactionEnabled() const;

private:
	bool bHasCachedBaseRotation = false;
	FQuat BaseRelativeRotation = FQuat::Identity;
	FQuat BaseWorldRotation = FQuat::Identity;

	UPROPERTY(Transient)
	TObjectPtr<UUOUPourReceiverComponent> BoundPourReceiverComponent = nullptr;

	UFUNCTION()
	void HandlePourReceived(UUOUPourReceiverComponent* Receiver, const FUOUPourInputContext& PourContext);

	UUOUPourReceiverComponent* ResolvePourReceiverComponent() const;
	USceneComponent* ResolveRotationTargetComponent() const;
	const USceneComponent* ResolveTorqueCenterComponent(const USceneComponent* TargetComponent) const;
	USceneComponent* FindRotationTargetComponent() const;
	USceneComponent* FindSceneComponentByNameOrTag(FName ComponentName) const;
	void BindToPourReceiver();
	void UnbindFromPourReceiver();
	void CacheBaseRotationIfNeeded();
	void SetTargetRotationAngle(float NewTargetAngleDegrees);
	void UpdateInterpolatedRotation(float DeltaTime);
	void ApplyRotationAngle(float AngleDegrees);
	float ResolveRotationSign(const FUOUPourInputContext& PourContext) const;
	float ResolvePourTorqueRotationSign(const FUOUPourInputContext& PourContext) const;
	FVector ResolveWorldRotationAxis(const USceneComponent* TargetComponent) const;
	float ClampRotationAngle(float AngleDegrees) const;
};
