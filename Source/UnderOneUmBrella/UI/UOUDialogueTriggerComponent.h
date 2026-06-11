// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "UOUDialogueTriggerComponent.generated.h"

class UUOUDialogueSourceComponent;
class UUOUUmbrellaComponent;
class UUOUCameraControllerComponent;
class UUOUUISubsystem;
class APlayerController;

// 플레이어가 가까이 왔을 때 대화 소스를 실행하는 트리거 컴포넌트입니다.
// 필요하면 우산을 펼친 채 일정 시간 동안 대상을 씌웠는지도 같이 검사합니다.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Trigger"))
class UNDERONEUMBRELLA_API UUOUDialogueTriggerComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UUOUDialogueTriggerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 지정한 액터를 대화 시작자로 보고 연결된 대화 소스를 실행합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Trigger")
	bool TryStartDialogue(AActor* InstigatorActor);

	// 한 번만 실행 옵션으로 막힌 상태를 풀어서 다시 대화가 시작될 수 있게 합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Trigger")
	void ResetTrigger();

	// 실행할 대화 소스입니다. 비어 있으면 이 컴포넌트가 붙은 액터에서 대화 소스를 찾습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Trigger")
	TObjectPtr<UUOUDialogueSourceComponent> DialogueSource = nullptr;

	// 켜져 있으면 Pawn 계열 액터만 대화를 시작할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bOnlyPawn = true;

	// 켜져 있으면 ResetTrigger가 호출되기 전까지 한 번만 실행됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bTriggerOnce = true;

	// 켜져 있으면 대화 시작자가 우산 컴포넌트를 가지고 있고 실제 우산도 소유해야 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bRequireUmbrella = true;

	// 켜져 있으면 우산이 펼쳐졌거나 뒤집힌 상태여야 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules", meta = (EditCondition = "bRequireUmbrella"))
	bool bRequireOpenUmbrella = true;

	// 켜져 있으면 우산이 현재 비 차단 상태여야 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules", meta = (EditCondition = "bRequireUmbrella"))
	bool bRequireBlockingRain = false;

	// 켜져 있으면 대화 대상이 플레이어 우산의 비 차단 박스 안에 일정 시간 머물러야 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Umbrella Cover", meta = (EditCondition = "bRequireUmbrella"))
	bool bRequireUmbrellaCoverHold = false;

	// 우산으로 씌워준 상태를 유지해야 하는 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Umbrella Cover", meta = (EditCondition = "bRequireUmbrellaCoverHold", ClampMin = "0.0"))
	float RequiredCoverDuration = 2.5f;

	// 대화 대상 위치에서 어느 지점을 우산으로 씌울지 보정하는 월드 오프셋입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Umbrella Cover", meta = (EditCondition = "bRequireUmbrellaCoverHold"))
	FVector CoverTargetOffset = FVector(0.0f, 0.0f, 90.0f);

	// PIE에서 우산 커버 판정과 대화 시작 상태를 화면에 띄웁니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Debug")
	bool bShowUmbrellaCoverDebug = true;

	// 켜져 있으면 대화 줌 카메라가 켜져 있는 동안 플레이어 이동 입력을 막습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Control")
	bool bLockMovementDuringDialogueFocus = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	bool bHasTriggered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	float CurrentCoverHoldTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	TObjectPtr<AActor> CurrentInstigatorActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	bool bIsCurrentlyCoveredByUmbrella = false;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void HandleDialogueEnded();

	UUOUDialogueSourceComponent* ResolveDialogueSource() const;
	bool PassesInstigatorRules(AActor* InstigatorActor) const;
	UUOUUmbrellaComponent* FindUmbrellaComponent(AActor* InstigatorActor) const;
	bool CanTrackOverlapActor(AActor* InstigatorActor) const;
	bool IsOwnerCoveredByUmbrella(const UUOUUmbrellaComponent& UmbrellaComponent, FString* OutDebugDetails = nullptr) const;
	UUOUUISubsystem* ResolveUISubsystem() const;
	UUOUCameraControllerComponent* FindCameraControllerComponent(AActor* InstigatorActor) const;
	void StartDialogueCameraFocus(AActor* InstigatorActor, AActor* SpeakerActor);
	void StopDialogueCameraFocus();
	void LockMovementForDialogue(AActor* InstigatorActor);
	void UnlockMovementForDialogue();
	void ClearCoverProgress();
	void ShowCoverDebugMessage(const FString& Message, const FColor& Color, float Duration = 1.5f) const;
	void ShowCoverDebugStatus(const FString& Message, const FColor& Color) const;

	UPROPERTY(Transient)
	TObjectPtr<UUOUCameraControllerComponent> ActiveDialogueCameraController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUUISubsystem> BoundUISubsystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> LockedMovementPlayerController = nullptr;

	bool bDialogueMovementLocked = false;
};
