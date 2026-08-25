// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugProvider.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightExposureTypes.h"
#include "UOULightCountBulbActor.generated.h"

class UMaterialInstanceDynamic;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UUOULightExposureReceiverComponent;
class UUOUPuzzleConditionSourceComponent;

UENUM(BlueprintType)
enum class EUOULightCountBulbState : uint8
{
	Off UMETA(DisplayName = "꺼짐"),
	Insufficient UMETA(DisplayName = "부족"),
	Satisfied UMETA(DisplayName = "만족"),
	Overheated UMETA(DisplayName = "과열")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnUOULightCountBulbStateChangedSignature,
	EUOULightCountBulbState,
	NewState,
	EUOULightCountBulbState,
	PreviousState,
	int32,
	ActiveLightCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnUOULightCountBulbSatisfiedChangedSignature,
	bool,
	bIsSatisfied);

// 동시에 도달한 고유 광원 개수를 전구의 부족, 만족, 과열 상태로 변환하는 퍼즐 액터입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Light Count Bulb"))
class UNDERONEUMBRELLA_API AUOULightCountBulbActor
	: public AActor
	, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	AUOULightCountBulbActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetDebugConnections_Implementation(
		TArray<FUOUDebugConnection>& OutConnections) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bulb|Components")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bulb|Components")
	TObjectPtr<UStaticMeshComponent> BulbMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bulb|Components", meta = (ToolTip = "메시 충돌과 독립적으로 게임플레이 빛을 감지하는 범위입니다."))
	TObjectPtr<USphereComponent> LightReceiverVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bulb|Components")
	TObjectPtr<UUOULightExposureReceiverComponent> LightReceiver = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bulb|Components")
	TObjectPtr<UUOUPuzzleConditionSourceComponent> PuzzleConditionSource = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|State", meta = (ClampMin = "1", ToolTip = "전구가 만족 상태가 되기 위해 동시에 받아야 하는 고유 광원 개수입니다."))
	int32 RequiredLightCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Puzzle", meta = (ToolTip = "이 전구의 정답으로 인정할 원본 광원 액터 목록입니다. 반사광은 반사판이 아니라 원본 광원 액터로 판정합니다."))
	TArray<TObjectPtr<AActor>> AllowedPuzzleSourceActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|State", meta = (ClampMin = "0.0", Units = "s", ToolTip = "마지막 노출 이후 광원이 사라졌다고 판정하기까지 기다리는 시간입니다. 광원의 Sample Interval보다 길게 설정합니다."))
	float LightSourceLossGraceTime = 0.15f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Bulb|Runtime")
	EUOULightCountBulbState CurrentState = EUOULightCountBulbState::Off;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Bulb|Runtime")
	int32 ActiveLightCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual")
	FLinearColor OffColor = FLinearColor(0.03f, 0.03f, 0.03f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual")
	FLinearColor InsufficientColor = FLinearColor(0.35f, 0.22f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual")
	FLinearColor SatisfiedColor = FLinearColor(1.0f, 0.72f, 0.32f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual")
	FLinearColor OverheatedColor = FLinearColor(1.0f, 0.08f, 0.03f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual", meta = (ClampMin = "0.0"))
	float OffEmissiveIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual", meta = (ClampMin = "0.0"))
	float InsufficientEmissiveIntensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual", meta = (ClampMin = "0.0"))
	float SatisfiedEmissiveIntensity = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual", meta = (ClampMin = "0.0"))
	float OverheatedEmissiveIntensity = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual", meta = (ClampMin = "0.0", Units = "s", ToolTip = "상태가 바뀔 때 현재 표시값에서 새 색상과 발광 세기로 전환하는 시간입니다. 0이면 즉시 변경합니다."))
	float VisualTransitionDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual|Material")
	FName PrimaryColorParameterName = TEXT("BaseColor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual|Material")
	FName SecondaryColorParameterName = TEXT("Color");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual|Material")
	FName EmissiveColorParameterName = TEXT("EmissiveColor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bulb|Visual|Material")
	FName EmissiveIntensityParameterName = TEXT("EmissiveIntensity");

	UPROPERTY(BlueprintAssignable, Category = "Bulb|Events")
	FOnUOULightCountBulbStateChangedSignature OnBulbStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Bulb|Events")
	FOnUOULightCountBulbSatisfiedChangedSignature OnBulbSatisfiedChanged;

	UFUNCTION(BlueprintPure, Category = "Bulb|State")
	bool IsSatisfied() const { return CurrentState == EUOULightCountBulbState::Satisfied; }

	UFUNCTION(BlueprintPure, Category = "Bulb|Puzzle")
	bool IsPuzzleSatisfied() const;

	UFUNCTION(BlueprintPure, Category = "Bulb|State")
	static EUOULightCountBulbState EvaluateState(int32 LightCount, int32 RequiredCount);

	UFUNCTION(BlueprintCallable, Category = "Bulb|State", meta = (ToolTip = "만료된 광원을 제거하고 현재 상태를 다시 계산하며, 필요하면 새 외형으로 전환을 시작합니다."))
	void RefreshBulbState();

protected:
	UFUNCTION()
	void HandleLightExposureReceived(const FUOULightExposureData& ExposureData);

	void SetBulbState(EUOULightCountBulbState NewState);
	void RefreshPuzzleSatisfiedState();
	bool IsPuzzleSourceAllowed(const UObject* SourceObject) const;
	void EnsureRuntimeMaterials();
	void BeginVisualTransition();
	void UpdateVisualTransition(float DeltaTime);
	void ApplyVisualValues(const FLinearColor& Color, float EmissiveIntensity);
	void UpdateTickEnabled();
	FLinearColor GetStateColor() const;
	float GetStateEmissiveIntensity() const;
	FString GetDebugStateName() const;
	FColor GetDebugStateColor() const;

	// 광원별 마지막 샘플 주기를 반영한 만료 시각입니다. 반복 샘플은 같은 키의 시각만 연장합니다.
	TMap<TWeakObjectPtr<UObject>, float> ActiveLightExpirationTimes;
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuntimeMaterialInstances;

	FLinearColor VisualStartColor = FLinearColor::Black;
	FLinearColor CurrentVisualColor = FLinearColor::Black;
	FLinearColor VisualTargetColor = FLinearColor::Black;
	float VisualStartEmissiveIntensity = 0.0f;
	float CurrentVisualEmissiveIntensity = 0.0f;
	float VisualTargetEmissiveIntensity = 0.0f;
	float VisualTransitionElapsedTime = 0.0f;
	bool bVisualTransitionActive = false;
};
