// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "World/Wind/UOUWindTypes.h"
#include "UOUWindEmitterActor.generated.h"

class UArrowComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUOUWindPathChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUOUWindPhaseChangedSignature, bool, bIsBlowing);

// 선풍기처럼 현재 방향으로 바람을 방출하고 반사 표면에 따라 실시간 경로를 계산합니다.
UCLASS(meta=(DisplayName="UOU Wind Emitter"))
class UNDERONEUMBRELLA_API AUOUWindEmitterActor
	: public AActor
	, public IUOUPuzzleResultReceiver
	, public IUOUPuzzleDebugInfoProvider
{
	GENERATED_BODY()

public:
	AUOUWindEmitterActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<UStaticMeshComponent> EmitterVisual = nullptr;

	// 이 화살표의 위치와 Forward가 바람의 시작점과 최초 방향이 됩니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<UArrowComponent> WindOrigin = nullptr;

	// 에디터에서 직선 바람의 최대 범위와 반경을 보여 주는 와이어 박스입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind|Preview")
	TObjectPtr<UBoxComponent> WindRangePreview = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Preview")
	bool bShowWindRangePreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay")
	bool bWindEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxWindDistance = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "1.0", Units = "cm"))
	float WindRadius = 120.0f;

	// Emitter가 바람 방향으로 직접 더하는 실제 가속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float WindAcceleration = 700.0f;

	// 수신체에 합산되는 바람 가속도 벡터의 최대 크기입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float MaximumWindAcceleration = 1200.0f;

	// 바람 방향으로 누적되는 캐릭터 속도의 최대값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MaximumWindSpeed = 1400.0f;

	// 바람에 처음 진입할 때 즉시 보장할 최소 바람 방향 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Entry", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MinimumWindEntrySpeed = 400.0f;

	// 진입 직전 낙하 속도 중 바람 방향 속도로 전환할 비율입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Entry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FallingMomentumConversion = 0.5f;

	// DeltaTime 기반 가속과 별개로 진입 순간 한 번만 더하는 고정 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Entry", meta = (ClampMin = "0.0", Units = "cm/s"))
	float InitialWindVelocityBoost = 150.0f;

	// 낙하 관성을 전환하더라도 진입 순간 이 속도를 넘지 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Entry", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MaximumWindEntrySpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay")
	bool bUseRadialFalloff = true;

	// 켜면 Wind On Duration과 Wind Off Duration을 번갈아 반복합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Pulse")
	bool bUsePulseCycle = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Pulse", meta = (EditCondition = "bUsePulseCycle", ClampMin = "0.05", Units = "s"))
	float WindOnDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Pulse", meta = (EditCondition = "bUsePulseCycle", ClampMin = "0.05", Units = "s"))
	float WindOffDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Pulse", meta = (EditCondition = "bUsePulseCycle"))
	bool bStartCycleWithWind = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Reflection", meta = (ClampMin = "0", ClampMax = "8"))
	int32 MaxReflections = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Reflection", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float MinimumReflectedStrength = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Collision")
	TEnumAsByte<ECollisionChannel> WindTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Collision")
	TArray<TEnumAsByte<EObjectTypeQuery>> ReceiverObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Debug")
	bool bDrawWindDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Debug", meta = (ClampMin = "0.0"))
	float DebugDrawTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	TArray<FUOUWindPathSegment> WindPathSegments;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	int32 LastAffectedReceiverCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wind|Runtime")
	FUOUWindPulseRuntimeState PulseRuntimeState;

	UPROPERTY(BlueprintAssignable, Category = "Wind|Events")
	FOnUOUWindPathChangedSignature OnWindPathChanged;

	UPROPERTY(BlueprintAssignable, Category = "Wind|Events")
	FOnUOUWindPhaseChangedSignature OnWindPhaseChanged;

	UFUNCTION(BlueprintCallable, Category = "Wind")
	void SetWindEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure, Category = "Wind")
	bool IsWindEnabled() const { return bWindEnabled; }

	UFUNCTION(BlueprintPure, Category = "Wind|Pulse")
	bool IsWindBlowing() const;

	UFUNCTION(BlueprintCallable, Category = "Wind|Pulse")
	void SetPulseCycleEnabled(bool bNewPulseCycleEnabled);

	UFUNCTION(BlueprintCallable, Category = "Wind|Pulse")
	void ResetPulseCycle();

	UFUNCTION(BlueprintCallable, Category = "Wind")
	void RebuildWindPath();

	UFUNCTION(BlueprintPure, Category = "Wind")
	TArray<FUOUWindPathSegment> GetWindPathSegments() const { return WindPathSegments; }

private:
	void ValidateSettings();
	void UpdateWindRangePreview();
	void InitializePulseCycleState();
	void UpdatePulseCycle(float DeltaSeconds);
	void HandleWindPhaseChanged(bool bWasBlowing);
	void ApplyWindToReceivers(float DeltaSeconds);
	void DrawWindDebug() const;
	FCollisionObjectQueryParams BuildReceiverObjectQueryParams() const;
	void AppendWindReceivers(
		AActor* TargetActor,
		UPrimitiveComponent* TargetComponent,
		TArray<UObject*>& OutReceivers) const;
};
