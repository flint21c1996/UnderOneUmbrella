// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightExposureTypes.h"
#include "World/Light/UOULightReceivableInterface.h"
#include "UOULightExposureReceiverComponent.generated.h"

class AActor;
class UPrimitiveComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUOULightExposureReceivedSignature, const FUOULightExposureData&, ExposureData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUOULightExposureStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUOULightExposureEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUOULightTemperatureChangedSignature, float, NewTemperature, float, PreviousTemperature);

// 게임플레이용 빛 노출을 받아 온도 값으로 변환하는 컴포넌트입니다.
UCLASS(ClassGroup=(Light), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Exposure Receiver", ToolTip = "게임플레이용 빛 노출을 받아 온도를 갱신합니다."))
class UNDERONEUMBRELLA_API UUOULightExposureReceiverComponent : public UActorComponent, public IUOULightReceivableInterface, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	UUOULightExposureReceiverComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;

#if UOU_WITH_DEVELOPMENT_TOOLS
	virtual bool ShouldDrawDevelopmentDebugLabel() const override { return false; }
	virtual void GatherDevelopmentDebugDraw(IUOUDevelopmentDebugDrawContext& Context) const override;
#endif

	virtual FVector GetLightReceiverPosition_Implementation() const override;
	virtual void ReceiveLightExposure_Implementation(const FUOULightExposureData& ExposureData) override;

	UPROPERTY(BlueprintAssignable, Category = "Light|Exposure")
	FOnUOULightExposureReceivedSignature OnLightExposureReceived;

	UPROPERTY(BlueprintAssignable, Category = "Light|Exposure")
	FOnUOULightExposureStartedSignature OnLightExposureStarted;

	UPROPERTY(BlueprintAssignable, Category = "Light|Exposure")
	FOnUOULightExposureEndedSignature OnLightExposureEnded;

	UPROPERTY(BlueprintAssignable, Category = "Light|Temperature")
	FOnUOULightTemperatureChangedSignature OnTemperatureChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent", DisplayName = "Receiver Transform", ToolTip = "빛을 받는 기준 위치로 사용할 Scene Component입니다."))
	FComponentReference ReceiverTransformReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver", meta = (ToolTip = "Receiver Transform이 비어 있으면 Primitive Component 또는 Root Component를 자동으로 사용합니다."))
	bool bAutoFindReceiverTransform = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver|Sampling", meta = (ToolTip = "한 점 대신 대상 볼륨의 중앙과 가장자리 샘플을 이용해 빛 수신 여부를 판정합니다."))
	bool bUseReceiverVolumeSampling = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver|Volume Overlap", meta = (ToolTip = "샘플 지점 대신 빔과 Receiver Volume의 겹침 깊이로 빛 수신 여부를 판정합니다."))
	bool bUseBeamVolumeOverlap = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver|Volume Overlap", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bUseBeamVolumeOverlap", ToolTip = "가장자리 스침을 제외하기 위해 빔과 Receiver Volume이 겹쳐야 하는 최소 깊이입니다."))
	float MinimumBeamOverlapDepth = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver|Sampling", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent", EditCondition = "bUseReceiverVolumeSampling || bUseBeamVolumeOverlap", ToolTip = "샘플 또는 빔 겹침 판정에 사용할 대상 볼륨입니다. 비워두면 Receiver Transform 또는 첫 Primitive Component를 사용합니다."))
	FComponentReference ReceiverVolumeReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver|Sampling", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseReceiverVolumeSampling", ToolTip = "판정 샘플을 볼륨 중심에서 가장자리 쪽으로 배치하는 비율입니다."))
	float ReceiverSampleInset = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver|Sampling", meta = (ClampMin = "1", ClampMax = "5", EditCondition = "bUseReceiverVolumeSampling", ToolTip = "중앙과 상하좌우 5개 샘플 중 빛에 닿아야 하는 최소 개수입니다."))
	int32 RequiredReceiverSampleHits = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver|Sampling", meta = (EditCondition = "bUseReceiverVolumeSampling", ToolTip = "빛 수신을 시작할 때보다 유지할 때 필요한 샘플 개수를 낮춰 경계에서 판정이 반복해서 켜지고 꺼지는 현상을 줄입니다."))
	bool bUseReceiverSampleHysteresis = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver|Sampling", meta = (ClampMin = "1", ClampMax = "5", EditCondition = "bUseReceiverVolumeSampling && bUseReceiverSampleHysteresis", ToolTip = "이미 빛을 받고 있는 동안 수신 상태를 유지하는 데 필요한 최소 샘플 개수입니다."))
	int32 RequiredReceiverSampleHitsWhileLit = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Receiver|Occlusion", meta = (ToolTip = "Pawn이 광원과 수신체 사이에 있으면 게임플레이용 빛 노출을 차단합니다."))
	bool bUsePawnOcclusion = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "BeginPlay 시 Current Temperature를 Ambient Temperature 값으로 초기화합니다."))
	bool bStartAtAmbientTemperature = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "빛을 받지 않을 때 회복될 기준 온도입니다."))
	float AmbientTemperature = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "현재 게임플레이 온도입니다. 빛 노출과 회복 처리로 갱신됩니다."))
	float CurrentTemperature = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "온도가 내려갈 수 있는 최소값입니다."))
	float MinTemperature = -50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "온도가 올라갈 수 있는 최대값입니다."))
	float MaxTemperature = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ClampMin = "0.0", ToolTip = "빛 세기가 1.0일 때 초당 증가하는 온도입니다."))
	float TemperatureRisePerIntensity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ClampMin = "0.0", ToolTip = "빛을 받지 않을 때 기준 온도로 회복되는 속도입니다."))
	float TemperatureRecoveryRate = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Temperature", meta = (ToolTip = "빛 노출이 끝난 뒤 기준 온도로 회복할지 여부입니다."))
	bool bRecoverToAmbientWhenNotExposed = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.0", ToolTip = "마지막 빛 샘플 이후 노출 종료로 판단하기까지의 유예 시간입니다."))
	float ExposureEndGraceTime = 0.15f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "현재 빛을 받고 있다고 판단되면 true입니다."))
	bool bIsReceivingLight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막으로 적용된 빛 노출 강도입니다."))
	float LastExposureIntensity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막으로 이 수신체에 빛을 준 광원 이름입니다."))
	FString LastExposureSourceName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막으로 이 수신체에 빛을 준 광원 액터입니다."))
	TObjectPtr<AActor> LastExposureSourceActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (Units = "cm", ToolTip = "마지막으로 수신한 빔과 Receiver Volume의 겹침 깊이입니다."))
	float LastBeamOverlapDepth = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막 노출이 빔과 Receiver Volume의 겹침 깊이 판정을 사용했으면 true입니다."))
	bool bLastExposureUsedBeamVolumeOverlap = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막 빔 겹침 평가가 최소 깊이를 통과했으면 true입니다."))
	bool bLastBeamOverlapAccepted = false;

	UFUNCTION(BlueprintCallable, Category = "Light|Temperature", meta = (ToolTip = "현재 온도를 설정합니다. Min Temperature와 Max Temperature 사이로 제한됩니다."))
	void SetTemperature(float NewTemperature);

	UFUNCTION(BlueprintCallable, Category = "Light|Temperature", meta = (ToolTip = "현재 온도에 변화량을 더하고 결과를 제한합니다."))
	void ApplyTemperatureDelta(float DeltaTemperature);

	UFUNCTION(BlueprintPure, Category = "Light|Exposure", meta = (ToolTip = "현재 게임플레이용 빛에 노출되어 있으면 true를 반환합니다."))
	bool IsReceivingLight() const;

	void GetLightReceiverSamplePositions(
		const FVector& BeamDirection,
		TArray<FVector>& OutSamplePositions) const;
	int32 GetRequiredLightSampleHits(int32 AvailableSampleCount) const;
	// 선택된 Receiver Volume의 월드 Bounds를 구형 겹침 계산에 사용할 중심과 반지름으로 반환합니다.
	bool GetLightReceiverVolumeSphere(FVector& OutCenter, float& OutRadius) const;
	// 성공 여부와 관계없이 마지막 겹침 깊이를 디버그 Runtime 값으로 기록합니다.
	void RecordBeamVolumeOverlapEvaluation(float OverlapDepth, bool bAccepted);

protected:
	float LastExposureWorldTime = -BIG_NUMBER;

	void ValidateTemperatureSettings();
	USceneComponent* GetReferencedReceiverTransform() const;
	USceneComponent* FindAutoReceiverTransform() const;
	UPrimitiveComponent* GetReceiverVolume() const;
	void SetReceivingLight(bool bNewReceivingLight);
	void RecoverTemperature(float DeltaTime);
};
