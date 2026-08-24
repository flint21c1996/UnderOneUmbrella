// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightExposureTypes.h"
#include "World/Light/UOULightReflectionPathTypes.h"
#include "UOULightExposureSourceComponent.generated.h"

class UPrimitiveComponent;
class USceneComponent;
class USpotLightComponent;
class UUOULightInteractionSurfaceComponent;
class UUOUUmbrellaLightShadeVolumeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOULightReflectionPathsUpdatedSignature,
	const TArray<FUOULightReflectionPathData>&,
	ReflectionPaths);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOULightPathsUpdatedSignature,
	const TArray<FUOULightPathData>&,
	LightPaths);

UENUM(BlueprintType)
enum class EUOULightBeamShape : uint8
{
	Cone UMETA(DisplayName = "원뿔", ToolTip = "SpotLight의 원뿔 각도를 사용하는 빛입니다."),
	Cylinder UMETA(DisplayName = "원기둥", ToolTip = "지정한 반지름을 끝까지 유지하는 평행광입니다.")
};

// 원뿔 또는 원기둥 형태의 광원에서 주변 수신체로 게임플레이용 빛 노출을 전달합니다.
UCLASS(ClassGroup=(Light), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Exposure Source", ToolTip = "선택한 광원 형상 안의 수신체에 게임플레이용 빛 노출을 전달합니다."))
class UNDERONEUMBRELLA_API UUOULightExposureSourceComponent
	: public UActorComponent
	, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	UUOULightExposureSourceComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;

#if UOU_WITH_DEVELOPMENT_TOOLS
	virtual bool ShouldDrawDevelopmentDebugLabel() const override { return false; }
	virtual void GatherDevelopmentDebugDraw(IUOUDevelopmentDebugDrawContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Source", meta = (ToolTip = "이 광원에서 게임플레이용 빛을 발사할지 여부입니다."))
	bool bEmitLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Source", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent", DisplayName = "Source Transform", ToolTip = "광원의 위치와 방향을 가져올 Scene Component입니다. 원뿔형에서 SpotLight를 선택하면 원뿔 각도도 함께 사용합니다."))
	FComponentReference SourceTransformReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Source", meta = (ToolTip = "Source Transform이 비어 있으면 소유 액터의 첫 번째 SpotLight 컴포넌트를 위치와 방향 기준으로 자동 사용합니다."))
	bool bAutoFindSourceSpotLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape", meta = (ToolTip = "게임플레이 빛의 형상을 선택합니다."))
	EUOULightBeamShape BeamShape = EUOULightBeamShape::Cone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape|Cone", meta = (ClampMin = "1.0", ClampMax = "89.0", EditCondition = "BeamShape == EUOULightBeamShape::Cone", EditConditionHides, ToolTip = "SpotLight 컴포넌트를 찾지 못했을 때 사용할 원뿔 각도입니다."))
	float FallbackOuterConeAngle = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape|Cone", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "BeamShape == EUOULightBeamShape::Cone", EditConditionHides, ToolTip = "SpotLight 컴포넌트를 찾지 못했을 때 사용할 내부 원뿔 비율입니다."))
	float FallbackInnerConeRatio = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape", meta = (ClampMin = "0.0", Units = "cm", DisplayName = "빛 총 길이", ToolTip = "원뿔과 원기둥 모두에서 직접광과 모든 반사 구간을 합친 최대 경로 길이입니다."))
	float BeamLength = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape|Cylinder", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "BeamShape == EUOULightBeamShape::Cylinder", EditConditionHides, ToolTip = "원기둥형 빛의 반지름입니다."))
	float CylinderRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape|Cylinder", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "BeamShape == EUOULightBeamShape::Cylinder && bUseAngleFalloff", EditConditionHides, ToolTip = "원기둥 반지름 중 최대 광량을 유지하는 내부 영역의 비율입니다."))
	float CylinderInnerRadiusRatio = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.0", ToolTip = "거리/각도 감쇠 전 수신체에 적용할 기본 게임플레이 빛 세기입니다."))
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "광원과의 거리에 따라 빛 세기를 줄입니다."))
	bool bUseDistanceFalloff = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.01", EditCondition = "bUseDistanceFalloff", DisplayName = "거리 감쇠 지수", ToolTip = "거리 감쇠 곡선의 강도를 조절합니다. 1은 선형이며, 값이 클수록 멀리까지 밝게 유지되고 값이 작을수록 가까이에서 빠르게 약해집니다."))
	float DistanceFalloffExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "광원 원뿔 가장자리로 갈수록 빛 세기를 줄입니다."))
	bool bUseAngleFalloff = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.0", ToolTip = "빛 샘플링 간격입니다. 0이면 매 틱 샘플링합니다."))
	float SampleInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision", meta = (ToolTip = "빛 수신체와 상호작용 표면을 찾을 오브젝트 타입 목록입니다."))
	TArray<TEnumAsByte<EObjectTypeQuery>> ReceiverObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision", meta = (ToolTip = "광원에서 수신체까지 막히지 않은 라인 트레이스가 필요하도록 합니다."))
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision", meta = (ToolTip = "광원, 표면, 수신체 사이의 차폐 여부를 검사할 트레이스 채널입니다."))
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision", meta = (ToolTip = "빛 가시성 트레이스에서 소유 액터를 무시합니다."))
	bool bIgnoreOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision|Water Ice", meta = (ToolTip = "아래로 향하는 빛이 물·얼음 바닥 타일을 관통하여 같은 빛줄기 안의 여러 타일에 도달하게 합니다."))
	bool bUseWaterIceAnglePassthrough = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision|Water Ice", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg", EditCondition = "bUseWaterIceAnglePassthrough", ToolTip = "수평면보다 이 각도 이상 아래로 향하는 빛은 물·얼음 타일을 통과합니다. 수평에 가까운 빛은 첫 얼음에서 차단됩니다."))
	float WaterIcePassthroughMinDownwardAngleDegrees = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ToolTip = "빛 상호작용 표면이 반사된 게임플레이 빛을 발생시킬 수 있게 합니다."))
	bool bEnableReflectedLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0", ToolTip = "한 틱에 처리할 최대 반사 표면 수입니다."))
	int32 MaxReflectionSurfacesPerTick = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "1", ClampMax = "16", ToolTip = "하나의 반사 경로에서 허용할 최대 반사 횟수입니다. 우산과 거울을 모두 포함합니다."))
	int32 MaxReflectionBouncesPerPath = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0", ToolTip = "누적 감쇠된 빛 세기가 이 값 이하가 되면 이후 반사 탐색을 중단합니다."))
	float MinimumReflectedIntensity = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Stability", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "반사 경로 위치 변화가 이 값 이하이면 같은 경로로 판단합니다."))
	float ReflectionPathPositionTolerance = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Stability", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg", ToolTip = "반사 경로 방향 변화가 이 각도 이하이면 같은 경로로 판단합니다."))
	float ReflectionPathDirectionToleranceDegrees = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Stability", meta = (ClampMin = "0.0", ToolTip = "반사 경로 광량 변화가 이 값 이하이면 같은 경로로 판단합니다."))
	float ReflectionPathIntensityTolerance = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection|Stability", meta = (ClampMin = "0.0", Units = "s", ToolTip = "거울 가장자리에서 반사 경로가 순간적으로 줄어들었을 때 직전 경로를 유지하는 시간입니다."))
	float ReflectionPathLossGraceTime = 0.1f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막 샘플링에서 검사한 수신체 개수입니다."))
	int32 LastReceiverCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막 샘플링에서 빛을 받은 수신체 개수입니다. 반사광도 포함됩니다."))
	int32 LastLitCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막 샘플링에서 차단된 트레이스 개수입니다."))
	int32 LastBlockedCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막 샘플링에서 반사광을 받은 수신체 개수입니다."))
	int32 LastReflectedCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막으로 이 광원에서 빛을 받은 수신체의 디버그 이름입니다."))
	FString LastLitTargetName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막으로 이 광원을 차단한 컴포넌트의 디버그 이름입니다."))
	FString LastBlockedName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막으로 이 광원을 반사한 표면의 디버그 이름입니다."))
	FString LastReflectorName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막 샘플링에서 확인된 가장 긴 연속 반사 횟수입니다."))
	int32 LastReflectionBounceCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (ToolTip = "마지막 샘플링에서 확인된 가장 긴 반사 경로입니다."))
	FString LastReflectionPath = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (DisplayName = "Reflection Paths", ToolTip = "마지막 빛 샘플링에서 계산된 반사 경로 데이터입니다. VFX와 퍼즐 로직에서 사용할 수 있습니다."))
	TArray<FUOULightReflectionPathData> ReflectionPaths;

	UPROPERTY(BlueprintAssignable, Category = "Light|Reflection", meta = (ToolTip = "계산된 반사 경로가 이전 결과와 달라졌을 때 호출됩니다."))
	FUOULightReflectionPathsUpdatedSignature OnReflectionPathsUpdated;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime", meta = (DisplayName = "Light Paths", ToolTip = "직접광과 반사광을 모두 포함한 최종 빛 경로 데이터입니다."))
	TArray<FUOULightPathData> LightPaths;

	UPROPERTY(BlueprintAssignable, Category = "Light|Path", meta = (ToolTip = "최종 빛 경로가 변경됐을 때 호출됩니다. VFX와 보조 조명은 이 이벤트를 사용합니다."))
	FUOULightPathsUpdatedSignature OnLightPathsUpdated;

	UFUNCTION(BlueprintCallable, Category = "Light", meta = (ToolTip = "입력된 DeltaTime으로 게임플레이 빛 샘플링을 한 번 실행합니다."))
	void EmitLight(float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Light|Reflection", meta = (ToolTip = "마지막으로 계산된 모든 반사 경로를 반환합니다."))
	TArray<FUOULightReflectionPathData> GetReflectionPaths() const
	{
		return ReflectionPaths;
	}

	UFUNCTION(BlueprintPure, Category = "Light|Path", meta = (ToolTip = "직접광과 반사광을 포함한 최종 빛 경로를 반환합니다."))
	TArray<FUOULightPathData> GetLightPaths() const
	{
		return LightPaths;
	}

	// 많은 타일이 같은 프레임의 경로를 읽을 때 배열 복사를 피하기 위한 C++ 전용 접근자입니다.
	const TArray<FUOULightPathData>& GetLightPathsView() const
	{
		return LightPaths;
	}

	UFUNCTION(BlueprintCallable, Category = "Light", meta = (ToolTip = "Fallback 원뿔 각도와 게임플레이 빛 세기를 한 번에 설정합니다."))
	void Configure(float NewConeAngle, float NewIntensity);

protected:
	struct FPendingExposureCandidate
	{
		FUOULightExposureData ExposureData;
		FString StablePathKey;
		bool bReflected = false;
	};

	using FPendingExposureMap = TMap<UObject*, FPendingExposureCandidate>;

	float PendingDeltaTime = 0.0f;

	UPROPERTY(Transient)
	TArray<FUOULightReflectionPathData> LastPublishedReflectionPaths;

	UPROPERTY(Transient)
	TArray<FUOULightPathData> LastPublishedLightPaths;

	bool bHasPublishedReflectionPaths = false;
	bool bHasPublishedLightPaths = false;
	float ReflectionPathLossStartWorldTime = -1.0f;

	void ValidateSettings();
	void PublishComputedPaths(bool bAllowLossGrace = true);
	void NotifyReflectionPathsUpdatedIfChanged(bool bAllowLossGrace = true);
	void RebuildLightPaths();
	void NotifyLightPathsUpdatedIfChanged();
	FUOULightPathSegmentData BuildDirectLightPathSegment(
		const FUOULightReflectionSegmentData* FirstReflectionSegment) const;
	EUOULightPathHitType ClassifyLightPathHit(
		const FHitResult& Hit,
		UUOULightInteractionSurfaceComponent*& OutInteractionSurface,
		TArray<TObjectPtr<UObject>>& OutReachedReceivers) const;
	bool AreLightPathsEquivalent(
		const TArray<FUOULightPathData>& A,
		const TArray<FUOULightPathData>& B) const;
	void NormalizeReflectionPathOrder();
	bool AreReflectionPathsEquivalent(
		const TArray<FUOULightReflectionPathData>& A,
		const TArray<FUOULightReflectionPathData>& B) const;
	static bool HasReflectionPathTopologyLoss(
		const TArray<FUOULightReflectionPathData>& PreviousPaths,
		const TArray<FUOULightReflectionPathData>& CurrentPaths);
	bool WasReflectingFromSurface(
		const UUOULightInteractionSurfaceComponent* SurfaceComponent) const;
	float ResolveRequiredBeamFootprintCoverageRatio(
		const UUOULightInteractionSurfaceComponent* SurfaceComponent) const;
	float ResolveMaximumReflectionIncidenceAngle(
		const UUOULightInteractionSurfaceComponent* SurfaceComponent) const;
	USceneComponent* GetReferencedSourceTransform() const;
	USpotLightComponent* GetSourceSpotLightComponent() const;
	FVector GetSourceLocation() const;
	FVector GetSourceForwardVector() const;
	float GetExposureRange() const;
	float GetReceiverSearchRadius() const;
	float GetEffectiveOuterConeAngle() const;
	float GetEffectiveInnerConeAngle(float OuterConeAngle) const;
	bool TryEvaluateSourceBeamPoint(
		const FVector& WorldPosition,
		float& OutDistance,
		FVector& OutDirection,
		float& OutDistanceFactor,
		float& OutShapeFactor) const;
	FCollisionObjectQueryParams BuildReceiverObjectQueryParams() const;
	void AppendReceivableObjects(AActor* TargetActor, UPrimitiveComponent* TargetComponent, TArray<UObject*>& OutReceivers) const;
	void AppendLightInteractionSurfaces(AActor* TargetActor, UPrimitiveComponent* TargetComponent, TArray<UUOULightInteractionSurfaceComponent*>& OutSurfaces) const;
	bool TryBuildExposureData(UObject* ReceiverObject, float DeltaTime, FUOULightExposureData& OutExposureData, FHitResult& OutBlockingHit) const;
	bool TryBuildExposureDataAtPosition(
		UObject* ReceiverObject,
		const FVector& ReceiverPosition,
		float DeltaTime,
		FUOULightExposureData& OutExposureData,
		FHitResult& OutBlockingHit) const;
	void GetReceiverSamplePositions(
		UObject* ReceiverObject,
		const FVector& BeamDirection,
		TArray<FVector>& OutSamplePositions,
		int32& OutRequiredHits) const;
	bool HasLineOfSight(UObject* ReceiverObject, const FVector& SourcePosition, const FVector& ReceiverPosition, FHitResult& OutBlockingHit) const;
	bool HasLineOfSightFrom(
		UObject* ReceiverObject,
		const FVector& TraceStart,
		const FVector& ReceiverPosition,
		FHitResult& OutBlockingHit,
		const AActor* IgnoredActor,
		const UPrimitiveComponent* IgnoredComponent) const;
	bool IsWorldPositionInsideUmbrellaLightShade(const FVector& WorldPosition) const;
	bool TraceLightPathSingle(
		FHitResult& OutHit,
		const FVector& TraceStart,
		const FVector& TraceEnd,
		const FCollisionQueryParams& QueryParams,
		const AActor* IgnoredShadeOwner = nullptr,
		float BeamStartRadius = -1.0f,
		float BeamConeAngle = 0.0f) const;
	bool FindNearestUmbrellaLightShadeHit(
		const FVector& TraceStart,
		const FVector& TraceEnd,
		FHitResult& OutHit,
		const AActor* IgnoredShadeOwner = nullptr,
		float BeamStartRadius = -1.0f,
		float BeamConeAngle = 0.0f) const;
	bool TryBuildLightInteractionSurfaceHit(UUOULightInteractionSurfaceComponent* SurfaceComponent, FHitResult& OutSurfaceHit) const;
	void RecordExposureCandidate(
		UObject* ReceiverObject,
		const FUOULightExposureData& ExposureData,
		bool bReflected,
		const FString& StablePathKey,
		FPendingExposureMap& PendingExposures) const;
	void DeliverPendingExposures(const FPendingExposureMap& PendingExposures);
	void EmitReflectedLightFromSurface(
		UUOULightInteractionSurfaceComponent* SurfaceComponent,
		const FHitResult& SurfaceHit,
		float DeltaTime,
		FPendingExposureMap& PendingExposures);
	void EmitReflectedLightToReceivers(
		UUOULightInteractionSurfaceComponent* SurfaceComponent,
		const FVector& ReflectionOrigin,
		const FVector& ReflectedDirection,
		float MaximumDistance,
		float BeamStartRadius,
		float BeamConeAngle,
		float SurfaceIntensity,
		float DeltaTime,
		const FString& StablePathKey,
		FPendingExposureMap& PendingExposures,
		TArray<TObjectPtr<UObject>>& OutReachedReceivers);
	bool TryFindNextReflectionSurface(
		const UUOULightInteractionSurfaceComponent* CurrentSurface,
		const FVector& ReflectionOrigin,
		const FVector& ReflectedDirection,
		float MaximumDistance,
		float BeamStartRadius,
		float BeamConeAngle,
		const TSet<const UUOULightInteractionSurfaceComponent*>& VisitedSurfaces,
		UUOULightInteractionSurfaceComponent*& OutSurface,
		FHitResult& OutSurfaceHit,
		float& OutDistance,
		float& OutAngle) const;
	float CalculateReflectedSegmentIntensity(
		const UUOULightInteractionSurfaceComponent* SurfaceComponent,
		float IncomingIntensity,
		float MaximumDistance,
		float Distance,
		float Angle,
		float BeamConeAngle) const;
	bool TryBuildReflectedExposureData(
		UObject* ReceiverObject,
		const UUOULightInteractionSurfaceComponent* SurfaceComponent,
		const FVector& ReflectionOrigin,
		const FVector& ReflectedDirection,
		float MaximumDistance,
		float BeamStartRadius,
		float BeamConeAngle,
		float SurfaceIntensity,
		float DeltaTime,
		FUOULightExposureData& OutExposureData,
		FHitResult& OutBlockingHit) const;
	bool TryBuildReflectedExposureDataAtPosition(
		UObject* ReceiverObject,
		const FVector& ReceiverPosition,
		const UUOULightInteractionSurfaceComponent* SurfaceComponent,
		const FVector& ReflectionOrigin,
		const FVector& ReflectedDirection,
		float MaximumDistance,
		float BeamStartRadius,
		float BeamConeAngle,
		float SurfaceIntensity,
		float DeltaTime,
		FUOULightExposureData& OutExposureData,
		FHitResult& OutBlockingHit) const;
	float CalculateIntensity(float Distance, float Angle, float& OutDistanceFactor, float& OutAngleFactor) const;
	float CalculateDistanceFalloffFactor(float Distance, float MaximumDistance) const;
	float CalculateConeFactor(float Angle, float ConeAngle) const;
	float CalculateCylinderFactor(float RadialDistance) const;
	static void AddActorPrimitiveComponentsToIgnore(
		const AActor* Actor,
		FCollisionQueryParams& QueryParams,
		const UPrimitiveComponent* ComponentToKeep = nullptr);
	static void AddReceiverSelfComponentsToIgnore(UObject* ReceiverObject, FCollisionQueryParams& QueryParams);
	static AActor* ResolveReceiverActor(UObject* ReceiverObject);
	static FString GetReceivableDebugName(UObject* ReceiverObject);
};
