// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "UOUPushPullInteractorComponent.generated.h"

class AUOUCharacter;
class UUOUInteractionComponent;
class UUOUPushPullObjectComponent;
class UUOUUmbrellaComponent;

// 플레이어의 밀기 당기기 입력과 축 고정 이동을 관리한다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOUPushPullInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPushPullInteractorComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Rules")
	bool bRequireClosedUmbrella = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Rules")
	bool bRequireGrounded = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Rules", meta = (ClampMin = "0.0"))
	float ReleaseDistanceBuffer = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Detection", meta = (ClampMin = "0.0"))
	float CandidateSearchRadius = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Detection")
	bool bDetectWorldDynamic = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Detection")
	bool bDetectPhysicsBody = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Detection")
	bool bDetectPuzzleWeight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Debug")
	bool bShowScreenDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Debug")
	bool bShowWorldDebug = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull|Runtime")
	TObjectPtr<UUOUPushPullObjectComponent> CurrentCandidateObject = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull|Runtime")
	TObjectPtr<UUOUPushPullObjectComponent> GrabbedObject = nullptr;

	UFUNCTION(BlueprintPure, Category = "PushPull")
	bool IsGrabbing() const { return GrabbedObject != nullptr; }

	UFUNCTION(BlueprintPure, Category = "PushPull")
	bool BlocksJumping() const { return IsGrabbing(); }

	UFUNCTION(BlueprintCallable, Category = "PushPull")
	void HandleGrabPressed();

	UFUNCTION(BlueprintCallable, Category = "PushPull")
	void HandleGrabReleased();

	UFUNCTION(BlueprintCallable, Category = "PushPull")
	bool HandleMoveInput(const FVector2D& MovementVector, float MovementYaw);

protected:
	UPROPERTY(Transient)
	TObjectPtr<AUOUCharacter> OwnerCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUInteractionComponent> InteractionComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUUmbrellaComponent> UmbrellaComponent = nullptr;

	UPROPERTY(Transient)
	FVector GrabbedMoveAxis = FVector::ZeroVector;

	UPROPERTY(Transient)
	float CurrentAxisInput = 0.0f;

	UPROPERTY(Transient)
	bool bGrabInputHeld = false;

	UPROPERTY(Transient)
	bool bCachedOrientRotationToMovement = true;

	UPROPERTY(Transient)
	FString LastFailureReason = TEXT("None");

	void RefreshCandidate();
	bool CanUseHands() const;
	bool IsGrabbedObjectTooFar() const;
	void TryBeginGrab();
	void EndGrab();
	void ApplyGrabbedRotation() const;
	void ResolveOwnerReferences();
	void UpdateMovementInputFallback();
	void UpdateScreenDebug() const;
	void DrawWorldDebug() const;
	bool TryResolveGrabAxis(UUOUPushPullObjectComponent* TargetObject, FVector& OutMoveAxis) const;
	UUOUPushPullObjectComponent* FindBestCandidate() const;
	FVector GetDetectionOriginLocation() const;
	static void CheckBetterAxis(const FVector& Axis, const FVector& ToPlayer, FVector& BestAxis, float& BestDot);
	static FVector GetHorizontalAxis(FVector Axis, const FVector& Fallback);
};
