// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUAudioTriggerActor.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UUOUAudioCueComponent;
class UUOUAudioSubsystem;

UCLASS(meta=(DisplayName="UOU Audio Trigger Actor"))
class UNDERONEUMBRELLA_API AUOUAudioTriggerActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUAudioTriggerActor();

	// Condition Group의 결과 액션을 오디오 재생과 정지로 변환합니다.
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "오디오 이벤트 재생"))
	bool PlayAudioEvent();

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "트리거 재사용 가능 상태로 초기화"))
	void ResetTrigger();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<UBoxComponent> TriggerVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<UUOUAudioCueComponent> AudioCueComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cue", meta = (ToolTip = "AudioCueComponent에 등록된 Cue ID입니다. 값이 있고 Cue를 찾을 수 있으면 이 경로를 먼저 사용합니다."))
	FName AudioCueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "오디오 DataAsset에 등록된 이벤트 ID입니다. Cue 재생에 실패하거나 AudioCueId가 비어 있을 때 fallback으로 사용합니다. 예: BGM.InGame, Ambience.Campfire"))
	FName AudioEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "관리형 루프 사운드를 구분하는 인스턴스 ID입니다. 비워두면 이 액터 이름을 사용합니다. 같은 환경음을 여러 곳에 배치할 때 중복 정지를 막는 용도입니다."))
	FName AudioInstanceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "BeginPlay에서 자동으로 오디오 이벤트를 재생합니다. 맵 시작 BGM이나 항상 켜지는 환경음에 사용합니다."))
	bool bPlayOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "액터가 트리거 볼륨에 들어왔을 때 오디오 이벤트를 재생합니다."))
	bool bPlayOnActorEnter = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "액터가 트리거 볼륨에서 나갔을 때 같은 오디오 이벤트를 정지합니다. 관리형 루프 환경음에 사용합니다."))
	bool bStopOnActorExit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "이 트리거 액터가 제거될 때 관리 중인 오디오 이벤트를 정지합니다. 레벨 전환 시 환경음이 남지 않게 하는 용도입니다."))
	bool bStopOnEndPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "켜져 있으면 플레이어가 조종 중인 Pawn만 트리거를 발동합니다."))
	bool bPlayerOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "켜져 있으면 첫 재생 이후 같은 트리거가 다시 재생되지 않습니다."))
	bool bPlayOnlyOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ToolTip = "오디오 트리거 박스의 절반 크기입니다."))
	FVector TriggerExtent = FVector(150.0f, 150.0f, 100.0f);

	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTriggerEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	void ApplyTriggerSettings();
	bool ShouldAcceptTriggerActor(const AActor* OtherActor) const;
	bool StopAudioEvent(float OverrideFadeOutTime = -1.0f);
	FName GetResolvedAudioInstanceId() const;
	UUOUAudioSubsystem* GetAudioSubsystem() const;

	bool bHasPlayed = false;
	bool bIsAudioPlaybackActive = false;
};
