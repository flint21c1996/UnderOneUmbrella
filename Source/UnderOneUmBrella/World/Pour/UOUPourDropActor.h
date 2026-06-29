// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Pour/UOUPourDropTypes.h"
#include "UOUPourDropActor.generated.h"

class AActor;
class UMaterialInterface;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UNiagaraComponent;
class UNiagaraSystem;
class AUOUPourDropActor;
class UUOUWaterBasinTargetComponent;

USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUPourDropContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop", meta = (ClampMin = "0.0", ToolTip = "이 DropActor가 충돌/겹침으로 전달할 내용물 양입니다."))
	float Volume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop", meta = (ClampMin = "0.0", ToolTip = "이 DropActor가 대표하는 붓기 지속 시간입니다. 수신 대상의 반응 계산에 사용됩니다."))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop", meta = (ToolTip = "붓기 방향입니다. 수직 낙하를 사용해도 impact/ripple 방향 정보로 전달됩니다."))
	FVector WorldDirection = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop", meta = (ToolTip = "이 붓기를 발생시킨 액터입니다. 자기 자신과 충돌하지 않도록 무시 대상에도 사용됩니다."))
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop")
	bool bApplyToConnectedWaterBasinGroup = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Visual")
	FUOUPourDropVisualSettings VisualSettings;
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
	virtual void Tick(float DeltaTime) override;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Visual", meta = (ToolTip = "디버그용 DropActor mesh입니다. 실제 붓기 시각 표현은 Stream Visual이 담당하므로 기본적으로 숨겨집니다."))
	TObjectPtr<UStaticMesh> VisualMeshAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Visual")
	TArray<TObjectPtr<UMaterialInterface>> VisualMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Visual")
	FVector VisualMeshRelativeScale = FVector(0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Visual")
	FVector VisualMeshRelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Visual")
	FRotator VisualMeshRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Debug", meta = (ToolTip = "켜면 DropActor의 mesh를 디버그용으로 표시합니다. 꺼져 있어도 충돌/물 전달 판정은 계속 동작합니다."))
	bool bShowDebugVisualMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Debug", meta = (ToolTip = "켜면 DropActor의 실제 충돌 판정 구체를 월드 디버그로 표시합니다. Debug Controller의 Player World Draw가 켜져 있어야 보입니다."))
	bool bDrawDebugCollisionRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Visual", meta = (ToolTip = "DropActor에 붙일 보조 trail Niagara입니다. 기본 물줄기 표현은 Stream Visual이 담당합니다."))
	TObjectPtr<UNiagaraSystem> TrailEffectAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Visual")
	bool bActivateTrailEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Collision", meta = (ClampMin = "0.0", ToolTip = "DropActor의 충돌/겹침 판정 구체 반지름입니다."))
	float CollisionRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Collision", meta = (ClampMin = "0.0", ToolTip = "WaterBasinTarget 수면 근처에 도달했는지 판단할 때 허용하는 수직 오차입니다."))
	float WaterBasinDeliveryVerticalTolerance = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Movement", meta = (ClampMin = "0.0"))
	float InitialSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Movement")
	float GravityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Movement", meta = (ToolTip = "켜면 입력 방향으로 발사하지 않고 소켓 위치에서 바로 수직 하강합니다. 현재 붓기 판정 기본 방식입니다."))
	bool bUseVerticalDescent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Lifetime", meta = (ClampMin = "0.0", ToolTip = "충돌하지 않았을 때 DropActor가 자동 제거되기까지의 시간입니다. 0이면 자동 수명 제거를 사용하지 않습니다."))
	float DropLifeSpan = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact", meta = (ToolTip = "켜면 유효한 수신 대상에 물을 전달한 뒤 DropActor를 즉시 제거합니다."))
	bool bDestroyOnFirstValidReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact", meta = (ToolTip = "켜면 물을 전달하지 못한 blocking hit에서도 DropActor를 제거합니다."))
	bool bDestroyOnBlockingHitWithoutReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact", meta = (ToolTip = "켜면 실제로 물을 전달한 impact에서만 splash를 재생합니다."))
	bool bSpawnSplashOnlyWhenDelivered = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact", meta = (ToolTip = "DropActor impact 시 재생할 splash Niagara입니다. 각 impact 인스턴스별로 한 번 재생되는 one-shot 에셋을 권장합니다."))
	TObjectPtr<UNiagaraSystem> ImpactSplashEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Drop|Impact", meta = (ClampMin = "0.0", ToolTip = "Impact splash Niagara의 월드 스케일입니다."))
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
	void ApplyVisualSettings();
	void ApplyContextVisualSettings(const FUOUPourDropVisualSettings& VisualSettings);
	void ApplyMovementSettings();
	void ApplyLaunchVelocity();
	FVector ResolveLaunchDirection() const;
	void IgnoreSourceActor();
	void DrawDebugCollisionRadius() const;
	void HandleImpact(const FHitResult& ImpactResult, AActor* OtherActor, bool bIsBlockingImpact);
	bool TryDeliverWater(AActor* HitActor, const FVector& ImpactLocation, EUOUPourDropReceiverType& OutReceiverType);
	bool TryDeliverWaterToBasinAtLocation(const FVector& ImpactLocation, EUOUPourDropReceiverType& OutReceiverType, AActor*& OutReceiverActor);
	bool IsWaterBasinDeliveryLocation(const UUOUWaterBasinTargetComponent* WaterBasinTarget, const FVector& ImpactLocation) const;
	bool ShouldHandleImpactSplashAtSource(bool bDeliveredWater) const;
	void SpawnImpactSplash(const FVector& ImpactLocation, const FVector& ImpactNormal, bool bDeliveredWater) const;
	bool ShouldIgnoreActor(const AActor* OtherActor) const;
};
