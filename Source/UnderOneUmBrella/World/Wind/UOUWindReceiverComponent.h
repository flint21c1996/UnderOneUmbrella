// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "World/Wind/UOUWindReceivableInterface.h"
#include "UOUWindReceiverComponent.generated.h"

class UPrimitiveComponent;
class UCharacterMovementComponent;

// 캐릭터나 물리 오브젝트에 붙여 동적 바람의 힘을 실제 이동으로 변환합니다.
UCLASS(ClassGroup=(Wind), meta=(BlueprintSpawnableComponent, DisplayName="UOU Wind Receiver"))
class UNDERONEUMBRELLA_API UUOUWindReceiverComponent
	: public UActorComponent
	, public IUOUWindReceivableInterface
{
	GENERATED_BODY()

public:
	UUOUWindReceiverComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Receiver")
	bool bReceiveWind = true;

	// Emitter의 실제 바람 가속도에 같은 방향으로 더하는 캐릭터별 보정값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float AdditionalCharacterWindAcceleration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character", meta = (ClampMin = "0.0"))
	float OpenUmbrellaStrengthMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character", meta = (ClampMin = "0.0"))
	float ClosedUmbrellaStrengthMultiplier = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character")
	bool bRequireOpenUmbrella = false;

	// 켜면 걷거나 착지한 캐릭터는 영향을 받지 않고 Falling 상태에서만 바람을 탑니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character|Flight")
	bool bOnlyAffectFallingCharacter = true;

	// 펼친 우산으로 공중 바람을 받는 동안 중력과 반대되는 힘을 더해 낙하를 막습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character|Flight")
	bool bCancelGravityWhileUmbrellaOpen = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character|Flight", meta = (EditCondition = "bCancelGravityWhileUmbrellaOpen", ClampMin = "0.0"))
	float GravityCancellationMultiplier = 1.0f;

	// 새 바람 구간에 진입할 때 기존 속도 벡터 중 유지할 비율입니다.
	// 최초 진입뿐 아니라 반사된 바람 구간으로 넘어갈 때도 적용됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character|Flight", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExistingVelocityRetentionOnWindEntry = 0.15f;

	// 바람을 타는 동안 무입력 공중 제동을 끄고, 바람이 끝나면 원래 값으로 복구합니다.
	// 기존 WASD 공중 조향은 바람 가속 위에 별도로 더해집니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character|Flight")
	bool bSuppressFallingBrakingWhileWindborne = true;

	// 바람 탑승 중 A/D 좌우와 W/S 상하 조향에 더할 가속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character|Flight", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float WindborneSteeringAcceleration = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Physics", meta = (ClampMin = "0.0", Units = "N"))
	float PhysicsForce = 250000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Physics", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent"))
	FComponentReference TargetPrimitiveReference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	FUOUWindExposureData LastWindData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	float LastAppliedCharacterAcceleration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	float LastUmbrellaStrengthMultiplier = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	float LastGravityCancellationAcceleration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime", meta = (Units = "cm/s"))
	float LastWindEntrySpeed = 0.0f;

	virtual void ReceiveWind_Implementation(const FUOUWindExposureData& WindData) override;
	virtual FVector GetWindReceiverLocation_Implementation() const override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Wind|Receiver")
	void SetReceiveWind(bool bNewReceiveWind);

	// 바람 탑승 중이면 입력을 좌우(XY 평면)/상하(Z) 조향 힘으로 적용하고 true를 반환합니다.
	bool ApplyWindborneSteeringInput(
		const FVector2D& MovementInput,
		float MovementYaw);

private:
	UPrimitiveComponent* ResolveTargetPrimitive() const;
	float ResolveCharacterUmbrellaMultiplier(bool& bOutUmbrellaOpen) const;
	void BeginWindborneMovement(
		UCharacterMovementComponent* MovementComponent,
		const FUOUWindExposureData& WindData);
	void RestoreFallingBraking();

	TWeakObjectPtr<UCharacterMovementComponent> WindborneMovementComponent;
	TWeakObjectPtr<AActor> ActiveWindSourceActor;
	float CachedBrakingDecelerationFalling = 0.0f;
	float LastCharacterWindReceiveTime = -1.0f;
	int32 ActiveWindReflectionIndex = INDEX_NONE;
	bool bCharacterWindActive = false;
	bool bHasCachedFallingBraking = false;
};
