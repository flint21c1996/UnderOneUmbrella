// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Pour/UOUPourReceiverComponent.h"
#include "UOUPourRotationReactionComponent.generated.h"

class AActor;
class USceneComponent;

UENUM(BlueprintType)
enum class EUOUPourRotationReactionSpace : uint8
{
	Relative UMETA(DisplayName = "Relative", ToolTip = "대상 컴포넌트의 부모 기준 상대 회전으로 적용합니다."),
	World UMETA(DisplayName = "World", ToolTip = "월드 기준 회전으로 적용합니다.")
};

UENUM(BlueprintType)
enum class EUOUPourRotationAmountMode : uint8
{
	Duration UMETA(DisplayName = "Duration", ToolTip = "pour가 유지된 시간에 비례해 매 프레임 회전합니다."),
	Event UMETA(DisplayName = "Event", ToolTip = "pour 입력 이벤트가 들어올 때마다 고정 각도만큼 회전합니다.")
};

UENUM(BlueprintType)
enum class EUOUPourRotationDirectionMode : uint8
{
	FixedPositive UMETA(DisplayName = "Fixed Positive", ToolTip = "붓는 방향과 관계없이 항상 양의 방향으로 회전합니다."),
	FixedNegative UMETA(DisplayName = "Fixed Negative", ToolTip = "붓는 방향과 관계없이 항상 음의 방향으로 회전합니다."),
	ByPourTorque UMETA(DisplayName = "By Pour Torque", ToolTip = "물이 닿은 위치와 떨어지는 방향을 이용해 수차처럼 회전 방향을 계산합니다.")
};

UENUM(BlueprintType)
enum class EUOUPourRotationDrivenMovementSpace : uint8
{
	Relative UMETA(DisplayName = "Relative", ToolTip = "이동 대상 컴포넌트의 부모 기준 상대 위치로 이동합니다."),
	World UMETA(DisplayName = "World", ToolTip = "이동 대상 컴포넌트를 월드 위치 기준으로 이동합니다.")
};

