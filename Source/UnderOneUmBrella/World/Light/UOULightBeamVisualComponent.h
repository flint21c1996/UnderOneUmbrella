// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightBeamVisualTypes.h"
#include "World/Light/UOULightReflectionPathTypes.h"
#include "UOULightBeamVisualComponent.generated.h"

class AActor;
class ULightComponent;
class USpotLightComponent;
class UUOULightExposureSourceComponent;

// 게임플레이 빛 경로 데이터를 외부 VFX BP에 전달하고 직접광·반사광 VFX 풀을 관리합니다.
UCLASS(
	ClassGroup = (Light),
	meta = (
		BlueprintSpawnableComponent,
		DisplayName = "UOU Light Beam Visual",
		ToolTip = "Light Exposure Source의 직접광과 반사 경로를 VFX 액터에 자동으로 전달합니다."))
class UNDERONEUMBRELLA_API UUOULightBeamVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOULightBeamVisualComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Visual", meta = (ToolTip = "직접광과 반사광에 사용할 VFX 액터 클래스입니다. LazyGodray V1/V2는 별도 BP 그래프 없이 자동 연동되며, 다른 VFX는 UOU Light Beam Visual 인터페이스로 연결할 수 있습니다."))
	TSubclassOf<AActor> VFXActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|LazyGodray", meta = (ToolTip = "UOU 인터페이스가 없는 LazyGodray BP의 공개 변수를 이름으로 찾아 자동 갱신합니다."))
	bool bEnableAutomaticLazyGodrayAdapter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|LazyGodray", meta = (ClampMin = "0.01", Units = "cm", EditCondition = "bEnableAutomaticLazyGodrayAdapter", ToolTip = "Overall Length Multiplier가 1일 때의 기준 길이입니다. LazyGodray 기본 메시 기준 권장값은 100입니다."))
	float LazyGodrayBaseLength = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|LazyGodray", meta = (ClampMin = "0.01", Units = "cm", EditCondition = "bEnableAutomaticLazyGodrayAdapter", ToolTip = "Overall Diameter Multiplier가 1일 때의 기준 지름입니다. LazyGodray 기본 메시 기준 권장값은 100입니다."))
	float LazyGodrayBaseDiameter = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|LazyGodray", meta = (ClampMin = "0.0", EditCondition = "bEnableAutomaticLazyGodrayAdapter", ToolTip = "게임플레이 광량을 LazyGodray Overall Intensity로 변환하는 배율입니다."))
	float LazyGodrayIntensityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|LazyGodray", meta = (EditCondition = "bEnableAutomaticLazyGodrayAdapter", ToolTip = "LazyGodray 에셋의 로컬 빛 진행축이 +X와 다를 때 적용할 회전 보정값입니다."))
	FRotator LazyGodrayRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|LazyGodray", meta = (EditCondition = "bEnableAutomaticLazyGodrayAdapter", DisplayName = "LazyGodray 빛 진행축 자동 감지", ToolTip = "VFX 내부 SpotLight의 방향을 빛줄기 로컬 진행축으로 사용합니다. LazyGodray V2처럼 빛 진행축이 +X가 아닌 에셋을 자동으로 경로 방향에 맞춥니다."))
	bool bAutoDetectLazyGodrayBeamAxis = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|LazyGodray", meta = (EditCondition = "bEnableAutomaticLazyGodrayAdapter", DisplayName = "LazyGodray 교차 카드 추가", ToolTip = "한 장짜리 Godray 카드를 빛 진행축 기준 90도로 복제하여 시점에 따른 평면 느낌을 줄입니다."))
	bool bAddCrossedLazyGodrayCard = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual", meta = (ToolTip = "광원에서 최초 충돌 지점까지의 직접광 VFX를 표시합니다."))
	bool bEnableDirectVFX = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual", meta = (ToolTip = "거울 이후의 반사 구간 VFX를 표시합니다."))
	bool bEnableReflectionVFX = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual", meta = (ClampMin = "0", ClampMax = "64", ToolTip = "동시에 표시할 수 있는 반사 빛줄기 VFX의 최대 개수입니다."))
	int32 MaxReflectionVFXCount = 16;

	UPROPERTY(BlueprintReadOnly, Category = "Light|Visual", meta = (DeprecatedProperty, DeprecationMessage = "빛줄기 VFX는 OnLightPathsUpdated 이벤트로 갱신되므로 별도 갱신 간격을 사용하지 않습니다."))
	float DirectVFXUpdateInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "벽이나 거울 표면과 빛줄기가 겹쳐 보이지 않도록 끝점을 앞당기는 거리입니다."))
	float EndPadding = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Light|Visual", meta = (DeprecatedProperty, DeprecationMessage = "VFX는 계산된 LightPath 종료점을 사용하므로 별도 충돌 채널을 사용하지 않습니다."))
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual", meta = (ToolTip = "VFX BP 내부에 포함된 Light 컴포넌트를 끄고 통합 광원 액터의 SpotLight만 사용합니다."))
	bool bDisableEmbeddedVFXLights = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual|Runtime")
	int32 ActiveReflectionVFXCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Light|Visual", meta = (ToolTip = "현재 광원 설정과 경로를 이용해 모든 VFX를 즉시 갱신합니다."))
	void RefreshVisuals();

protected:
	UPROPERTY(Transient)
	TObjectPtr<UUOULightExposureSourceComponent> BoundSourceComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpotLightComponent> BoundSourceSpotLight = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> DirectVFXActor = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> ReflectionVFXPool;

	TSet<TWeakObjectPtr<UClass>> WarnedIncompatibleVFXClasses;
	bool bHasWarnedReflectionVFXLimit = false;

	UFUNCTION()
	void HandleLightPathsUpdated(const TArray<FUOULightPathData>& LightPaths);

	UUOULightExposureSourceComponent* ResolveSourceComponent() const;
	USpotLightComponent* ResolveSourceSpotLight() const;
	AActor* AcquireDirectVFXActor();
	AActor* AcquireReflectionVFXActor(int32 PoolIndex);
	AActor* SpawnVFXActor();
	void ConfigureSpawnedVFXActor(AActor* VFXActor) const;
	void UpdateDirectVFX(const TArray<FUOULightPathData>& LightPaths);
	void UpdateReflectionVFX(const TArray<FUOULightPathData>& LightPaths);
	void HideUnusedReflectionVFX(int32 FirstUnusedIndex);
	void ApplySegmentToVFX(AActor* VFXActor, const FUOULightBeamVisualSegmentData& SegmentData);
	bool ApplySegmentToLazyGodray(AActor* VFXActor, const FUOULightBeamVisualSegmentData& SegmentData);
	void UpdateCrossedLazyGodrayCard(AActor* VFXActor, const FVector& WorldBeamDirection) const;
	void SetVFXActive(AActor* VFXActor, bool bActive) const;
	FUOULightBeamVisualSegmentData BuildVisualSegment(
		const FUOULightPathSegmentData& SegmentData,
		int32 VisualSegmentIndex,
		float AdditionalEndPadding = 0.0f) const;
	FLinearColor ResolveLightColor() const;
	void DestroyVFXActors();
};
