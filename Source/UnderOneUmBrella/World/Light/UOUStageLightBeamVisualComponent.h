// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Light/UOULightBeamVisualTypes.h"
#include "World/Light/UOULightReflectionPathTypes.h"
#include "UOUStageLightBeamVisualComponent.generated.h"

class AActor;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
class USpotLightComponent;
class UStaticMesh;
class UUOULightExposureSourceComponent;

// 직접광 표현을 관리하고 우산 차단 시 빔 크기를 유지하며 전체 알파를 보간하는 무대 조명 전용 컴포넌트입니다.
UCLASS(
	ClassGroup = (Light),
	meta = (
		BlueprintSpawnableComponent,
		DisplayName = "UOU Stage Light Beam Visual",
		ToolTip = "직접광 빔 외형을 표시하고 우산 차단 시 크기를 유지하며 알파만 보간합니다."))
class UNDERONEUMBRELLA_API UUOUStageLightBeamVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUStageLightBeamVisualComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Visual", meta = (ToolTip = "직접광에 생성할 VFX 액터 클래스입니다. UOU Light Beam Visual 인터페이스를 통해 외형과 페이드 알파를 적용합니다."))
	TSubclassOf<AActor> VFXActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Asset Overrides", meta = (DisplayName = "빔 메시 Override", ToolTip = "켜면 BP_SpotRay의 프리셋 메시 대신 아래 메시 하나를 사용합니다."))
	bool bOverrideBeamMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Asset Overrides", meta = (EditCondition = "bOverrideBeamMesh", DisplayName = "빔 메시", ToolTip = "지정한 메시를 단일 레이어로 사용합니다. None이면 메시 표현을 끄니다."))
	TObjectPtr<UStaticMesh> BeamMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Asset Overrides", meta = (DisplayName = "빔 머티리얼 Override", ToolTip = "켜면 BP_SpotRay의 머티리얼 대신 아래 머티리얼로 Dynamic Material을 만듭니다."))
	bool bOverrideBeamMaterial = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Asset Overrides", meta = (EditCondition = "bOverrideBeamMaterial", DisplayName = "빔 머티리얼", ToolTip = "지정한 머티리얼로 Dynamic Material을 만듭니다. None이면 메시 표현을 끄니다."))
	TObjectPtr<UMaterialInterface> BeamMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Asset Overrides", meta = (DisplayName = "Niagara System Override", ToolTip = "켜면 BP_SpotRay의 Sparkle Effect 대신 아래 Niagara System을 사용합니다."))
	bool bOverrideNiagaraSystem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Asset Overrides", meta = (EditCondition = "bOverrideNiagaraSystem", DisplayName = "Niagara System", ToolTip = "지정한 Niagara System을 사용합니다. None이면 반짝임 표현을 끄니다."))
	TObjectPtr<UNiagaraSystem> NiagaraSystemOverride;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|LazyGodray", meta = (EditCondition = "bEnableAutomaticLazyGodrayAdapter", DisplayName = "LazyGodray 교차 카드 추가", ToolTip = "Godray 카드를 빛 진행축을 중심으로 균등하게 복제하여 시점에 따른 평면 느낌을 줄입니다."))
	bool bAddCrossedLazyGodrayCard = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|LazyGodray", meta = (ClampMin = "1", ClampMax = "8", EditCondition = "bEnableAutomaticLazyGodrayAdapter && bAddCrossedLazyGodrayCard", DisplayName = "LazyGodray 카드 면 개수", ToolTip = "빛 진행축을 중심으로 배치할 Godray 카드의 총 개수입니다. 4면이면 0/45/90/135도로 배치됩니다."))
	int32 LazyGodrayCardPlaneCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual", meta = (ToolTip = "광원에서 최초 충돌 지점까지의 직접광 VFX를 표시합니다."))
	bool bEnableDirectVFX = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "장애물 표면과 빛줄기가 겹쳐 보이지 않도록 끝점을 앞당기는 거리입니다."))
	float EndPadding = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual", meta = (ToolTip = "VFX BP 내부에 포함된 Light 컴포넌트를 끄고 통합 광원 액터의 SpotLight만 사용합니다."))
	bool bDisableEmbeddedVFXLights = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Range Decal", meta = (DisplayName = "끝단 범위 데칼 사용", ToolTip = "빛이 표면에 닿아 끝나는 위치에 표면 굴곡을 따라가는 범위 데칼을 표시합니다."))
	bool bEnableEndRangeDecal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Range Decal", meta = (EditCondition = "bEnableEndRangeDecal", DisplayName = "끝단 범위 데칼 머티리얼", ToolTip = "Deferred Decal 도메인의 머티리얼을 지정합니다. BeamColor와 Opacity 파라미터가 있으면 광원 색상과 투명도를 자동 적용합니다."))
	TObjectPtr<UMaterialInterface> EndRangeDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Range Decal", meta = (ClampMin = "0.01", EditCondition = "bEnableEndRangeDecal", DisplayName = "끝단 범위 크기 배율", ToolTip = "계산된 빛 끝 반지름에 적용할 데칼 크기 배율입니다."))
	float EndRangeDecalRadiusScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Range Decal", meta = (ClampMin = "1.0", Units = "cm", EditCondition = "bEnableEndRangeDecal", DisplayName = "데칼 투영 깊이", ToolTip = "표면 요철을 감싸는 데칼 박스 깊이입니다. 너무 크면 뒤쪽 표면까지 투영될 수 있습니다."))
	float EndRangeDecalDepth = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Range Decal", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bEnableEndRangeDecal", DisplayName = "표면 띄움 거리", ToolTip = "Z-Fighting을 피하기 위해 충돌 표면 법선 방향으로 데칼 중심을 띄우는 거리입니다."))
	float EndRangeDecalSurfaceOffset = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Range Decal", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableEndRangeDecal", DisplayName = "끝단 범위 투명도"))
	float EndRangeDecalOpacity = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Range Decal", meta = (EditCondition = "bEnableEndRangeDecal", DisplayName = "거리 종료 시 지면 투영", ToolTip = "빛이 장애물이 아닌 최대 거리에서 끝나면 끝점 아래의 표면을 찾아 범위 데칼을 투영합니다."))
	bool bProjectRangeEndDecalToGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Range Decal", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bEnableEndRangeDecal && bProjectRangeEndDecalToGround", DisplayName = "지면 탐색 시작 높이"))
	float RangeEndGroundTraceHeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Range Decal", meta = (ClampMin = "1.0", Units = "cm", EditCondition = "bEnableEndRangeDecal && bProjectRangeEndDecalToGround", DisplayName = "지면 탐색 거리"))
	float RangeEndGroundTraceDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Overrides", meta = (ClampMin = "0.0", DisplayName = "개별 밝기 배율", ToolTip = "이 광원 인스턴스의 정규화된 빛줄기 밝기 배율입니다. 1이 권장 기본값이며 퍼즐 판정용 빛 세기에는 영향을 주지 않습니다."))
	float VisualBrightnessMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Overrides", meta = (ClampMin = "0.0", DisplayName = "개별 투명도 배율", ToolTip = "이 광원 인스턴스의 빛줄기 투명도에만 곱하는 값입니다. 0이면 완전히 투명하고 1이면 VFX 액터 클래스의 기본 투명도를 그대로 사용합니다."))
	float VisualOpacityMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Overrides", meta = (ClampMin = "0", ClampMax = "8", DisplayName = "Dynamic Ray 프리셋 Override", ToolTip = "0이면 VFX 액터 클래스의 기본 프리셋을 사용하고, 1~8이면 이 광원 인스턴스에서 해당 프리셋으로 덮어씁니다."))
	int32 LumenDynamicRayPreset = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Visual|Overrides", meta = (ClampMin = "0", ClampMax = "19", DisplayName = "Static Ray 프리셋 Override", ToolTip = "0이면 VFX 액터 클래스의 기본 프리셋을 사용하고, 1~19이면 이 광원 인스턴스에서 해당 프리셋으로 덮어씁니다."))
	int32 LumenStaticRayPreset = 0;

	// 우산 차단 시 빔 전체에 적용할 알파와 전환 시간입니다. 판정 광량은 변경하지 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Visual|Fade", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "우산 차단 알파"))
	float UmbrellaBlockedAlpha = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Visual|Fade", meta = (ClampMin = "0.0", Units = "s", DisplayName = "알파 전환 시간"))
	float BeamFadeDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Visual|Fade", meta = (ClampMin = "1.0", ClampMax = "5.0", DisplayName = "알파 전환 곡선", ToolTip = "1이면 선형이며 값이 클수록 페이드 시작과 끝의 변화가 완만해집니다."))
	float BeamFadeEaseExponent = 2.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Light|Visual|Fade", meta = (DisplayName = "현재 빔 알파"))
	float CurrentBeamAlpha = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Visual|Fade", meta = (ToolTip = "빔 메시 머티리얼에서 전체 알파로 사용할 Scalar Parameter 이름입니다."))
	FName MeshOpacityParameter = TEXT("Opacity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Visual|Fade", meta = (ToolTip = "Niagara에 같은 이름의 User Float를 만들고 렌더링 알파에 연결해야 합니다. None이면 적용하지 않습니다."))
	FName NiagaraOpacityParameter = TEXT("User.BeamOpacity");

	UFUNCTION(BlueprintCallable, Category = "Light|Visual", meta = (ToolTip = "현재 광원 설정과 경로를 이용해 모든 VFX를 즉시 갱신합니다."))
	void RefreshVisuals();

