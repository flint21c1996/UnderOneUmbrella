// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUUmbrellaPickupActor.generated.h"

class AActor;
class UPrimitiveComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

// 바닥에 배치된 우산 픽업 액터입니다.
// 플레이어가 겹치면 우산을 지급하고 필요하면 자기 자신을 제거합니다.
UCLASS(meta=(DisplayName="UOU Umbrella Pickup"))
class AUOUUmbrellaPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUUmbrellaPickupActor();

protected:
	// 시작 시 픽업 트리거를 초기화합니다.
	virtual void BeginPlay() override;

	// 호버 연출이 켜져 있을 때 메쉬 위치를 매 틱 갱신합니다.
	virtual void Tick(float DeltaSeconds) override;

	// 에디터에서 반지름이나 메쉬 배치 값이 바뀌면 외형을 다시 맞춥니다.
	virtual void OnConstruction(const FTransform& Transform) override;

	// 픽업 액터 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 플레이어와 겹침을 감지하는 구형 트리거입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USphereComponent> PickupTrigger = nullptr;

	// 바닥에 보이는 우산 비주얼 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	// 픽업 판정 반경입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "0.0"))
	float TriggerRadius = 75.0f;

	// 우산 지급 후 액터를 제거할지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	bool bDestroyOnPickup = true;

	// 가벼운 호버 연출을 사용할지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	bool bUseSimpleHoverMotion = true;

	// 메쉬를 루트 기준으로 미세 조정할 위치 오프셋입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FVector MeshRelativeOffset = FVector::ZeroVector;

	// 메쉬를 루트 기준으로 미세 조정할 회전값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FRotator MeshRelativeRotation = FRotator::ZeroRotator;

	// 메쉬를 루트 기준으로 미세 조정할 스케일입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FVector MeshRelativeScale = FVector::OneVector;

	// 호버 연출의 세로 진폭입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "0.0"))
	float HoverAmplitude = 8.0f;

	// 호버 연출 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "0.0"))
	float HoverSpeed = 2.0f;

	// 픽업 트리거에 플레이어가 들어왔을 때 우산 지급을 시도합니다.
	UFUNCTION()
	void HandlePickupTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// 메쉬 위치와 회전을 현재 설정값에 맞게 다시 적용합니다.
	void ApplyVisualMeshPlacement();

	// 겹친 액터에게 실제로 우산을 줄 수 있는지 검사하고 지급합니다.
	bool TryGiveUmbrellaToActor(AActor* OtherActor);
};
