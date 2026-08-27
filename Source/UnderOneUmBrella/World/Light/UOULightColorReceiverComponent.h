// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightExposureReceiverComponent.h"
#include "UOULightColorReceiverComponent.generated.h"

class UMaterialInterface;
class UMaterialInstance;
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

// 여러 게임플레이 광원의 RGB 조합을 판정하고 0~6번 상태 머티리얼을 대상 Mesh에 적용합니다.
UCLASS(
	ClassGroup = (Light),
	meta = (
		BlueprintSpawnableComponent,
		DisplayName = "UOU Light Color Receiver",
		ToolTip = "받은 RGB 빛 조합에 맞춰 0~6번 상태 머티리얼을 적용하고 BP 이벤트를 발생시킵니다."))
class UNDERONEUMBRELLA_API UUOULightColorReceiverComponent
	: public UUOULightExposureReceiverComponent
{
	GENERATED_BODY()

public:
	UUOULightColorReceiverComponent();

	virtual void PostLoad() override;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Material", meta = (ToolTip = "켜면 RGB 조합이 바뀔 때 State Materials의 머티리얼을 대상 Mesh에 적용합니다."))
	bool bApplyStateMaterials = true;

	// 0=R, 1=G, 2=B, 3=RG, 4=RB, 5=GB, 6=RGB입니다. 빛이 없으면 시작할 때 저장한 머티리얼을 복원합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Material", meta = (EditFixedSize, ToolTip = "RGB 조합별 머티리얼입니다. 0 R, 1 G, 2 B, 3 RG, 4 RB, 5 GB, 6 RGB 순서입니다."))
	TArray<TObjectPtr<UMaterialInterface>> StateMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Material", meta = (ToolTip = "머티리얼을 교체할 Mesh Component입니다. 비어 있으면 소유 액터의 Mesh들을 자동으로 찾습니다."))
	TArray<FComponentReference> TargetMeshReferences;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Material", meta = (ToolTip = "Target Mesh 목록이 비었을 때 소유 액터의 Mesh Component를 자동으로 찾습니다."))
	bool bAutoFindMeshComponents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Material", meta = (ClampMin = "0", ToolTip = "각 대상 Mesh에서 실제로 교체할 Material Slot 번호입니다. State Materials 배열 인덱스와는 별개입니다."))
	int32 TargetMaterialSlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Material|Transition", meta = (ToolTip = "같은 마스터를 사용하는 Material Instance 사이의 Scalar/Vector 파라미터를 서서히 보간합니다. 조건이 맞지 않으면 즉시 교체합니다."))
	bool bSmoothMaterialTransitions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Color|Material|Transition", meta = (ClampMin = "0.0", Units = "s", EditCondition = "bSmoothMaterialTransitions", ToolTip = "새 RGB 상태 머티리얼로 전환하는 시간입니다."))
	float MaterialTransitionDuration = 0.25f;

	UFUNCTION(BlueprintPure, Category = "Light|Color")
	FLinearColor GetMixedLightColor() const { return MixedLightColor; }

	UFUNCTION(BlueprintPure, Category = "Light|Color")
	EUOULightColorState GetCurrentColorState() const { return CurrentColorState; }

	UFUNCTION(BlueprintPure, Category = "Light|Color")
	int32 GetCurrentStateMaterialIndex() const;

	UFUNCTION(BlueprintPure, Category = "Light|Color")
	bool HasAnyColorLight() const { return bHasAnyColorLight; }

	UFUNCTION(BlueprintCallable, Category = "Light|Color", meta = (ToolTip = "활성 광원 기록을 비우고 시작할 때 저장한 원래 머티리얼로 되돌립니다."))
	void ClearColorExposures();

	UFUNCTION(BlueprintCallable, Category = "Light|Color|Material", meta = (ToolTip = "현재 Target Mesh 설정을 다시 읽고 현재 RGB 상태의 머티리얼을 적용합니다."))
	void RefreshTargetMaterials();

protected:
	struct FActiveColorExposure
	{
		FLinearColor Color = FLinearColor::White;
		float Intensity = 0.0f;
		float LastReceivedWorldTime = -BIG_NUMBER;
	};

	struct FMaterialStateTarget
	{
		TWeakObjectPtr<UMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInterface> OriginalMaterial;
		TWeakObjectPtr<UMaterialInstanceDynamic> BlendMaterial;
		TWeakObjectPtr<UMaterialInstance> DestinationMaterial;
		float TransitionTimeRemaining = 0.0f;
	};

	TMap<TWeakObjectPtr<UObject>, FActiveColorExposure> ActiveColorExposures;
	TArray<FMaterialStateTarget> MaterialStateTargets;

	void RemoveExpiredColorExposures();
	void UpdateMaterialTransitions(float DeltaTime);
	void RecalculateMixedLightColor(bool bForceApply = false);
	void ApplyStateMaterial(EUOULightColorState NewState);
	void ApplyOrTransitionMaterial(FMaterialStateTarget& Target, UMaterialInterface* DesiredMaterial);
	void AddMeshComponentTarget(UMeshComponent* MeshComponent);
	static EUOULightColorState ResolveColorState(bool bRed, bool bGreen, bool bBlue);
	static int32 ResolveStateMaterialIndex(EUOULightColorState State);
};
