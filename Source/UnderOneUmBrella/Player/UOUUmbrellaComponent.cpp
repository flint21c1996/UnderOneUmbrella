// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaComponent.h"

#include "Player/UOUUmbrellaRuntimeVisualPresenter.h"
#include "Player/UOUUmbrellaSkeletalVisualPresenter.h"
#include "Player/UOUUmbrellaVisualPolicy.h"

#include "Audio/UOUAudioCueComponent.h"
#include "Audio/UOUAudioSubsystem.h"
#include "Components/ArrowComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "Player/UOURainReceiverComponent.h"
#include "Player/UOUWaterContainerComponent.h"
#include "World/Pour/UOUPourContentProfile.h"
#include "World/Pour/UOUPourDropActor.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

namespace
{
// 물 붓기 디버그 라벨에 표시할 수 있도록 수신 대상 enum을 짧은 문자열로 바꿉니다.
constexpr float RainBlockedAudioRefreshInterval = 0.1f;

const TCHAR* GetPourReceiverTypeText(EUOUUmbrellaPourReceiverType ReceiverType)
{
	switch (ReceiverType)
	{
	case EUOUUmbrellaPourReceiverType::PurePourReceiver:
		return TEXT("PurePourReceiver");
	case EUOUUmbrellaPourReceiverType::UmbrellaWaterTarget:
		return TEXT("UmbrellaWaterTarget");
	case EUOUUmbrellaPourReceiverType::WaterBasinTarget:
		return TEXT("WaterBasinTarget");
	case EUOUUmbrellaPourReceiverType::WaterContainer:
		return TEXT("WaterContainer");
	case EUOUUmbrellaPourReceiverType::WaterWheel:
		return TEXT("WaterWheel");
	case EUOUUmbrellaPourReceiverType::None:
	default:
		return TEXT("None");
	}
}

EUOUUmbrellaPourReceiverType ConvertPourDropReceiverType(EUOUPourDropReceiverType ReceiverType)
{
	switch (ReceiverType)
	{
	case EUOUPourDropReceiverType::PurePourReceiver:
		return EUOUUmbrellaPourReceiverType::PurePourReceiver;
	case EUOUPourDropReceiverType::UmbrellaWaterTarget:
		return EUOUUmbrellaPourReceiverType::UmbrellaWaterTarget;
	case EUOUPourDropReceiverType::WaterBasinTarget:
		return EUOUUmbrellaPourReceiverType::WaterBasinTarget;
	case EUOUPourDropReceiverType::WaterContainer:
		return EUOUUmbrellaPourReceiverType::WaterContainer;
	case EUOUPourDropReceiverType::WaterWheel:
		return EUOUUmbrellaPourReceiverType::WaterWheel;
	case EUOUPourDropReceiverType::None:
	default:
		return EUOUUmbrellaPourReceiverType::None;
	}
}

bool ShouldLogRainBlockedAudioDiagnostic(const UObject* WorldContext, double& LastLogTime, double IntervalSeconds = 0.5)
{
	const UWorld* World = WorldContext != nullptr ? WorldContext->GetWorld() : nullptr;
	const double CurrentTime = World != nullptr ? World->GetTimeSeconds() : FPlatformTime::Seconds();
	if (CurrentTime - LastLogTime < IntervalSeconds)
	{
		return false;
	}

	LastLogTime = CurrentTime;
	return true;
}
}

// 우산 컴포넌트는 물 붓기, 조준 회전, 디버그 표시를 계속 갱신해야 해서 틱을 켭니다.
UUOUUmbrellaComponent::UUOUUmbrellaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// 시작 시 필요한 참조를 찾고, 시작 보유 옵션과 기본 우산 비주얼을 현재 상태에 맞춥니다.
void UUOUUmbrellaComponent::BeginPlay()
{
	Super::BeginPlay();

	// 블루프린트에서 직접 연결하지 않은 참조는 컴포넌트 이름과 타입으로 보완합니다.
	ResolveReferences();
	EnsureRuntimeHeldVisual();
	EnsurePouringEffect();

	if (StoredWaterContainer != nullptr)
	{
		// 우산에 저장된 물이 퍼즐 무게로 환산될 때 쓰는 배율을 동기화합니다.
		StoredWaterContainer->WeightMultiplier = FMath::Max(0.0f, StoredWaterWeightMultiplier);
		StoredWaterContainer->OnPourContentProfileChanged.RemoveDynamic(this, &UUOUUmbrellaComponent::HandlePourContentProfileChanged);
		StoredWaterContainer->OnPourContentProfileChanged.AddDynamic(this, &UUOUUmbrellaComponent::HandlePourContentProfileChanged);
	}

	bHasUmbrella = bStartWithUmbrella;
	CurrentState = EUOUUmbrellaState::Closed;

	if (bHasUmbrella && DefaultHeldMesh != nullptr)
	{
		// 우산을 들고 시작하는 경우 픽업 과정 없이 기본 메쉬를 손 비주얼에 적용합니다.
		ApplyHeldVisualFromAssets(DefaultHeldMesh, {}, FVector::OneVector);
	}

	RefreshVisuals();
	UpdatePouringEffectState();
}

void UUOUUmbrellaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopRainBlockedAudio();
	ClearPourAimFacing();

	Super::EndPlay(EndPlayReason);
}

// 매 프레임 우산 상태에 따라 물 붓기, 마우스 조준 회전, 디버그 표시를 갱신합니다.
void UUOUUmbrellaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHasUmbrella)
	{
		StopRainBlockedAudio();
		// 우산이 없어도 디버그는 상태 확인용으로 남기고, 조준과 물줄기 기록은 정리합니다.
		ClearPourAimFacing();
		ClearPourTraceDebug();
		DrawScreenDebug();
		DrawRainBlockerDebug();
		return;
	}

	UpdateUmbrellaAimFacing();
	UpdatePouring(DeltaTime);
	UpdatePouringEffectState();
	DrawScreenDebug();
	DrawRainBlockerDebug();
	DrawPourSocketAndDropSpawnDebug();
	DrawPourTraceDebug();
	UpdateRainBlockedAudioState();
}

// 우산을 새로 획득했을 때 보유 상태와 저장 물을 초기화합니다.
void UUOUUmbrellaComponent::AcquireUmbrella()
{
	if (bHasUmbrella)
	{
		return;
	}

	bHasUmbrella = true;
	CurrentDirectionState = EUOUUmbrellaDirectionState::Normal;

	if (StoredWaterContainer != nullptr)
	{
		StoredWaterContainer->SetAmount(0.0f);
	}

	PlayUmbrellaAudioCue(AcquireAudioCueId, AcquireAudioEventId);
	SetState(EUOUUmbrellaState::Closed, true);
}

// 월드에 놓인 픽업 우산의 메쉬와 머티리얼을 복사한 뒤 플레이어가 들고 있는 우산으로 바꿉니다.
void UUOUUmbrellaComponent::AcquireUmbrellaFromMeshComponent(UStaticMeshComponent* SourceMeshComponent)
{
	AcquireUmbrella();
	ApplyHeldVisualFromMeshComponent(SourceMeshComponent);
}

// 우산을 잃거나 내려놓는 상황에서 보유 상태, 저장 물, 비 노출을 모두 정리합니다.
void UUOUUmbrellaComponent::RemoveUmbrella()
{
	if (!bHasUmbrella)
	{
		return;
	}

	StopRainBlockedAudio();
	bHasUmbrella = false;
	CurrentDirectionState = EUOUUmbrellaDirectionState::Normal;
	ResetPendingPourDrop();

	if (StoredWaterContainer != nullptr)
	{
		StoredWaterContainer->SetAmount(0.0f);
	}

	if (RainReceiver != nullptr)
	{
		RainReceiver->ClearExposure();
	}

	ClearPourAimFacing();
	SetState(EUOUUmbrellaState::Closed, true);
}

// 우산을 펼쳐서 비를 막는 상태로 전환합니다.
void UUOUUmbrellaComponent::OpenUmbrella()
{
	if (!bHasUmbrella)
	{
		return;
	}

	SetState(GetOpenStateForCurrentDirection());
}

// 우산을 닫아서 이동이나 잡기 동작에 쓰기 쉬운 상태로 전환합니다.
void UUOUUmbrellaComponent::CloseUmbrella()
{
	if (!bHasUmbrella)
	{
		return;
	}

	SetState(EUOUUmbrellaState::Closed);
}

// 우산을 뒤집어 비를 받아 저장할 수 있는 상태로 전환합니다.
void UUOUUmbrellaComponent::TurnUmbrellaUpsideDown()
{
	if (!bHasUmbrella)
	{
		return;
	}

	CurrentDirectionState = EUOUUmbrellaDirectionState::Reversed;
	SetState(EUOUUmbrellaState::UpsideDown);
}

// 뒤집힌 우산에 물이 있을 때만 붓기 상태로 들어갑니다.
void UUOUUmbrellaComponent::BeginPour()
{
	if (!bHasUmbrella || CurrentState != EUOUUmbrellaState::UpsideDown)
	{
		return;
	}

	if (GetCurrentStoredWater() <= 0.0f)
	{
		return;
	}

	SetState(EUOUUmbrellaState::Pouring);
}

// 붓기 입력이 끝나면 다시 물을 담고 있는 뒤집힌 상태로 돌아갑니다.
void UUOUUmbrellaComponent::EndPour()
{
	if (CurrentState != EUOUUmbrellaState::Pouring)
	{
		return;
	}

	SetState(GetOpenStateForCurrentDirection());
}

void UUOUUmbrellaComponent::BeginLightReflecting()
{
	if (!bHasUmbrella || CurrentState != EUOUUmbrellaState::Open)
	{
		return;
	}

	SetState(EUOUUmbrellaState::LightReflecting);
}

void UUOUUmbrellaComponent::EndLightReflecting()
{
	if (CurrentState != EUOUUmbrellaState::LightReflecting)
	{
		return;
	}

	SetState(EUOUUmbrellaState::Open);
}

void UUOUUmbrellaComponent::ToggleLightReflectingState()
{
	if (CurrentState == EUOUUmbrellaState::LightReflecting)
	{
		EndLightReflecting();
		return;
	}

	BeginLightReflecting();
}

// 비나 다른 시스템에서 전달받은 물 양을 우산 저장 컨테이너에 더합니다.
void UUOUUmbrellaComponent::AddCollectedWater(float WaterAmount)
{
	if (!CanCollectWater() || StoredWaterContainer == nullptr || WaterAmount <= 0.0f)
	{
		return;
	}

	StoredWaterContainer->AddAmount(WaterAmount);
}

