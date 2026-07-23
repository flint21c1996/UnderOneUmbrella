// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOULadderClimbComponent.generated.h"

class AUOUCharacter;
class AUOULadderActor;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EUOULadderClimbState : uint8
{
	None,
	EnteringFromBottom,
	EnteringFromTop,
	Climbing,
	ExitingAtBottom,
	ExitingAtTop,
	JumpingOff
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUOULadderStateChangedSignature,
	EUOULadderClimbState, PreviousState,
	EUOULadderClimbState, NewState);

// Owns automatic ladder entry, constrained climbing movement, and ladder exit transitions.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOULadderClimbComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOULadderClimbComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Consumes movement while climbing and starts climbing when movement points into a nearby ladder.
	UFUNCTION(BlueprintCallable, Category = "Ladder")
	bool HandleMoveInput(const FVector2D& MovementVector, float MovementYaw);

	// Returns true when the jump was consumed as a ladder jump-off.
	UFUNCTION(BlueprintCallable, Category = "Ladder")
	bool HandleJumpInput();

	UFUNCTION(BlueprintPure, Category = "Ladder")
	bool IsClimbing() const { return ClimbState != EUOULadderClimbState::None; }

	UFUNCTION(BlueprintPure, Category = "Ladder")
	EUOULadderClimbState GetClimbState() const { return ClimbState; }

	UFUNCTION(BlueprintPure, Category = "Ladder")
	float GetClimbInput() const { return CurrentClimbInput; }

	UFUNCTION(BlueprintPure, Category = "Ladder")
	float GetNormalizedHeight() const { return NormalizedHeight; }

	UFUNCTION(BlueprintPure, Category = "Ladder")
	AUOULadderActor* GetCurrentLadder() const { return CurrentLadder; }

	UPROPERTY(BlueprintAssignable, Category = "Ladder")
	FUOULadderStateChangedSignature OnLadderStateChanged;

protected:
	// Minimum world-direction agreement required before movement automatically enters a ladder.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Entry", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float EntryDirectionDotThreshold = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Entry", meta=(ClampMin="0.05"))
	float EntryTransitionDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Exit", meta=(ClampMin="0.05"))
	float ExitTransitionDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Movement", meta=(ClampMin="0.0"))
	float AlignmentStrength = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Movement", meta=(ClampMin="0.0"))
	float MaximumAlignmentSpeed = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Movement", meta=(ClampMin="0.0"))
	float RotationInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Exit", meta=(ClampMin="0.0"))
	float JumpOffHorizontalSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Exit", meta=(ClampMin="0.0"))
	float JumpOffVerticalSpeed = 360.0f;

private:
	UFUNCTION()
	void HandleCapsuleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleCapsuleEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	AUOULadderActor* FindEntryLadder(const FVector& DesiredWorldDirection, bool& bOutEnterFromTop) const;
	void BeginClimbing(AUOULadderActor* Ladder, bool bEnterFromTop, float InputMagnitude);
	void BeginExit(bool bExitAtTop);
	void FinishExit();
	void JumpOffLadder();
	void UpdateTransition(float DeltaTime);
	void UpdateClimbing(float DeltaTime);
	void SetClimbState(EUOULadderClimbState NewState);
	void RestoreMovementMode(bool bForceFalling);

	UPROPERTY(Transient)
	TObjectPtr<AUOUCharacter> OwnerCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AUOULadderActor> CurrentLadder = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AUOULadderActor>> NearbyLadders;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ladder|Runtime", meta=(AllowPrivateAccess="true"))
	EUOULadderClimbState ClimbState = EUOULadderClimbState::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ladder|Runtime", meta=(AllowPrivateAccess="true"))
	float CurrentClimbInput = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ladder|Runtime", meta=(AllowPrivateAccess="true"))
	float NormalizedHeight = 0.0f;

	FVector TransitionStartLocation = FVector::ZeroVector;
	FVector TransitionTargetLocation = FVector::ZeroVector;
	FRotator TransitionStartRotation = FRotator::ZeroRotator;
	FRotator TransitionTargetRotation = FRotator::ZeroRotator;
	float TransitionElapsed = 0.0f;
	float TransitionDuration = 0.0f;
	uint8 SavedMovementMode = 0;
	uint8 SavedCustomMovementMode = 0;
	bool bSavedOrientRotationToMovement = true;
	bool bWaitForTopEntryInputRelease = false;
};
