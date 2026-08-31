// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "UOULightInteractionSurfaceComponent.generated.h"

UENUM(BlueprintType)
enum class EUOULightReflectionConeAngleMode : uint8
{
	PreserveIncoming UMETA(DisplayName = "입사 확산각 유지", ToolTip = "광원에서 전달된 확산각을 다음 반사 구간에도 그대로 사용합니다."),
	Override UMETA(DisplayName = "직접 지정", ToolTip = "이 반사면의 Reflection Cone Angle을 사용합니다.")
};

UENUM(BlueprintType)
enum class EUOULightReflectionFrontNormalMode : uint8
{
	ComponentForward UMETA(DisplayName = "Component Forward", ToolTip = "컴포넌트의 Forward 방향을 반사면 앞면으로 사용합니다."),
	ComponentUp UMETA(DisplayName = "Component Up", ToolTip = "컴포넌트의 Up 방향을 반사면 앞면으로 사용합니다."),
	OwnerForward UMETA(DisplayName = "Owner Forward", ToolTip = "소유 액터의 Forward 방향을 반사면 앞면으로 사용합니다.")
};

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
class UNDERONEUMBRELLA_API UUOULightInteractionSurfaceComponent : public UBoxComponent
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Facing", meta = (ToolTip = "켜면 지정한 앞면을 기준으로 반사 각도를 판정합니다. 최대 입사각이 90도를 넘으면 그 초과분만큼 뒷면 방향도 허용합니다."))
	bool bReflectFrontFaceOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Facing", meta = (EditCondition = "bReflectFrontFaceOnly", ToolTip = "반사면의 앞쪽을 결정할 기준 방향입니다."))
	EUOULightReflectionFrontNormalMode ReflectionFrontNormalMode =
		EUOULightReflectionFrontNormalMode::ComponentUp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection|Facing", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg", DisplayName = "반사 시작 최대 입사각", ToolTip = "새 반사를 시작할 때 앞면 법선과 입사광 사이에 허용할 최대 각도입니다. 90도를 넘기면 평행한 입사광과 제한된 범위의 뒷면 입사광도 허용합니다."))
	float MaximumReflectionIncidenceAngle = 89.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection|Facing", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg", DisplayName = "반사 유지 최대 입사각", ToolTip = "이미 반사 중일 때 허용할 최대 입사각입니다. 시작 각도보다 크게 두면 각도 경계에서 반사가 깜빡이지 않습니다."))
	float RetainedMaximumReflectionIncidenceAngle = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Facing", meta = (ToolTip = "입사각 또는 빛 단면 포함 조건 때문에 반사가 거절된 빛을 막지 않고 통과시킵니다. 우산처럼 유효한 조건에서만 빛과 상호작용해야 하는 표면에 사용합니다."))
	bool bPassThroughWhenReflectionRejected = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ToolTip = "입사 광원의 확산각을 유지할지, 이 반사면의 각도를 사용할지 결정합니다."))
	EUOULightReflectionConeAngleMode ReflectionConeAngleMode =
		EUOULightReflectionConeAngleMode::PreserveIncoming;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0", ClampMax = "89.0", ToolTip = "반사된 게임플레이 빛의 확산각입니다. 0이면 원기둥 형태를 유지합니다."))
	float ReflectionConeAngle = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0", ToolTip = "광원의 감쇠 계산 뒤 반사광 세기에 곱할 값입니다."))
	float ReflectionIntensityMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0", ToolTip = "반사광 시작 위치를 충돌 지점에서 살짝 띄우는 거리입니다."))
	float ReflectionStartPadding = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Aperture", meta = (ToolTip = "반사되는 빛의 시작 폭을 이 Box 컴포넌트의 실제 크기로 제한합니다."))
	bool bLimitReflectionBySurfaceAperture = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Aperture", meta = (ClampMin = "0.0", ToolTip = "Box 크기에서 계산한 반사 유효 반지름에 곱할 값입니다."))
	float ReflectionApertureScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Aperture", meta = (ToolTip = "켜면 반사면 중심에서 벗어난 충돌일수록 반사광의 유효 반지름을 줄입니다. 우산 가장자리에 빛이 조금만 걸렸을 때 굵은 반사광이 생기는 현상을 방지합니다."))
	bool bLimitReflectionByImpactOffset = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Aperture", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bLimitReflectionByImpactOffset", ToolTip = "반사면 가장자리에서 추가로 제외할 안전 여백입니다."))
	float ReflectionImpactEdgeInset = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Aperture", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bLimitReflectionByImpactOffset", ToolTip = "입사광 반지름 대비 남은 반사 반지름의 최소 비율입니다. 이 값보다 적게 걸친 빛은 반사하지 않습니다."))
	float MinimumReflectionCoverageRatio = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Aperture", meta = (ToolTip = "켜면 거울에 투영된 빛 단면이 지정한 최소 비율 이상 Box 범위 안에 들어올 때만 반사합니다. 부족할 때 통과할지는 '반사 실패 시 통과' 설정을 따릅니다."))
	bool bRequireFullBeamFootprint = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Aperture", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bRequireFullBeamFootprint", ToolTip = "빛 단면 전체 포함 판정에서 거울 가장자리로부터 제외할 안전 여백입니다."))
	float FullBeamFootprintEdgeInset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection|Aperture", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", EditCondition = "bRequireFullBeamFootprint", DisplayName = "거울 밖 빛 허용 비율 (%)", ToolTip = "빛 단면이 거울 밖으로 벗어나도 반사할 수 있는 비율입니다. 값이 클수록 더 많이 벗어나도 반사하며, 반사 유지 중에는 깜빡임 방지를 위해 자동으로 10% 더 허용합니다."))
	float BeamFootprintOverflowAllowancePercent = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Sampling", meta = (ToolTip = "중심점 하나가 아니라 반사면의 중앙과 상하좌우를 빛 적중 후보로 사용합니다."))
	bool bUseSurfaceAreaSampling = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Sampling", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseSurfaceAreaSampling", ToolTip = "반사면 중심에서 가장자리 방향으로 샘플을 배치하는 비율입니다."))
	float SurfaceSampleInset = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Sampling", meta = (ToolTip = "원기둥형 빛의 중심축이 표면을 빗나가더라도 빛 단면 안의 표면 샘플이 맞으면 반사를 허용합니다. 우산처럼 빛의 일부가 걸친 경우도 반사해야 하는 표면에 사용합니다."))
	bool bAllowEdgeOnlyCylinderReflection = false;

	UFUNCTION(BlueprintCallable, Category = "Light|Interaction", meta = (ToolTip = "이 표면이 게임플레이 빛을 차단하거나 반사하는 방식을 변경합니다."))
	void SetLightInteractionMode(EUOULightInteractionMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Light|Interaction", meta = (ToolTip = "이 표면이 들어오는 게임플레이 빛을 차단할 수 있으면 true를 반환합니다."))
	bool CanBlockLight() const;

	UFUNCTION(BlueprintPure, Category = "Light|Interaction", meta = (ToolTip = "이 표면이 반사된 게임플레이 빛을 발생시킬 수 있으면 true를 반환합니다."))
	bool CanReflectLight() const;

	UFUNCTION(BlueprintPure, Category = "Light|Reflection", meta = (ToolTip = "들어오는 레이를 기준으로 반사된 게임플레이 빛 방향을 계산합니다."))
	FVector GetReflectionDirection(const FVector& IncomingDirection, const FVector& HitNormal) const;

	UFUNCTION(BlueprintPure, Category = "Light|Reflection", meta = (ToolTip = "입사 방향이 앞면과 최대 입사각 조건을 만족하면 true를 반환합니다."))
	bool CanReflectIncomingLight(const FVector& IncomingDirection, const FVector& HitNormal) const;
	bool CanReflectIncomingLightWithMaximumAngle(
		const FVector& IncomingDirection,
		const FVector& HitNormal,
		float MaximumIncidenceAngleDegrees) const;

	UFUNCTION(BlueprintPure, Category = "Light|Reflection", meta = (ToolTip = "반사 각도 조건을 만족하지 못해 입사광을 통과시켜야 하면 true를 반환합니다."))
	bool ShouldPassThroughIncomingLight(const FVector& IncomingDirection, const FVector& HitNormal) const;
	bool ShouldPassThroughIncomingLightWithMaximumAngle(
		const FVector& IncomingDirection,
		const FVector& HitNormal,
		float MaximumIncidenceAngleDegrees) const;

	UFUNCTION(BlueprintCallable, Category = "Light|Reflection|Facing", meta = (DisplayName = "반사 시작 및 유지 각도 설정", ToolTip = "새 반사를 시작할 최대 입사각과 이미 반사 중일 때 유지할 최대 입사각을 설정합니다."))
	void SetReflectionIncidenceAngles(
		float StartMaximumAngleDegrees,
		float RetainedMaximumAngleDegrees);

	UFUNCTION(BlueprintPure, Category = "Light|Reflection", meta = (ToolTip = "입사 빛 반지름을 반사면 크기와 입사각에 맞게 잘라 반환합니다."))
	float ClampReflectionBeamRadius(
		float IncomingBeamRadius,
		const FVector& IncomingDirection,
		const FVector& HitNormal,
		const FVector& ImpactPoint) const;

	UFUNCTION(BlueprintPure, Category = "Light|Reflection", meta = (ToolTip = "충돌 위치에 남은 반사 폭이 최소 반사 비율을 만족하면 true를 반환합니다."))
	bool HasSufficientReflectionCoverage(
		float IncomingBeamRadius,
		const FVector& IncomingDirection,
		const FVector& HitNormal,
		const FVector& ImpactPoint) const;
	bool HasSufficientReflectionCoverageAtRatio(
		float IncomingBeamRadius,
		const FVector& IncomingDirection,
		const FVector& HitNormal,
		const FVector& ImpactPoint,
		float RequiredFootprintCoverageRatio) const;

	// 원형 빔 단면을 입사 방향으로 반사면에 투영한 뒤 기본 최소 포함 비율을 만족하는지 검사합니다.
	bool ContainsFullBeamFootprint(
		float IncomingBeamRadius,
		const FVector& IncomingDirection,
		const FVector& HitNormal,
		const FVector& ImpactPoint) const;
	float CalculateBeamFootprintCoverageRatio(
		float IncomingBeamRadius,
		const FVector& IncomingDirection,
		const FVector& HitNormal,
		const FVector& ImpactPoint) const;
	bool HasMinimumBeamFootprintCoverage(
		float IncomingBeamRadius,
		const FVector& IncomingDirection,
		const FVector& HitNormal,
		const FVector& ImpactPoint,
		float RequiredCoverageRatio) const;
	float GetStartingBeamFootprintCoverageRatio() const;
	float GetRetainedBeamFootprintCoverageRatio() const;

	UFUNCTION(BlueprintCallable, Category = "Light|Reflection|Aperture", meta = (DisplayName = "거울 밖 빛 허용 비율 설정", ToolTip = "빛 단면이 거울 밖으로 벗어나도 반사할 수 있는 비율을 0~100%로 설정합니다."))
	void SetBeamFootprintOverflowAllowance(float OverflowAllowancePercent);

	UFUNCTION(BlueprintPure, Category = "Light|Reflection", meta = (ToolTip = "입사 확산각과 반사면 설정을 바탕으로 실제 반사 확산각을 반환합니다."))
	float ResolveReflectionConeAngle(float IncomingConeAngle) const;

	void GetReflectionSamplePositions(TArray<FVector>& OutSamplePositions) const;

protected:
	void ValidateSettings();
	void ApplyCollisionSettings();
	FVector GetReflectionNormal(const FVector& HitNormal) const;
	FVector GetReflectionFrontNormal() const;
	float GetReflectionApertureRadius() const;
};
