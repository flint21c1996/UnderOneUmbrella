// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightExposureTypes.h"
#include "World/Light/UOULightReflectionPathTypes.h"
#include "UOULightExposureSourceComponent.generated.h"

class UPrimitiveComponent;
class ULocalLightComponent;
class USceneComponent;
class USpotLightComponent;
class UUOULightInteractionSurfaceComponent;
class UUOUUmbrellaLightShadeVolumeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOULightReflectionPathsUpdatedSignature,
	const TArray<FUOULightReflectionPathData>&,
	ReflectionPaths);

// 스포트라이트 형태의 광원에서 주변 수신체로 게임플레이용 빛 노출을 전달합니다.
UCLASS(ClassGroup=(Light), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Exposure Source", ToolTip = "광원 원뿔 범위 안의 수신체에 게임플레이용 빛 노출을 전달합니다."))
class UUOULightExposureSourceComponent : public UActorComponent, public IUOUPuzzleDebugInfoProvider
{
	GENERATED_BODY()

public:
	UUOULightExposureSourceComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Source", meta = (ToolTip = "이 광원에서 게임플레이용 빛을 발사할지 여부입니다."))
	bool bEmitLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Source", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SpotLightComponent", DisplayName = "Source Transform", ToolTip = "광원의 위치, 방향, 거리, 원뿔 각도를 가져올 SpotLight 컴포넌트입니다."))
	FComponentReference SourceTransformReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Source", meta = (ToolTip = "Source Transform이 비어 있으면 소유 액터의 첫 번째 SpotLight 컴포넌트를 자동으로 사용합니다."))
	bool bAutoFindSourceSpotLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape", meta = (ClampMin = "1.0", ClampMax = "89.0", ToolTip = "SpotLight 컴포넌트를 찾지 못했을 때 사용할 원뿔 각도입니다."))
	float FallbackOuterConeAngle = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "SpotLight 컴포넌트를 찾지 못했을 때 사용할 내부 원뿔 비율입니다."))
	float FallbackInnerConeRatio = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.0", ToolTip = "거리/각도 감쇠 전 수신체에 적용할 기본 게임플레이 빛 세기입니다."))
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "광원과의 거리에 따라 빛 세기를 줄입니다."))
	bool bUseDistanceFalloff = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "광원 원뿔 가장자리로 갈수록 빛 세기를 줄입니다."))
	bool bUseAngleFalloff = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.0", ToolTip = "빛 샘플링 간격입니다. 0이면 매 틱 샘플링합니다."))
	float SampleInterval = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision", meta = (ToolTip = "빛 수신체와 상호작용 표면을 찾을 오브젝트 타입 목록입니다."))
	TArray<TEnumAsByte<EObjectTypeQuery>> ReceiverObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision", meta = (ToolTip = "광원에서 수신체까지 막히지 않은 라인 트레이스가 필요하도록 합니다."))
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision", meta = (ToolTip = "광원, 표면, 수신체 사이의 차폐 여부를 검사할 트레이스 채널입니다."))
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision", meta = (ToolTip = "빛 가시성 트레이스에서 소유 액터를 무시합니다."))
	bool bIgnoreOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ToolTip = "빛 상호작용 표면이 반사된 게임플레이 빛을 발생시킬 수 있게 합니다."))
	bool bEnableReflectedLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0", ToolTip = "한 틱에 처리할 최대 반사 표면 수입니다."))
	int32 MaxReflectionSurfacesPerTick = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "1", ClampMax = "16", ToolTip = "하나의 반사 경로에서 허용할 최대 반사 횟수입니다. 우산과 거울을 모두 포함합니다."))
	int32 MaxReflectionBouncesPerPath = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0.0", ToolTip = "누적 감쇠된 빛 세기가 이 값 이하가 되면 이후 반사 탐색을 중단합니다."))
	float MinimumReflectedIntensity = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ToolTip = "광원 원뿔, 히트 라인, 반사 디버그 도형을 그립니다."))
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ClampMin = "0.0", ToolTip = "디버그 드로우 유지 시간입니다. 0이면 한 프레임만 표시합니다."))
	float DebugDrawTime = 0.0f;

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

	UFUNCTION(BlueprintCallable, Category = "Light", meta = (ToolTip = "입력된 DeltaTime으로 게임플레이 빛 샘플링을 한 번 실행합니다."))
	void EmitLight(float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Light|Reflection", meta = (ToolTip = "마지막으로 계산된 모든 반사 경로를 반환합니다."))
	TArray<FUOULightReflectionPathData> GetReflectionPaths() const
	{
		return ReflectionPaths;
	}

	UFUNCTION(BlueprintCallable, Category = "Light", meta = (ToolTip = "Fallback 원뿔 각도와 게임플레이 빛 세기를 한 번에 설정합니다."))
	void Configure(float NewConeAngle, float NewIntensity);

protected:
	float PendingDeltaTime = 0.0f;

	UPROPERTY(Transient)
	TArray<FUOULightReflectionPathData> LastPublishedReflectionPaths;

	bool bHasPublishedReflectionPaths = false;

	void ValidateSettings();
	void NotifyReflectionPathsUpdatedIfChanged();
	static bool AreReflectionPathsEquivalent(
		const TArray<FUOULightReflectionPathData>& A,
		const TArray<FUOULightReflectionPathData>& B);
	USceneComponent* GetReferencedSourceTransform() const;
	USpotLightComponent* GetSourceSpotLightComponent() const;
	ULocalLightComponent* GetSourceLocalLightComponent() const;
	FVector GetSourceLocation() const;
	FVector GetSourceForwardVector() const;
	float GetExposureRange() const;
	float GetEffectiveOuterConeAngle() const;
	float GetEffectiveInnerConeAngle(float OuterConeAngle) const;
	FCollisionObjectQueryParams BuildReceiverObjectQueryParams() const;
	void AppendReceivableObjects(AActor* TargetActor, UPrimitiveComponent* TargetComponent, TArray<UObject*>& OutReceivers) const;
	void AppendLightInteractionSurfaces(AActor* TargetActor, UPrimitiveComponent* TargetComponent, TArray<UUOULightInteractionSurfaceComponent*>& OutSurfaces) const;
	bool TryBuildExposureData(UObject* ReceiverObject, float DeltaTime, FUOULightExposureData& OutExposureData, FHitResult& OutBlockingHit) const;
	bool HasLineOfSight(UObject* ReceiverObject, const FVector& SourcePosition, const FVector& ReceiverPosition, FHitResult& OutBlockingHit) const;
	bool HasLineOfSightFrom(
		UObject* ReceiverObject,
		const FVector& TraceStart,
		const FVector& ReceiverPosition,
		FHitResult& OutBlockingHit,
		const AActor* IgnoredActor,
		const UPrimitiveComponent* IgnoredComponent) const;
	bool IsWorldPositionInsideUmbrellaLightShade(const FVector& WorldPosition) const;
	bool TryBuildLightInteractionSurfaceHit(UUOULightInteractionSurfaceComponent* SurfaceComponent, FHitResult& OutSurfaceHit) const;
	void EmitReflectedLightFromSurface(
		UUOULightInteractionSurfaceComponent* SurfaceComponent,
		const FHitResult& SurfaceHit,
		float DeltaTime,
		TSet<UObject*>& LitReceivers);
	void EmitReflectedLightToReceivers(
		UUOULightInteractionSurfaceComponent* SurfaceComponent,
		const FVector& ReflectionOrigin,
		const FVector& ReflectedDirection,
		float BeamStartRadius,
		float BeamConeAngle,
		float SurfaceIntensity,
		float DeltaTime,
		TSet<UObject*>& LitReceivers,
		TArray<TObjectPtr<UObject>>& OutReachedReceivers);
	bool TryFindNextReflectionSurface(
		const UUOULightInteractionSurfaceComponent* CurrentSurface,
		const FVector& ReflectionOrigin,
		const FVector& ReflectedDirection,
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
		float Distance,
		float Angle,
		float BeamConeAngle) const;
	bool TryBuildReflectedExposureData(
		UObject* ReceiverObject,
		const UUOULightInteractionSurfaceComponent* SurfaceComponent,
		const FVector& ReflectionOrigin,
		const FVector& ReflectedDirection,
		float BeamStartRadius,
		float BeamConeAngle,
		float SurfaceIntensity,
		float DeltaTime,
		FUOULightExposureData& OutExposureData,
		FHitResult& OutBlockingHit) const;
	float CalculateIntensity(float Distance, float Angle, float& OutDistanceFactor, float& OutAngleFactor) const;
	float CalculateConeFactor(float Angle, float ConeAngle) const;
	void DrawDebugSource() const;
	void DrawDebugResult(const FUOULightExposureData& ExposureData, bool bLit) const;
	void DrawDebugBlockedHit(const FVector& SourcePosition, const FHitResult& BlockingHit) const;
	void DrawDebugReflectionFrustum(
		const FVector& Start,
		const FVector& Direction,
		float Length,
		float ConeAngleDegrees,
		float StartRadius,
		const FColor& Color) const;
	static void AddActorPrimitiveComponentsToIgnore(
		const AActor* Actor,
		FCollisionQueryParams& QueryParams,
		const UPrimitiveComponent* ComponentToKeep = nullptr);
	static void AddReceiverSelfComponentsToIgnore(UObject* ReceiverObject, FCollisionQueryParams& QueryParams);
	static AActor* ResolveReceiverActor(UObject* ReceiverObject);
	static FString GetReceivableDebugName(UObject* ReceiverObject);
};
