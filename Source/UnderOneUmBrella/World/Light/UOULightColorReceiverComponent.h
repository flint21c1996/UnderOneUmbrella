// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightExposureReceiverComponent.h"
#include "UOULightColorReceiverComponent.generated.h"

class UMaterialInstanceDynamic;
class UMeshComponent;

UENUM(BlueprintType)
enum class EUOULightColorState : uint8
{
	None = 0 UMETA(DisplayName = "빛 없음"),
	Red = 1 UMETA(DisplayName = "빨강 R"),
	Green = 2 UMETA(DisplayName = "초록 G"),
	Blue = 3 UMETA(DisplayName = "파랑 B"),
	RedGreen = 4 UMETA(DisplayName = "빨강+초록 RG"),
	RedBlue = 5 UMETA(DisplayName = "빨강+파랑 RB"),
	GreenBlue = 6 UMETA(DisplayName = "초록+파랑 GB"),
	RedGreenBlue = 7 UMETA(DisplayName = "빨강+초록+파랑 RGB")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnUOUMixedLightColorChangedSignature,
	FLinearColor,
	NewMixedColor,
	bool,
	bHasAnyLight);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnUOULightColorStateChangedSignature,
	EUOULightColorState,
	NewState,
	int32,
	MaterialIndex);

// 여러 게임플레이 광원의 RGB 조합을 판정하고 대상 Mesh의 PaintTint 파라미터에 누적합니다.
UCLASS(
	ClassGroup = (Light),
	meta = (
		BlueprintSpawnableComponent,
		DisplayName = "UOU Light Color Receiver",
		ToolTip = "받은 RGB 빛을 물감처럼 머티리얼에 남기고 BP 이벤트를 발생시킵니다."))
