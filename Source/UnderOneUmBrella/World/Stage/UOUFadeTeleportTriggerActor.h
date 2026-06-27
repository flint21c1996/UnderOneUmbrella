// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "UI/UOUTransitionMessagePresenter.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition|Message", meta = (DisplayName = "페이드 아웃 문구", ToolTip = "위치 이동 전 화면이 검게 가려진 뒤 표시할 문구입니다. 비워두면 표시하지 않습니다."))
	FUOUTransitionMessageSettings FadeOutMessageSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition|Message", meta = (DisplayName = "페이드 인 문구", ToolTip = "위치 이동 후 화면이 밝아지는 동안 표시할 문구입니다. 비워두면 표시하지 않습니다."))
	FUOUTransitionMessageSettings FadeInMessageSettings;

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
	void ShowTransitionMessage(const FUOUTransitionMessageSettings& MessageSettings);
	void HideTransitionMessage();
	bool TeleportPendingActor();
	void StopActorMovement(AActor* TargetActor) const;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PendingTransitionActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> PendingPlayerController = nullptr;

	FTimerHandle FadeOutTimerHandle;
	FTimerHandle BlackHoldTimerHandle;
	FTimerHandle FadeInTimerHandle;
	FUOUTransitionMessagePresenter TransitionMessagePresenter;

	bool bHasTriggered = false;
	bool bIsTransitioning = false;
};
