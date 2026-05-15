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

// 우산이 현재 어떤 상태인지 구분하는 열거형이다.
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

// 우산 소유 여부, 상태 전환, 물 저장, 붓기 동작을 한 번에 관리하는 핵심 컴포넌트다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella"))
class UUOUUmbrellaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 우산 기본 키와 물 관련 수치를 초기화한다.
	UUOUUmbrellaComponent();

	// 시작 시 플레이어 기준점과 의존 컴포넌트를 연결한다.
	virtual void BeginPlay() override;

	// 붓는 중이라면 매 프레임 실제 붓기 처리를 진행한다.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Umbrella")
	FOnUmbrellaStateChangedSignature OnUmbrellaStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Umbrella")
	FOnUmbrellaRainBlockedSignature OnRainBlocked;

	// 시작부터 우산을 가진 상태로 테스트할지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Ownership")
	bool bStartWithUmbrella = false;

	// 우산 열기와 닫기에 대응하는 기본 키다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey ToggleUmbrellaKey = EKeys::F;

	// 우산 뒤집기에 대응하는 기본 키다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey InvertUmbrellaKey = EKeys::G;

	// 우산 붓기 시작과 유지에 대응하는 기본 키다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey PourKey = EKeys::RightMouseButton;

	// 우산 테스트용 물 채우기에 대응하는 기본 키다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey DebugFillKey = EKeys::T;

	// 테스트용 물 채우기 키 사용 여부를 제어한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	bool bEnableDebugFillKey = true;

	// 테스트용 물 채우기 키를 한 번 눌렀을 때 더할 양이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input", meta = (ClampMin = "0.0"))
	float DebugFillAmount = 1.0f;

	// 우산을 손에 붙이는 기준점이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<USceneComponent> PickupAttachPoint = nullptr;

	// 손에 든 우산 메시를 미세하게 보정하는 앵커다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<USceneComponent> HeldVisualAnchor = nullptr;

	// 붓기 시작 위치와 방향을 잡는 화살표 기준점이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UArrowComponent> PourOrigin = nullptr;

	// 에디터에서 개별 메시를 따로 넣고 싶을 때 쓰는 닫힌 우산 비주얼이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> ClosedVisual = nullptr;

	// 에디터에서 개별 메시를 따로 넣고 싶을 때 쓰는 펼친 우산 비주얼이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> OpenVisual = nullptr;

	// 에디터에서 개별 메시를 따로 넣고 싶을 때 쓰는 뒤집힌 우산 비주얼이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> UpsideDownVisual = nullptr;

	// 실제 플레이 중 손에 붙는 런타임 우산 메시다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> RuntimeHeldVisual = nullptr;

	// 별도 픽업 메시가 없을 때 기본으로 쓸 손 우산 메시다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual")
	TObjectPtr<UStaticMesh> DefaultHeldMesh = nullptr;

	// 손에 든 우산 메시의 최종 상대 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual")
	FVector HeldVisualRelativeScale = FVector::OneVector;

	// 픽업 메시의 원래 스케일을 손 우산에도 반영할지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual")
	bool bUsePickupMeshRelativeScale = true;

	// 화면 왼쪽 위 우산 디버그 출력 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Debug")
	bool bShowScreenDebug = true;

	// 초당 얼마나 빨리 물을 붓는지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water", meta = (ClampMin = "0.0"))
	float PourRate = 1.5f;

	// 우산에서 실제로 물을 전달할 최대 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water", meta = (ClampMin = "0.0"))
	float PourDistance = 300.0f;

	// 물 붓기 판정을 위한 트레이스 채널이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water")
	TEnumAsByte<ECollisionChannel> PourTraceChannel = ECC_Visibility;

	// 붓는 동안 플레이어를 마우스 방향으로 돌릴지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim")
	bool bRotateOwnerTowardsPourDirection = true;

	// 마우스 에임 방향을 계산할 최대 레이 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim", meta = (ClampMin = "0.0"))
	float MouseAimRayDistance = 10000.0f;

	// 마우스 에임 판정에 쓸 채널이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim")
	TEnumAsByte<ECollisionChannel> MouseAimTraceChannel = ECC_Visibility;

	// 우산에 저장된 물이 무게에 얼마나 기여할지 정하는 배수다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain", meta = (ClampMin = "0.0"))
	float StoredWaterWeightMultiplier = 1.0f;

	// 우산이 모은 물을 저장하는 컨테이너다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Dependencies")
	TObjectPtr<UUOUWaterContainerComponent> StoredWaterContainer = nullptr;

	// 플레이어가 직접 비를 맞는 양을 추적하는 의존 컴포넌트다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Dependencies")
	TObjectPtr<UUOURainReceiverComponent> RainReceiver = nullptr;

	// 현재 실제로 우산을 들고 있는지 나타내는 상태값이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	bool bHasUmbrella = false;

	// 현재 우산이 닫힘, 펼침, 뒤집힘, 붓기 중 어디에 있는지 나타낸다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	EUOUUmbrellaState CurrentState = EUOUUmbrellaState::Closed;

	// 마지막으로 물 붓기 트레이스가 맞은 컴포넌트 이름을 남긴다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	FString LastPourHitName = TEXT("None");

	// 마지막으로 실제 물을 전달한 대상 이름을 남긴다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	FString LastPourTargetName = TEXT("None");

	// 우산 픽업을 획득 처리하고 손에 붙는 비주얼을 준비한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void AcquireUmbrella();

	// 소스 메시를 그대로 가져와 런타임 우산으로 붙인다.
	void AcquireUmbrellaFromMeshComponent(UStaticMeshComponent* SourceMeshComponent);

	// 우산 소유 상태를 지우고 손에 든 비주얼을 정리한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void RemoveUmbrella();

	// 우산을 펼친 상태로 전환한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void OpenUmbrella();

	// 우산을 닫힌 상태로 전환한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void CloseUmbrella();

	// 우산을 물 받기 가능한 뒤집힌 상태로 전환한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void TurnUmbrellaUpsideDown();

	// 현재 저장된 물을 바닥이나 퍼즐 대상에 붓기 시작한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void BeginPour();

	// 붓기 상태를 종료하고 에임 보정을 정리한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void EndPour();

	// 빗물이나 테스트 입력으로 들어온 물을 우산 저장량에 더한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void AddCollectedWater(float WaterAmount);

	// 우산 상태에 따라 플레이어 비 노출이나 빗물 차단을 반영한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ApplyRainExposure(float ExposureAmount);

	// 열린 상태와 닫힌 상태를 상황에 맞게 토글한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ToggleOpenState();

	// 뒤집힌 상태와 일반 상태를 상황에 맞게 토글한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ToggleInvertState();

	// 키 입력 하나를 우산 상태 전환 규칙으로 해석한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void HandleInputPressed(FKey InputKey);

	// 키 해제 입력을 받아 붓기 종료 같은 후속 처리를 한다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void HandleInputReleased(FKey InputKey);

	// 현재 상태에서 우산이 물을 받을 수 있는지 계산한다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool CanCollectWater() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool HasUmbrella() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsClosed() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsOpen() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsUpsideDown() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsPouring() const;

	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool BlocksJumping() const;

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
	// 내부 상태를 바꾸고 관련 이벤트와 비주얼을 함께 갱신한다.
	void SetState(EUOUUmbrellaState NewState);

	// 현재 우산 상태에 맞는 비주얼만 보이게 정리한다.
	void RefreshVisuals();

	// 필요한 기준점과 의존 컴포넌트를 자동으로 찾는다.
	void ResolveReferences();

	// 손에 들 런타임 메시가 없으면 생성한다.
	void EnsureRuntimeHeldVisual();

	// 픽업 메시 정보를 손에 든 우산 메시로 복사한다.
	void ApplyHeldVisualFromMeshComponent(UStaticMeshComponent* SourceMeshComponent);

	// 메시와 머티리얼 자산으로 손에 든 우산 비주얼을 구성한다.
	void ApplyHeldVisualFromAssets(UStaticMesh* Mesh, const TArray<TObjectPtr<UMaterialInterface>>& Materials, const FVector& SourceRelativeScale);

	// 손에 든 우산의 최종 상대 위치와 스케일을 계산한다.
	FTransform GetHeldVisualRelativeTransform(const FVector& SourceRelativeScale) const;

	// 우산 상태와 저장 물을 화면 왼쪽 위에 출력한다.
	void DrawScreenDebug() const;

	// 붓는 동안 플레이어가 마우스 방향을 보도록 회전 보정한다.
	void UpdatePourAimFacing();

	// 붓기 종료 후 강제 회전 보정을 해제한다.
	void ClearPourAimFacing();

	// 매 프레임 실제 저장 물을 줄이며 타깃에 전달한다.
	void UpdatePouring(float DeltaTime);

	// 마우스 커서 방향에서 실제 월드 에임 방향을 계산한다.
	bool TryGetMouseAimDirection(FVector& AimDirection, FVector& AimPoint) const;

	// 실제 붓기 시작점과 방향을 계산한다.
	bool TryGetPourDirection(FVector& PourOriginLocation, FVector& PourDirection) const;

	// 라인트레이스로 맞은 대상이 물을 받을 수 있는지 검사하고 전달한다.
	bool TryReceiveWaterAtHit(const FHitResult& HitResult, float WaterAmount);

	// 상태 전환 과정에서 물을 쏟아야 하는지 판단한다.
	bool ShouldSpillStoredWater(EUOUUmbrellaState PreviousState, EUOUUmbrellaState NextState) const;

	// 현재 저장된 물을 모두 버리고 관련 상태를 갱신한다.
	void SpillStoredWater();
};