UENUM(BlueprintType)
enum class EUOUPourRotationDrivenMovementAxis : uint8
{
	X UMETA(DisplayName = "X", ToolTip = "X축 방향으로 이동합니다."),
	Y UMETA(DisplayName = "Y", ToolTip = "Y축 방향으로 이동합니다."),
	Z UMETA(DisplayName = "Z", ToolTip = "Z축 방향으로 이동합니다. 엘리베이터 퍼즐의 기본 선택입니다."),
	Custom UMETA(DisplayName = "Custom", ToolTip = "Custom Movement Axis에 입력한 방향으로 이동합니다.")
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Activation", meta = (ToolTip = "false이면 pour 입력을 받아도 회전과 회전 기반 이동을 적용하지 않습니다."))
	bool bRotationReactionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Input", meta = (ToolTip = "이 회전 반응이 구독할 Pour Receiver입니다. 비워두면 자동 탐색 옵션에 따라 Owner에서 찾습니다."))
	TObjectPtr<UUOUPourReceiverComponent> PourReceiverComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Input", meta = (ToolTip = "Pour Receiver가 직접 지정되지 않았을 때 Owner 액터에서 UOUPourReceiverComponent를 자동으로 찾습니다."))
	bool bAutoFindPourReceiverComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Target", meta = (ToolTip = "실제로 회전시킬 씬 컴포넌트입니다. 지정하면 이름 탐색보다 우선 사용됩니다."))
	TObjectPtr<USceneComponent> RotationTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Target", meta = (ToolTip = "Rotation Target Component가 비어 있을 때 찾을 컴포넌트 이름 또는 태그입니다."))
	FName RotationTargetComponentName = TEXT("Platform");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Target", meta = (ToolTip = "회전 대상을 찾지 못했을 때 Owner의 RootComponent를 회전 대상으로 사용할지 여부입니다."))
	bool bUseOwnerRootWhenTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (ToolTip = "회전을 상대 회전으로 적용할지 월드 회전으로 적용할지 정합니다."))
	EUOUPourRotationReactionSpace RotationSpace = EUOUPourRotationReactionSpace::Relative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (ToolTip = "회전축입니다. 기본값 Z축은 일반적인 수평 플랫폼 또는 수차의 세로축 회전에 사용합니다."))
	FVector RotationAxis = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (ToolTip = "회전 방향을 고정할지, 물이 떨어지는 방향과 충돌 위치로 계산할지 정합니다."))
	EUOUPourRotationDirectionMode RotationDirectionMode = EUOUPourRotationDirectionMode::FixedPositive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (ToolTip = "회전량을 pour 지속 시간 기준으로 계산할지, 입력 이벤트 1회당 고정 각도로 계산할지 정합니다."))
	EUOUPourRotationAmountMode RotationAmountMode = EUOUPourRotationAmountMode::Duration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (EditCondition = "RotationAmountMode == EUOUPourRotationAmountMode::Duration", EditConditionHides, ToolTip = "Duration 모드에서 pour 1초당 회전할 각도입니다. 값이 클수록 물을 붓는 동안 더 빠르게 회전합니다."))
	float DegreesPerPourSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (EditCondition = "RotationAmountMode == EUOUPourRotationAmountMode::Event", EditConditionHides, ToolTip = "Event 모드에서 pour 입력 한 번마다 누적할 회전 각도입니다. 동작 확인용으로는 15도 이상이 보기 쉽습니다."))
	float DegreesPerPourEvent = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Torque", meta = (EditCondition = "RotationDirectionMode == EUOUPourRotationDirectionMode::ByPourTorque", EditConditionHides, ToolTip = "토크 방향 계산에 사용할 중심 컴포넌트입니다. 비워두면 이름 탐색 후 회전 대상 컴포넌트를 중심으로 사용합니다."))
	TObjectPtr<USceneComponent> TorqueCenterComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Torque", meta = (EditCondition = "RotationDirectionMode == EUOUPourRotationDirectionMode::ByPourTorque", EditConditionHides, ToolTip = "Torque Center Component가 비어 있을 때 찾을 중심 컴포넌트 이름 또는 태그입니다."))
	FName TorqueCenterComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Torque", meta = (ClampMin = "0.0", EditCondition = "RotationDirectionMode == EUOUPourRotationDirectionMode::ByPourTorque", EditConditionHides, ToolTip = "계산된 토크 값의 절댓값이 이 값보다 작으면 방향이 불명확하다고 보고 회전하지 않습니다."))
	float TorqueDeadZone = 0.001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Torque", meta = (EditCondition = "RotationDirectionMode == EUOUPourRotationDirectionMode::ByPourTorque", EditConditionHides, ToolTip = "토크 기반으로 계산된 회전 방향을 반대로 뒤집습니다. 수차 모델의 축 방향이 반대로 잡혔을 때 사용합니다."))
	bool bInvertPourTorqueDirection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (ToolTip = "true이면 목표 각도까지 즉시 이동하지 않고 Tick에서 부드럽게 보간합니다."))
	bool bUseRotationInterpolation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Rotation", meta = (ClampMin = "0.0", EditCondition = "bUseRotationInterpolation", EditConditionHides, ToolTip = "회전 보간을 사용할 때 초당 따라갈 수 있는 최대 각도입니다."))
	float RotationSpeedDegreesPerSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Clamp", meta = (ToolTip = "true이면 누적 회전 각도를 최소/최대 각도 사이로 제한합니다."))
	bool bClampRotationAngle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides, ToolTip = "허용할 최소 누적 회전 각도입니다."))
	float MinRotationAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Clamp", meta = (EditCondition = "bClampRotationAngle", EditConditionHides, ToolTip = "허용할 최대 누적 회전 각도입니다."))
	float MaxRotationAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (ToolTip = "true이면 누적 회전 각도를 거리로 변환해 다른 컴포넌트를 함께 이동시킵니다. 엘리베이터 퍼즐에 사용합니다."))
	bool bDriveMovementFromRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides, ToolTip = "회전값에 의해 위치가 변할 씬 컴포넌트입니다. 지정하면 이름 탐색보다 우선 사용됩니다."))
	TObjectPtr<USceneComponent> MovementTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides, ToolTip = "회전값에 의해 위치가 변할 플랫폼 액터입니다. Movement Target Component가 비어 있으면 이 액터의 RootComponent를 이동 대상으로 사용합니다."))
	TObjectPtr<AActor> MovementTargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides, ToolTip = "Movement Target Component가 비어 있을 때 찾을 컴포넌트 이름 또는 태그입니다."))
	FName MovementTargetComponentName = TEXT("Platform");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides, ToolTip = "이동 대상을 찾지 못했을 때 Owner의 RootComponent를 이동 대상으로 사용할지 여부입니다."))
	bool bUseOwnerRootWhenMovementTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides, ToolTip = "이동을 상대 위치로 적용할지 월드 위치로 적용할지 정합니다."))
	EUOUPourRotationDrivenMovementSpace MovementSpace = EUOUPourRotationDrivenMovementSpace::Relative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides, ToolTip = "회전값을 어느 축 방향 이동으로 바꿀지 정합니다. 엘리베이터는 보통 Z를 사용합니다."))
	EUOUPourRotationDrivenMovementAxis MovementAxisMode = EUOUPourRotationDrivenMovementAxis::Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation && MovementAxisMode == EUOUPourRotationDrivenMovementAxis::Custom", EditConditionHides, ToolTip = "Movement Axis Mode가 Custom일 때 사용할 이동 방향입니다. 예: (0, 0, 1)은 위/아래 이동입니다."))
	FVector MovementAxis = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides, ToolTip = "회전 1도당 이동할 거리입니다. 예를 들어 2이면 90도 회전 시 180만큼 이동합니다. 음수로 두면 회전 방향과 반대로 이동합니다."))
	float DistancePerRotationDegree = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides, ToolTip = "true이면 회전으로 계산된 이동 거리를 최소/최대 거리 사이로 제한합니다."))
	bool bClampMovementDistance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation && bClampMovementDistance", EditConditionHides, ToolTip = "허용할 최소 이동 거리입니다. 기준 위치에서 이 거리보다 덜 이동하지 않도록 제한합니다."))
	float MinMovementDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Rotation|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation && bClampMovementDistance", EditConditionHides, ToolTip = "허용할 최대 이동 거리입니다. 기준 위치에서 이 거리보다 더 멀리 이동하지 않도록 제한합니다."))
	float MaxMovementDistance = 300.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Pour Rotation|Runtime", meta = (ToolTip = "pour 입력으로 누적된 목표 회전 각도입니다. 보간 사용 시 CurrentAppliedAngleDegrees가 이 값을 따라갑니다."))
	float TargetAngleDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Pour Rotation|Runtime", meta = (ToolTip = "현재 실제 회전 대상에 적용된 누적 회전 각도입니다."))
	float CurrentAppliedAngleDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Pour Rotation|Runtime", meta = (ToolTip = "현재 이동 대상에 적용된 누적 이동 거리입니다."))
	float CurrentMovementDistance = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Pour Rotation")
	void ResetRotationReaction(bool bApplyBaseRotation = true);

	UFUNCTION(BlueprintCallable, Category = "Pour Rotation")
	void SetRotationReactionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Pour Rotation")
	bool IsRotationReactionEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Pour Rotation", meta = (ToolTip = "현재 이동 대상 위치를 기준으로 회전 기반 이동의 기준 위치를 다시 저장합니다. 플랫폼 시작 위치를 런타임에 바꾼 뒤 호출하면 이후 이동 계산에 반영됩니다."))
	void RefreshDrivenMovementBaseLocation();

