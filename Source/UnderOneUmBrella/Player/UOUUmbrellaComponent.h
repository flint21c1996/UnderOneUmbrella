// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimationAsset.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "InputCoreTypes.h"
#include "World/Pour/UOUPourDropTypes.h"
#include "UOUUmbrellaComponent.generated.h"

class UArrowComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class AUOUPourDropActor;
class UUOURainReceiverComponent;
class UUOUAudioCueComponent;
class UUOUPourContentProfile;
class UUOUWaterContainerComponent;

// 우산이 현재 어떤 형태로 사용되는지 나타내는 상태입니다.
// 입력 처리, 점프 제한, 비 차단, 물 붓기 판단이 이 값을 기준으로 갈라집니다.
UENUM(BlueprintType)
enum class EUOUUmbrellaState : uint8
{
	Closed,
	Open,
	UpsideDown,
	Pouring,
	LightReflecting UMETA(DisplayName = "Light Reflecting")
};

// 우산 손잡이 방향을 나타내는 독립 상태입니다.
// 펼침/접힘 상태와 분리해서 갈고리 같은 역방향 상호작용 조건에 사용합니다.
UENUM(BlueprintType)
enum class EUOUUmbrellaDirectionState : uint8
{
	Normal,
	Reversed
};

// 우산을 손에 들고 있을 때 비주얼과 부착 위치를 고르는 파생 상태입니다.
UENUM(BlueprintType)
enum class EUOUUmbrellaVisualState : uint8
{
	Closed,
	Open,
	ClosedReversed,
	OpenReversed
};

// 우산에서 부은 물이 실제로 전달된 대상의 종류입니다.
// 디버그 텍스트와 라인트레이스 결과 확인에 사용합니다.
UENUM(BlueprintType)
enum class EUOUUmbrellaPourReceiverType : uint8
{
	None,
	UmbrellaWaterTarget,
	WaterBasinTarget,
	WaterContainer,
	PurePourReceiver,
	WaterWheel
};

// 우산 보유 여부나 상태가 바뀌었을 때 블루프린트와 다른 시스템에 알려주는 이벤트입니다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUmbrellaStateChangedSignature, EUOUUmbrellaState, NewState, bool, bHasUmbrella);

// 펼친 우산이 비를 막았을 때 막아낸 양을 전달하는 이벤트입니다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUmbrellaRainBlockedSignature, float, BlockedAmount);