// 비 노출량을 받아 현재 우산 상태에 맞게 차단, 플레이어 피격, 물 저장으로 나눕니다.
void UUOUUmbrellaComponent::ApplyRainExposure(float ExposureAmount)
{
	if (ExposureAmount <= 0.0f)
	{
		return;
	}

	static double LastApplyRainExposureLogTime = -1000.0;
	if (ShouldLogRainBlockedAudioDiagnostic(this, LastApplyRainExposureLogTime))
	{
		UE_LOG(
			LogUOUAudio,
			Verbose,
			TEXT("[RainBlockedAudio][ApplyRainExposure] Owner=%s Exposure=%.4f HasUmbrella=%s State=%d IsOpen=%s Cue=%s Event=%s"),
			*GetNameSafe(GetOwner()),
			ExposureAmount,
			bHasUmbrella ? TEXT("true") : TEXT("false"),
			static_cast<int32>(CurrentState),
			IsOpen() ? TEXT("true") : TEXT("false"),
			*RainBlockedAudioCueId.ToString(),
			*RainBlockedAudioEventId.ToString());
	}

	if (IsOpen())
	{
		// 펼친 우산은 플레이어에게 비를 넘기지 않고 차단 이벤트만 보냅니다.
		MarkRainBlockedAudioActive();
		OnRainBlocked.Broadcast(ExposureAmount);
		return;
	}

	if (RainReceiver != nullptr)
	{
		// 우산이 비를 막지 못하는 상태라면 플레이어 비 노출량으로 기록합니다.
		RainReceiver->ApplyRainExposure(ExposureAmount);
	}

	if (CanCollectWater())
	{
		// 뒤집힌 우산은 비를 맞는 동시에 물을 저장합니다.
		AddCollectedWater(ExposureAmount);
	}
}

// 닫힘과 펼침을 오가되, 뒤집힘이나 붓기 상태에서는 펼침으로 빠져나오게 합니다.
void UUOUUmbrellaComponent::ToggleOpenState()
{
	switch (CurrentState)
	{
	case EUOUUmbrellaState::Closed:
		OpenUmbrella();
		break;
	case EUOUUmbrellaState::Open:
	case EUOUUmbrellaState::UpsideDown:
	case EUOUUmbrellaState::Pouring:
	case EUOUUmbrellaState::LightReflecting:
		CloseUmbrella();
		break;
	}
}

// 우산 손잡이 방향만 전환하고, 펼친 상태라면 방향에 맞는 기존 상태로 다시 계산합니다.
void UUOUUmbrellaComponent::ToggleInvertState()
{
	if (!bHasUmbrella || CurrentState == EUOUUmbrellaState::Closed)
	{
		return;
	}

	CurrentDirectionState = CurrentDirectionState == EUOUUmbrellaDirectionState::Normal
		? EUOUUmbrellaDirectionState::Reversed
		: EUOUUmbrellaDirectionState::Normal;

	switch (CurrentState)
	{
	case EUOUUmbrellaState::Closed:
		break;
	case EUOUUmbrellaState::Open:
	case EUOUUmbrellaState::UpsideDown:
	case EUOUUmbrellaState::Pouring:
	case EUOUUmbrellaState::LightReflecting:
		SetState(GetOpenStateForCurrentDirection());
		break;
	}
}

// 키 입력을 우산 기능별로 나누는 단순 입력 라우터입니다.
void UUOUUmbrellaComponent::HandleInputPressed(FKey InputKey)
{
	if (bEnableDebugFillKey && InputKey == DebugFillKey)
	{
		if (StoredWaterContainer != nullptr)
		{
			// 테스트 중 물 퍼즐을 빠르게 확인하기 위한 수동 채우기 경로입니다.
			StoredWaterContainer->AddAmount(DebugFillAmount);
		}
		return;
	}

	if (!bHasUmbrella)
	{
		return;
	}

	if (InputKey == ToggleUmbrellaKey)
	{
		ToggleOpenState();
		return;
	}

	if (InputKey == InvertUmbrellaKey)
	{
		ToggleInvertState();
		return;
	}

	if (InputKey == PourKey)
	{
		BeginPour();
		return;
	}

	if (InputKey == LightReflectingKey)
	{
		ToggleLightReflectingState();
		return;
	}

}

// 유지형 입력인 물 붓기는 키를 놓았을 때 종료합니다.
void UUOUUmbrellaComponent::HandleInputReleased(FKey InputKey)
{
	if (InputKey == PourKey)
	{
		EndPour();
	}

}

// 우산을 가지고 있고 뒤집힌 상태일 때만 비를 받아 물로 저장할 수 있습니다.
bool UUOUUmbrellaComponent::CanCollectWater() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::UpsideDown;
}

// 우산을 실제로 들고 있는지 외부 시스템이 확인할 때 사용합니다.
bool UUOUUmbrellaComponent::HasUmbrella() const
{
	return bHasUmbrella;
}

// 닫힌 우산 상태는 상자 밀기처럼 손을 써야 하는 동작의 조건으로도 사용됩니다.
bool UUOUUmbrellaComponent::IsClosed() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::Closed;
}

// 펼친 우산 상태는 비 차단과 점프 허용 여부 판단에 사용됩니다.
bool UUOUUmbrellaComponent::IsOpen() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::Open;
}

// 뒤집힌 우산 상태는 물 저장 가능 여부와 점프 제한 판단에 사용됩니다.
bool UUOUUmbrellaComponent::IsUpsideDown() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::UpsideDown;
}

// 물 붓기 상태는 입력 유지 중인지와 점프 제한 판단에 사용됩니다.
bool UUOUUmbrellaComponent::IsPouring() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::Pouring;
}

bool UUOUUmbrellaComponent::IsLightReflecting() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::LightReflecting;
}

bool UUOUUmbrellaComponent::IsNormalDirection() const
{
	return bHasUmbrella && CurrentDirectionState == EUOUUmbrellaDirectionState::Normal;
}

bool UUOUUmbrellaComponent::IsReversedDirection() const
{
	return bHasUmbrella && CurrentDirectionState == EUOUUmbrellaDirectionState::Reversed;
}

EUOUUmbrellaVisualState UUOUUmbrellaComponent::GetCurrentVisualState() const
{
	return CurrentVisualState;
}

void UUOUUmbrellaComponent::SetClosedReversedVisualOverride(bool bEnable)
{
	const bool bShouldEnable = bEnable && bHasUmbrella && CurrentState == EUOUUmbrellaState::Closed;
	if (bUseClosedReversedVisualOverride == bShouldEnable)
	{
		return;
	}

	bUseClosedReversedVisualOverride = bShouldEnable;
	RefreshVisuals();
	OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
}

// ?곗궛???ㅼ쭛?붽굅??臾쇱쓣 遺볥뒗 以묒씠硫??먰봽瑜?留됱븘 ?뚮젅??媛먭컖???덉젙?쒗궢?덈떎.
// 우산이 뒤집힌 계열의 동작 중이면 점프를 막아 플레이 감각을 안정시킵니다.
bool UUOUUmbrellaComponent::BlocksJumping() const
{
	return bHasUmbrella && (CurrentState == EUOUUmbrellaState::UpsideDown ||
		CurrentState == EUOUUmbrellaState::Pouring ||
		CurrentState == EUOUUmbrellaState::LightReflecting);
}

// 현재 구현에서는 펼친 우산만 비를 막는 상태로 봅니다.
bool UUOUUmbrellaComponent::IsBlockingRain() const
{
	return IsOpen();
}

// 현재 설정된 비 차단 박스의 중심, 회전, 절반 크기를 계산합니다. 실제 차단 활성 여부는 호출자가 IsBlockingRain()으로 판단합니다.
bool UUOUUmbrellaComponent::TryGetRainBlockerVolumeData(FVector& OutWorldCenter, FRotator& OutWorldRotation, FVector& OutHalfExtent) const
{
	OutWorldCenter = FVector::ZeroVector;
	OutWorldRotation = FRotator::ZeroRotator;
	OutHalfExtent = FVector::ZeroVector;

	const FVector SafeHalfExtent(
		FMath::Max(0.0f, RainBlockerVolumeHalfExtent.X),
		FMath::Max(0.0f, RainBlockerVolumeHalfExtent.Y),
		FMath::Max(0.0f, RainBlockerVolumeHalfExtent.Z));

	if (SafeHalfExtent.IsNearlyZero())
	{
		return false;
	}

	const bool bUseUpsideDownBlocker = CurrentState == EUOUUmbrellaState::UpsideDown;
	const bool bUseSkeletalBlocker = IsSkeletalHeldVisualAvailable() && (IsOpen() || bUseUpsideDownBlocker);
	const USceneComponent* BlockerComponent = bUseSkeletalBlocker
		? static_cast<const USceneComponent*>(SkeletalHeldVisual.Get())
		: static_cast<const USceneComponent*>(bUseUpsideDownBlocker ? UpsideDownVisual.Get() : OpenVisual.Get());
	if (BlockerComponent == nullptr)
	{
		BlockerComponent = RuntimeHeldVisual;
	}
	if (BlockerComponent == nullptr && bUseUpsideDownBlocker)
	{
		BlockerComponent = OpenVisual;
	}
	if (BlockerComponent == nullptr)
	{
		BlockerComponent = PickupAttachPoint;
	}

	if (BlockerComponent != nullptr)
	{
		const FTransform BlockerTransform = BlockerComponent->GetComponentTransform();
		OutWorldCenter = BlockerTransform.TransformPosition(RainBlockerLocalOffset);
		OutWorldRotation = BlockerTransform.Rotator();
		OutHalfExtent = SafeHalfExtent;
		return true;
	}

	if (const AActor* Owner = GetOwner())
	{
		const FTransform OwnerTransform = Owner->GetActorTransform();
		OutWorldCenter = OwnerTransform.TransformPosition(RainBlockerLocalOffset);
		OutWorldRotation = OwnerTransform.Rotator();
		OutHalfExtent = SafeHalfExtent;
		return true;
	}

	return false;
}

bool UUOUUmbrellaComponent::TryGetGameplayRainBlockerVolumeData(FVector& OutWorldCenter, FRotator& OutWorldRotation, FVector& OutHalfExtent) const
{
	OutWorldCenter = FVector::ZeroVector;
	OutWorldRotation = FRotator::ZeroRotator;
	OutHalfExtent = FVector::ZeroVector;

	const FVector SafeHalfExtent(
		FMath::Max(0.0f, RainBlockerVolumeHalfExtent.X),
		FMath::Max(0.0f, RainBlockerVolumeHalfExtent.Y),
		FMath::Max(0.0f, RainBlockerVolumeHalfExtent.Z));

	if (SafeHalfExtent.IsNearlyZero())
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return false;
	}

	const FTransform OwnerTransform = Owner->GetActorTransform();
	OutWorldCenter = OwnerTransform.TransformPosition(RainBlockerLocalOffset);
	OutWorldRotation = OwnerTransform.Rotator();
	OutHalfExtent = SafeHalfExtent;
	return true;
}

float UUOUUmbrellaComponent::GetCurrentStoredWater() const
{
	return StoredWaterContainer != nullptr ? StoredWaterContainer->CurrentAmount : 0.0f;
}

// 비 노출 컴포넌트가 없을 때도 디버그와 UI가 안전하게 0을 받을 수 있게 감쌉니다.
float UUOUUmbrellaComponent::GetCurrentPlayerRainAmount() const
{
	return RainReceiver != nullptr ? RainReceiver->CurrentExposure : 0.0f;
}

EUOUUmbrellaState UUOUUmbrellaComponent::GetOpenStateForCurrentDirection() const
{
	return CurrentDirectionState == EUOUUmbrellaDirectionState::Reversed
		? EUOUUmbrellaState::UpsideDown
		: EUOUUmbrellaState::Open;
}

