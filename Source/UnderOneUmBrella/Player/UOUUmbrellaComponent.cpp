// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaComponent.h"

#include "Audio/UOUAudioCueComponent.h"
#include "Audio/UOUAudioSubsystem.h"
#include "Components/ArrowComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Player/UOURainReceiverComponent.h"
#include "Player/UOUWaterContainerComponent.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"
#include "World/WaterTarget/UOUUmbrellaWaterTarget.h"

namespace
{
// 물 붓기 디버그 라벨에 표시할 수 있도록 수신 대상 enum을 짧은 문자열로 바꿉니다.
const TCHAR* GetPourReceiverTypeText(EUOUUmbrellaPourReceiverType ReceiverType)
{
	switch (ReceiverType)
	{
	case EUOUUmbrellaPourReceiverType::UmbrellaWaterTarget:
		return TEXT("UmbrellaWaterTarget");
	case EUOUUmbrellaPourReceiverType::WaterBasinTarget:
		return TEXT("WaterBasinTarget");
	case EUOUUmbrellaPourReceiverType::WaterContainer:
		return TEXT("WaterContainer");
	case EUOUUmbrellaPourReceiverType::None:
	default:
		return TEXT("None");
	}
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

	if (StoredWaterContainer != nullptr)
	{
		// 우산에 저장된 물이 퍼즐 무게로 환산될 때 쓰는 배율을 동기화합니다.
		StoredWaterContainer->WeightMultiplier = FMath::Max(0.0f, StoredWaterWeightMultiplier);
	}

	bHasUmbrella = bStartWithUmbrella;
	CurrentState = EUOUUmbrellaState::Closed;

	if (bHasUmbrella && DefaultHeldMesh != nullptr)
	{
		// 우산을 들고 시작하는 경우 픽업 과정 없이 기본 메쉬를 손 비주얼에 적용합니다.
		ApplyHeldVisualFromAssets(DefaultHeldMesh, {}, FVector::OneVector);
	}

	RefreshVisuals();
}

// 매 프레임 우산 상태에 따라 물 붓기, 마우스 조준 회전, 디버그 표시를 갱신합니다.
void UUOUUmbrellaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHasUmbrella)
	{
		// 우산이 없어도 디버그는 상태 확인용으로 남기고, 조준과 물줄기 기록은 정리합니다.
		ClearPourAimFacing();
		ClearPourTraceDebug();
		DrawScreenDebug();
		DrawRainBlockerDebug();
		return;
	}

	UpdatePourAimFacing();
	UpdatePouring(DeltaTime);
	DrawScreenDebug();
	DrawRainBlockerDebug();
	DrawPourTraceDebug();
}

// 우산을 새로 획득했을 때 보유 상태와 저장 물을 초기화합니다.
void UUOUUmbrellaComponent::AcquireUmbrella()
{
	if (bHasUmbrella)
	{
		return;
	}

	bHasUmbrella = true;
	SetState(EUOUUmbrellaState::Closed);

	if (StoredWaterContainer != nullptr)
	{
		StoredWaterContainer->SetAmount(0.0f);
	}

	PlayUmbrellaAudioCue(AcquireAudioCueId, AcquireAudioEventId);
	OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
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

	bHasUmbrella = false;
	SetState(EUOUUmbrellaState::Closed);

	if (StoredWaterContainer != nullptr)
	{
		StoredWaterContainer->SetAmount(0.0f);
	}

	if (RainReceiver != nullptr)
	{
		RainReceiver->ClearExposure();
	}

	ClearPourAimFacing();
	OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
}