// 플레이어가 들고 있는 우산의 보유 상태, 형태 전환, 물 저장과 붓기, 비 차단을 담당하는 컴포넌트입니다.
// 캐릭터 블루프린트에 붙여두고 참조 컴포넌트와 입력 키를 디테일 창에서 조정할 수 있게 열어둡니다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella"))
class UNDERONEUMBRELLA_API UUOUUmbrellaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 기본 틱을 켜서 물 붓기, 조준 회전, 디버그 표시를 매 프레임 갱신합니다.
	UUOUUmbrellaComponent();

	// 시작 시 참조 컴포넌트를 찾고 초기 우산 보유 상태와 비주얼을 맞춥니다.
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	FKey ToggleUmbrellaKey = EKeys::LeftMouseButton;

	// 우산을 뒤집거나 닫힘으로 되돌리는 기본 키입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Input")
	FKey InvertUmbrellaKey = EKeys::R;

	// 우산 상태에 따라 빛 반사를 토글하거나, 뒤집힌 우산의 물을 붓는 문맥 입력 키입니다.
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Audio", meta = (ToolTip = "Cue played while an open umbrella is actively blocking rain from a RainArea."))
	FName RainBlockedAudioCueId = TEXT("UmbrellaBlock");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Audio", meta = (ToolTip = "Fallback audio event played while an open umbrella is actively blocking rain from a RainArea. Configure this event as a managed loop for continuous rain sound."))
	FName RainBlockedAudioEventId = TEXT("RainOnUmbrella");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Audio", meta = (ClampMin = "0.0", ToolTip = "Seconds to keep the rain block loop alive after the latest RainArea exposure tick."))
	float RainBlockedAudioStopDelay = 0.15f;

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

	// 리그가 있는 우산을 손에 들 때 사용하는 스켈레탈 메쉬 컴포넌트입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<USkeletalMeshComponent> SkeletalHeldVisual = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Pour Socket", meta = (ToolTip = "Stream Visual과 DropActor 생성 위치로 사용할 소켓을 가진 컴포넌트 이름 또는 태그입니다. 기본값은 플레이어의 우산 스켈레탈 메시입니다."))
	FName PouringSocketSourceComponentName = TEXT("UmbrellaSkeletalVisual");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Pour Socket", meta = (ToolTip = "Stream Visual과 DropActor가 시작되는 소켓 이름입니다. 우산 리소스마다 이 소켓 위치를 맞추면 붓기 시작점이 일관됩니다."))
	FName PouringSocketName = TEXT("PouringPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Pour Socket", meta = (ToolTip = "Socket-space offset in world units applied after resolving PouringSocketName. This is not reduced by the skeletal mesh component scale."))
	FVector PouringSocketWorldUnitOffset = FVector::ZeroVector;

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
	bool bUsePickupMeshRelativeScale = false;

	// 스켈레탈 우산 비주얼을 캐릭터 메쉬의 소켓/본에 직접 붙일지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	bool bAttachSkeletalVisualToOwnerMeshSocket = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	FName ClosedSkeletalVisualSocketName = TEXT("Socket_Hand_L");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	FName OpenSkeletalVisualSocketName = TEXT("Socket_Hand_L");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	FName ClosedReversedSkeletalVisualSocketName = TEXT("Socket_Hand_L");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	FName OpenReversedSkeletalVisualSocketName = TEXT("Socket_Spine");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	FTransform ClosedSkeletalVisualOffset = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.01f));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	FTransform OpenSkeletalVisualOffset = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.01f));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	FTransform ClosedReversedSkeletalVisualOffset = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.01f));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	FTransform OpenReversedSkeletalVisualOffset = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.01f));

	// 별도 AnimBP 없이 에셋을 직접 재생할 때만 켭니다. AnimBP를 쓰면 꺼둡니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual")
	bool bPlaySkeletalVisualAnimationsDirectly = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual", meta = (EditCondition = "bPlaySkeletalVisualAnimationsDirectly"))
	TObjectPtr<UAnimationAsset> ClosedSkeletalVisualAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual", meta = (EditCondition = "bPlaySkeletalVisualAnimationsDirectly"))
	TObjectPtr<UAnimationAsset> OpenSkeletalVisualAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual", meta = (EditCondition = "bPlaySkeletalVisualAnimationsDirectly"))
	TObjectPtr<UAnimationAsset> ClosedReversedSkeletalVisualAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Skeletal Visual", meta = (EditCondition = "bPlaySkeletalVisualAnimationsDirectly"))
	TObjectPtr<UAnimationAsset> OpenReversedSkeletalVisualAnimation = nullptr;

	// 화면 디버그 표시 여부는 이제 Debug Controller의 Player HUD가 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Umbrella|Debug", meta = (ToolTip = "이 값은 더 이상 화면 디버그 표시 여부를 결정하지 않습니다. Debug Controller의 Player HUD 옵션을 사용합니다."))
	bool bShowScreenDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Pour Drop", meta = (ToolTip = "Overrides spawned PourDropActor collision radius after profile and Blueprint defaults are applied."))
	bool bOverridePourDropCollisionRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Pour Drop", meta = (ClampMin = "0.0", EditCondition = "bOverridePourDropCollisionRadius", EditConditionHides))
	float PourDropCollisionRadiusOverride = 4.0f;

	// 초당 붓는 물의 양입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water", meta = (ClampMin = "0.0"))
	float PourRate = 1.5f;

	// 물을 부을 때 저장된 물이 줄어드는 양에 곱할 배율입니다. 1이면 기존과 같고, 0.5이면 절반만 소모합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Water", meta = (ClampMin = "0.0", ToolTip = "물을 부을 때 우산에 저장된 물이 줄어드는 양에 곱할 배율입니다. 1이면 기존과 같고, 0.5이면 절반만 소모합니다."))
	float PourStoredWaterConsumptionMultiplier = 1.0f;

	// 물 붓기 라인트레이스가 닿을 수 있는 최대 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water", meta = (ClampMin = "0.0"))
	float PourDistance = 300.0f;

	// 물을 부을 대상을 찾을 때 사용하는 충돌 채널입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Water")
	TEnumAsByte<ECollisionChannel> PourTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Pour Drop", meta = (ClampMin = "0.0", ToolTip = "물방울 액터를 생성하는 최소 간격입니다. 0이면 매 Tick 생성합니다."))
	float PourDropSpawnInterval = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Pour Drop", meta = (ToolTip = "WaterBasinTarget에 닿았을 때 연결된 Basin 그룹 전체에 물을 분배할지 정합니다."))
	bool bPourDropAppliesToConnectedWaterBasinGroup = true;

	// 물을 붓는 동안 플레이어가 마우스 방향을 바라보게 할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim")
	bool bRotateOwnerTowardsPourDirection = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Aim", meta = (ToolTip = "While pouring, uses camera-relative movement input (WASD) to choose the pour direction and keeps the character in place."))
	bool bUseMovementInputPourAim = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Aim", meta = (ToolTip = "빛 반사 중 카메라 기준 이동 입력(WASD)으로 반사 방향을 선택하고 캐릭터를 제자리에 유지합니다."))
	bool bUseMovementInputLightReflectingAim = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Aim", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseMovementInputPourAim", EditConditionHides, ToolTip = "Movement input smaller than this value does not change the current pour direction."))
	float MovementInputPourAimDeadZone = 0.1f;

	// 마우스 위치를 월드 방향으로 환산할 때 사용하는 보조 레이 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim", meta = (ClampMin = "0.0"))
	float MouseAimRayDistance = 10000.0f;

	// 마우스 아래 월드 지점을 찾을 때 사용하는 충돌 채널입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim")
	TEnumAsByte<ECollisionChannel> MouseAimTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Aim", meta = (ToolTip = "Uses the mouse position relative to the player on the 2D screen before falling back to world picking."))
	bool bUseScreenSpacePourAim = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Aim", meta = (ClampMin = "0.0", EditCondition = "bUseScreenSpacePourAim", EditConditionHides))
	float ScreenSpacePourAimDeadZone = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Aim", meta = (ToolTip = "물 붓기와 빛 반사 조준을 플레이어 주변의 고정 각도 단위로 보정합니다. 45도는 8방향입니다."))
	bool bSnapPourAimDirection = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Aim", meta = (ClampMin = "1.0", ClampMax = "180.0", EditCondition = "bSnapPourAimDirection", EditConditionHides, DisplayName = "Movement Aim Snap Angle Degrees"))
	float PourAimSnapAngleDegrees = 45.0f;

	// 물 붓기와 빛 반사 중 캐릭터가 목표 조준 방향을 따라가는 일정한 회전 속도입니다. 0이면 즉시 회전합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Aim", meta = (ClampMin = "0.0", Units = "deg/s"))
	float MovementAimRotationSpeedDegreesPerSecond = 720.0f;

	// 대각선 입력에서 키 하나를 먼저 놓았을 때 단일 방향으로 잘못 확정하지 않도록 기다리는 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Umbrella|Aim", meta = (ClampMin = "0.0", Units = "s"))
	float MovementAimDiagonalReleaseGraceSeconds = 0.08f;

	// 빛 반사 중 플레이어가 마우스 방향을 바라보게 할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Aim")
	bool bRotateOwnerTowardsLightReflectingDirection = true;

	// 우산에 저장된 물이 무게 계산에 얼마나 영향을 줄지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain", meta = (ClampMin = "0.0"))
	float StoredWaterWeightMultiplier = 1.0f;

	// 우산이 비 파티클을 제거할 박스 볼륨의 절반 크기입니다. XY는 우산 면적, Z는 얇은 차단 두께로 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain Block", meta = (ClampMin = "0.0", ToolTip = "우산이 만드는 Kill Volume Box의 절반 크기입니다. 파티클이 이 박스 안에 들어오면 제거됩니다."))
	FVector RainBlockerVolumeHalfExtent = FVector(90.0f, 90.0f, 20.0f);

	// 비 차단 중심을 우산 기준 위치에서 얼마나 옮길지 정하는 로컬 오프셋입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain Block", meta = (ToolTip = "비 차단 중심을 우산 표시 컴포넌트 기준으로 보정하는 로컬 오프셋입니다. 우산 문양 위치가 맞지 않을 때 조정합니다."))
	FVector RainBlockerLocalOffset = FVector::ZeroVector;

	// 스켈레탈 우산은 메시 피벗 대신 천 중심 본을 차폐 판정의 기준으로 사용할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Rain Block", meta = (DisplayName = "차폐 기준 스켈레탈 본", ToolTip = "스켈레탈 우산에서 비와 빛 차폐 박스의 중심으로 사용할 본 또는 소켓입니다. 존재하지 않으면 우산 메시 피벗을 사용합니다."))
	FName RainBlockerSkeletalAnchorName = TEXT("Umbrella");

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

	// 현재 상태와 방향에서 파생된 우산 비주얼 상태입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	EUOUUmbrellaVisualState CurrentVisualState = EUOUUmbrellaVisualState::Closed;

	// 훅 몽타주처럼 닫힌 우산을 손잡이 반대 방향으로 잡아야 하는 연출 전용 오버라이드입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Runtime")
	bool bUseClosedReversedVisualOverride = false;

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

	// 펼친 우산을 앞으로 내미는 조준 가능한 빛 반사 상태로 전환합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void BeginLightReflecting();

	// 빛 반사를 끝내고 다시 펼친 상태로 돌아갑니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void EndLightReflecting();

	// 펼친 우산과 빛 반사 상태를 토글합니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void ToggleLightReflectingState();

	UFUNCTION(BlueprintCallable, Category = "Umbrella|Aim")
	void SetPourAimMovementInput(FVector2D MovementInput, float MovementYaw);

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

	// 우산으로 빛을 반사하는 중인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsLightReflecting() const;

	// 우산 손잡이가 정방향인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsNormalDirection() const;

	// 우산 손잡이가 역방향인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsReversedDirection() const;

	// 현재 우산 비주얼 상태를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	EUOUUmbrellaVisualState GetCurrentVisualState() const;

	// 훅 몽타주 등 닫힌 우산을 반대 방향으로 잡는 연출에서만 잠시 켭니다.
	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void SetClosedReversedVisualOverride(bool bEnable);

	// 우산 상태 때문에 점프가 막혀야 하는지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool BlocksJumping() const;

	// 현재 우산이 비를 막는 상태인지 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool IsBlockingRain() const;

	// 현재 설정된 우산 비 차단 박스의 중심, 회전, 절반 크기를 계산합니다. 실제 차단 활성 여부는 IsBlockingRain()으로 따로 확인합니다.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool TryGetRainBlockerVolumeData(FVector& OutWorldCenter, FRotator& OutWorldRotation, FVector& OutHalfExtent) const;

	// Gameplay rain blocker uses the owning player transform instead of the umbrella visual transform.
	UFUNCTION(BlueprintPure, Category = "Umbrella")
	bool TryGetGameplayRainBlockerVolumeData(FVector& OutWorldCenter, FRotator& OutWorldRotation, FVector& OutHalfExtent) const;

	// 비 차단 판정 중심에 적용되는 에디터 설정 로컬 오프셋을 반환합니다.
	const FVector& GetRainBlockerLocalOffset() const { return RainBlockerLocalOffset; }

	// 현재 우산 리소스에서 실제 물줄기 시작 Transform을 계산합니다.
	bool TryGetPouringPointTransform(FTransform& OutTransform) const;

	// 실제 물방울 생성 위치와 방향을 계산합니다.
	bool TryGetPourDropSpawnPlacement(FVector& OutDropLocation, FVector& OutDropDirection) const;

	// 물줄기 시작 소켓을 제공하는 현재 스켈레탈 메시 컴포넌트를 반환합니다.
	const USkeletalMeshComponent* GetPouringSocketSourceComponent() const
	{
		return ResolvePouringSocketSourceComponent();
	}

	FName GetPouringSocketName() const { return PouringSocketName; }
	const FVector& GetPouringSocketWorldUnitOffset() const { return PouringSocketWorldUnitOffset; }

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

	// 우산의 문맥 동작(빛 반사/물 붓기)에 쓰는 키를 반환합니다.
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
	void SetState(EUOUUmbrellaState NewState, bool bBroadcastIfUnchanged = false);

	// 현재 방향으로 우산을 펼쳤을 때 외부에 노출할 기존 상태를 계산합니다.
	EUOUUmbrellaState GetOpenStateForCurrentDirection() const;

	// 현재 상태와 보유 여부에 맞게 우산 비주얼 표시를 맞춥니다.
	void RefreshVisuals();

	void RefreshSkeletalVisual();

	bool IsSkeletalHeldVisualAvailable() const;

	void HideStaticHeldVisuals();

	// 우산 관련 오디오 이벤트를 플레이어 위치에서 재생합니다.
	void PlayUmbrellaAudioEvent(FName AudioEventId) const;

	// AudioCueComponent가 있으면 Cue를 우선 재생하고, 없으면 기존 EventId를 재생합니다.
	void PlayUmbrellaAudioCue(FName CueId, FName FallbackAudioEventId) const;

	void MarkRainBlockedAudioActive();

	void StartRainBlockedAudio();

	void StopRainBlockedAudio();

	void UpdateRainBlockedAudioState();

	FName ResolveRainBlockedAudioEventId() const;

	FName BuildRainBlockedAudioInstanceId() const;

	// Owner에 붙은 AudioCueComponent를 반환합니다.
	UUOUAudioCueComponent* GetAudioCueComponent() const;

	// 우산 오디오를 재생할 월드 위치를 반환합니다.
	FVector GetUmbrellaAudioLocation() const;

	// 블루프린트에서 비워둔 참조를 이름이나 컴포넌트 타입으로 자동 보완합니다.
	void ResolveReferences();

	// 픽업한 우산을 손에 보여줄 런타임 메쉬 컴포넌트를 준비합니다.
	void EnsureRuntimeHeldVisual();

	void EnsurePouringEffect();

	void UpdatePouringEffectState();

	void UpdatePouringEffectTransform();

	// 물줄기 시작점 바로 아래에 있는 가장 가까운 WaterBasinTarget 수면까지의 거리를 구합니다.
	bool TryGetPouringStreamHeightToWaterBasin(const FVector& StreamStart, float& OutWorldHeight) const;

	const USkeletalMeshComponent* ResolvePouringSocketSourceComponent() const;

	const UUOUPourContentProfile* ResolvePourContentProfile() const;

	TSubclassOf<AUOUPourDropActor> ResolvePourDropActorClass() const;

	// 픽업 액터의 메쉬와 머티리얼 정보를 손에 든 우산 비주얼로 복사합니다.
	void ApplyHeldVisualFromMeshComponent(UStaticMeshComponent* SourceMeshComponent);

	// 직접 전달받은 메쉬와 머티리얼을 런타임 우산 비주얼에 적용합니다.
	void ApplyHeldVisualFromAssets(UStaticMesh* Mesh, const TArray<UMaterialInterface*>& Materials, const FVector& SourceRelativeScale);

	// 우산 상태와 물 정보를 화면 디버그 텍스트로 표시합니다.
	void DrawScreenDebug() const;

	// 물 붓기 디버그에 사용하는 마지막 트레이스 기록을 비웁니다.
	void ClearPourTraceDebug();

	// 물 붓기와 빛 반사 중 플레이어가 목표 방향을 부드럽게 바라보도록 회전을 보정합니다.
	void UpdateUmbrellaAimFacing(float DeltaTime);
	void UpdatePendingMovementAimDirection(float DeltaTime);
	void CommitMovementAimDirection(const FVector& AimDirection, bool bIsDiagonalInput);
	void ClearPendingMovementAimDirection();

	// 붓기 조준 회전에서 남겨둔 상태를 정리하기 위한 자리입니다.
	void ClearPourAimFacing();
	void ApplyAimFacingMovementOverride();

	// 저장된 물을 시간에 따라 줄이고 일정 간격으로 낙하 물 액터를 생성합니다.
	void UpdatePouring(float DeltaTime);

	bool SpawnPendingPourDrop();

	void ResetPendingPourDrop();

	void PrimeNextPourDropSpawn();

	UFUNCTION()
	void HandlePourDropImpacted(AUOUPourDropActor* DropActor, AActor* ImpactActor, FVector ImpactLocation, EUOUPourDropReceiverType ReceiverType, bool bDeliveredWater);

	UFUNCTION()
	void HandlePourContentProfileChanged(UUOUPourContentProfile* NewProfile);

	// 마우스 위치를 기준으로 플레이어가 바라볼 평면 방향을 계산합니다.
	bool TryGetMouseAimDirection(FVector& AimDirection, FVector& AimPoint) const;

	bool TryGetScreenSpaceMouseAimDirection(APlayerController* PlayerController, FVector& AimDirection, FVector& AimPoint) const;

	bool TryGetActivePourAimDirection(FVector& AimDirection) const;

	FVector SnapPourDirectionToAngleStep(const FVector& Direction) const;

	// 물 붓기 시작 위치와 최종 방향을 계산합니다.
	bool TryGetPourDirection(FVector& PourOriginLocation, FVector& PourDirection) const;

	// 상태 전환 과정에서 저장된 물을 버려야 하는지 판단합니다.
	bool ShouldSpillStoredWater(EUOUUmbrellaState PreviousState, EUOUUmbrellaState NextState) const;

	// 우산에 저장된 물을 모두 비웁니다.
	void SpillStoredWater();

	FTransform RuntimeHeldVisualBaseRelativeTransform = FTransform::Identity;

	UPROPERTY(Transient)
	bool bHasAppliedSkeletalVisualAnimation = false;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> LastAppliedSkeletalVisualAnimation = nullptr;

	bool bHasAimFacingMovementOverride = false;
	bool bSavedOrientRotationToMovement = false;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> PouringEffectComponent = nullptr;

	TWeakObjectPtr<UNiagaraSystem> CachedPouringStreamHeightAsset;
	FName CachedPouringStreamHeightParameterName = NAME_None;
	float DefaultPouringStreamHeight = 0.0f;
	bool bHasDefaultPouringStreamHeight = false;

	float PendingPourDropVolume = 0.0f;

	float PendingPourDropDuration = 0.0f;

	float TimeSinceLastPourDropSpawn = 0.0f;

	FVector PourAimWorldDirection = FVector::ZeroVector;
	FVector PendingMovementAimWorldDirection = FVector::ZeroVector;
	float PendingMovementAimTimeRemaining = 0.0f;
	bool bHasPendingMovementAimDirection = false;
	bool bMovementAimInputActive = false;
	bool bCommittedMovementAimWasDiagonal = false;

	bool bRainBlockedAudioPlaying = false;

	float LastRainBlockedAudioTime = -1000.0f;

	float LastRainBlockedAudioRefreshAttemptTime = -1000.0f;

	FName ActiveRainBlockedAudioEventId = NAME_None;
};
