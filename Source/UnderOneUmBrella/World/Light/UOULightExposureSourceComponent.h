// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightExposureTypes.h"
#include "UOULightExposureSourceComponent.generated.h"

class UPrimitiveComponent;
class ULocalLightComponent;
class USceneComponent;
class USpotLightComponent;
class UUOULightInteractionSurfaceComponent;

UCLASS(ClassGroup=(Light), meta=(BlueprintSpawnableComponent, DisplayName="UOU Light Exposure Source"))
class UUOULightExposureSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOULightExposureSourceComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Source")
	bool bEmitLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Source", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SpotLightComponent", DisplayName = "Source Transform"))
	FComponentReference SourceTransformReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Source")
	bool bAutoFindSourceSpotLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float FallbackOuterConeAngle = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FallbackInnerConeRatio = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.0"))
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	bool bUseDistanceFalloff = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	bool bUseAngleFalloff = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ClampMin = "0.0"))
	float SampleInterval = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision")
	TArray<TEnumAsByte<EObjectTypeQuery>> ReceiverObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision")
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Collision")
	bool bIgnoreOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection")
	bool bEnableReflectedLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection", meta = (ClampMin = "0"))
	int32 MaxReflectionSurfacesPerTick = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Debug", meta = (ClampMin = "0.0"))
	float DebugDrawTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	int32 LastReceiverCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	int32 LastLitCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	int32 LastBlockedCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	int32 LastReflectedCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	FString LastLitTargetName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	FString LastBlockedName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Runtime")
	FString LastReflectorName = TEXT("None");

	UFUNCTION(BlueprintCallable, Category = "Light")
	void EmitLight(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Light")
	void Configure(float NewConeAngle, float NewIntensity);

protected:
	float PendingDeltaTime = 0.0f;

	void ValidateSettings();
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
	bool TryBuildLightInteractionSurfaceHit(UUOULightInteractionSurfaceComponent* SurfaceComponent, FHitResult& OutSurfaceHit) const;
	void EmitReflectedLightFromSurface(
		UUOULightInteractionSurfaceComponent* SurfaceComponent,
		const FHitResult& SurfaceHit,
		float DeltaTime,
		TSet<UObject*>& LitReceivers);
	bool TryBuildReflectedExposureData(
		UObject* ReceiverObject,
		const UUOULightInteractionSurfaceComponent* SurfaceComponent,
		const FVector& ReflectionOrigin,
		const FVector& ReflectedDirection,
		float SurfaceIntensity,
		float DeltaTime,
		FUOULightExposureData& OutExposureData,
		FHitResult& OutBlockingHit) const;
	float CalculateIntensity(float Distance, float Angle, float& OutDistanceFactor, float& OutAngleFactor) const;
	float CalculateConeFactor(float Angle, float ConeAngle) const;
	void DrawDebugSource() const;
	void DrawDebugResult(const FUOULightExposureData& ExposureData, bool bLit) const;
	void DrawDebugBlockedHit(const FVector& SourcePosition, const FHitResult& BlockingHit) const;
	void DrawDebugReflectionRay(const FVector& Start, const FVector& End) const;
	static void AddActorPrimitiveComponentsToIgnore(
		const AActor* Actor,
		FCollisionQueryParams& QueryParams,
		const UPrimitiveComponent* ComponentToKeep = nullptr);
	static void AddReceiverSelfComponentsToIgnore(UObject* ReceiverObject, FCollisionQueryParams& QueryParams);
	static AActor* ResolveReceiverActor(UObject* ReceiverObject);
	static FString GetReceivableDebugName(UObject* ReceiverObject);
};
