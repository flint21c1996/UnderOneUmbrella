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
class UUOUAudioCueComponent;
class UUOUWaterContainerComponent;

// 우산이 현재 어떤 형태로 사용되는지 나타내는 상태입니다.
// 입력 처리, 점프 제한, 비 차단, 물 붓기 판단이 이 값을 기준으로 갈라집니다.
UENUM(BlueprintType)
enum class EUOUUmbrellaState : uint8
{
	Closed,
	Open,
	UpsideDown,
	Pouring
};

// 우산 손잡이 방향을 나타내는 독립 상태입니다.
// 펼침/접힘 상태와 분리해서 갈고리 같은 역방향 상호작용 조건에 사용합니다.
UENUM(BlueprintType)
enum class EUOUUmbrellaDirectionState : uint8
{
	Normal,
	Reversed
};

// 우산에서 부은 물이 실제로 전달된 대상의 종류입니다.
// 디버그 텍스트와 라인트레이스 결과 확인에 사용합니다.
UENUM(BlueprintType)
enum class EUOUUmbrellaPourReceiverType : uint8
{
	None,
	UmbrellaWaterTarget,
	WaterBasinTarget,
	WaterContainer
};

// 우산 보유 여부나 상태가 바뀌었을 때 블루프린트와 다른 시스템에 알려주는 이벤트입니다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUmbrellaStateChangedSignature, EUOUUmbrellaState, NewState, bool, bHasUmbrella);

// 펼친 우산이 비를 막았을 때 막아낸 양을 전달하는 이벤트입니다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUmbrellaRainBlockedSignature, float, BlockedAmount);

