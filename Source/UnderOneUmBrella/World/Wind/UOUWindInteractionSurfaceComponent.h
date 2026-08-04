// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "UOUWindInteractionSurfaceComponent.generated.h"

UENUM(BlueprintType)
enum class EUOUWindInteractionMode : uint8
{
	Disabled UMETA(DisplayName = "Disabled", ToolTip = "바람 경로와 상호작용하지 않습니다."),
	Blocking UMETA(DisplayName = "Blocking", ToolTip = "들어오는 바람을 이 표면에서 차단합니다."),
	Reflecting UMETA(DisplayName = "Reflecting", ToolTip = "표면 법선을 기준으로 바람을 반사합니다."),
	Redirecting UMETA(DisplayName = "Redirecting", ToolTip = "들어오는 각도와 무관하게 지정된 Forward 방향으로 바람을 내보냅니다.")
};

UENUM(BlueprintType)
enum class EUOUWindReflectionNormalMode : uint8
{
	HitNormal UMETA(DisplayName = "Hit Normal"),
	ComponentUp UMETA(DisplayName = "Component Up"),
	ComponentForward UMETA(DisplayName = "Component Forward")
};

UENUM(BlueprintType)
enum class EUOUWindRedirectDirectionMode : uint8
{
	ComponentForward UMETA(DisplayName = "Component Forward"),
	OwnerForward UMETA(DisplayName = "Owner Forward")
};

// 특정 벽이나 회전판에 붙여 바람을 차단하거나 반사하는 표면입니다.
UCLASS(ClassGroup=(Wind), meta=(BlueprintSpawnableComponent, DisplayName="UOU Wind Interaction Surface"))
class UNDERONEUMBRELLA_API UUOUWindInteractionSurfaceComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UUOUWindInteractionSurfaceComponent();

	virtual void OnRegister() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Interaction")
	EUOUWindInteractionMode InteractionMode = EUOUWindInteractionMode::Reflecting;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Reflection")
	EUOUWindReflectionNormalMode ReflectionNormalMode = EUOUWindReflectionNormalMode::HitNormal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Redirect")
	EUOUWindRedirectDirectionMode RedirectDirectionMode = EUOUWindRedirectDirectionMode::ComponentForward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Reflection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrengthRetention = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Reflection", meta = (ClampMin = "0.1", Units = "cm"))
	float ReflectionStartPadding = 4.0f;

	UFUNCTION(BlueprintCallable, Category = "Wind|Interaction")
	void SetInteractionMode(EUOUWindInteractionMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Wind|Interaction")
	bool CanReflectWind() const;

	UFUNCTION(BlueprintPure, Category = "Wind|Interaction")
	bool CanBlockWind() const;

	UFUNCTION(BlueprintPure, Category = "Wind|Reflection")
	FVector GetOutgoingDirection(const FVector& IncomingDirection, const FVector& HitNormal) const;

private:
	void ValidateSettings();
	void ApplyCollisionSettings();
	FVector ResolveReflectionNormal(const FVector& HitNormal) const;
};