class UNDERONEUMBRELLA_API UUOULightColorReceiverComponent
	: public UUOULightExposureReceiverComponent
{
	GENERATED_BODY()

public:
	UUOULightColorReceiverComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		enum ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void ReceiveLightExposure_Implementation(
		const FUOULightExposureData& ExposureData) override;

	UPROPERTY(BlueprintAssignable, Category = "Light|Color|Event", meta = (ToolTip = "혼합된 실제 색상 값이 변경될 때 발생합니다."))
	FOnUOUMixedLightColorChangedSignature OnMixedLightColorChanged;

	UPROPERTY(BlueprintAssignable, Category = "Light|Color|Event", meta = (ToolTip = "R/G/B 조합 상태가 바뀔 때 한 번만 발생합니다. 빛 없음은 -1, R부터 RGB까지는 0~6번입니다."))
	FOnUOULightColorStateChangedSignature OnLightColorStateChanged;

	// 꺼두면 빛의 세기와 무관하게 닿은 색 채널을 1로 더합니다. RGB 세 빛이 닿으면 정확히 흰색이 됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color", meta = (ToolTip = "켜면 거리와 각도로 감쇠된 노출 세기를 색 밝기에 곱합니다. 끄면 닿은 색 자체만 합칩니다."))
	bool bWeightColorByExposureIntensity = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color", meta = (ClampMin = "0.0", ToolTip = "이 세기보다 약한 노출은 색상 조합에서 무시합니다."))
	float MinimumColorExposureIntensity = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color", meta = (ClampMin = "0.0", Units = "s", ToolTip = "광원의 마지막 샘플 이후 해당 색을 제거하기까지 기다리는 시간입니다. Source의 Sample Interval보다 크게 두는 것을 권장합니다."))
	float ColorExposureEndGraceTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "R/G/B 채널이 활성 상태라고 판단할 최소 혼합값입니다."))
	float ActiveChannelThreshold = 0.05f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Color|Runtime")
	FLinearColor MixedLightColor = FLinearColor::Black;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Color|Runtime")
	EUOULightColorState CurrentColorState = EUOULightColorState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Color|Runtime")
	bool bHasAnyColorLight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Color|Runtime")
	bool bHasRedLight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Color|Runtime")
	bool bHasGreenLight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Color|Runtime")
	bool bHasBlueLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Paint", meta = (ToolTip = "켜면 받은 RGB 빛을 대상 머티리얼의 Vector Parameter에 물감처럼 적용합니다."))
	bool bApplyPaintTint = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Paint", meta = (ToolTip = "대상 머티리얼의 Base Color에 곱하도록 만든 Vector Parameter 이름입니다."))
	FName PaintTintParameterName = TEXT("PaintTint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Paint", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "선택되지 않은 색 채널에 남길 최소값입니다. 0이면 완전히 제거되고 값이 높을수록 원본 질감과 밝기가 더 남습니다."))
	float MinimumPaintChannel = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Paint", meta = (ToolTip = "PaintTint를 적용할 Mesh Component입니다. 비어 있으면 소유 액터의 Mesh들을 자동으로 찾습니다."))
	TArray<FComponentReference> TargetMeshReferences;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Paint", meta = (ToolTip = "Target Mesh 목록이 비었을 때 소유 액터의 Mesh Component를 자동으로 찾습니다."))
	bool bAutoFindMeshComponents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Paint", meta = (ClampMin = "0", ToolTip = "각 대상 Mesh에서 PaintTint를 적용할 Material Slot 번호입니다."))
	int32 TargetMaterialSlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Paint", meta = (ClampMin = "0.0", Units = "s", ToolTip = "현재 물감색에서 새 RGB 물감색까지 변하는 시간입니다. 0이면 즉시 적용합니다."))
	float MaterialTransitionDuration = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Color|Paint|Runtime", meta = (ToolTip = "현재 머티리얼에 적용 중인 지속형 PaintTint입니다. 빛 밖으로 나가도 유지됩니다."))
	FLinearColor CurrentPaintTint = FLinearColor::White;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Color|Paint|Runtime", meta = (ToolTip = "현재 RGB 빛으로부터 계산된 목표 PaintTint입니다."))
	FLinearColor TargetPaintTint = FLinearColor::White;

	UFUNCTION(BlueprintPure, Category = "Light|Color")
	FLinearColor GetMixedLightColor() const { return MixedLightColor; }

	UFUNCTION(BlueprintPure, Category = "Light|Color")
	EUOULightColorState GetCurrentColorState() const { return CurrentColorState; }

	UFUNCTION(BlueprintPure, Category = "Light|Color")
	int32 GetCurrentStateMaterialIndex() const;

	UFUNCTION(BlueprintPure, Category = "Light|Color")
	bool HasAnyColorLight() const { return bHasAnyColorLight; }

	UFUNCTION(BlueprintCallable, Category = "Light|Color", meta = (ToolTip = "활성 광원 기록만 비웁니다. 현재 물감색은 그대로 유지됩니다."))
	void ClearColorExposures();

	UFUNCTION(BlueprintCallable, Category = "Light|Color|Paint", meta = (ToolTip = "현재 Target Mesh 설정을 다시 읽고 동적 머티리얼과 현재 PaintTint를 적용합니다."))
	void RefreshTargetMaterials();

	UFUNCTION(BlueprintCallable, Category = "Light|Color|Paint", meta = (ToolTip = "현재 물감색을 흰색으로 되돌려 원래 알베도를 복구합니다. 흰색 RGB 빛도 같은 결과를 냅니다."))
	void ResetPaintTint(bool bImmediate = false);

protected:
	struct FActiveColorExposure
	{
		FLinearColor Color = FLinearColor::White;
		float Intensity = 0.0f;
		float LastReceivedWorldTime = -BIG_NUMBER;
	};

	struct FPaintMaterialTarget
	{
		TWeakObjectPtr<UMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
	};

	TMap<TWeakObjectPtr<UObject>, FActiveColorExposure> ActiveColorExposures;
	TArray<FPaintMaterialTarget> PaintMaterialTargets;
	FLinearColor PaintTransitionStart = FLinearColor::White;
	float PaintTransitionElapsed = 0.0f;
	bool bPaintTransitionActive = false;

	void RemoveExpiredColorExposures();
	void UpdatePaintTransition(float DeltaTime);
	void RecalculateMixedLightColor(bool bForceApply = false);
	void SetPaintTargetFromLight(const FLinearColor& LightColor);
	void SetPaintTarget(const FLinearColor& NewTarget, bool bImmediate = false);
	void HoldCurrentPaintTint();
	void ApplyCurrentPaintTint();
	void AddMeshComponentTarget(UMeshComponent* MeshComponent);
	FLinearColor CalculatePaintTint(const FLinearColor& LightColor) const;
	static EUOULightColorState ResolveColorState(bool bRed, bool bGreen, bool bBlue);
	static int32 ResolveStateMaterialIndex(EUOULightColorState State);
};