// 플레이어가 들고 있는 우산의 보유 상태, 형태 전환, 물 저장과 붓기, 비 차단을 담당하는 컴포넌트입니다.
// 캐릭터 블루프린트에 붙여두고 참조 컴포넌트와 입력 키를 디테일 창에서 조정할 수 있게 열어둡니다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella"))
class UUOUUmbrellaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 기본 틱을 켜서 물 붓기, 조준 회전, 디버그 표시를 매 프레임 갱신합니다.
	UUOUUmbrellaComponent();

	// 시작 시 참조 컴포넌트를 찾고 초기 우산 보유 상태와 비주얼을 맞춥니다.
	virtual void BeginPlay() override;

	// 물 붓기 진행, 마우스 조준 회전, 화면과 월드 디버그 표시를 갱신합니다.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 우산 상태가 바뀔 때 외부 블루프린트가 후속 처리를 붙일 수 있는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Umbrella")
	FOnUmbrellaStateChangedSignature OnUmbrellaStateChanged;

	// 우산이 비를 막아낸 값을 비 시스템이나 연출 쪽으로 넘기기 위한 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Umbrella")
	FOnUmbrellaRainBlockedSignature OnRainBlocked;

	// 플레이 시작 시 우산을 이미 들고 시작할지 정하는 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Ownership")
	bool bStartWithUmbrella = false;

	// 닫힘과 펼침을 전환하는 기본 키입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey ToggleUmbrellaKey = EKeys::F;

	// 우산을 뒤집거나 닫힘으로 되돌리는 기본 키입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey InvertUmbrellaKey = EKeys::R;

	// 뒤집힌 우산에 담긴 물을 붓기 시작하고 멈추는 기본 키입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey PourKey = EKeys::RightMouseButton;

	// 테스트 중 우산 물 저장량을 빠르게 채우는 디버그 키입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey DebugFillKey = EKeys::T;

	// 디버그 물 채우기 키를 사용할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	bool bEnableDebugFillKey = true;

	// 디버그 키를 한 번 누를 때 채워지는 물의 양입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input", meta = (ClampMin = "0.0"))
	float DebugFillAmount = 1.0f;

	// 우산을 새로 획득했을 때 재생할 오디오 이벤트 ID입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Audio", meta = (ToolTip = "오디오 DataAsset에 등록된 이벤트 ID입니다."))
	FName AcquireAudioEventId = TEXT("Umbrella.Acquire");

	// 우산을 펼쳤을 때 재생할 오디오 이벤트 ID입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Audio", meta = (ToolTip = "오디오 DataAsset에 등록된 이벤트 ID입니다."))
	FName OpenAudioEventId = TEXT("Umbrella.Open");

	// 우산을 접었을 때 재생할 오디오 이벤트 ID입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Audio", meta = (ToolTip = "오디오 DataAsset에 등록된 이벤트 ID입니다."))
	FName CloseAudioEventId = TEXT("Umbrella.Close");

	// AudioCueComponent가 있을 때 우산 획득 상황에 사용할 Cue ID입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Audio", meta = (ToolTip = "Owner에 AudioCueComponent가 있으면 이 Cue를 먼저 재생합니다. 실패하면 AcquireAudioEventId를 fallback으로 사용합니다."))
	FName AcquireAudioCueId = TEXT("Acquire");

	// AudioCueComponent가 있을 때 우산 펼침 상황에 사용할 Cue ID입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Audio", meta = (ToolTip = "Owner에 AudioCueComponent가 있으면 이 Cue를 먼저 재생합니다. 실패하면 OpenAudioEventId를 fallback으로 사용합니다."))
	FName OpenAudioCueId = TEXT("Open");

	// AudioCueComponent가 있을 때 우산 접힘 상황에 사용할 Cue ID입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Audio", meta = (ToolTip = "Owner에 AudioCueComponent가 있으면 이 Cue를 먼저 재생합니다. 실패하면 CloseAudioEventId를 fallback으로 사용합니다."))
	FName CloseAudioCueId = TEXT("Close");

	// 우산을 캐릭터에 붙일 기준 위치입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<USceneComponent> PickupAttachPoint = nullptr;

	// 픽업한 우산 메쉬가 실제로 보일 위치와 회전을 잡는 보조 앵커입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<USceneComponent> HeldVisualAnchor = nullptr;

	// 물을 부을 때 라인트레이스가 시작되는 위치와 기본 방향입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UArrowComponent> PourOrigin = nullptr;

	// 닫힌 상태 전용 비주얼을 별도로 둘 때 사용하는 메쉬 컴포넌트입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> ClosedVisual = nullptr;

	// 펼친 상태 전용 비주얼을 별도로 둘 때 사용하는 메쉬 컴포넌트입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> OpenVisual = nullptr;

	// 뒤집힌 상태 전용 비주얼을 별도로 둘 때 사용하는 메쉬 컴포넌트입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> UpsideDownVisual = nullptr;

	// 픽업한 우산 메쉬를 런타임에 복사해서 플레이어 손에 보여주는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UStaticMeshComponent> RuntimeHeldVisual = nullptr;

	// 픽업 메쉬가 없거나 시작 보유 상태일 때 사용할 기본 우산 메쉬입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual")
	TObjectPtr<UStaticMesh> DefaultHeldMesh = nullptr;

	// 손에 붙은 우산 비주얼의 최종 크기를 보정하는 배율입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual")
	FVector HeldVisualRelativeScale = FVector::OneVector;

	// 상태별 전용 비주얼이 없을 때, 뒤집힘/붓기 상태에서 런타임 우산 메쉬를 임시로 뒤집어 보여줍니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual", meta = (ToolTip = "UpsideDownVisual을 따로 만들기 전까지 RuntimeHeldVisual을 회전시켜 우산이 뒤집힌 상태임을 보여줍니다."))
	bool bFlipRuntimeHeldVisualWhenUpsideDown = true;

	// 런타임 우산 메쉬를 뒤집힌 상태로 보여줄 때 추가할 로컬 회전 오프셋입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual", meta = (EditCondition = "bFlipRuntimeHeldVisualWhenUpsideDown", ToolTip = "뒤집힘/붓기 상태에서 RuntimeHeldVisual에 추가로 적용할 로컬 회전입니다. 메쉬 축이 맞지 않으면 BP에서 조정합니다."))
	FRotator UpsideDownHeldVisualRotationOffset = FRotator(180.0f, 0.0f, 0.0f);

	// 런타임 우산 메쉬를 뒤집힌 상태로 보여줄 때 추가할 상대 위치 오프셋입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual", meta = (EditCondition = "bFlipRuntimeHeldVisualWhenUpsideDown", ToolTip = "뒤집힌 메쉬의 중심점이 맞지 않을 때 손 위치 기준으로 보정할 상대 위치입니다."))
	FVector UpsideDownHeldVisualLocationOffset = FVector(0.0f, 0.0f, 150.0f);

	// 월드에 놓인 픽업 메쉬의 상대 스케일을 손에 든 비주얼에도 반영할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Visual")
	bool bUsePickupMeshRelativeScale = true;

	// 화면 디버그 표시 여부는 이제 Debug Controller의 Player HUD가 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Umbrella|Debug", meta = (ToolTip = "이 값은 더 이상 화면 디버그 표시 여부를 결정하지 않습니다. Debug Controller의 Player HUD 옵션을 사용합니다."))
	bool bShowScreenDebug = false;

	// 우산 월드 디버그 표시 여부는 이제 Debug Controller의 Player 카테고리가 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Umbrella|Debug", meta = (ToolTip = "이 값은 더 이상 비 차단 월드 디버그 표시 여부를 결정하지 않습니다. Debug Controller의 Player World Debug 옵션을 사용합니다."))
	bool bDrawRainBlockerDebug = true;

	// 비 차단 디버그 선과 중심점의 두께입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Debug", meta = (ClampMin = "0.0", ToolTip = "비 차단 디버그 선과 중심점의 두께입니다."))
	float RainBlockerDebugThickness = 2.0f;

	// 물 붓기 라인트레이스 표시 여부는 이제 Debug Controller의 Player 카테고리가 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Umbrella|Debug", meta = (ToolTip = "이 값은 더 이상 물 붓기 라인트레이스 표시 여부를 결정하지 않습니다. Debug Controller의 Player World Debug 옵션을 사용합니다."))
	bool bDrawPourTraceDebug = true;

	// 물 붓기 라벨 표시 여부는 이제 Debug Controller의 Player 카테고리가 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Umbrella|Debug", meta = (ToolTip = "이 값은 더 이상 물 붓기 라벨 표시 여부를 결정하지 않습니다. Debug Controller의 Player World Label 옵션을 사용합니다."))
	bool bDrawPourTraceDebugLabel = true;

	// 물 붓기 라인트레이스 디버그 선의 두께입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Debug", meta = (ClampMin = "0.0"))
	float PourTraceDebugThickness = 3.0f;

	// 물 붓기 디버그 선과 라벨이 유지되는 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Debug", meta = (ClampMin = "0.0"))
	float PourTraceDebugLifeTime = 0.0f;

	// 초당 붓는 물의 양입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water", meta = (ClampMin = "0.0"))
	float PourRate = 1.5f;

	// 물 붓기 라인트레이스가 닿을 수 있는 최대 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water", meta = (ClampMin = "0.0"))
	float PourDistance = 300.0f;

	// 물을 부을 대상을 찾을 때 사용하는 충돌 채널입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water")
	TEnumAsByte<ECollisionChannel> PourTraceChannel = ECC_Visibility;

	// 물을 붓는 동안 플레이어가 마우스 방향을 바라보게 할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim")
	bool bRotateOwnerTowardsPourDirection = true;

	// 마우스 위치를 월드 방향으로 환산할 때 사용하는 보조 레이 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim", meta = (ClampMin = "0.0"))
	float MouseAimRayDistance = 10000.0f;

	// 마우스 아래 월드 지점을 찾을 때 사용하는 충돌 채널입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim")
	TEnumAsByte<ECollisionChannel> MouseAimTraceChannel = ECC_Visibility;

	// 우산에 저장된 물이 무게 계산에 얼마나 영향을 줄지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain", meta = (ClampMin = "0.0"))
	float StoredWaterWeightMultiplier = 1.0f;

	// 우산이 비 파티클을 제거할 박스 볼륨의 절반 크기입니다. XY는 우산 면적, Z는 얇은 차단 두께로 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain Block", meta = (ClampMin = "0.0", ToolTip = "우산이 만드는 Kill Volume Box의 절반 크기입니다. 파티클이 이 박스 안에 들어오면 제거됩니다."))
	FVector RainBlockerVolumeHalfExtent = FVector(90.0f, 90.0f, 20.0f);

	// 비 차단 중심을 우산 기준 위치에서 얼마나 옮길지 정하는 로컬 오프셋입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain Block", meta = (ToolTip = "비 차단 중심을 우산 표시 컴포넌트 기준으로 보정하는 로컬 오프셋입니다. 우산 문양 위치가 맞지 않을 때 조정합니다."))
	FVector RainBlockerLocalOffset = FVector::ZeroVector;

	// 우산 안에 모인 물을 저장하고 무게 계산에 넘기는 컨테이너입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Dependencies")
	TObjectPtr<UUOUWaterContainerComponent> StoredWaterContainer = nullptr;

	// 플레이어가 직접 비를 맞은 양을 저장하는 컴포넌트입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Dependencies")
	TObjectPtr<UUOURainReceiverComponent> RainReceiver = nullptr;

	// 현재 플레이어가 우산을 소유하고 있는지 나타내는 런타임 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	bool bHasUmbrella = false;

	// 현재 우산의 형태와 동작 상태입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	EUOUUmbrellaState CurrentState = EUOUUmbrellaState::Closed;

	// 현재 우산 손잡이가 정방향인지 역방향인지 나타냅니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	EUOUUmbrellaDirectionState CurrentDirectionState = EUOUUmbrellaDirectionState::Normal;

	// 마지막 물 붓기 라인트레이스가 맞춘 컴포넌트 이름입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	FString LastPourHitName = TEXT("None");

	// 마지막으로 물을 실제로 받은 대상 이름입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	FString LastPourTargetName = TEXT("None");

	// 마지막으로 물을 받은 대상의 종류입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	EUOUUmbrellaPourReceiverType LastPourReceiverType = EUOUUmbrellaPourReceiverType::None;

	// 마지막 물 붓기 라인트레이스를 그릴 수 있는 데이터가 있는지 나타냅니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	bool bHasLastPourTrace = false;

	// 마지막 물 붓기 라인트레이스가 무언가를 맞췄는지 나타냅니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	bool bLastPourTraceHit = false;

	// 마지막 물 붓기 라인트레이스가 실제로 물을 전달했는지 나타냅니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	bool bLastPourDeliveredWater = false;

	// 마지막 물 붓기 라인트레이스의 시작 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	FVector LastPourTraceStart = FVector::ZeroVector;

	// 마지막 물 붓기 라인트레이스의 끝 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	FVector LastPourTraceEnd = FVector::ZeroVector;

	// 마지막 물 붓기 라인트레이스가 맞은 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	FVector LastPourTraceImpactPoint = FVector::ZeroVector;

	// 마지막 ImpactPoint가 WaterBasinTarget 영역 판정에 사용되었는지 나타냅니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	bool bLastPourCheckedWaterBasinImpactPoint = false;

	// 마지막 ImpactPoint가 맞은 WaterBasinTarget의 Basin 영역 안에 있었는지 나타냅니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	bool bLastPourImpactPointInsideWaterBasin = false;

	// 마지막 프레임에 붓기로 사용한 물의 양입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	float LastPourAmount = 0.0f;

	// 마지막 물 붓기 직전 저장된 물의 양입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	float LastPourStoredWaterBefore = 0.0f;

	// 마지막 물 붓기 직후 저장된 물의 양입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	float LastPourStoredWaterAfter = 0.0f;

	// 우산을 획득 상태로 바꾸고 기본 닫힘 상태로 초기화합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void AcquireUmbrella();

	// 월드 픽업 메쉬의 외형을 복사해서 우산을 획득합니다.
	void AcquireUmbrellaFromMeshComponent(UStaticMeshComponent* SourceMeshComponent);

	// 우산 보유 상태를 해제하고 저장된 물과 비 노출 값을 정리합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void RemoveUmbrella();

	// 우산을 펼친 상태로 전환합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void OpenUmbrella();

	// 우산을 닫힌 상태로 전환합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void CloseUmbrella();

	// 우산을 뒤집어 물을 받을 수 있는 상태로 전환합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void TurnUmbrellaUpsideDown();

	// 뒤집힌 우산에 물이 있을 때 붓기 상태로 진입합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void BeginPour();

	// 붓기 상태를 끝내고 다시 뒤집힌 상태로 돌아갑니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void EndPour();

	// 비나 외부 시스템에서 전달한 물 양을 우산 저장소에 더합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void AddCollectedWater(float WaterAmount);

	// 비에 노출된 양을 받아 우산 상태에 따라 차단, 피격, 저장으로 나눕니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ApplyRainExposure(float ExposureAmount);

	// 현재 상태에 맞춰 닫힘과 펼침 계열 상태를 토글합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ToggleOpenState();

	// 현재 상태에 맞춰 뒤집힘과 닫힘 계열 상태를 토글합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ToggleInvertState();

	// 캐릭터나 입력 시스템에서 전달한 키 입력을 우산 동작으로 분기합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void HandleInputPressed(FKey InputKey);

	// 키 릴리즈 입력을 받아 유지형 동작인 물 붓기를 종료합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void HandleInputReleased(FKey InputKey);

	// 현재 우산이 물을 받을 수 있는 상태인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool CanCollectWater() const;

	// 우산 보유 여부를 외부에서 읽기 위한 함수입니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool HasUmbrella() const;

	// 우산을 들고 있고 닫힌 상태인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsClosed() const;

	// 우산을 들고 있고 펼친 상태인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsOpen() const;

	// 우산을 들고 있고 뒤집힌 상태인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsUpsideDown() const;

	// 우산을 들고 있고 물을 붓는 중인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsPouring() const;

	// 우산 손잡이가 정방향인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsNormalDirection() const;

	// 우산 손잡이가 역방향인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsReversedDirection() const;

	// 우산 상태 때문에 점프가 막혀야 하는지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool BlocksJumping() const;

	// 현재 우산이 비를 막는 상태인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsBlockingRain() const;

	// 현재 설정된 우산 비 차단 박스의 중심, 회전, 절반 크기를 계산합니다. 실제 차단 활성 여부는 IsBlockingRain()으로 따로 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool TryGetRainBlockerVolumeData(FVector& OutWorldCenter, FRotator& OutWorldRotation, FVector& OutHalfExtent) const;

	// 우산에 현재 저장된 물 양을 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	float GetCurrentStoredWater() const;

	// 플레이어가 현재 비를 맞아 누적된 양을 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	float GetCurrentPlayerRainAmount() const;

	// 우산 펼침 토글에 쓰는 키를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	FKey GetToggleUmbrellaKey() const { return ToggleUmbrellaKey; }

	// 우산 뒤집기 토글에 쓰는 키를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	FKey GetInvertUmbrellaKey() const { return InvertUmbrellaKey; }

	// 물 붓기에 쓰는 키를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	FKey GetPourKey() const { return PourKey; }

	// 디버그 물 채우기에 쓰는 키를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	FKey GetDebugFillKey() const { return DebugFillKey; }

	// 디버그 물 채우기 입력이 켜져 있는지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsDebugFillEnabled() const { return bEnableDebugFillKey; }

