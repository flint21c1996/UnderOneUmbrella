// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "UOULightInteractionSurfaceComponent.generated.h"

UENUM(BlueprintType)
enum class EUOULightInteractionMode : uint8
{
	Disabled,
	Blocking,
	Reflecting
};

UENUM(BlueprintType)
enum class EUOULightReflectionNormalMode : uint8
{
	HitNormal,
	ComponentUp,
	ComponentForward
};

UENUM(BlueprintType)
enum class EUOULightReflectionDirectionMode : uint8
{
	MirrorByNormal,
	ComponentForward,
	OwnerForward
};

UCLASS(ClassGroup=(Light), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Interaction Surface"))
class UUOULightInteractionSurfaceComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UUOULightInteractionSurfaceComponent();

	virtual void OnRegister() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Interaction")
	EUOULightInteractionMode LightInteractionMode = EUOULightInteractionMode::Blocking;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection")
	EUOULightReflectionNormalMode ReflectionNormalMode = EUOULightReflectionNormalMode::HitNormal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection")
	EUOULightReflectionDirectionMode ReflectionDirectionMode = EUOULightReflectionDirectionMode::MirrorByNormal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0"))
	float ReflectionRange = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float ReflectionConeAngle = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0"))
	float ReflectionIntensityMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0"))
	float ReflectionStartPadding = 4.0f;

	UFUNCTION(BlueprintCallable, Category = "Light|Interaction")
	void SetLightInteractionMode(EUOULightInteractionMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Light|Interaction")
	bool CanBlockLight() const;

	UFUNCTION(BlueprintPure, Category = "Light|Interaction")
	bool CanReflectLight() const;

	UFUNCTION(BlueprintPure, Category = "Light|Reflection")
	FVector GetReflectionDirection(const FVector& IncomingDirection, const FVector& HitNormal) const;

protected:
	void ValidateSettings();
	void ApplyCollisionSettings();
	FVector GetReflectionNormal(const FVector& HitNormal) const;
};