// 우산 상태 변경을 한 곳으로 모아 물 버림, 비주얼 갱신, 이벤트 호출 순서를 고정합니다.
void UUOUUmbrellaComponent::SetState(EUOUUmbrellaState NewState, bool bBroadcastIfUnchanged)
{
	const EUOUUmbrellaState PreviousState = CurrentState;
	const EUOUUmbrellaState ResolvedState = bHasUmbrella ? NewState : EUOUUmbrellaState::Closed;

	bUseClosedReversedVisualOverride = false;

	if (ResolvedState == EUOUUmbrellaState::Closed)
	{
		CurrentDirectionState = EUOUUmbrellaDirectionState::Normal;
	}
	else if (ResolvedState == EUOUUmbrellaState::Open || ResolvedState == EUOUUmbrellaState::LightReflecting)
	{
		CurrentDirectionState = EUOUUmbrellaDirectionState::Normal;
	}
	else if (ResolvedState == EUOUUmbrellaState::UpsideDown ||
		ResolvedState == EUOUUmbrellaState::Pouring)
	{
		CurrentDirectionState = EUOUUmbrellaDirectionState::Reversed;
	}

	if (PreviousState != EUOUUmbrellaState::Pouring && ResolvedState == EUOUUmbrellaState::Pouring)
	{
		ResetPendingPourDrop();
		PrimeNextPourDropSpawn();
	}
	else if (PreviousState == EUOUUmbrellaState::Pouring && ResolvedState != EUOUUmbrellaState::Pouring)
	{
		if (!SpawnPendingPourDrop())
		{
			ResetPendingPourDrop();
		}
	}

	if (PreviousState == ResolvedState)
	{
		if (ResolvedState != EUOUUmbrellaState::Open)
		{
			StopRainBlockedAudio();
		}

		// 같은 상태여도 에디터 세팅 변경 뒤 비주얼을 다시 맞출 수 있게 갱신은 수행합니다.
		RefreshVisuals();
		UpdatePouringEffectState();
		if (bBroadcastIfUnchanged)
		{
			OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
		}
		return;
	}

	if (ShouldSpillStoredWater(PreviousState, ResolvedState))
	{
		// 물을 담을 수 없는 상태로 바뀌면 저장된 물을 흘린 것으로 처리합니다.
		SpillStoredWater();
	}

	CurrentState = ResolvedState;
	if (CurrentState != EUOUUmbrellaState::Pouring && CurrentState != EUOUUmbrellaState::LightReflecting)
	{
		ClearPourAimFacing();
	}
	if (CurrentState != EUOUUmbrellaState::Open)
	{
		StopRainBlockedAudio();
	}

	if (bHasUmbrella)
	{
		if (CurrentState == EUOUUmbrellaState::Open)
		{
			PlayUmbrellaAudioCue(OpenAudioCueId, OpenAudioEventId);
		}
		else if (CurrentState == EUOUUmbrellaState::Closed)
		{
			PlayUmbrellaAudioCue(CloseAudioCueId, CloseAudioEventId);
		}
	}

	if (CurrentState != EUOUUmbrellaState::Pouring)
	{
		// 붓기 상태가 아니면 마지막 라인트레이스 결과를 초기화해 디버그 오해를 줄입니다.
		LastPourHitName = TEXT("None");
		LastPourTargetName = TEXT("None");
		LastPourReceiverType = EUOUUmbrellaPourReceiverType::None;
		ClearPourTraceDebug();
	}

	RefreshVisuals();
	UpdatePouringEffectState();
	OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
}

// 전용 상태 비주얼이 있으면 그 비주얼을 쓰고, 없으면 런타임 복사 메쉬 하나로 표시합니다.
void UUOUUmbrellaComponent::RefreshVisuals()
{
	CurrentVisualState = FUOUUmbrellaVisualPolicy::ResolveVisualState(
		CurrentState,
		bUseClosedReversedVisualOverride);
	if (IsSkeletalHeldVisualAvailable())
	{
		RefreshSkeletalVisual();
		HideStaticHeldVisuals();
		return;
	}

	const bool bHasDedicatedVisuals = ClosedVisual != nullptr || OpenVisual != nullptr || UpsideDownVisual != nullptr;
	const FUOUUmbrellaVisualVisibility Visibility = FUOUUmbrellaVisualPolicy::ResolveVisibility(
		bHasUmbrella,
		CurrentVisualState,
		bHasDedicatedVisuals,
		UpsideDownVisual != nullptr,
		RuntimeHeldVisual != nullptr,
		bFlipRuntimeHeldVisualWhenUpsideDown);

	if (!bHasUmbrella)
	{
		// 우산을 들고 있지 않으면 어떤 비주얼도 보이지 않도록 모두 숨깁니다.
		if (ClosedVisual != nullptr)
		{
			ClosedVisual->SetVisibility(false, true);
		}

		if (OpenVisual != nullptr)
		{
			OpenVisual->SetVisibility(false, true);
		}

		if (UpsideDownVisual != nullptr)
		{
			UpsideDownVisual->SetVisibility(false, true);
		}

		if (RuntimeHeldVisual != nullptr)
		{
			RuntimeHeldVisual->SetVisibility(false, true);
		}

		return;
	}

	if (bHasDedicatedVisuals)
	{
		// 상태별 전용 비주얼이 하나라도 있으면 그 방식이 우선입니다.
		if (ClosedVisual != nullptr)
		{
			ClosedVisual->SetVisibility(Visibility.bShowClosed, true);
		}

		if (OpenVisual != nullptr)
		{
			OpenVisual->SetVisibility(Visibility.bShowOpen, true);
		}

		if (UpsideDownVisual != nullptr)
		{
			UpsideDownVisual->SetVisibility(Visibility.bShowUpsideDown, true);
		}

		if (RuntimeHeldVisual != nullptr)
		{
			if (Visibility.bFlipRuntime)
			{
				FUOUUmbrellaRuntimeVisualPresenter::ApplyStateTransform(
					RuntimeHeldVisual,
					RuntimeHeldVisualBaseRelativeTransform,
					bFlipRuntimeHeldVisualWhenUpsideDown,
					CurrentVisualState,
					UpsideDownHeldVisualRotationOffset,
					UpsideDownHeldVisualLocationOffset);
			}

			RuntimeHeldVisual->SetVisibility(Visibility.bShowRuntime, true);
		}

		return;
	}

	if (RuntimeHeldVisual != nullptr)
	{
		// 상태별 비주얼이 없다면 픽업에서 복사한 런타임 메쉬 하나를 계속 보여줍니다.
		FUOUUmbrellaRuntimeVisualPresenter::ApplyStateTransform(
			RuntimeHeldVisual,
			RuntimeHeldVisualBaseRelativeTransform,
			bFlipRuntimeHeldVisualWhenUpsideDown,
			CurrentVisualState,
			UpsideDownHeldVisualRotationOffset,
			UpsideDownHeldVisualLocationOffset);
		RuntimeHeldVisual->SetVisibility(Visibility.bShowRuntime, true);
	}
}

bool UUOUUmbrellaComponent::IsSkeletalHeldVisualAvailable() const
{
	return SkeletalHeldVisual != nullptr;
}

void UUOUUmbrellaComponent::HideStaticHeldVisuals()
{
	if (ClosedVisual != nullptr)
	{
		ClosedVisual->SetVisibility(false, true);
	}

	if (OpenVisual != nullptr)
	{
		OpenVisual->SetVisibility(false, true);
	}

	if (UpsideDownVisual != nullptr)
	{
		UpsideDownVisual->SetVisibility(false, true);
	}

	if (RuntimeHeldVisual != nullptr)
	{
		RuntimeHeldVisual->SetVisibility(false, true);
	}
}

void UUOUUmbrellaComponent::RefreshSkeletalVisual()
{
	if (SkeletalHeldVisual == nullptr)
	{
		return;
	}

	const FUOUUmbrellaSkeletalVisualVariants Variants = {
		{ ClosedSkeletalVisualSocketName, ClosedSkeletalVisualOffset, ClosedSkeletalVisualAnimation.Get() },
		{ OpenSkeletalVisualSocketName, OpenSkeletalVisualOffset, OpenSkeletalVisualAnimation.Get() },
		{ ClosedReversedSkeletalVisualSocketName, ClosedReversedSkeletalVisualOffset, ClosedReversedSkeletalVisualAnimation.Get() },
		{ OpenReversedSkeletalVisualSocketName, OpenReversedSkeletalVisualOffset, OpenReversedSkeletalVisualAnimation.Get() }
	};

	FUOUUmbrellaSkeletalVisualRequest Request;
	Request.Visual = SkeletalHeldVisual;
	Request.HeldVisualAnchor = HeldVisualAnchor;
	Request.PickupAttachPoint = PickupAttachPoint;
	Request.Owner = GetOwner();
	Request.bHasUmbrella = bHasUmbrella;
	Request.bAttachToOwnerMeshSocket = bAttachSkeletalVisualToOwnerMeshSocket;
	Request.bPlayAnimationDirectly = bPlaySkeletalVisualAnimationsDirectly;
	Request.State = CurrentState;
	Request.DirectionState = CurrentDirectionState;
	Request.VisualState = CurrentVisualState;
	Request.Variant = Variants.Resolve(CurrentVisualState);

	FUOUUmbrellaSkeletalVisualPlaybackState PlaybackState;
	PlaybackState.bHasAppliedAnimation = bHasAppliedSkeletalVisualAnimation;
	PlaybackState.LastAppliedAnimation = LastAppliedSkeletalVisualAnimation;
	FUOUUmbrellaSkeletalVisualPresenter::Apply(Request, PlaybackState);
	bHasAppliedSkeletalVisualAnimation = PlaybackState.bHasAppliedAnimation;
	LastAppliedSkeletalVisualAnimation = PlaybackState.LastAppliedAnimation;
}