// 우산을 펼쳐서 비를 막는 상태로 전환합니다.
void UUOUUmbrellaComponent::OpenUmbrella()
{
	if (!bHasUmbrella)
	{
		return;
	}

	SetState(EUOUUmbrellaState::Open);
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

	SetState(EUOUUmbrellaState::UpsideDown);
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

	if (IsOpen())
	{
		// 펼친 우산은 플레이어에게 비를 넘기지 않고 차단 이벤트만 보냅니다.
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
		CloseUmbrella();
		break;
	case EUOUUmbrellaState::UpsideDown:
	case EUOUUmbrellaState::Pouring:
		OpenUmbrella();
		break;
	}
}

// 뒤집힌 상태는 닫힘으로 되돌리고, 그 외 상태에서는 뒤집힌 상태로 진입합니다.
void UUOUUmbrellaComponent::ToggleInvertState()
{
	switch (CurrentState)
	{
	case EUOUUmbrellaState::UpsideDown:
		CloseUmbrella();
		break;
	case EUOUUmbrellaState::Pouring:
		EndPour();
		break;
	default:
		TurnUmbrellaUpsideDown();
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

// 우산이 뒤집혔거나 물을 붓는 중이면 점프를 막아 플레이 감각을 안정시킵니다.
bool UUOUUmbrellaComponent::BlocksJumping() const
{
	return bHasUmbrella && (CurrentState == EUOUUmbrellaState::UpsideDown || CurrentState == EUOUUmbrellaState::Pouring);
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

	const USceneComponent* BlockerComponent = OpenVisual;
	if (BlockerComponent == nullptr)
	{
		BlockerComponent = RuntimeHeldVisual;
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

float UUOUUmbrellaComponent::GetCurrentStoredWater() const
{
	return StoredWaterContainer != nullptr ? StoredWaterContainer->CurrentAmount : 0.0f;
}

// 비 노출 컴포넌트가 없을 때도 디버그와 UI가 안전하게 0을 받을 수 있게 감쌉니다.
float UUOUUmbrellaComponent::GetCurrentPlayerRainAmount() const
{
	return RainReceiver != nullptr ? RainReceiver->CurrentExposure : 0.0f;
}

// 우산 상태 변경을 한 곳으로 모아 물 버림, 비주얼 갱신, 이벤트 호출 순서를 고정합니다.
void UUOUUmbrellaComponent::SetState(EUOUUmbrellaState NewState)
{
	const EUOUUmbrellaState PreviousState = CurrentState;
	const EUOUUmbrellaState ResolvedState = bHasUmbrella ? NewState : EUOUUmbrellaState::Closed;
	if (PreviousState == ResolvedState)
	{
		// 같은 상태여도 에디터 세팅 변경 뒤 비주얼을 다시 맞출 수 있게 갱신은 수행합니다.
		RefreshVisuals();
		return;
	}

	if (ShouldSpillStoredWater(PreviousState, ResolvedState))
	{
		// 물을 담을 수 없는 상태로 바뀌면 저장된 물을 흘린 것으로 처리합니다.
		SpillStoredWater();
	}

	CurrentState = ResolvedState;

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
	OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
}

// 전용 상태 비주얼이 있으면 그 비주얼을 쓰고, 없으면 런타임 복사 메쉬 하나로 표시합니다.
void UUOUUmbrellaComponent::RefreshVisuals()
{
	const bool bHasDedicatedVisuals = ClosedVisual != nullptr || OpenVisual != nullptr || UpsideDownVisual != nullptr;

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
			ClosedVisual->SetVisibility(CurrentState == EUOUUmbrellaState::Closed, true);
		}

		if (OpenVisual != nullptr)
		{
			OpenVisual->SetVisibility(CurrentState == EUOUUmbrellaState::Open || CurrentState == EUOUUmbrellaState::Pouring, true);
		}

		if (UpsideDownVisual != nullptr)
		{
			UpsideDownVisual->SetVisibility(CurrentState == EUOUUmbrellaState::UpsideDown, true);
		}

		if (RuntimeHeldVisual != nullptr)
		{
			RuntimeHeldVisual->SetVisibility(false, true);
		}

		return;
	}

	if (RuntimeHeldVisual != nullptr)
	{
		// 상태별 비주얼이 없다면 픽업에서 복사한 런타임 메쉬 하나를 계속 보여줍니다.
		RuntimeHeldVisual->SetVisibility(true, true);
	}
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
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	RuntimeHeldVisual = NewObject<UStaticMeshComponent>(Owner, TEXT("RuntimeHeldUmbrellaVisual"));
	if (RuntimeHeldVisual == nullptr)
	{
		return;
	}

	// 런타임 비주얼은 충돌과 그림자를 끄고 순수 표시용으로만 사용합니다.
	Owner->AddInstanceComponent(RuntimeHeldVisual);
	RuntimeHeldVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RuntimeHeldVisual->SetGenerateOverlapEvents(false);
	RuntimeHeldVisual->SetCastShadow(false);
	RuntimeHeldVisual->SetVisibility(false, true);

	USceneComponent* AttachParent = PickupAttachPoint != nullptr ? PickupAttachPoint.Get() : Owner->GetRootComponent();
	// 우산 부착 지점이 있으면 그 아래에 붙이고, 없으면 루트에 붙여 최소 동작을 보장합니다.
	RuntimeHeldVisual->SetupAttachment(AttachParent);
	RuntimeHeldVisual->RegisterComponent();
	RuntimeHeldVisual->SetRelativeTransform(GetHeldVisualRelativeTransform(FVector::OneVector));
}

// 월드 픽업 메쉬에서 스태틱 메쉬와 머티리얼을 읽어 손에 든 비주얼로 복사합니다.
void UUOUUmbrellaComponent::ApplyHeldVisualFromMeshComponent(UStaticMeshComponent* SourceMeshComponent)
{
	if (SourceMeshComponent == nullptr)
	{
		return;
	}

	TArray<TObjectPtr<UMaterialInterface>> Materials;
	const int32 MaterialCount = SourceMeshComponent->GetNumMaterials();
	Materials.Reserve(MaterialCount);
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		// 픽업에서 보이던 재질을 그대로 들고 있는 우산에도 유지합니다.
		Materials.Add(SourceMeshComponent->GetMaterial(MaterialIndex));
	}

	ApplyHeldVisualFromAssets(SourceMeshComponent->GetStaticMesh(), Materials, SourceMeshComponent->GetRelativeScale3D());
}

// 전달받은 메쉬와 머티리얼을 런타임 우산 비주얼에 적용하고 상태별 표시를 다시 맞춥니다.
void UUOUUmbrellaComponent::ApplyHeldVisualFromAssets(UStaticMesh* Mesh, const TArray<TObjectPtr<UMaterialInterface>>& Materials, const FVector& SourceRelativeScale)
{
	EnsureRuntimeHeldVisual();
	if (RuntimeHeldVisual == nullptr)
	{
		return;
	}

	RuntimeHeldVisual->SetStaticMesh(Mesh != nullptr ? Mesh : DefaultHeldMesh.Get());
	RuntimeHeldVisual->SetRelativeTransform(GetHeldVisualRelativeTransform(SourceRelativeScale));

	for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
	{
		RuntimeHeldVisual->SetMaterial(MaterialIndex, Materials[MaterialIndex]);
	}

	RefreshVisuals();
}

// 우산 비주얼의 위치와 회전은 앵커를 따르고, 스케일은 픽업 배율과 보정 배율을 곱해서 만듭니다.
FTransform UUOUUmbrellaComponent::GetHeldVisualRelativeTransform(const FVector& SourceRelativeScale) const
{
	FVector RelativeLocation = FVector::ZeroVector;
	FRotator RelativeRotation = FRotator::ZeroRotator;

	if (HeldVisualAnchor != nullptr)
	{
		RelativeLocation = HeldVisualAnchor->GetRelativeLocation();
		RelativeRotation = HeldVisualAnchor->GetRelativeRotation();
	}

	const FVector EffectiveSourceScale = bUsePickupMeshRelativeScale ? SourceRelativeScale : FVector::OneVector;
	const FVector RelativeScale = FVector(
		HeldVisualRelativeScale.X * EffectiveSourceScale.X,
		HeldVisualRelativeScale.Y * EffectiveSourceScale.Y,
		HeldVisualRelativeScale.Z * EffectiveSourceScale.Z);

	return FTransform(RelativeRotation, RelativeLocation, RelativeScale);
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
	if (!TryGetRainBlockerVolumeData(BlockerWorldCenter, BlockerWorldRotation, BlockerHalfExtent))
	{
		// 비를 막는 상태가 아니면 그릴 기준 데이터가 없으므로 바로 종료합니다.
		return;
	}

	const float Thickness = FMath::Max(0.0f, RainBlockerDebugThickness);
	const float LifeTime = 0.0f;
	const bool bIsActiveBlocker = IsBlockingRain();
	const FColor PlayerDebugColor = bIsActiveBlocker
		? UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::Player, FColor::Cyan)
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
			TEXT("RainBlocker %s Half %.1f %.1f %.1f Offset %.1f %.1f %.1f"),
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
void UUOUUmbrellaComponent::UpdatePourAimFacing()
{
	if (!bRotateOwnerTowardsPourDirection || CurrentState != EUOUUmbrellaState::Pouring)
	{
		ClearPourAimFacing();
		return;
	}

	FVector AimDirection = FVector::ZeroVector;
	FVector AimPoint = FVector::ZeroVector;
	if (!TryGetMouseAimDirection(AimDirection, AimPoint))
	{
		ClearPourAimFacing();
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
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
}

// 붓기 상태에서 저장된 물을 줄이고, 라인트레이스로 맞은 대상에 물을 전달합니다.
void UUOUUmbrellaComponent::UpdatePouring(float DeltaTime)
{
	if (CurrentState != EUOUUmbrellaState::Pouring || StoredWaterContainer == nullptr)
	{
		ClearPourTraceDebug();
		return;
	}

	const float StoredWaterBefore = GetCurrentStoredWater();
	const float SafePourRate = FMath::Max(0.0f, PourRate);
	const float RequestedPourAmount = SafePourRate * FMath::Max(0.0f, DeltaTime);
	const float PourAmount = FMath::Min(StoredWaterBefore, RequestedPourAmount);
	if (PourAmount <= KINDA_SMALL_NUMBER)
	{
		EndPour();
		return;
	}

	// Basin 채우기 모드는 실제로 붓는 시간만 사용해서 마지막 부분 프레임에서 과하게 채워지지 않게 합니다.
	const float EffectivePourDuration = SafePourRate > KINDA_SMALL_NUMBER ? PourAmount / SafePourRate : 0.0f;

	// 저장된 양보다 많이 붓지 않도록 제한한 뒤 실제 저장량을 줄입니다.
	StoredWaterContainer->RemoveAmount(PourAmount);
	const float StoredWaterAfter = GetCurrentStoredWater();

	FVector TraceStart = FVector::ZeroVector;
	FVector TraceDirection = FVector::ForwardVector;
	LastPourHitName = TEXT("No Hit");
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

	if (TryGetPourDirection(TraceStart, TraceDirection))
	{
		UWorld* World = GetWorld();
		AActor* Owner = GetOwner();
		if (World != nullptr && Owner != nullptr)
		{
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UmbrellaPourTrace), false, Owner);
			QueryParams.AddIgnoredActor(Owner);

			const FVector TraceEnd = TraceStart + TraceDirection * PourDistance;
			LastPourTraceStart = TraceStart;
			LastPourTraceEnd = TraceEnd;
			LastPourTraceImpactPoint = TraceEnd;
			bHasLastPourTrace = true;

			TArray<FHitResult> HitResults;
			if (World->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, PourTraceChannel, QueryParams))
			{
				// 여러 대상이 겹쳐 맞을 수 있으므로 가까운 히트부터 검사합니다.
				HitResults.Sort([](const FHitResult& Left, const FHitResult& Right)
				{
					return Left.Distance < Right.Distance;
				});

				for (const FHitResult& HitResult : HitResults)
				{
					AActor* HitActor = HitResult.GetActor();
					if (HitActor == nullptr || HitActor == Owner)
					{
						continue;
					}

					LastPourHitName = GetNameSafe(HitResult.GetComponent());
					LastPourTraceImpactPoint = HitResult.ImpactPoint;
					bLastPourTraceHit = true;

					EUOUUmbrellaPourReceiverType ReceiverType = EUOUUmbrellaPourReceiverType::None;
					bLastPourDeliveredWater = TryReceiveWaterAtHit(HitResult, PourAmount, EffectivePourDuration, TraceDirection, ReceiverType);
					LastPourReceiverType = ReceiverType;
					break;
				}
			}
		}
	}
	else
	{
		ClearPourTraceDebug();
		LastPourHitName = TEXT("Invalid Ray");
		LastPourTargetName = TEXT("None");
	}

	if (GetCurrentStoredWater() <= 0.0f)
	{
		// 물이 다 떨어지면 입력을 계속 누르고 있어도 붓기를 종료합니다.
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
bool UUOUUmbrellaComponent::TryGetPourDirection(FVector& PourOriginLocation, FVector& PourDirection) const
{
	AActor* Owner = GetOwner();
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

	FVector AimDirection = FVector::ZeroVector;
	FVector AimPoint = FVector::ZeroVector;
	if (TryGetMouseAimDirection(AimDirection, AimPoint))
	{
		// 물줄기가 손 위치에서 마우스가 가리키는 지점으로 향하도록 방향을 다시 계산합니다.
		const FVector MouseAimDirection = AimPoint - PourOriginLocation;
		if (!MouseAimDirection.IsNearlyZero())
		{
			PourDirection = MouseAimDirection.GetSafeNormal();
		}
	}

	return !PourDirection.IsNearlyZero();
}

// 라인트레이스에 맞은 액터가 물을 받을 수 있는 타입이면 물을 전달하고 받은 대상 종류를 기록합니다.
bool UUOUUmbrellaComponent::TryReceiveWaterAtHit(const FHitResult& HitResult, float WaterAmount, float PourDuration, const FVector& PourDirection, EUOUUmbrellaPourReceiverType& OutReceiverType)
{
	OutReceiverType = EUOUUmbrellaPourReceiverType::None;

	AActor* HitActor = HitResult.GetActor();
	if (HitActor == nullptr)
	{
		return false;
	}

	if (AUOUUmbrellaWaterTarget* WaterTargetActor = Cast<AUOUUmbrellaWaterTarget>(HitActor))
	{
		// 전용 물 받기 액터는 ReceiveWater 함수로 물을 넘깁니다.
		LastPourTargetName = HitActor->GetName();
		OutReceiverType = EUOUUmbrellaPourReceiverType::UmbrellaWaterTarget;
		WaterTargetActor->ReceiveWater(WaterAmount);
		return true;
	}

	if (AUOUUmbrellaWaterTarget* ParentWaterTargetActor = HitActor->GetAttachParentActor() != nullptr ? Cast<AUOUUmbrellaWaterTarget>(HitActor->GetAttachParentActor()) : nullptr)
	{
		// 맞은 액터가 자식 액터일 수 있어 부모 물 받기 액터도 한 번 더 확인합니다.
		LastPourTargetName = ParentWaterTargetActor->GetName();
		OutReceiverType = EUOUUmbrellaPourReceiverType::UmbrellaWaterTarget;
		ParentWaterTargetActor->ReceiveWater(WaterAmount);
		return true;
	}

	if (UUOUWaterBasinTargetComponent* WaterBasinTarget = HitActor->FindComponentByClass<UUOUWaterBasinTargetComponent>())
	{
		// 물 조절 장치 쪽 타겟 컴포넌트도 우산 물 붓기를 받을 수 있게 처리합니다.
		LastPourTargetName = HitActor->GetName();
		OutReceiverType = EUOUUmbrellaPourReceiverType::WaterBasinTarget;
		const bool bHasValidImpactPoint = HitResult.bBlockingHit
			&& WaterBasinTarget->IsWorldLocationInsideBasin(HitResult.ImpactPoint);
		bLastPourCheckedWaterBasinImpactPoint = true;
		bLastPourImpactPointInsideWaterBasin = bHasValidImpactPoint;
		FUOUWaterBasinInputContext InputContext;
		InputContext.Volume = WaterAmount;
		InputContext.Duration = PourDuration;
		InputContext.Source = EUOUWaterBasinInputSource::PlayerPour;
		InputContext.WorldDirection = PourDirection;
		InputContext.WorldLocation = HitResult.ImpactPoint;
		InputContext.bHasValidWorldLocation = bHasValidImpactPoint;
		InputContext.InstigatorActor = GetOwner();
		InputContext.bApplyToConnectedGroup = true;
		WaterBasinTarget->ReceiveWaterInput(InputContext);
		return true;
	}

	if (AActor* ParentActor = HitActor->GetAttachParentActor())
	{
		if (UUOUWaterBasinTargetComponent* ParentWaterBasinTarget = ParentActor->FindComponentByClass<UUOUWaterBasinTargetComponent>())
		{
			// 히트된 자식 액터 대신 부모가 물 조절 컴포넌트를 들고 있는 경우를 처리합니다.
			LastPourTargetName = ParentActor->GetName();
			OutReceiverType = EUOUUmbrellaPourReceiverType::WaterBasinTarget;
			const bool bHasValidImpactPoint = HitResult.bBlockingHit
				&& ParentWaterBasinTarget->IsWorldLocationInsideBasin(HitResult.ImpactPoint);
			bLastPourCheckedWaterBasinImpactPoint = true;
			bLastPourImpactPointInsideWaterBasin = bHasValidImpactPoint;
			FUOUWaterBasinInputContext InputContext;
			InputContext.Volume = WaterAmount;
			InputContext.Duration = PourDuration;
			InputContext.Source = EUOUWaterBasinInputSource::PlayerPour;
			InputContext.WorldDirection = PourDirection;
			InputContext.WorldLocation = HitResult.ImpactPoint;
			InputContext.bHasValidWorldLocation = bHasValidImpactPoint;
			InputContext.InstigatorActor = GetOwner();
			InputContext.bApplyToConnectedGroup = true;
			ParentWaterBasinTarget->ReceiveWaterInput(InputContext);
			return true;
		}
	}

	if (UUOUWaterContainerComponent* WaterTargetContainer = HitActor->FindComponentByClass<UUOUWaterContainerComponent>())
	{
		// 상자처럼 물 저장 컴포넌트만 가진 액터도 직접 물을 받을 수 있게 처리합니다.
		LastPourTargetName = HitActor->GetName();
		OutReceiverType = EUOUUmbrellaPourReceiverType::WaterContainer;
		WaterTargetContainer->AddAmount(WaterAmount);
		return true;
	}

	return false;
}

// 물을 담고 있던 상태에서 닫힘이나 펼침으로 바뀌면 더 이상 담을 수 없으므로 버려야 합니다.
bool UUOUUmbrellaComponent::ShouldSpillStoredWater(EUOUUmbrellaState PreviousState, EUOUUmbrellaState NextState) const
{
	const bool bWasHoldingWater = PreviousState == EUOUUmbrellaState::UpsideDown || PreviousState == EUOUUmbrellaState::Pouring;
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
