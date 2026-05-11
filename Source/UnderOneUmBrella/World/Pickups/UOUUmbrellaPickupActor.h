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

// 이 클래스는 바닥에 배치한 우산을 플레이어가 줍고 손에 들 수 있게 만드는 픽업 액터를 담당한다.
UCLASS(meta=(DisplayName="UOU Umbrella Pickup"))
class AUOUUmbrellaPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUUmbrellaPickupActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USphereComponent> PickupTrigger = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "0.0"))
	float TriggerRadius = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	bool bDestroyOnPickup = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	bool bUseSimpleHoverMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FVector MeshRelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FRotator MeshRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FVector MeshRelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "0.0"))
	float HoverAmplitude = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "0.0"))
	float HoverSpeed = 2.0f;

	UFUNCTION()
	void HandlePickupTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void ApplyVisualMeshPlacement();
	bool TryGiveUmbrellaToActor(AActor* OtherActor);
};
