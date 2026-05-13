// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "InputCoreTypes.h"
#include "UOUUmbrellaComponent.generated.h"

class UArrowComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UUOURainReceiverComponent;
class UUOUWaterContainerComponent;

// ???닿굅?뺤? ?곗궛???꾩옱 ?대뼡 ?곹깭濡??숈옉 以묒씤吏 ?섑??몃떎.
UENUM(BlueprintType)
enum class EUOUUmbrellaState : uint8
{
	Closed,
	Open,
	UpsideDown,
	Pouring
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUmbrellaStateChangedSignature, EUOUUmbrellaState, NewState, bool, bHasUmbrella);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUmbrellaRainBlockedSignature, float, BlockedAmount);

// ???대옒?ㅻ뒗 ?곗궛 ?뚯쑀 ?щ?? ?곹깭 ?꾪솚, 鍮??몄텧 諛섏쓳, 臾?諛쏄린? 遺볤린 ?먮쫫???대떦?쒕떎.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella"))
class UUOUUmbrellaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUUmbrellaComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Umbrella")
	FOnUmbrellaStateChangedSignature OnUmbrellaStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Umbrella")
	FOnUmbrellaRainBlockedSignature OnRainBlocked;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Ownership")
	bool bStartWithUmbrella = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey ToggleUmbrellaKey = EKeys::F;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey InvertUmbrellaKey = EKeys::G;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey PourKey = EKeys::RightMouseButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey DebugFillKey = EKeys::T;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	bool bEnableDebugFillKey = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input", meta = (ClampMin = "0.0"))
	float DebugFillAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<USceneComponent> PickupAttachPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<USceneComponent> HeldVisualAnchor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UArrowComponent> PourOrigin = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> ClosedVisual = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> OpenVisual = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> UpsideDownVisual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> RuntimeHeldVisual = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual")
	TObjectPtr<UStaticMesh> DefaultHeldMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual")
	FVector HeldVisualRelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual")
	bool bUsePickupMeshRelativeScale = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Debug")
	bool bShowScreenDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Debug", meta = (ToolTip = "열린 우산이 비를 막는 중심 위치와 반지름을 월드에 표시합니다."))
	bool bDrawRainBlockerDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Debug", meta = (ClampMin = "0.0", ToolTip = "Rain Blocker 디버그 원과 중심점의 선 두께입니다."))
	float RainBlockerDebugThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water", meta = (ClampMin = "0.0"))
	float PourRate = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water", meta = (ClampMin = "0.0"))
	float PourDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water")
	TEnumAsByte<ECollisionChannel> PourTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim")
	bool bRotateOwnerTowardsPourDirection = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim", meta = (ClampMin = "0.0"))
	float MouseAimRayDistance = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim")
	TEnumAsByte<ECollisionChannel> MouseAimTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain", meta = (ClampMin = "0.0"))
	float StoredWaterWeightMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain Block", meta = (ClampMin = "0.0", ToolTip = "우산이 비를 막는 시각적 반지름입니다. RainArea와 Niagara가 이 값을 사용해 빗줄기 차단과 우산 물 튐 범위를 표현합니다."))
	float RainBlockerRadius = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain Block", meta = (ToolTip = "비 차단 중심을 우산 표시 컴포넌트 기준으로 보정하는 로컬 오프셋입니다. 우산 물 튐 위치가 맞지 않을 때 조정합니다."))
	FVector RainBlockerLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Dependencies")
	TObjectPtr<UUOUWaterContainerComponent> StoredWaterContainer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Dependencies")
	TObjectPtr<UUOURainReceiverComponent> RainReceiver = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	bool bHasUmbrella = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	EUOUUmbrellaState CurrentState = EUOUUmbrellaState::Closed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	FString LastPourHitName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	FString LastPourTargetName = TEXT("None");

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void AcquireUmbrella();

	void AcquireUmbrellaFromMeshComponent(UStaticMeshComponent* SourceMeshComponent);

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void RemoveUmbrella();

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void OpenUmbrella();

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void CloseUmbrella();

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void TurnUmbrellaUpsideDown();

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void BeginPour();

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void EndPour();

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void AddCollectedWater(float WaterAmount);

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ApplyRainExposure(float ExposureAmount);

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ToggleOpenState();

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ToggleInvertState();

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void HandleInputPressed(FKey InputKey);

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void HandleInputReleased(FKey InputKey);

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool CanCollectWater() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsOpen() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsUpsideDown() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsPouring() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool BlocksJumping() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsBlockingRain() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool TryGetRainBlockerData(FVector& OutWorldLocation, float& OutRadius) const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	float GetCurrentStoredWater() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	float GetCurrentPlayerRainAmount() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	FKey GetToggleUmbrellaKey() const { return ToggleUmbrellaKey; }

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	FKey GetInvertUmbrellaKey() const { return InvertUmbrellaKey; }

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	FKey GetPourKey() const { return PourKey; }

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	FKey GetDebugFillKey() const { return DebugFillKey; }

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsDebugFillEnabled() const { return bEnableDebugFillKey; }

protected:
	void SetState(EUOUUmbrellaState NewState);
	void RefreshVisuals();
	void ResolveReferences();
	void EnsureRuntimeHeldVisual();
	void ApplyHeldVisualFromMeshComponent(UStaticMeshComponent* SourceMeshComponent);
	void ApplyHeldVisualFromAssets(UStaticMesh* Mesh, const TArray<TObjectPtr<UMaterialInterface>>& Materials, const FVector& SourceRelativeScale);
	FTransform GetHeldVisualRelativeTransform(const FVector& SourceRelativeScale) const;
	void DrawScreenDebug() const;
	void DrawRainBlockerDebug() const;
	void UpdatePourAimFacing();
	void ClearPourAimFacing();
	void UpdatePouring(float DeltaTime);
	bool TryGetMouseAimDirection(FVector& AimDirection, FVector& AimPoint) const;
	bool TryGetPourDirection(FVector& PourOriginLocation, FVector& PourDirection) const;
	bool TryReceiveWaterAtHit(const FHitResult& HitResult, float WaterAmount);
	bool ShouldSpillStoredWater(EUOUUmbrellaState PreviousState, EUOUUmbrellaState NextState) const;
	void SpillStoredWater();
};