protected:
	// 우산 상태를 안전하게 바꾸고 비주얼과 이벤트를 함께 갱신합니다.
	void SetState(EUOUUmbrellaState NewState);

	// 현재 상태와 보유 여부에 맞게 우산 비주얼 표시를 맞춥니다.
	void RefreshVisuals();

	// 우산 관련 오디오 이벤트를 플레이어 위치에서 재생합니다.
	void PlayUmbrellaAudioEvent(FName AudioEventId) const;

	// AudioCueComponent가 있으면 Cue를 우선 재생하고, 없으면 기존 EventId를 재생합니다.
	void PlayUmbrellaAudioCue(FName CueId, FName FallbackAudioEventId) const;

	// Owner에 붙은 AudioCueComponent를 반환합니다.
	UUOUAudioCueComponent* GetAudioCueComponent() const;

	// 우산 오디오를 재생할 월드 위치를 반환합니다.
	FVector GetUmbrellaAudioLocation() const;

	// 블루프린트에서 비워둔 참조를 이름이나 컴포넌트 타입으로 자동 보완합니다.
	void ResolveReferences();

	// 픽업한 우산을 손에 보여줄 런타임 메쉬 컴포넌트를 준비합니다.
	void EnsureRuntimeHeldVisual();

	// 픽업 액터의 메쉬와 머티리얼 정보를 손에 든 우산 비주얼로 복사합니다.
	void ApplyHeldVisualFromMeshComponent(UStaticMeshComponent* SourceMeshComponent);

	// 직접 전달받은 메쉬와 머티리얼을 런타임 우산 비주얼에 적용합니다.
	void ApplyHeldVisualFromAssets(UStaticMesh* Mesh, const TArray<TObjectPtr<UMaterialInterface>>& Materials, const FVector& SourceRelativeScale);

	// 손에 든 우산 비주얼의 로컬 위치, 회전, 스케일을 계산합니다.
	FTransform GetHeldVisualRelativeTransform(const FVector& SourceRelativeScale) const;

	// 현재 우산 상태에 맞게 런타임 우산 메쉬의 임시 회전 보정을 적용합니다.
	void ApplyRuntimeHeldVisualStateTransform();

	// 런타임 우산 메쉬를 뒤집힌 상태로 보여줘야 하는지 확인합니다.
	bool ShouldFlipRuntimeHeldVisual() const;

	// 우산 상태와 물 정보를 화면 디버그 텍스트로 표시합니다.
	void DrawScreenDebug() const;

	// 우산이 실제로 비를 막는 위치와 범위를 월드 디버그 선과 구로 그립니다.
	void DrawRainBlockerDebug() const;

	// 물 붓기 라인트레이스와 마지막 전달 결과를 월드 디버그로 그립니다.
	void DrawPourTraceDebug() const;

	// 물 붓기 디버그에 사용하는 마지막 트레이스 기록을 비웁니다.
	void ClearPourTraceDebug();

	// 물을 붓는 동안 플레이어가 마우스 방향을 바라보도록 회전을 보정합니다.
	void UpdatePourAimFacing();

	// 붓기 조준 회전에서 남겨둔 상태를 정리하기 위한 자리입니다.
	void ClearPourAimFacing();

	// 저장된 물을 시간에 따라 줄이고 라인트레이스로 물을 받을 대상을 찾습니다.
	void UpdatePouring(float DeltaTime);

	// 마우스 위치를 기준으로 플레이어가 바라볼 평면 방향을 계산합니다.
	bool TryGetMouseAimDirection(FVector& AimDirection, FVector& AimPoint) const;

	// 물 붓기 시작 위치와 최종 방향을 계산합니다.
	bool TryGetPourDirection(FVector& PourOriginLocation, FVector& PourDirection) const;

	// 라인트레이스에 맞은 액터가 물을 받을 수 있으면 물을 전달하고 대상 종류를 기록합니다.
	bool TryReceiveWaterAtHit(const FHitResult& HitResult, float WaterAmount, float PourDuration, const FVector& PourDirection, EUOUUmbrellaPourReceiverType& OutReceiverType);

	// 상태 전환 과정에서 저장된 물을 버려야 하는지 판단합니다.
	bool ShouldSpillStoredWater(EUOUUmbrellaState PreviousState, EUOUUmbrellaState NextState) const;

	// 우산에 저장된 물을 모두 비웁니다.
	void SpillStoredWater();

	FTransform RuntimeHeldVisualBaseRelativeTransform = FTransform::Identity;
};
