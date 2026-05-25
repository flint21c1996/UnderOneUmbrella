// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUAudioTriggerActor.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;

UCLASS(meta=(DisplayName="UOU Audio Trigger Actor"))
class UNDERONEUMBRELLA_API AUOUAudioTriggerActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUAudioTriggerActor();

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "오디오 이벤트 재생"))
	bool PlayAudioEvent();

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "트리거 재사용 가능 상태로 초기화"))
	void ResetTrigger();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<UBoxComponent> TriggerVolume = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "오디오 DataAsset에 등록된 이벤트 ID입니다. 예: BGM.InGame, Ambience.Campfire"))
	FName AudioEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "BeginPlay에서 자동으로 오디오 이벤트를 재생합니다. 맵 시작 BGM이나 항상 켜지는 환경음에 사용합니다."))
	bool bPlayOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ToolTip = "액터가 트리거 볼륨에 들어왔을 때 오디오 이벤트를 재생합니다."))
	bool bPlayOnActorEnter = true;

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

private:
	void ApplyTriggerSettings();
	bool ShouldAcceptTriggerActor(const AActor* OtherActor) const;

	bool bHasPlayed = false;
};