private:
	bool bHasCachedBaseRotation = false;
	bool bHasCachedBaseMovementLocation = false;
	FQuat BaseRelativeRotation = FQuat::Identity;
	FQuat BaseWorldRotation = FQuat::Identity;
	FVector BaseRelativeMovementLocation = FVector::ZeroVector;
	FVector BaseWorldMovementLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UUOUPourReceiverComponent> BoundPourReceiverComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CachedBaseMovementTargetComponent = nullptr;

	UFUNCTION()
	void HandlePourReceived(UUOUPourReceiverComponent* Receiver, const FUOUPourInputContext& PourContext);

	UUOUPourReceiverComponent* ResolvePourReceiverComponent() const;
	USceneComponent* ResolveRotationTargetComponent() const;
	USceneComponent* ResolveMovementTargetComponent() const;
	const USceneComponent* ResolveTorqueCenterComponent(const USceneComponent* TargetComponent) const;
	USceneComponent* FindRotationTargetComponent() const;
	USceneComponent* FindMovementTargetComponent() const;
	USceneComponent* FindSceneComponentByNameOrTag(FName ComponentName) const;
	void BindToPourReceiver();
	void UnbindFromPourReceiver();
	void CacheBaseRotationIfNeeded();
	void CacheBaseMovementLocationIfNeeded();
	void SetTargetRotationAngle(float NewTargetAngleDegrees);
	void UpdateInterpolatedRotation(float DeltaTime);
	void ApplyRotationAngle(float AngleDegrees);
	void ApplyDrivenMovement(float AngleDegrees);
	FVector ResolveMovementAxis() const;
	float ResolveRotationSign(const FUOUPourInputContext& PourContext) const;
	float ResolvePourTorqueRotationSign(const FUOUPourInputContext& PourContext) const;
	FVector ResolveWorldRotationAxis(const USceneComponent* TargetComponent) const;
	float ClampRotationAngle(float AngleDegrees) const;
};
