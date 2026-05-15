// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUInteractionComponent.generated.h"

class UPrimitiveComponent;
class USceneComponent;

// 플레이어 앞쪽에서 상호작용 가능한 후보를 찾는 범용 탐지 컴포넌트다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOUInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 탐지 거리와 반경 기본값을 설정한다.
	UUOUInteractionComponent();

	// 시작할 때 기준 컴포넌트와 현재 후보를 초기화한다.
	virtual void BeginPlay() override;

	// 상황에 따라 상호작용 전체를 잠시 끄고 켤 수 있게 하는 플래그다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bInteractionEnabled = true;

	// 앞쪽으로 얼마나 멀리 탐지할지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionRange = 150.0f;

	// 스윕할 구체 반경을 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionProbeRadius = 32.0f;

	// 기준점에서 얼마나 앞과 위로 보정해서 탐지할지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FVector DetectionOffset = FVector(50.0f, 0.0f, 40.0f);

	// 탐지 시작점을 별도 컴포넌트 기준으로 맞추기 위한 참조다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USceneComponent> DetectionOrigin = nullptr;

	// 가장 최근에 탐지한 상호작용 후보 컴포넌트다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPrimitiveComponent> CurrentCandidateComponent = nullptr;

	// 상호작용 활성 상태를 외부에서 바꾸기 위한 함수다.
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

	// 현재 위치를 기준으로 상호작용 후보를 다시 찾는다.
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RefreshCandidate();

protected:
	// 실제 탐지 시작 위치를 계산한다.
	FVector GetDetectionStart() const;

	// 실제 탐지 끝 위치를 계산한다.
	FVector GetDetectionEnd() const;
};