protected:
	// 알파가 누적 곱해지지 않도록 원본 데이터와 우산 차단 전 크기를 보관합니다.
	TMap<TWeakObjectPtr<AActor>, FUOULightBeamVisualSegmentData> AppliedVisualSegments;
	TMap<int32, FUOULightBeamVisualSegmentData> UnblockedVisualSegments;
	// 차단 상태가 바뀔 때 현재 알파부터 다시 보간하여 급격한 점프를 방지합니다.
	float FadeStartAlpha = 1.0f;
	float FadeTargetAlpha = 1.0f;
	float FadeElapsed = 0.0f;

	// 기존 경로의 우산 충돌 여부를 읽고, 표현 알파만 갱신합니다.
	void UpdateUmbrellaFadeTarget(const TArray<FUOULightPathData>& LightPaths);
	void ApplyBeamOpacity(AActor* VFXActor) const;

	UPROPERTY(Transient)
	TObjectPtr<UUOULightExposureSourceComponent> BoundSourceComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpotLightComponent> BoundSourceSpotLight = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> DirectVFXActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> DirectEndRangeDecal = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DirectEndRangeDecalMaterial = nullptr;

	UFUNCTION()
	void HandleLightPathsUpdated(const TArray<FUOULightPathData>& LightPaths);

	UUOULightExposureSourceComponent* ResolveSourceComponent() const;
	USpotLightComponent* ResolveSourceSpotLight() const;
	AActor* AcquireDirectVFXActor();
	AActor* SpawnVFXActor();
	void ConfigureSpawnedVFXActor(AActor* VFXActor) const;
	void ApplyVFXAssetOverrides(AActor* VFXActor) const;
	// 메시 초기 렌더링 설정을 적용합니다.
	void ConfigureVFXMesh(AActor* VFXActor) const;
	void UpdateDirectVFX(
		const TArray<FUOULightPathData>& LightPaths,
		float ReferenceVisualLength);
	void UpdateEndRangeDecal(
		TObjectPtr<UDecalComponent>& DecalComponent,
		TObjectPtr<UMaterialInstanceDynamic>& DynamicMaterial,
		const FUOULightPathSegmentData& SegmentData,
		const FLinearColor& LightColor,
		int32 SortOrder);
	void DestroyEndRangeDecals();
	// 원본 표현 구간을 저장한 뒤 현재 알파 배율을 적용합니다.
	void ApplySegmentToVFX(AActor* VFXActor, const FUOULightBeamVisualSegmentData& SegmentData);
	bool ApplySegmentToLazyGodray(AActor* VFXActor, const FUOULightBeamVisualSegmentData& SegmentData);
	void UpdateCrossedLazyGodrayCard(AActor* VFXActor, const FVector& WorldBeamDirection) const;
	void SetVFXActive(AActor* VFXActor, bool bActive);
	FUOULightBeamVisualSegmentData BuildVisualSegment(
		const FUOULightPathSegmentData& SegmentData,
		int32 VisualSegmentIndex,
		float ReferenceVisualLength);
	FLinearColor ResolveLightColor() const;
	void DestroyVFXActors();
};