void UUOUUmbrellaComponent::PlayUmbrellaAudioEvent(FName AudioEventId) const
{
	if (AudioEventId.IsNone())
	{
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	UUOUAudioSubsystem* AudioSubsystem = GameInstance != nullptr ? GameInstance->GetSubsystem<UUOUAudioSubsystem>() : nullptr;
	if (AudioSubsystem == nullptr)
	{
		return;
	}

	AudioSubsystem->PlayAudioEventAtLocation(AudioEventId, GetUmbrellaAudioLocation());
}

void UUOUUmbrellaComponent::PlayUmbrellaAudioCue(FName CueId, FName FallbackAudioEventId) const
{
	if (!CueId.IsNone())
	{
		if (UUOUAudioCueComponent* AudioCueComponent = GetAudioCueComponent())
		{
			if (AudioCueComponent->HasCue(CueId)
				&& AudioCueComponent->PlayCueAtLocation(CueId, GetUmbrellaAudioLocation()))
			{
				return;
			}
		}
	}

	PlayUmbrellaAudioEvent(FallbackAudioEventId);
}

void UUOUUmbrellaComponent::MarkRainBlockedAudioActive()
{
	if (UWorld* World = GetWorld())
	{
		LastRainBlockedAudioTime = World->GetTimeSeconds();
	}
	else
	{
		LastRainBlockedAudioTime = 0.0f;
	}

	static double LastMarkLogTime = -1000.0;
	if (ShouldLogRainBlockedAudioDiagnostic(this, LastMarkLogTime))
	{
		UE_LOG(
			LogUOUAudio,
			Verbose,
			TEXT("[RainBlockedAudio][MarkActive] Owner=%s Time=%.3f Playing=%s IsOpen=%s"),
			*GetNameSafe(GetOwner()),
			LastRainBlockedAudioTime,
			bRainBlockedAudioPlaying ? TEXT("true") : TEXT("false"),
			IsOpen() ? TEXT("true") : TEXT("false"));
	}

	StartRainBlockedAudio();
}

void UUOUUmbrellaComponent::StartRainBlockedAudio()
{
	const UWorld* CurrentWorld = GetWorld();
	const float CurrentAudioTime = CurrentWorld != nullptr
		? CurrentWorld->GetTimeSeconds()
		: static_cast<float>(FPlatformTime::Seconds());
	if (CurrentAudioTime - LastRainBlockedAudioRefreshAttemptTime < RainBlockedAudioRefreshInterval)
	{
		return;
	}
	LastRainBlockedAudioRefreshAttemptTime = CurrentAudioTime;

	if (!IsOpen())
	{
		UE_LOG(
			LogUOUAudio,
			Verbose,
			TEXT("[RainBlockedAudio][StartFailed] Owner=%s Reason=NotOpen HasUmbrella=%s State=%d"),
			*GetNameSafe(GetOwner()),
			bHasUmbrella ? TEXT("true") : TEXT("false"),
			static_cast<int32>(CurrentState));
		return;
	}

	const FVector AudioLocation = GetUmbrellaAudioLocation();
	const FName AudioInstanceId = BuildRainBlockedAudioInstanceId();
	const FName AudioEventId = ResolveRainBlockedAudioEventId();

	UE_LOG(
		LogUOUAudio,
		Verbose,
		TEXT("[RainBlockedAudio][Start] Owner=%s Cue=%s ResolvedEvent=%s Instance=%s Location=(%.1f %.1f %.1f)"),
		*GetNameSafe(GetOwner()),
		*RainBlockedAudioCueId.ToString(),
		*AudioEventId.ToString(),
		*AudioInstanceId.ToString(),
		AudioLocation.X,
		AudioLocation.Y,
		AudioLocation.Z);

	bool bPlayed = false;
	if (!AudioEventId.IsNone())
	{
		UWorld* World = GetWorld();
		UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
		if (UUOUAudioSubsystem* AudioSubsystem = GameInstance != nullptr ? GameInstance->GetSubsystem<UUOUAudioSubsystem>() : nullptr)
		{
			bPlayed = AudioSubsystem->PlayManagedAudioEventInstance(AudioEventId, AudioInstanceId, AudioLocation);
		}
		else
		{
			UE_LOG(
				LogUOUAudio,
				Warning,
				TEXT("[RainBlockedAudio][StartFailed] Owner=%s Reason=MissingAudioSubsystem World=%s GameInstance=%s Event=%s"),
				*GetNameSafe(GetOwner()),
				World != nullptr ? TEXT("valid") : TEXT("null"),
				GameInstance != nullptr ? TEXT("valid") : TEXT("null"),
				*AudioEventId.ToString());
		}
	}
	else
	{
		UE_LOG(
			LogUOUAudio,
			Warning,
			TEXT("[RainBlockedAudio][StartFailed] Owner=%s Reason=NoneEvent Cue=%s FallbackEvent=%s"),
			*GetNameSafe(GetOwner()),
			*RainBlockedAudioCueId.ToString(),
			*RainBlockedAudioEventId.ToString());
	}

	bRainBlockedAudioPlaying = bPlayed;
	ActiveRainBlockedAudioEventId = bPlayed ? AudioEventId : NAME_None;

	UE_LOG(
		LogUOUAudio,
		Verbose,
		TEXT("[RainBlockedAudio][StartResult] Owner=%s Event=%s Instance=%s Played=%s"),
		*GetNameSafe(GetOwner()),
		*AudioEventId.ToString(),
		*AudioInstanceId.ToString(),
		bPlayed ? TEXT("true") : TEXT("false"));
}

void UUOUUmbrellaComponent::StopRainBlockedAudio()
{
	if (!bRainBlockedAudioPlaying)
	{
		return;
	}

	const FName AudioInstanceId = BuildRainBlockedAudioInstanceId();
	const FName AudioEventId = !ActiveRainBlockedAudioEventId.IsNone()
		? ActiveRainBlockedAudioEventId
		: ResolveRainBlockedAudioEventId();

	if (!AudioEventId.IsNone())
	{
		UWorld* World = GetWorld();
		UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
		if (UUOUAudioSubsystem* AudioSubsystem = GameInstance != nullptr ? GameInstance->GetSubsystem<UUOUAudioSubsystem>() : nullptr)
		{
			UE_LOG(
				LogUOUAudio,
				Verbose,
				TEXT("[RainBlockedAudio][Stop] Owner=%s Event=%s Instance=%s"),
				*GetNameSafe(GetOwner()),
				*AudioEventId.ToString(),
				*AudioInstanceId.ToString());
			AudioSubsystem->StopAudioEvent(AudioEventId, AudioInstanceId, 0.0f);
		}
	}

	bRainBlockedAudioPlaying = false;
	LastRainBlockedAudioTime = -1000.0f;
	LastRainBlockedAudioRefreshAttemptTime = -1000.0f;
	ActiveRainBlockedAudioEventId = NAME_None;
}

void UUOUUmbrellaComponent::UpdateRainBlockedAudioState()
{
	if (!bRainBlockedAudioPlaying)
	{
		return;
	}

	if (!IsOpen())
	{
		StopRainBlockedAudio();
		return;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float StopDelay = FMath::Max(0.0f, RainBlockedAudioStopDelay);
	if (World->GetTimeSeconds() - LastRainBlockedAudioTime > StopDelay)
	{
		StopRainBlockedAudio();
	}
}

FName UUOUUmbrellaComponent::ResolveRainBlockedAudioEventId() const
{
	if (!RainBlockedAudioCueId.IsNone())
	{
		if (UUOUAudioCueComponent* AudioCueComponent = GetAudioCueComponent())
		{
			FName CueAudioEventId = NAME_None;
			if (AudioCueComponent->ResolveAudioEventId(RainBlockedAudioCueId, CueAudioEventId))
			{
				UE_LOG(
					LogUOUAudio,
					Verbose,
					TEXT("[RainBlockedAudio][ResolveEvent] Owner=%s Source=Cue Cue=%s Event=%s AudioCueComponent=%s"),
					*GetNameSafe(GetOwner()),
					*RainBlockedAudioCueId.ToString(),
					*CueAudioEventId.ToString(),
					*GetNameSafe(AudioCueComponent));
				return CueAudioEventId;
			}

			UE_LOG(
				LogUOUAudio,
				Verbose,
				TEXT("[RainBlockedAudio][ResolveEvent] Owner=%s Source=CueFailed Cue=%s FallbackEvent=%s AudioCueComponent=%s"),
				*GetNameSafe(GetOwner()),
				*RainBlockedAudioCueId.ToString(),
				*RainBlockedAudioEventId.ToString(),
				*GetNameSafe(AudioCueComponent));
		}
		else
		{
			UE_LOG(
				LogUOUAudio,
				Verbose,
				TEXT("[RainBlockedAudio][ResolveEvent] Owner=%s Source=NoCueComponent Cue=%s FallbackEvent=%s"),
				*GetNameSafe(GetOwner()),
				*RainBlockedAudioCueId.ToString(),
				*RainBlockedAudioEventId.ToString());
		}
	}

	UE_LOG(
		LogUOUAudio,
		Verbose,
		TEXT("[RainBlockedAudio][ResolveEvent] Owner=%s Source=Fallback Event=%s"),
		*GetNameSafe(GetOwner()),
		*RainBlockedAudioEventId.ToString());
	return RainBlockedAudioEventId;
}

FName UUOUUmbrellaComponent::BuildRainBlockedAudioInstanceId() const
{
	const AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return TEXT("Umbrella.RainBlocked");
	}

	return FName(*FString::Printf(TEXT("%s.RainBlocked"), *Owner->GetName()));
}

UUOUAudioCueComponent* UUOUUmbrellaComponent::GetAudioCueComponent() const
{
	AActor* Owner = GetOwner();
	return Owner != nullptr ? Owner->FindComponentByClass<UUOUAudioCueComponent>() : nullptr;
}

FVector UUOUUmbrellaComponent::GetUmbrellaAudioLocation() const
{
	const AActor* Owner = GetOwner();
	return Owner != nullptr ? Owner->GetActorLocation() : FVector::ZeroVector;
}

// 블루프린트 세팅이 비어 있어도 약속된 이름과 컴포넌트 타입으로 필요한 참조를 찾아 채웁니다.
void UUOUUmbrellaComponent::ResolveReferences()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (PickupAttachPoint == nullptr)
	{
		// 플레이어 손이나 우산 부착 지점으로 쓰는 씬 컴포넌트를 이름으로 찾습니다.
		TInlineComponentArray<USceneComponent*> SceneComponents(Owner);
		for (USceneComponent* SceneComponent : SceneComponents)
		{
			if (SceneComponent != nullptr && SceneComponent->GetFName() == TEXT("UmbrellaAttachPoint"))
			{
				PickupAttachPoint = SceneComponent;
				break;
			}
		}
	}

	if (HeldVisualAnchor == nullptr)
	{
		// 픽업 메쉬를 손에 붙일 때 세부 위치와 회전을 잡는 앵커를 이름으로 찾습니다.
		TInlineComponentArray<USceneComponent*> SceneComponents(Owner);
		for (USceneComponent* SceneComponent : SceneComponents)
		{
			if (SceneComponent != nullptr && SceneComponent->GetFName() == TEXT("UmbrellaHeldVisualAnchor"))
			{
				HeldVisualAnchor = SceneComponent;
				break;
			}
		}
	}

	if (SkeletalHeldVisual == nullptr)
	{
		TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents(Owner);
		for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
		{
			if (SkeletalMeshComponent != nullptr && SkeletalMeshComponent->GetFName() == TEXT("UmbrellaSkeletalVisual"))
			{
				SkeletalHeldVisual = SkeletalMeshComponent;
				break;
			}
		}
	}

	if (PourOrigin == nullptr)
	{
		// 물 붓기 라인트레이스가 시작될 화살표 컴포넌트를 이름으로 찾습니다.
		TInlineComponentArray<UArrowComponent*> ArrowComponents(Owner);
		for (UArrowComponent* ArrowComponent : ArrowComponents)
		{
			if (ArrowComponent != nullptr && ArrowComponent->GetFName() == TEXT("PourOrigin"))
			{
				PourOrigin = ArrowComponent;
				break;
			}
		}
	}

	if (StoredWaterContainer == nullptr)
	{
		// 우산과 같은 액터에 붙은 물 저장 컴포넌트를 자동으로 연결합니다.
		StoredWaterContainer = Owner->FindComponentByClass<UUOUWaterContainerComponent>();
	}

	if (RainReceiver == nullptr)
	{
		// 우산과 같은 액터에 붙은 비 노출 컴포넌트를 자동으로 연결합니다.
		RainReceiver = Owner->FindComponentByClass<UUOURainReceiverComponent>();
	}
}

