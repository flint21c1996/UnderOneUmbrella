// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "UOULightInteractionSurfaceComponent.generated.h"

UENUM(BlueprintType)
enum class EUOULightInteractionMode : uint8
{
	Disabled UMETA(DisplayName = "Disabled", ToolTip = "게임플레이 빛 상호작용을 무시합니다."),
	Blocking UMETA(DisplayName = "Blocking", ToolTip = "게임플레이 빛 트레이스를 차단합니다."),
	Reflecting UMETA(DisplayName = "Reflecting", ToolTip = "들어오는 빛을 차단하고 반사된 게임플레이 빛을 발생시킵니다.")
};

UENUM(BlueprintType)
enum class EUOULightReflectionNormalMode : uint8
{
	HitNormal UMETA(DisplayName = "Hit Normal", ToolTip = "트레이스 충돌 지점의 노멀을 반사 노멀로 사용합니다."),
	ComponentUp UMETA(DisplayName = "Component Up", ToolTip = "이 컴포넌트의 Up 벡터를 반사 노멀로 사용합니다."),
	ComponentForward UMETA(DisplayName = "Component Forward", ToolTip = "이 컴포넌트의 Forward 벡터를 반사 노멀로 사용합니다.")
};

UENUM(BlueprintType)
enum class EUOULightReflectionDirectionMode : uint8
{
	MirrorByNormal UMETA(DisplayName = "Mirror By Normal", ToolTip = "들어오는 빛 방향을 선택된 노멀 기준으로 반사합니다."),
	ComponentForward UMETA(DisplayName = "Component Forward", ToolTip = "이 컴포넌트의 Forward 방향으로 빛을 반사합니다."),
	OwnerForward UMETA(DisplayName = "Owner Forward", ToolTip = "소유 액터의 Forward 방향으로 빛을 반사합니다.")
};

// 우산 표면처럼 게임플레이 빛 트레이스를 차단하거나 반사하는 표면 컴포넌트입니다.
UCLASS(ClassGroup=(Light), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Interaction Surface", ToolTip = "UOU Light Exposure Source에서 나온 게임플레이 빛을 차단하거나 반사합니다."))
class UUOULightInteractionSurfaceComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UUOULightInteractionSurfaceComponent();

	virtual void OnRegister() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Interaction", meta = (ToolTip = "이 표면이 게임플레이 빛 트레이스와 상호작용하는 방식입니다."))
	EUOULightInteractionMode LightInteractionMode = EUOULightInteractionMode::Blocking;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ToolTip = "거울 반사 방식으로 방향을 계산할 때 사용할 노멀입니다."))
	EUOULightReflectionNormalMode ReflectionNormalMode = EUOULightReflectionNormalMode::HitNormal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ToolTip = "반사된 게임플레이 빛의 방향을 정하는 방식입니다."))
	EUOULightReflectionDirectionMode ReflectionDirectionMode = EUOULightReflectionDirectionMode::MirrorByNormal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0", ToolTip = "반사된 게임플레이 빛이 도달할 수 있는 최대 거리입니다."))
	float ReflectionRange = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "1.0", ClampMax = "89.0", ToolTip = "반사된 게임플레이 빛의 원뿔 각도입니다."))
	float ReflectionConeAngle = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0", ToolTip = "광원의 감쇠 계산 뒤 반사광 세기에 곱할 값입니다."))
	float ReflectionIntensityMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0", ToolTip = "반사광 시작 위치를 충돌 지점에서 살짝 띄우는 거리입니다."))
	float ReflectionStartPadding = 4.0f;

	UFUNCTION(BlueprintCallable, Category = "Light|Interaction", meta = (ToolTip = "이 표면이 게임플레이 빛을 차단하거나 반사하는 방식을 변경합니다."))
	void SetLightInteractionMode(EUOULightInteractionMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Light|Interaction", meta = (ToolTip = "이 표면이 들어오는 게임플레이 빛을 차단할 수 있으면 true를 반환합니다."))
	bool CanBlockLight() const;

	UFUNCTION(BlueprintPure, Category = "Light|Interaction", meta = (ToolTip = "이 표면이 반사된 게임플레이 빛을 발생시킬 수 있으면 true를 반환합니다."))
	bool CanReflectLight() const;

	UFUNCTION(BlueprintPure, Category = "Light|Reflection", meta = (ToolTip = "들어오는 레이를 기준으로 반사된 게임플레이 빛 방향을 계산합니다."))
	FVector GetReflectionDirection(const FVector& IncomingDirection, const FVector& HitNormal) const;

protected:
	void ValidateSettings();
	void ApplyCollisionSettings();
	FVector GetReflectionNormal(const FVector& HitNormal) const;
};
