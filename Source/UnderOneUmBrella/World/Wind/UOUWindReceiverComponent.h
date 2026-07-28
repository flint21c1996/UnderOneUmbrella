// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "World/Wind/UOUWindReceivableInterface.h"
#include "UOUWindReceiverComponent.generated.h"

class UPrimitiveComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float CharacterAcceleration = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character", meta = (ClampMin = "0.0"))
	float OpenUmbrellaStrengthMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character", meta = (ClampMin = "0.0"))
	float ClosedUmbrellaStrengthMultiplier = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Character")
	bool bRequireOpenUmbrella = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Physics", meta = (ClampMin = "0.0", Units = "N"))
	float PhysicsForce = 250000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Physics", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent"))
	FComponentReference TargetPrimitiveReference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	FUOUWindExposureData LastWindData;

	virtual void ReceiveWind_Implementation(const FUOUWindExposureData& WindData) override;
	virtual FVector GetWindReceiverLocation_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Wind|Receiver")
	void SetReceiveWind(bool bNewReceiveWind);

private:
	UPrimitiveComponent* ResolveTargetPrimitive() const;
	float ResolveCharacterUmbrellaMultiplier() const;
};
