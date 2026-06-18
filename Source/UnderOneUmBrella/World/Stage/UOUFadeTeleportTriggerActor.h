// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "UOUFadeTeleportTriggerActor.generated.h"

class APlayerController;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;

UCLASS(meta=(DisplayName="UOU Fade Teleport Trigger Actor"))
class UNDERONEUMBRELLA_API AUOUFadeTeleportTriggerActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUFadeTeleportTriggerActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Stage|Transition")
	bool TriggerTransition(AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category = "Stage|Transition")
	void ResetTrigger();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	TObjectPtr<UBoxComponent> TriggerVolume = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage|Transition")
	TObjectPtr<AActor> DestinationActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	bool bTriggerOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	bool bPlayerOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	bool bUseDestinationRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	bool bStopMovementOnTeleport = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition", meta = (ClampMin = "0.0"))
	float BlackHoldDuration = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition", meta = (ClampMin = "0.0"))
	float FadeInDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	FLinearColor FadeColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition", meta = (ClampMin = "0.0"))
	FVector TriggerExtent = FVector(150.0f, 150.0f, 100.0f);

protected:
	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

private:
	void ApplyTriggerSettings();
	bool ShouldAcceptTriggerActor(const AActor* OtherActor) const;
	APlayerController* ResolvePlayerController(AActor* InstigatorActor) const;
	void FinishFadeOut();
	void StartFadeIn();
	void FinishTransition();
	bool TeleportPendingActor();
	void StopActorMovement(AActor* TargetActor) const;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PendingTransitionActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> PendingPlayerController = nullptr;

	FTimerHandle FadeOutTimerHandle;
	FTimerHandle BlackHoldTimerHandle;
	FTimerHandle FadeInTimerHandle;

	bool bHasTriggered = false;
	bool bIsTransitioning = false;
};