// 픽업한 우산 외형을 런타임에 표시할 메쉬 컴포넌트를 없으면 새로 만듭니다.
void UUOUUmbrellaComponent::EnsureRuntimeHeldVisual()
{
	if (RuntimeHeldVisual != nullptr)
	{
		RuntimeHeldVisualBaseRelativeTransform = RuntimeHeldVisual->GetRelativeTransform();
		return;
	}

	const FTransform AnchorRelativeTransform = HeldVisualAnchor != nullptr
		? HeldVisualAnchor->GetRelativeTransform()
		: FTransform::Identity;
	RuntimeHeldVisualBaseRelativeTransform = FUOUUmbrellaRuntimeVisualPresenter::CalculateBaseRelativeTransform(
		AnchorRelativeTransform,
		HeldVisualRelativeScale,
		FVector::OneVector,
		bUsePickupMeshRelativeScale);
	RuntimeHeldVisual = FUOUUmbrellaRuntimeVisualPresenter::EnsureVisual(
		GetOwner(),
		PickupAttachPoint,
		RuntimeHeldVisual,
		RuntimeHeldVisualBaseRelativeTransform);
}

void UUOUUmbrellaComponent::EnsurePouringEffect()
{
	AActor* Owner = GetOwner();
	const UUOUPourContentProfile* ContentProfile = ResolvePourContentProfile();
	if (Owner == nullptr || ContentProfile == nullptr || ContentProfile->StreamEffect == nullptr)
	{
		return;
	}

	USceneComponent* AttachParent = nullptr;
	FName AttachSocketName = NAME_None;
	if (SkeletalHeldVisual != nullptr && SkeletalHeldVisual->DoesSocketExist(PouringSocketName))
	{
		AttachParent = SkeletalHeldVisual;
		AttachSocketName = PouringSocketName;
	}
	else
	{
		AttachParent = PourOrigin != nullptr
			? static_cast<USceneComponent*>(PourOrigin.Get())
			: HeldVisualAnchor.Get();
	}
	if (AttachParent == nullptr)
	{
		AttachParent = Owner->GetRootComponent();
	}

	if (PouringEffectComponent == nullptr)
	{
		PouringEffectComponent = NewObject<UNiagaraComponent>(Owner, TEXT("PouringNiagaraEffect"));
		if (PouringEffectComponent == nullptr)
		{
			return;
		}

		Owner->AddInstanceComponent(PouringEffectComponent);
		PouringEffectComponent->SetAutoActivate(false);
		PouringEffectComponent->SetAutoDestroy(false);
		PouringEffectComponent->SetVisibility(false, true);
		PouringEffectComponent->SetHiddenInGame(true, true);
		PouringEffectComponent->SetupAttachment(AttachParent, AttachSocketName);
		PouringEffectComponent->RegisterComponent();
	}
	else if (AttachParent != nullptr
		&& (PouringEffectComponent->GetAttachParent() != AttachParent
			|| PouringEffectComponent->GetAttachSocketName() != AttachSocketName))
	{
		PouringEffectComponent->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform, AttachSocketName);
	}

	PouringEffectComponent->SetAsset(ContentProfile->StreamEffect);
	PouringEffectComponent->SetWorldScale3D(ContentProfile->StreamRelativeScale);
	UpdatePouringEffectTransform();
}

void UUOUUmbrellaComponent::UpdatePouringEffectState()
{
	const UUOUPourContentProfile* ContentProfile = ResolvePourContentProfile();
	const bool bShouldPlayPouringEffect = bHasUmbrella
		&& CurrentState == EUOUUmbrellaState::Pouring
		&& ContentProfile != nullptr
		&& ContentProfile->StreamEffect != nullptr;

	if (bShouldPlayPouringEffect)
	{
		EnsurePouringEffect();
		if (PouringEffectComponent != nullptr)
		{
			UpdatePouringEffectTransform();
			PouringEffectComponent->SetHiddenInGame(false, true);
			PouringEffectComponent->SetVisibility(true, true);
			if (!PouringEffectComponent->IsActive())
			{
				PouringEffectComponent->Activate(true);
			}
		}
		return;
	}

	if (PouringEffectComponent != nullptr)
	{
		if (PouringEffectComponent->IsActive())
		{
			PouringEffectComponent->DeactivateImmediate();
		}

		PouringEffectComponent->SetVisibility(false, true);
		PouringEffectComponent->SetHiddenInGame(true, true);
	}
}

bool UUOUUmbrellaComponent::TryGetPouringPointTransform(FTransform& OutTransform) const
{
	const USkeletalMeshComponent* PouringSocketSource = ResolvePouringSocketSourceComponent();
	if (PouringSocketSource == nullptr || PouringSocketName.IsNone() || !PouringSocketSource->DoesSocketExist(PouringSocketName))
	{
		return false;
	}

	OutTransform = PouringSocketSource->GetSocketTransform(PouringSocketName, RTS_World);
	if (!PouringSocketWorldUnitOffset.IsNearlyZero())
	{
		OutTransform.AddToTranslation(OutTransform.GetRotation().RotateVector(PouringSocketWorldUnitOffset));
	}
	return true;
}

