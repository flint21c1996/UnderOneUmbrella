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

	// 자동 착시 이동도 수동 트리거의 재진입 잠금을 공유하여 왕복 루프를 방지한다.
	static bool TeleportForIllusion(AActor* Actor, const FVector& Location, bool bPreserveCamera = true);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stage|Transition|Camera Angle", meta = (DisplayName = "현재 카메라 각도 가져오기", ToolTip = "플레이 중에는 게임 카메라, 편집 중에는 현재 레벨 뷰포트 각도를 저장하고 각도 제한을 켭니다. PIE에서 변경한 값은 종료 시 유지되지 않습니다."))
	void CaptureCurrentCameraAngle();

	UFUNCTION(BlueprintPure, Category = "Stage|Transition|Camera Angle")
	bool IsCameraRotationAllowed(FRotator CameraRotation) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Transition|Camera Angle", meta = (DisplayName = "카메라 각도 제한 사용"))
	bool bRestrictCameraAngle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Transition|Camera Angle", meta = (DisplayName = "허용 카메라 각도", EditCondition = "bRestrictCameraAngle", ToolTip = "월드 기준 카메라 회전입니다. 기본적으로 Yaw만 검사하며 Roll은 검사하지 않습니다."))
	FRotator AllowedCameraRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Transition|Camera Angle", meta = (DisplayName = "각도 허용 오차", EditCondition = "bRestrictCameraAngle", ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float CameraAngleTolerance = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Transition|Camera Angle", meta = (DisplayName = "Pitch도 검사", EditCondition = "bRestrictCameraAngle"))
	bool bCheckCameraPitch = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	TObjectPtr<UBoxComponent> TriggerVolume = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage|Transition", meta = (ToolTip = "일반 텔레포트의 도착 지점입니다. 텔레포트 대상 액터를 지정한 경우에는 대상 복귀 위치를 사용합니다."))
	TObjectPtr<AActor> DestinationActor = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage|Transition", meta = (DisplayName = "텔레포트 대상 액터", ToolTip = "지정하면 이 액터가 트리거 영역에 진입했을 때만 대상 복귀 위치로 이동시킵니다. 비워두면 기존 트리거 조건과 Destination Actor를 사용합니다."))
	TObjectPtr<AActor> TeleportTargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition", meta = (DisplayName = "대상 복귀 위치", ToolTip = "텔레포트 대상 액터를 되돌릴 절대 월드 좌표입니다."))
	FVector TeleportTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition", meta = (DisplayName = "카메라 페이드 사용", ToolTip = "일반 텔레포트에서 카메라 페이드를 사용할지 결정합니다. 텔레포트 대상 액터를 지정한 경우에는 항상 페이드 없이 즉시 복귀합니다."))
	bool bUseCameraFade = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	bool bTriggerOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	bool bPlayerOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	bool bUseDestinationRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition")
	bool bStopMovementOnTeleport = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Transition", meta = (DisplayName = "즉시 이동 시 카메라 구도 유지", ToolTip = "페이드와 도착 회전을 사용하지 않는 일반 순간이동에서 이동 거리만큼 카메라 추적을 보정합니다. Q/E 회전 중심도 유지합니다."))
	bool bPreserveCameraOnInstantTeleport = true;

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
	UFUNCTION()
	void HandleTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	// 도착 잠금은 액터별로 관리하여 다른 플레이어의 진입까지 막지 않는다.
	TSet<TWeakObjectPtr<AActor>> ArrivalBlockedActors;
	TSet<TWeakObjectPtr<AActor>> TeleportingActors;
	void ReleaseArrivalLockIfOutside(TWeakObjectPtr<AActor> Actor);

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
