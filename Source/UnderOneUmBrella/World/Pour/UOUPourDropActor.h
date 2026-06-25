// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUPourDropActor.generated.h"

class AActor;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class AUOUPourDropActor;

UENUM(BlueprintType)
enum class EUOUPourDropReceiverType : uint8
{
	None,
	PurePourReceiver,
	UmbrellaWaterTarget,
	WaterBasinTarget,
	WaterContainer
};

USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUPourDropContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop", meta = (ClampMin = "0.0"))
	float Volume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop")
	FVector WorldDirection = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop")
	bool bApplyToConnectedWaterBasinGroup = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FUOUPourDropImpactSignature,
	AUOUPourDropActor*, DropActor,
	AActor*, ImpactActor,
	FVector, ImpactLocation,
	EUOUPourDropReceiverType, ReceiverType,
	bool, bDeliveredWater);

// A physical pour payload spawned by the player umbrella.
// It owns the falling visual and converts hit/overlap events into existing pour and water-basin inputs.
UCLASS(meta=(DisplayName="UOU Pour Drop Actor"))
class UNDERONEUMBRELLA_API AUOUPourDropActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUPourDropActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "Pour Drop")
	FUOUPourDropImpactSignature OnPourDropImpacted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop")
	TObjectPtr<USphereComponent> CollisionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop")
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop")
	TObjectPtr<UNiagaraComponent> TrailEffect = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Collision", meta = (ClampMin = "0.0"))
	float CollisionRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Movement", meta = (ClampMin = "0.0"))
	float InitialSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Movement")
	float GravityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Lifetime", meta = (ClampMin = "0.0"))
	float DropLifeSpan = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact")
	bool bDestroyOnFirstValidReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact")
	bool bDestroyOnBlockingHitWithoutReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact")
	bool bSpawnSplashOnlyWhenDelivered = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact")
	TObjectPtr<UNiagaraSystem> ImpactSplashEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact", meta = (ClampMin = "0.0"))
	float ImpactSplashScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop|Runtime")
	float CurrentVolume = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop|Runtime")
	float CurrentDuration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop|Runtime")
	FVector CurrentWorldDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop|Runtime")
	bool bCurrentApplyToConnectedWaterBasinGroup = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop|Runtime")
	EUOUPourDropReceiverType LastReceiverType = EUOUPourDropReceiverType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pour Drop|Runtime")
	bool bHasDeliveredWater = false;

	UFUNCTION(BlueprintCallable, Category = "Pour Drop")
	void InitializePourDrop(const FUOUPourDropContext& DropContext);

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> SourceInstigatorActor = nullptr;

	UPROPERTY(Transient)
	bool bHasInitializedVelocity = false;

	UFUNCTION()
	void HandleCollisionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

	void ApplyCollisionSettings();
	void ApplyMovementSettings();
	void ApplyLaunchVelocity();
	void IgnoreSourceActor();
	void HandleImpact(const FHitResult& ImpactResult, AActor* OtherActor, bool bIsBlockingImpact);
	bool TryDeliverWater(AActor* HitActor, const FVector& ImpactLocation, EUOUPourDropReceiverType& OutReceiverType);
	void SpawnImpactSplash(const FVector& ImpactLocation, const FVector& ImpactNormal, bool bDeliveredWater) const;
	bool ShouldIgnoreActor(const AActor* OtherActor) const;
};