const USkeletalMeshComponent* UUOUUmbrellaComponent::ResolvePouringSocketSourceComponent() const
{
	if (!PouringSocketSourceComponentName.IsNone())
	{
		if (const AActor* Owner = GetOwner())
		{
			TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents(Owner);
			for (const USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
			{
				if (SkeletalMeshComponent != nullptr
					&& (SkeletalMeshComponent->GetFName() == PouringSocketSourceComponentName
						|| SkeletalMeshComponent->ComponentTags.Contains(PouringSocketSourceComponentName)))
				{
					return SkeletalMeshComponent;
				}
			}
		}
	}

	return SkeletalHeldVisual.Get();
}

void UUOUUmbrellaComponent::UpdatePouringEffectTransform()
{
	if (PouringEffectComponent == nullptr)
	{
		return;
	}

	FVector DropLocation = FVector::ZeroVector;
	FVector DropDirection = FVector::ForwardVector;
	if (!TryGetPourDropSpawnPlacement(DropLocation, DropDirection))
	{
		return;
	}

	FVector StreamForward = FVector(DropDirection.X, DropDirection.Y, 0.0f);
	if (StreamForward.IsNearlyZero())
	{
		if (const AActor* Owner = GetOwner())
		{
			StreamForward = FVector(Owner->GetActorForwardVector().X, Owner->GetActorForwardVector().Y, 0.0f);
		}
	}

	if (StreamForward.IsNearlyZero())
	{
		StreamForward = FVector::ForwardVector;
	}

	StreamForward.Normalize();

	const FQuat DirectionRotation = FRotationMatrix::MakeFromYZ(StreamForward, FVector::UpVector).ToQuat();
	const UUOUPourContentProfile* ContentProfile = ResolvePourContentProfile();
	const FRotator RelativeRotation = ContentProfile != nullptr ? ContentProfile->StreamRelativeRotation : FRotator::ZeroRotator;
	const FVector RelativeScale = ContentProfile != nullptr ? ContentProfile->StreamRelativeScale : FVector::OneVector;
	const FQuat EffectRotation = DirectionRotation * FRotator(0.0f, RelativeRotation.Yaw, 0.0f).Quaternion();

	PouringEffectComponent->SetWorldLocationAndRotation(DropLocation, EffectRotation.Rotator());
	PouringEffectComponent->SetWorldScale3D(RelativeScale);
}

bool UUOUUmbrellaComponent::TryGetPourDropSpawnPlacement(FVector& OutDropLocation, FVector& OutDropDirection) const
{
	FVector DropOrigin = FVector::ZeroVector;
	FVector DropDirection = FVector::ForwardVector;
	if (!TryGetPourDirection(DropOrigin, DropDirection))
	{
		return false;
	}

	if (DropDirection.IsNearlyZero())
	{
		return false;
	}

	DropDirection = DropDirection.GetSafeNormal();
	OutDropLocation = DropOrigin;
	OutDropDirection = DropDirection;
	return true;
}

const UUOUPourContentProfile* UUOUUmbrellaComponent::ResolvePourContentProfile() const
{
	return StoredWaterContainer != nullptr ? StoredWaterContainer->PourContentProfile.Get() : nullptr;
}

TSubclassOf<AUOUPourDropActor> UUOUUmbrellaComponent::ResolvePourDropActorClass() const
{
	const UUOUPourContentProfile* ContentProfile = ResolvePourContentProfile();
	if (ContentProfile != nullptr && ContentProfile->DropActorClass != nullptr)
	{
		return ContentProfile->DropActorClass;
	}

	return AUOUPourDropActor::StaticClass();
}

// 월드 픽업 메쉬에서 스태틱 메쉬와 머티리얼을 읽어 손에 든 비주얼로 복사합니다.
void UUOUUmbrellaComponent::ApplyHeldVisualFromMeshComponent(UStaticMeshComponent* SourceMeshComponent)
{
	if (SourceMeshComponent == nullptr)
	{
		return;
	}

	const FUOUUmbrellaRuntimeVisualAssets Assets = FUOUUmbrellaRuntimeVisualPresenter::CaptureAssets(SourceMeshComponent);
	ApplyHeldVisualFromAssets(Assets.Mesh, Assets.Materials, Assets.SourceRelativeScale);
}

// 전달받은 메쉬와 머티리얼을 런타임 우산 비주얼에 적용하고 상태별 표시를 다시 맞춥니다.
void UUOUUmbrellaComponent::ApplyHeldVisualFromAssets(UStaticMesh* Mesh, const TArray<UMaterialInterface*>& Materials, const FVector& SourceRelativeScale)
{
	EnsureRuntimeHeldVisual();
	if (RuntimeHeldVisual == nullptr)
	{
		return;
	}

	FUOUUmbrellaRuntimeVisualAssets Assets;
	Assets.Mesh = Mesh;
	Assets.Materials = Materials;
	Assets.SourceRelativeScale = SourceRelativeScale;
	FUOUUmbrellaRuntimeVisualPresenter::ApplyAssets(RuntimeHeldVisual, Assets, DefaultHeldMesh);

	const FTransform AnchorRelativeTransform = HeldVisualAnchor != nullptr
		? HeldVisualAnchor->GetRelativeTransform()
		: FTransform::Identity;
	RuntimeHeldVisualBaseRelativeTransform = FUOUUmbrellaRuntimeVisualPresenter::CalculateBaseRelativeTransform(
		AnchorRelativeTransform,
		HeldVisualRelativeScale,
		SourceRelativeScale,
		bUsePickupMeshRelativeScale);
	FUOUUmbrellaRuntimeVisualPresenter::ApplyStateTransform(
		RuntimeHeldVisual,
		RuntimeHeldVisualBaseRelativeTransform,
		bFlipRuntimeHeldVisualWhenUpsideDown,
		CurrentVisualState,
		UpsideDownHeldVisualRotationOffset,
		UpsideDownHeldVisualLocationOffset);

	RefreshVisuals();
}

// 플레이 중 우산 보유 상태, 저장된 물, 마지막 붓기 대상을 화면에 표시합니다.
void UUOUUmbrellaComponent::DrawScreenDebug() const
{
	// 플레이어/우산 화면 디버그는 Debug Controller의 Player HUD에서 통합 표시합니다.
}

// 우산이 비를 막는 중심과 범위를 월드에 그려 RainArea 판정 위치를 눈으로 확인합니다.
void UUOUUmbrellaComponent::DrawRainBlockerDebug() const
{
	if (!UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Player))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FVector BlockerWorldCenter = FVector::ZeroVector;
	FRotator BlockerWorldRotation = FRotator::ZeroRotator;
	FVector BlockerHalfExtent = FVector::ZeroVector;
	if (!TryGetGameplayRainBlockerVolumeData(BlockerWorldCenter, BlockerWorldRotation, BlockerHalfExtent))
	{
		// 비를 막는 상태가 아니면 그릴 기준 데이터가 없으므로 바로 종료합니다.
		return;
	}

	const float Thickness = FMath::Max(0.0f, RainBlockerDebugThickness);
	const float LifeTime = 0.0f;
	const bool bIsActiveBlocker = IsBlockingRain();
	const FColor PlayerDebugColor = bIsActiveBlocker
		? FColor::Cyan
		: FColor(90, 90, 90);

	DrawDebugSphere(
		World,
		BlockerWorldCenter,
		8.0f,
		12,
		PlayerDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugBox(
		World,
		BlockerWorldCenter,
		BlockerHalfExtent,
		BlockerWorldRotation.Quaternion(),
		PlayerDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugLine(
		World,
		BlockerWorldCenter + BlockerWorldRotation.Quaternion().GetAxisZ() * BlockerHalfExtent.Z,
		BlockerWorldCenter - BlockerWorldRotation.Quaternion().GetAxisZ() * BlockerHalfExtent.Z,
		PlayerDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugString(
		World,
		BlockerWorldCenter + BlockerWorldRotation.Quaternion().GetAxisZ() * (BlockerHalfExtent.Z + 18.0f),
		FString::Printf(
			TEXT("Gameplay RainBlocker %s Half %.1f %.1f %.1f Offset %.1f %.1f %.1f"),
			bIsActiveBlocker ? TEXT("Active") : TEXT("Inactive"),
			BlockerHalfExtent.X,
			BlockerHalfExtent.Y,
			BlockerHalfExtent.Z,
			RainBlockerLocalOffset.X,
			RainBlockerLocalOffset.Y,
			RainBlockerLocalOffset.Z),
		nullptr,
		PlayerDebugColor,
		LifeTime,
		false,
		1.0f);
}

// 물 붓기 라인트레이스의 마지막 결과를 월드에 그려 어느 대상에 닿았는지 확인합니다.
void UUOUUmbrellaComponent::DrawPourTraceDebug() const
{
	if (!bHasLastPourTrace
		|| !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Player))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float LifeTime = FMath::Max(0.0f, PourTraceDebugLifeTime);
	const float Thickness = FMath::Max(0.0f, PourTraceDebugThickness);
	const FVector DrawEnd = bLastPourTraceHit ? LastPourTraceImpactPoint : LastPourTraceEnd;
	const FColor ImpactPointColor = bLastPourCheckedWaterBasinImpactPoint
		? (bLastPourImpactPointInsideWaterBasin ? FColor::Green : FColor::Red)
		: FColor::Orange;
	const FColor TraceColor = UUOUDebugSubsystem::GetDebugCategoryColor(
		this,
		EUOUDebugCategory::Player,
		bLastPourDeliveredWater ? FColor::Green : (bLastPourTraceHit ? FColor::Red : FColor::Cyan));

	DrawDebugLine(
		World,
		LastPourTraceStart,
		DrawEnd,
		TraceColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugSphere(
		World,
		LastPourTraceStart,
		6.0f,
		12,
		TraceColor,
		false,
		LifeTime,
		0,
		Thickness);

	if (bLastPourTraceHit)
	{
		DrawDebugSphere(
			World,
			LastPourTraceImpactPoint,
			8.0f,
			12,
			ImpactPointColor,
			false,
			LifeTime,
			0,
			Thickness);

		const float ImpactCrossSize = 18.0f;
		DrawDebugLine(
			World,
			LastPourTraceImpactPoint - FVector(ImpactCrossSize, 0.0f, 0.0f),
			LastPourTraceImpactPoint + FVector(ImpactCrossSize, 0.0f, 0.0f),
			ImpactPointColor,
			false,
			LifeTime,
			0,
			Thickness);
		DrawDebugLine(
			World,
			LastPourTraceImpactPoint - FVector(0.0f, ImpactCrossSize, 0.0f),
			LastPourTraceImpactPoint + FVector(0.0f, ImpactCrossSize, 0.0f),
			ImpactPointColor,
			false,
			LifeTime,
			0,
			Thickness);
		DrawDebugLine(
			World,
			LastPourTraceImpactPoint - FVector(0.0f, 0.0f, ImpactCrossSize),
			LastPourTraceImpactPoint + FVector(0.0f, 0.0f, ImpactCrossSize),
			ImpactPointColor,
			false,
			LifeTime,
			0,
			Thickness);
	}

	if (UUOUDebugSubsystem::IsDebugWorldLabelEnabled(this, EUOUDebugCategory::Player))
	{
		const FVector LabelLocation = DrawEnd + FVector(0.0f, 0.0f, 24.0f);
		const FString ImpactPointText = bLastPourTraceHit
			? FString::Printf(
				TEXT("\nImpactPoint: X %.1f / Y %.1f / Z %.1f"),
				LastPourTraceImpactPoint.X,
				LastPourTraceImpactPoint.Y,
				LastPourTraceImpactPoint.Z)
			: FString();
		const FString WaterBasinImpactText = bLastPourCheckedWaterBasinImpactPoint
			? FString::Printf(
				TEXT("\nBasin 판정: %s"),
				bLastPourImpactPointInsideWaterBasin ? TEXT("내부") : TEXT("외부"))
			: FString();
		const FString LabelText = FString::Printf(
			TEXT("Pour Trace\nHit: %s\nTarget: %s\nReceiver: %s\nAmount: %.2f\nStored: %.2f -> %.2f%s%s"),
			*LastPourHitName,
			*LastPourTargetName,
			GetPourReceiverTypeText(LastPourReceiverType),
			LastPourAmount,
			LastPourStoredWaterBefore,
			LastPourStoredWaterAfter,
			*ImpactPointText,
			*WaterBasinImpactText);

		DrawDebugString(
			World,
			LabelLocation,
			LabelText,
			nullptr,
			TraceColor,
			LifeTime,
			true,
			1.0f);
	}
}

// 물 붓기 디버그 기록을 초기 상태로 돌립니다.
void UUOUUmbrellaComponent::ClearPourTraceDebug()
{
	bHasLastPourTrace = false;
	bLastPourTraceHit = false;
	bLastPourDeliveredWater = false;
	LastPourTraceStart = FVector::ZeroVector;
	LastPourTraceEnd = FVector::ZeroVector;
	LastPourTraceImpactPoint = FVector::ZeroVector;
	bLastPourCheckedWaterBasinImpactPoint = false;
	bLastPourImpactPointInsideWaterBasin = false;
	LastPourAmount = 0.0f;
	LastPourStoredWaterBefore = GetCurrentStoredWater();
	LastPourStoredWaterAfter = GetCurrentStoredWater();
	LastPourReceiverType = EUOUUmbrellaPourReceiverType::None;
}

// 물을 붓는 동안 캐릭터 몸 방향을 마우스 조준 방향에 맞춥니다.
void UUOUUmbrellaComponent::DrawPourSocketAndDropSpawnDebug() const
{
	if (!bHasUmbrella || (!bDrawPourSocketDebug && !bDrawPourDropSpawnDebug))
	{
		return;
	}

	if (!UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Player))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float Radius = FMath::Max(1.0f, PourSocketDebugRadius);
	const float LifeTime = 0.0f;
	const float Thickness = 2.0f;

	FTransform SocketTransform = FTransform::Identity;
	if (bDrawPourSocketDebug && TryGetPouringPointTransform(SocketTransform))
	{
		const FVector SocketLocation = SocketTransform.GetLocation();
		const USkeletalMeshComponent* SocketSource = ResolvePouringSocketSourceComponent();
		const FString SocketSourceName = GetNameSafe(SocketSource);
		const FString SocketMeshName = SocketSource != nullptr ? GetNameSafe(SocketSource->GetSkeletalMeshAsset()) : TEXT("None");
		const FString SocketDebugText = FString::Printf(
			TEXT("PourSocket\nComponent: %s\nMesh: %s\nSocket: %s\nOffset: %.1f %.1f %.1f"),
			*SocketSourceName,
			*SocketMeshName,
			*PouringSocketName.ToString(),
			PouringSocketWorldUnitOffset.X,
			PouringSocketWorldUnitOffset.Y,
			PouringSocketWorldUnitOffset.Z);
		DrawDebugSphere(World, SocketLocation, Radius, 16, FColor::Magenta, false, LifeTime, 0, Thickness);
		DrawDebugCoordinateSystem(World, SocketLocation, SocketTransform.Rotator(), Radius * 2.5f, false, LifeTime, 0, Thickness);
		DrawDebugString(World, SocketLocation + FVector(0.0f, 0.0f, Radius + 18.0f), SocketDebugText, nullptr, FColor::Magenta, LifeTime, true);
	}

	FVector DropLocation = FVector::ZeroVector;
	FVector DropDirection = FVector::ForwardVector;
	if (bDrawPourDropSpawnDebug && TryGetPourDropSpawnPlacement(DropLocation, DropDirection))
	{
		const FVector SafeDirection = DropDirection.IsNearlyZero() ? FVector::DownVector : DropDirection.GetSafeNormal();
		DrawDebugSphere(World, DropLocation, Radius * 0.7f, 16, FColor::Yellow, false, LifeTime, 0, Thickness);
		DrawDebugLine(World, DropLocation, DropLocation + SafeDirection * 120.0f, FColor::Yellow, false, LifeTime, 0, Thickness);
		DrawDebugLine(World, DropLocation, DropLocation + FVector::DownVector * 120.0f, FColor::Cyan, false, LifeTime, 0, Thickness);
		DrawDebugString(World, DropLocation + FVector(0.0f, 0.0f, Radius + 36.0f), TEXT("DropSpawn"), nullptr, FColor::Yellow, LifeTime, true);
	}
}

void UUOUUmbrellaComponent::UpdateUmbrellaAimFacing()
{
	const bool bIsPourAimActive = CurrentState == EUOUUmbrellaState::Pouring && bRotateOwnerTowardsPourDirection;
	const bool bIsLightReflectingAimActive = CurrentState == EUOUUmbrellaState::LightReflecting &&
		bRotateOwnerTowardsLightReflectingDirection;
	if (!bIsPourAimActive && !bIsLightReflectingAimActive)
	{
		ClearPourAimFacing();
		return;
	}

	ApplyAimFacingMovementOverride();

	FVector AimDirection = FVector::ZeroVector;
	FVector AimPoint = FVector::ZeroVector;
	if (!TryGetMouseAimDirection(AimDirection, AimPoint))
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	AimDirection = bIsLightReflectingAimActive
		? SnapLightReflectingDirectionToAngleStep(AimDirection)
		: SnapPourDirectionToAngleStep(AimDirection);
	if (AimDirection.IsNearlyZero())
	{
		return;
	}

	FRotator AimRotation = AimDirection.Rotation();
	// 캐릭터가 위아래로 기울어지지 않도록 평면 회전만 적용합니다.
	AimRotation.Pitch = 0.0f;
	AimRotation.Roll = 0.0f;
	Owner->SetActorRotation(AimRotation);
}

// 현재는 별도 보관 상태가 없지만, 조준 보정 정리 지점을 명확히 남겨둡니다.
void UUOUUmbrellaComponent::ClearPourAimFacing()
{
	if (!bHasAimFacingMovementOverride)
	{
		return;
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement())
		{
			CharacterMovement->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		}
	}

	bHasAimFacingMovementOverride = false;
}

void UUOUUmbrellaComponent::ApplyAimFacingMovementOverride()
{
	if (bHasAimFacingMovementOverride)
	{
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* CharacterMovement = OwnerCharacter != nullptr
		? OwnerCharacter->GetCharacterMovement()
		: nullptr;
	if (CharacterMovement == nullptr)
	{
		return;
	}

	bSavedOrientRotationToMovement = CharacterMovement->bOrientRotationToMovement;
	CharacterMovement->bOrientRotationToMovement = false;
	bHasAimFacingMovementOverride = true;
}

bool UUOUUmbrellaComponent::SpawnPendingPourDrop()
{
	if (PendingPourDropVolume <= KINDA_SMALL_NUMBER || PendingPourDropDuration <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	TSubclassOf<AUOUPourDropActor> ResolvedPourDropActorClass = ResolvePourDropActorClass();
	if (World == nullptr || Owner == nullptr || ResolvedPourDropActorClass == nullptr)
	{
		return false;
	}

	FVector DropLocation = FVector::ZeroVector;
	FVector DropDirection = FVector::ForwardVector;
	if (!TryGetPourDropSpawnPlacement(DropLocation, DropDirection))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Owner;
	SpawnParameters.Instigator = Cast<APawn>(Owner);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AUOUPourDropActor* DropActor = World->SpawnActor<AUOUPourDropActor>(
		ResolvedPourDropActorClass,
		DropLocation,
		DropDirection.Rotation(),
		SpawnParameters);
	if (DropActor == nullptr)
	{
		return false;
	}

	DropActor->OnPourDropImpacted.RemoveDynamic(this, &UUOUUmbrellaComponent::HandlePourDropImpacted);
	DropActor->OnPourDropImpacted.AddDynamic(this, &UUOUUmbrellaComponent::HandlePourDropImpacted);

	FUOUPourDropContext DropContext;
	DropContext.Volume = PendingPourDropVolume;
	DropContext.Duration = PendingPourDropDuration;
	DropContext.WorldDirection = DropDirection;
	DropContext.InstigatorActor = Owner;
	DropContext.bApplyToConnectedWaterBasinGroup = bPourDropAppliesToConnectedWaterBasinGroup;
	if (const UUOUPourContentProfile* ContentProfile = ResolvePourContentProfile())
	{
		DropContext.VisualSettings = ContentProfile->DropVisual;
	}
	DropActor->InitializePourDrop(DropContext);
	DropActor->bDrawDebugCollisionRadius = bDrawPourDropCollisionDebug;
	if (bOverridePourDropCollisionRadius)
	{
		DropActor->CollisionRadius = FMath::Max(0.0f, PourDropCollisionRadiusOverride);
		if (DropActor->CollisionComponent != nullptr)
		{
			DropActor->CollisionComponent->SetSphereRadius(DropActor->CollisionRadius, true);
		}
	}

	LastPourTraceStart = DropLocation;
	LastPourTraceEnd = DropLocation + DropDirection * PourDistance;
	LastPourTraceImpactPoint = LastPourTraceEnd;
	bHasLastPourTrace = true;
	bLastPourTraceHit = false;
	bLastPourDeliveredWater = false;
	bLastPourCheckedWaterBasinImpactPoint = false;
	bLastPourImpactPointInsideWaterBasin = false;
	LastPourHitName = TEXT("PourDrop Spawned");
	LastPourTargetName = DropActor->GetName();
	LastPourReceiverType = EUOUUmbrellaPourReceiverType::None;
	LastPourAmount = PendingPourDropVolume;
	LastPourStoredWaterAfter = GetCurrentStoredWater();

	ResetPendingPourDrop();
	return true;
}

void UUOUUmbrellaComponent::ResetPendingPourDrop()
{
	PendingPourDropVolume = 0.0f;
	PendingPourDropDuration = 0.0f;
	TimeSinceLastPourDropSpawn = 0.0f;
}

void UUOUUmbrellaComponent::PrimeNextPourDropSpawn()
{
	TimeSinceLastPourDropSpawn = FMath::Max(0.0f, PourDropSpawnInterval);
}

// Drop impact delegate updates the existing pour debug fields after the spawned actor reaches a receiver.
void UUOUUmbrellaComponent::HandlePourDropImpacted(
	AUOUPourDropActor* DropActor,
	AActor* ImpactActor,
	FVector ImpactLocation,
	EUOUPourDropReceiverType ReceiverType,
	bool bDeliveredWater)
{
	LastPourTraceImpactPoint = ImpactLocation;
	LastPourTraceEnd = ImpactLocation;
	bHasLastPourTrace = true;
	bLastPourTraceHit = ImpactActor != nullptr;
	bLastPourDeliveredWater = bDeliveredWater;
	LastPourHitName = bDeliveredWater ? TEXT("PourDrop Delivered") : TEXT("PourDrop Impact");
	LastPourTargetName = IsValid(ImpactActor) ? ImpactActor->GetName() : TEXT("None");
	LastPourReceiverType = ConvertPourDropReceiverType(ReceiverType);
	LastPourAmount = IsValid(DropActor) ? DropActor->CurrentVolume : 0.0f;
	LastPourStoredWaterAfter = GetCurrentStoredWater();

	bLastPourCheckedWaterBasinImpactPoint = false;
	bLastPourImpactPointInsideWaterBasin = false;
	if (ReceiverType == EUOUPourDropReceiverType::WaterBasinTarget && IsValid(ImpactActor))
	{
		UUOUWaterBasinTargetComponent* WaterBasinTarget = ImpactActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
		if (WaterBasinTarget == nullptr)
		{
			if (AActor* ParentActor = ImpactActor->GetAttachParentActor())
			{
				WaterBasinTarget = ParentActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
			}
		}

		if (WaterBasinTarget != nullptr)
		{
			bLastPourCheckedWaterBasinImpactPoint = true;
			bLastPourImpactPointInsideWaterBasin = WaterBasinTarget->IsWorldLocationInsideBasin(ImpactLocation);
		}
	}
}

// 붓기 상태에서 저장된 물을 줄이고, 일정 간격으로 낙하 물 액터를 생성합니다.
void UUOUUmbrellaComponent::HandlePourContentProfileChanged(UUOUPourContentProfile* NewProfile)
{
	(void)NewProfile;
	UpdatePouringEffectState();
}

void UUOUUmbrellaComponent::UpdatePouring(float DeltaTime)
{
	if (CurrentState != EUOUUmbrellaState::Pouring || StoredWaterContainer == nullptr)
	{
		ClearPourTraceDebug();
		ResetPendingPourDrop();
		return;
	}

	const float StoredWaterBefore = GetCurrentStoredWater();
	if (StoredWaterBefore <= KINDA_SMALL_NUMBER)
	{
		if (!SpawnPendingPourDrop())
		{
			ResetPendingPourDrop();
		}
		EndPour();
		return;
	}

	const float SafePourRate = FMath::Max(0.0f, PourRate);
	const float SafeConsumptionMultiplier = FMath::Max(0.0f, PourStoredWaterConsumptionMultiplier);
	const float RequestedPourAmount = SafePourRate * FMath::Max(0.0f, DeltaTime);
	const float MaxPourAmountByStoredWater = SafeConsumptionMultiplier > KINDA_SMALL_NUMBER
		? StoredWaterBefore / SafeConsumptionMultiplier
		: RequestedPourAmount;
	const float PourAmount = FMath::Min(MaxPourAmountByStoredWater, RequestedPourAmount);
	if (PourAmount <= KINDA_SMALL_NUMBER)
	{
		EndPour();
		return;
	}

	// Basin 채우기 모드는 실제로 붓는 시간만 사용해서 마지막 부분 프레임에서 과하게 채워지지 않게 합니다.
	const float EffectivePourDuration = SafePourRate > KINDA_SMALL_NUMBER ? PourAmount / SafePourRate : 0.0f;

	// 저장된 양보다 많이 소모하지 않도록 제한한 뒤 실제 저장량을 줄입니다.
	const float ConsumedStoredWater = PourAmount * SafeConsumptionMultiplier;
	StoredWaterContainer->RemoveAmount(ConsumedStoredWater);
	const float StoredWaterAfter = GetCurrentStoredWater();

	PendingPourDropVolume += PourAmount;
	PendingPourDropDuration += EffectivePourDuration;
	TimeSinceLastPourDropSpawn += FMath::Max(0.0f, DeltaTime);

	FVector DropOrigin = FVector::ZeroVector;
	FVector DropDirection = FVector::ForwardVector;
	LastPourHitName = TEXT("PourDrop Pending");
	LastPourTargetName = TEXT("None");
	LastPourReceiverType = EUOUUmbrellaPourReceiverType::None;
	bHasLastPourTrace = false;
	bLastPourTraceHit = false;
	bLastPourDeliveredWater = false;
	bLastPourCheckedWaterBasinImpactPoint = false;
	bLastPourImpactPointInsideWaterBasin = false;
	LastPourAmount = PourAmount;
	LastPourStoredWaterBefore = StoredWaterBefore;
	LastPourStoredWaterAfter = StoredWaterAfter;

	if (TryGetPourDropSpawnPlacement(DropOrigin, DropDirection))
	{
		LastPourTraceStart = DropOrigin;
		LastPourTraceEnd = DropOrigin + DropDirection * PourDistance;
		LastPourTraceImpactPoint = LastPourTraceEnd;
		bHasLastPourTrace = true;
	}
	else
	{
		ClearPourTraceDebug();
		LastPourHitName = TEXT("Invalid Pour Direction");
		LastPourTargetName = TEXT("None");
		ResetPendingPourDrop();
	}

	const float SafeSpawnInterval = FMath::Max(0.0f, PourDropSpawnInterval);
	if (PendingPourDropVolume > KINDA_SMALL_NUMBER
		&& (SafeSpawnInterval <= KINDA_SMALL_NUMBER || TimeSinceLastPourDropSpawn >= SafeSpawnInterval)
		&& !SpawnPendingPourDrop())
	{
		LastPourHitName = ResolvePourDropActorClass() == nullptr ? TEXT("Missing PourDrop Class") : TEXT("PourDrop Spawn Failed");
	}

	if (GetCurrentStoredWater() <= 0.0f)
	{
		// 물이 다 떨어지면 입력을 계속 누르고 있어도 붓기를 종료합니다.
		if (!SpawnPendingPourDrop())
		{
			ResetPendingPourDrop();
		}
		EndPour();
	}
}

// 마우스 아래 월드 지점 또는 마우스 레이를 바닥 평면에 투영해 조준 방향을 구합니다.
bool UUOUUmbrellaComponent::TryGetMouseAimDirection(FVector& AimDirection, FVector& AimPoint) const
{
	AimDirection = FVector::ZeroVector;
	AimPoint = FVector::ZeroVector;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = OwnerPawn != nullptr ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (PlayerController == nullptr)
	{
		return false;
	}

	if (TryGetScreenSpaceMouseAimDirection(PlayerController, AimDirection, AimPoint))
	{
		return true;
	}

	FHitResult CursorHit;
	if (PlayerController->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(MouseAimTraceChannel), true, CursorHit))
	{
		if (CursorHit.GetActor() != GetOwner())
		{
			// 커서가 실제 월드 물체를 찍으면 그 지점을 플레이어 기준 평면 방향으로 사용합니다.
			FVector FlatDirection = CursorHit.ImpactPoint - GetOwner()->GetActorLocation();
			FlatDirection.Z = 0.0f;
			if (!FlatDirection.IsNearlyZero())
			{
				AimPoint = CursorHit.ImpactPoint;
				AimDirection = FlatDirection.GetSafeNormal();
				return true;
			}
		}
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	if (!PlayerController->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	// 커서가 아무 물체도 찍지 못한 경우 마우스 레이를 플레이어 높이의 평면과 교차시킵니다.
	const FVector OwnerLocation = GetOwner() != nullptr ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	const FPlane GroundPlane(OwnerLocation, FVector::UpVector);
	const FVector PlaneIntersection = FMath::LinePlaneIntersection(WorldOrigin, WorldOrigin + WorldDirection * MouseAimRayDistance, GroundPlane);
	FVector FlatDirection = PlaneIntersection - OwnerLocation;
	FlatDirection.Z = 0.0f;
	if (FlatDirection.IsNearlyZero())
	{
		return false;
	}

	AimPoint = PlaneIntersection;
	AimDirection = FlatDirection.GetSafeNormal();
	return true;
}

// 물을 부을 시작점과 방향을 정합니다. 마우스 조준이 가능하면 우산 앞 방향보다 마우스 방향을 우선합니다.
bool UUOUUmbrellaComponent::TryGetScreenSpaceMouseAimDirection(APlayerController* PlayerController, FVector& AimDirection, FVector& AimPoint) const
{
	AimDirection = FVector::ZeroVector;
	AimPoint = FVector::ZeroVector;

	const AActor* Owner = GetOwner();
	if (!bUseScreenSpacePourAim || PlayerController == nullptr || Owner == nullptr)
	{
		return false;
	}

	FVector2D OwnerScreenPosition = FVector2D::ZeroVector;
	if (!PlayerController->ProjectWorldLocationToScreen(Owner->GetActorLocation(), OwnerScreenPosition, true))
	{
		return false;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	FVector2D ScreenDirection(MouseX - OwnerScreenPosition.X, OwnerScreenPosition.Y - MouseY);
	if (ScreenDirection.SizeSquared() <= FMath::Square(FMath::Max(0.0f, ScreenSpacePourAimDeadZone)))
	{
		return false;
	}

	ScreenDirection.Normalize();
	const bool bUseLightReflectingSnap = CurrentState == EUOUUmbrellaState::LightReflecting;
	const bool bShouldSnapAim = bUseLightReflectingSnap
		? bSnapLightReflectingAimDirection
		: bSnapPourAimDirection;
	if (bShouldSnapAim)
	{
		const float SnapAngleDegrees = bUseLightReflectingSnap
			? LightReflectingAimSnapAngleDegrees
			: PourAimSnapAngleDegrees;
		const float SafeStep = FMath::Clamp(SnapAngleDegrees, 1.0f, 180.0f);
		const float ScreenAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(ScreenDirection.Y, ScreenDirection.X));
		const float SnappedScreenAngleDegrees = FMath::GridSnap(ScreenAngleDegrees, SafeStep);
		const float SnappedScreenAngleRadians = FMath::DegreesToRadians(SnappedScreenAngleDegrees);
		ScreenDirection = FVector2D(FMath::Cos(SnappedScreenAngleRadians), FMath::Sin(SnappedScreenAngleRadians));
	}

	const FRotator CameraRotation = PlayerController->PlayerCameraManager != nullptr
		? PlayerController->PlayerCameraManager->GetCameraRotation()
		: PlayerController->GetControlRotation();
	const FRotationMatrix CameraRotationMatrix(CameraRotation);
	FVector ScreenRightWorld = CameraRotationMatrix.GetScaledAxis(EAxis::Y);
	FVector ScreenUpWorld = CameraRotationMatrix.GetScaledAxis(EAxis::Z);
	ScreenRightWorld.Z = 0.0f;
	ScreenUpWorld.Z = 0.0f;

	if (ScreenRightWorld.IsNearlyZero())
	{
		ScreenRightWorld = FVector::RightVector;
	}
	if (ScreenUpWorld.IsNearlyZero())
	{
		ScreenUpWorld = CameraRotationMatrix.GetScaledAxis(EAxis::X);
		ScreenUpWorld.Z = 0.0f;
	}
	if (ScreenUpWorld.IsNearlyZero())
	{
		ScreenUpWorld = FVector::ForwardVector;
	}

	ScreenRightWorld.Normalize();
	ScreenUpWorld.Normalize();

	FVector WorldDirection = ScreenRightWorld * ScreenDirection.X + ScreenUpWorld * ScreenDirection.Y;
	WorldDirection.Z = 0.0f;
	if (WorldDirection.IsNearlyZero())
	{
		return false;
	}

	AimDirection = WorldDirection.GetSafeNormal();
	AimPoint = Owner->GetActorLocation() + AimDirection * MouseAimRayDistance;
	return true;
}

FVector UUOUUmbrellaComponent::SnapPourDirectionToAngleStep(const FVector& Direction) const
{
	FVector FlatDirection(Direction.X, Direction.Y, 0.0f);
	if (!bSnapPourAimDirection || FlatDirection.IsNearlyZero())
	{
		return FlatDirection.GetSafeNormal();
	}

	const float SafeStep = FMath::Clamp(PourAimSnapAngleDegrees, 1.0f, 180.0f);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(FlatDirection.Y, FlatDirection.X));
	const float SnappedAngleDegrees = FMath::GridSnap(AngleDegrees, SafeStep);
	const float SnappedAngleRadians = FMath::DegreesToRadians(SnappedAngleDegrees);
	return FVector(FMath::Cos(SnappedAngleRadians), FMath::Sin(SnappedAngleRadians), 0.0f).GetSafeNormal();
}

FVector UUOUUmbrellaComponent::SnapLightReflectingDirectionToAngleStep(const FVector& Direction) const
{
	FVector FlatDirection(Direction.X, Direction.Y, 0.0f);
	if (!bSnapLightReflectingAimDirection || FlatDirection.IsNearlyZero())
	{
		return FlatDirection.GetSafeNormal();
	}

	const float SafeStep = FMath::Clamp(LightReflectingAimSnapAngleDegrees, 1.0f, 180.0f);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(FlatDirection.Y, FlatDirection.X));
	const float SnappedAngleDegrees = FMath::GridSnap(AngleDegrees, SafeStep);
	const float SnappedAngleRadians = FMath::DegreesToRadians(SnappedAngleDegrees);
	return FVector(FMath::Cos(SnappedAngleRadians), FMath::Sin(SnappedAngleRadians), 0.0f).GetSafeNormal();
}

bool UUOUUmbrellaComponent::TryGetPourDirection(FVector& PourOriginLocation, FVector& PourDirection) const
{
	AActor* Owner = GetOwner();
	FTransform PouringPointTransform = FTransform::Identity;
	if (TryGetPouringPointTransform(PouringPointTransform))
	{
		PourOriginLocation = PouringPointTransform.GetLocation();
		PourDirection = PouringPointTransform.GetRotation().GetForwardVector();
		PourDirection.Z = 0.0f;
	}
	else
	{
		const USceneComponent* OriginComponent = nullptr;
		if (PourOrigin != nullptr)
		{
			OriginComponent = PourOrigin;
		}
		else if (Owner != nullptr)
		{
			OriginComponent = Cast<USceneComponent>(Owner->GetRootComponent());
		}

		if (OriginComponent == nullptr)
		{
			return false;
		}

		PourOriginLocation = OriginComponent->GetComponentLocation();
		PourDirection = OriginComponent->GetForwardVector();
		PourDirection.Z = 0.0f;
	}

	FVector AimDirection = FVector::ZeroVector;
	FVector AimPoint = FVector::ZeroVector;
	if (TryGetMouseAimDirection(AimDirection, AimPoint))
	{
		// 물줄기가 손 위치에서 마우스가 가리키는 지점으로 향하도록 방향을 다시 계산합니다.
		if (!AimDirection.IsNearlyZero())
		{
			PourDirection = AimDirection.GetSafeNormal();
		}
	}

	if (PourDirection.IsNearlyZero() && Owner != nullptr)
	{
		PourDirection = Owner->GetActorForwardVector();
		PourDirection.Z = 0.0f;
	}

	if (PourDirection.IsNearlyZero())
	{
		PourDirection = FVector::ForwardVector;
	}

	PourDirection = SnapPourDirectionToAngleStep(PourDirection);
	return !PourDirection.IsNearlyZero();
}

// 물을 담고 있던 상태에서 닫힘이나 펼침으로 바뀌면 더 이상 담을 수 없으므로 버려야 합니다.
bool UUOUUmbrellaComponent::ShouldSpillStoredWater(EUOUUmbrellaState PreviousState, EUOUUmbrellaState NextState) const
{
	const bool bWasHoldingWater = PreviousState == EUOUUmbrellaState::UpsideDown ||
		PreviousState == EUOUUmbrellaState::Pouring;
	const bool bWillNotHoldWater = NextState == EUOUUmbrellaState::Open || NextState == EUOUUmbrellaState::Closed;
	return bWasHoldingWater && bWillNotHoldWater && GetCurrentStoredWater() > 0.0f;
}

// 저장 컨테이너의 물 양을 0으로 만들어 우산에서 물을 흘린 상태로 처리합니다.
void UUOUUmbrellaComponent::SpillStoredWater()
{
	if (StoredWaterContainer != nullptr)
	{
		StoredWaterContainer->SetAmount(0.0f);
	}
}
