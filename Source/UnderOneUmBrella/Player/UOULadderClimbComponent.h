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

	// 기존 블루프린트 설정값 호환을 위해 이름은 유지하며, 아래쪽에서 올라갈 때 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Entry", meta=(ClampMin="0.05", DisplayName="아래쪽 진입 전환 시간", ToolTip="사다리 아래쪽에서 올라가기 시작할 때 등반 위치로 이동하는 시간입니다."))
	float EntryTransitionDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Entry", meta=(ClampMin="0.05", DisplayName="위쪽 진입 전환 시간", ToolTip="사다리 위쪽에서 내려가기 시작할 때 등반 위치로 이동하는 시간입니다."))
	float TopEntryTransitionDuration = 1.3f;

	// 기존 블루프린트 설정값 호환을 위해 이름은 유지하며, 아래쪽으로 나갈 때 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Exit", meta=(ClampMin="0.05", DisplayName="아래쪽 퇴장 전환 시간", ToolTip="사다리 아래쪽으로 내려온 뒤 바닥 위치로 이동하는 시간입니다."))
	float ExitTransitionDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Exit", meta=(ClampMin="0.05", DisplayName="위쪽 퇴장 전환 시간", ToolTip="사다리 꼭대기에서 플랫폼 위로 올라가는 시간입니다."))
	float TopExitTransitionDuration = 1.f;

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
	void BeginClimbing(AUOULadderActor* Ladder, bool bEnterFromTop);
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
	// 사다리 퇴장에 사용한 이동 입력이 해제되기 전까지 자동 재진입을 막는다.
	bool bBlockExitReentryUntilInputRelease = false;
};
