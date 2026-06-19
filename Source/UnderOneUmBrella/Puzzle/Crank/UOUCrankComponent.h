// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUCrankComponent.generated.h"

class AActor;
class USceneComponent;

UENUM(BlueprintType)
enum class EUOUCrankTransformSpace : uint8
{
	Relative UMETA(DisplayName = "Relative"),
	World UMETA(DisplayName = "World")
};

UENUM(BlueprintType)
enum class EUOUCrankAxisMode : uint8
{
	X UMETA(DisplayName = "X"),
	Y UMETA(DisplayName = "Y"),
	Z UMETA(DisplayName = "Z"),
	Custom UMETA(DisplayName = "Custom")
};

UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Crank"))
class UNDERONEUMBRELLA_API UUOUCrankComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUCrankComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Activation")
	bool bCrankEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Input")
	EUOUCrankAxisMode InputAxisMode = EUOUCrankAxisMode::X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Input", meta = (EditCondition = "InputAxisMode == EUOUCrankAxisMode::Custom", EditConditionHides))
	FVector CustomInputAxis = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Input")
	bool bInvertInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Target")
	TObjectPtr<USceneComponent> RotationTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Target")
	FName RotationTargetComponentName = TEXT("Crank");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Target")
	bool bUseOwnerRootWhenTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Rotation")
	EUOUCrankTransformSpace RotationSpace = EUOUCrankTransformSpace::Relative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Rotation")
	EUOUCrankAxisMode RotationAxisMode = EUOUCrankAxisMode::Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Rotation", meta = (EditCondition = "RotationAxisMode == EUOUCrankAxisMode::Custom", EditConditionHides))
	FVector CustomRotationAxis = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Rotation", meta = (ClampMin = "0.0"))
	float DegreesPerInputSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Rotation")
	bool bClampRotationAngle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Rotation", meta = (EditCondition = "bClampRotationAngle", EditConditionHides))
	float MinRotationAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Rotation", meta = (EditCondition = "bClampRotationAngle", EditConditionHides))
	float MaxRotationAngleDegrees = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation")
	bool bDriveRotationFromCrank = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank", EditConditionHides))
	TObjectPtr<USceneComponent> DrivenRotationTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank", EditConditionHides))
	TObjectPtr<AActor> DrivenRotationTargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank", EditConditionHides))
	FName DrivenRotationTargetComponentName = TEXT("Gear");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank", EditConditionHides))
	bool bUseOwnerRootWhenDrivenRotationTargetMissing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank", EditConditionHides))
	bool bForceDrivenRotationTargetMovable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank", EditConditionHides))
	EUOUCrankTransformSpace DrivenRotationSpace = EUOUCrankTransformSpace::World;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank", EditConditionHides))
	EUOUCrankAxisMode DrivenRotationAxisMode = EUOUCrankAxisMode::Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank && DrivenRotationAxisMode == EUOUCrankAxisMode::Custom", EditConditionHides))
	FVector CustomDrivenRotationAxis = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank", EditConditionHides))
	float DrivenRotationDegreesPerCrankDegree = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank", EditConditionHides))
	bool bClampDrivenRotationAngle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank && bClampDrivenRotationAngle", EditConditionHides))
	float MinDrivenRotationAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Rotation", meta = (EditCondition = "bDriveRotationFromCrank && bClampDrivenRotationAngle", EditConditionHides))
	float MaxDrivenRotationAngleDegrees = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement")
	bool bDriveMovementFromRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides))
	TObjectPtr<USceneComponent> MovementTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides))
	TObjectPtr<AActor> MovementTargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides))
	FName MovementTargetComponentName = TEXT("Platform");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides))
	bool bUseOwnerRootWhenMovementTargetMissing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides))
	bool bForceMovementTargetMovable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides))
	EUOUCrankTransformSpace MovementSpace = EUOUCrankTransformSpace::World;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides))
	EUOUCrankAxisMode MovementAxisMode = EUOUCrankAxisMode::Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation && MovementAxisMode == EUOUCrankAxisMode::Custom", EditConditionHides))
	FVector CustomMovementAxis = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides))
	float DistancePerRotationDegree = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation", EditConditionHides))
	bool bClampMovementDistance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation && bClampMovementDistance", EditConditionHides))
	float MinMovementDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crank|Driven Movement", meta = (EditCondition = "bDriveMovementFromRotation && bClampMovementDistance", EditConditionHides))
	float MaxMovementDistance = 300.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	bool bIsGrabbed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	float CurrentRotationAngleDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	float CurrentMovementDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	float CurrentDrivenRotationAngleDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	float LastInputValue = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	bool bLastDrivenRotationApplied = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	FName LastDrivenRotationFailureReason = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	bool bLastDrivenMovementApplied = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	FName LastDrivenMovementFailureReason = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	FVector LastDrivenMovementAxis = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	float LastRawMovementDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	FVector LastDesiredMovementLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crank|Runtime")
	FVector LastActualMovementLocation = FVector::ZeroVector;

	UFUNCTION(BlueprintCallable, Category = "Crank")
	bool CanGrab(AActor* Interactor) const;

	UFUNCTION(BlueprintCallable, Category = "Crank")
	bool TryBeginGrab(AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Crank")
	void EndGrab(AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Crank")
	float ApplyCrankInput(float AxisInput, float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Crank")
	void ResetCrank(bool bApplyBaseTransform = true);

	UFUNCTION(BlueprintCallable, Category = "Crank")
	void RefreshDrivenMovementBaseLocation();

	UFUNCTION(BlueprintPure, Category = "Crank")
	FVector GetGrabReferenceLocation() const;

	UFUNCTION(BlueprintPure, Category = "Crank")
	FVector GetWorldInputAxisForInteractor(AActor* Interactor) const;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentGrabber = nullptr;

	bool bHasCachedBaseRotation = false;
	bool bHasCachedBaseMovementLocation = false;
	bool bHasCachedBaseDrivenRotation = false;
	FQuat BaseRelativeRotation = FQuat::Identity;
	FQuat BaseWorldRotation = FQuat::Identity;
	FQuat BaseRelativeDrivenRotation = FQuat::Identity;
	FQuat BaseWorldDrivenRotation = FQuat::Identity;
	FVector BaseRelativeMovementLocation = FVector::ZeroVector;
	FVector BaseWorldMovementLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CachedBaseMovementTargetComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CachedBaseDrivenRotationTargetComponent = nullptr;

	USceneComponent* ResolveRotationTargetComponent() const;
	USceneComponent* ResolveDrivenRotationTargetComponent() const;
	USceneComponent* ResolveMovementTargetComponent() const;
	USceneComponent* FindSceneComponentByNameOrTag(FName ComponentName) const;
	void CacheBaseRotationIfNeeded();
	void CacheBaseDrivenRotationIfNeeded();
	void CacheBaseMovementLocationIfNeeded();
	void ApplyRotationAngle(float AngleDegrees);
	void ApplyDrivenRotation(float CrankAngleDegrees);
	void ApplyDrivenMovement(float AngleDegrees);
	float ClampRotationAngle(float AngleDegrees) const;
	float ClampDrivenRotationAngle(float AngleDegrees) const;
	FVector ResolveLocalAxis(EUOUCrankAxisMode AxisMode, const FVector& CustomAxis) const;
	FVector ResolveWorldAxis(EUOUCrankAxisMode AxisMode, const FVector& CustomAxis, const USceneComponent* BasisComponent) const;
};
