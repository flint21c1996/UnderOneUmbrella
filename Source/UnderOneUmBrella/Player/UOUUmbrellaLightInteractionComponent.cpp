// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaLightInteractionComponent.h"

#include "Components/SceneComponent.h"
#include "Debug/UOUDevelopmentDebugDrawContext.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Player/UOUUmbrellaLightShadeVolumeComponent.h"
#include "World/Light/UOULightInteractionSurfaceComponent.h"

UUOUUmbrellaLightInteractionComponent::UUOUUmbrellaLightInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UUOUUmbrellaLightInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveReferences();
	EnsureRuntimeLightSurface();
	EnsureRuntimeLightShadeVolume();

	if (UmbrellaComponent != nullptr)
	{
		UmbrellaComponent->OnUmbrellaStateChanged.AddUniqueDynamic(
			this,
			&UUOUUmbrellaLightInteractionComponent::HandleUmbrellaStateChanged);
	}

	RefreshLightInteractionMode();

	// 액터 컴포넌트 BeginPlay 순서와 무관하게 UmbrellaComponent의 초기 상태를 한 번 더 반영합니다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UUOUUmbrellaLightInteractionComponent::RefreshLightInteractionMode));
	}
}

void UUOUUmbrellaLightInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UmbrellaComponent != nullptr)
	{
		UmbrellaComponent->OnUmbrellaStateChanged.RemoveDynamic(
			this,
			&UUOUUmbrellaLightInteractionComponent::HandleUmbrellaStateChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UUOUUmbrellaLightInteractionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ApplyRuntimeLightSurfacePlacement();
	ApplyRuntimeLightShadeVolumePlacement();
}

EUOUDebugCategory UUOUUmbrellaLightInteractionComponent::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

#if UOU_WITH_DEVELOPMENT_TOOLS
void UUOUUmbrellaLightInteractionComponent::GatherDevelopmentDebugDraw(
	IUOUDevelopmentDebugDrawContext& Context) const
{
	const UUOULightInteractionSurfaceComponent* LightSurface = LightSurfaceComponent;
	if (!IsValid(LightSurface))
	{
		return;
	}

	constexpr float ReflectorArrowLength = 180.0f;
	constexpr float ReflectorThickness = 3.0f;
	const EUOULightInteractionMode InteractionMode = LightSurface->LightInteractionMode;
	const bool bShadeActive = IsValid(LightShadeVolumeComponent)
		&& LightShadeVolumeComponent->CanShadeLight();
	const bool bDebugBlocking = InteractionMode == EUOULightInteractionMode::Blocking
		|| (InteractionMode == EUOULightInteractionMode::Disabled && bShadeActive);
	const FColor SurfaceColor = InteractionMode == EUOULightInteractionMode::Reflecting
		? FColor::Magenta
		: (bDebugBlocking ? FColor::Yellow : FColor::Silver);
	const FVector SurfaceLocation = LightSurface->GetComponentLocation();
	const FVector SurfaceExtent = LightSurface->GetScaledBoxExtent();

	Context.DrawBox(
		SurfaceLocation,
		SurfaceExtent,
		LightSurface->GetComponentQuat(),
		SurfaceColor,
		ReflectorThickness);

	const AActor* Owner = GetOwner();
	const FVector ReflectionDirection = Owner != nullptr
		? Owner->GetActorForwardVector().GetSafeNormal()
		: LightSurface->GetForwardVector().GetSafeNormal();
	if (InteractionMode == EUOULightInteractionMode::Reflecting
		&& !ReflectionDirection.IsNearlyZero())
	{
		Context.DrawArrow(
			SurfaceLocation,
			SurfaceLocation + ReflectionDirection * ReflectorArrowLength,
			24.0f,
			FColor::Green,
			ReflectorThickness);
	}

	const TCHAR* ModeText = InteractionMode == EUOULightInteractionMode::Reflecting
		? TEXT("Reflecting")
		: (bDebugBlocking ? TEXT("Blocking") : TEXT("Disabled"));
	Context.DrawString(
		SurfaceLocation + FVector(0.0f, 0.0f, SurfaceExtent.Z + 20.0f),
		FString::Printf(
			TEXT("Umbrella Reflector: %s\nExtent: %.1f %.1f %.1f"),
			ModeText,
			SurfaceExtent.X,
			SurfaceExtent.Y,
			SurfaceExtent.Z),
		SurfaceColor,
		1.0f);
}
#endif

void UUOUUmbrellaLightInteractionComponent::RefreshLightInteractionMode()
{
	ResolveReferences();

	if (LightSurfaceComponent == nullptr)
	{
		EnsureRuntimeLightSurface();
	}
	if (LightShadeVolumeComponent == nullptr)
	{
		EnsureRuntimeLightShadeVolume();
	}

	ApplyRuntimeLightSurfacePlacement();
	ApplyRuntimeLightShadeVolumePlacement();

	const bool bHasUmbrella = UmbrellaComponent != nullptr && UmbrellaComponent->bHasUmbrella;
	const EUOUUmbrellaState UmbrellaState = bHasUmbrella
		? UmbrellaComponent->CurrentState
		: EUOUUmbrellaState::Closed;
	const bool bIsLightReflecting = UmbrellaState == EUOUUmbrellaState::LightReflecting;
	const bool bIsNormallySpread = UmbrellaState == EUOUUmbrellaState::Open;

	if (LightShadeVolumeComponent != nullptr)
	{
		const bool bShouldBlockNormally = bIsNormallySpread && bSpreadUmbrellaBlocksLight;
		const bool bShouldBlockWhileReflecting =
			bIsLightReflecting && bLightReflectingStateBlocksLight;
		LightShadeVolumeComponent->SetShadeEnabled(
			bHasUmbrella && (bShouldBlockNormally || bShouldBlockWhileReflecting));
	}

	EUOULightInteractionMode NextMode = EUOULightInteractionMode::Disabled;
	if (bHasUmbrella && bIsLightReflecting)
	{
		if (bLightReflectingStateReflectsLight)
		{
			NextMode = EUOULightInteractionMode::Reflecting;
		}
		else if (bLightReflectingStateBlocksLight)
		{
			NextMode = EUOULightInteractionMode::Blocking;
		}
	}

	if (LightSurfaceComponent != nullptr)
	{
		LightSurfaceComponent->SetLightInteractionMode(NextMode);
	}
}

void UUOUUmbrellaLightInteractionComponent::ResolveReferences()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (bAutoFindUmbrellaComponent && UmbrellaComponent == nullptr)
	{
		UmbrellaComponent = Owner->FindComponentByClass<UUOUUmbrellaComponent>();
	}

	if (bAutoFindLightSurfaceComponent && LightSurfaceComponent == nullptr)
	{
		LightSurfaceComponent = Owner->FindComponentByClass<UUOULightInteractionSurfaceComponent>();
	}

	if (bAutoFindLightShadeVolumeComponent && LightShadeVolumeComponent == nullptr)
	{
		LightShadeVolumeComponent = Owner->FindComponentByClass<UUOUUmbrellaLightShadeVolumeComponent>();
	}
}

void UUOUUmbrellaLightInteractionComponent::EnsureRuntimeLightSurface()
{
	if (LightSurfaceComponent != nullptr || !bCreateRuntimeLightSurfaceWhenMissing)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	USceneComponent* AttachParent = GetLightInteractionAttachParent();
	if (AttachParent == nullptr)
	{
		AttachParent = Owner->GetRootComponent();
	}

	if (AttachParent == nullptr)
	{
		return;
	}

	LightSurfaceComponent = NewObject<UUOULightInteractionSurfaceComponent>(Owner, TEXT("RuntimeUmbrellaLightSurface"));
	if (LightSurfaceComponent == nullptr)
	{
		return;
	}

	Owner->AddInstanceComponent(LightSurfaceComponent);
	LightSurfaceComponent->SetupAttachment(AttachParent);
	LightSurfaceComponent->InitBoxExtent(RuntimeSurfaceBoxExtent);
	LightSurfaceComponent->SetLightInteractionMode(EUOULightInteractionMode::Disabled);
	LightSurfaceComponent->RegisterComponent();
	ApplyRuntimeLightSurfacePlacement();
}

void UUOUUmbrellaLightInteractionComponent::EnsureRuntimeLightShadeVolume()
{
	if (LightShadeVolumeComponent != nullptr || !bCreateRuntimeLightShadeVolumeWhenMissing)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	USceneComponent* AttachParent = GetLightInteractionAttachParent();
	if (AttachParent == nullptr)
	{
		AttachParent = Owner->GetRootComponent();
	}
	if (AttachParent == nullptr)
	{
		return;
	}

	LightShadeVolumeComponent = NewObject<UUOUUmbrellaLightShadeVolumeComponent>(Owner, TEXT("RuntimeUmbrellaLightShadeVolume"));
	if (LightShadeVolumeComponent == nullptr)
	{
		return;
	}

	Owner->AddInstanceComponent(LightShadeVolumeComponent);
	LightShadeVolumeComponent->SetupAttachment(AttachParent);
	LightShadeVolumeComponent->InitBoxExtent(RuntimeShadeVolumeBoxExtent);
	LightShadeVolumeComponent->SetShadeEnabled(false);
	LightShadeVolumeComponent->RegisterComponent();
	ApplyRuntimeLightShadeVolumePlacement();
}

USceneComponent* UUOUUmbrellaLightInteractionComponent::GetLightInteractionAttachParent() const
{
	AActor* Owner = GetOwner();
	if (UmbrellaComponent != nullptr)
	{
		if (UmbrellaComponent->HeldVisualAnchor != nullptr)
		{
			return UmbrellaComponent->HeldVisualAnchor;
		}

		if (UmbrellaComponent->PickupAttachPoint != nullptr)
		{
			return UmbrellaComponent->PickupAttachPoint;
		}
	}

	if (Owner != nullptr)
	{
		TInlineComponentArray<USceneComponent*> SceneComponents(Owner);
		for (USceneComponent* SceneComponent : SceneComponents)
		{
			if (SceneComponent != nullptr && SceneComponent->GetFName() == TEXT("UmbrellaHeldVisualAnchor"))
			{
				return SceneComponent;
			}
		}
	}

	return Owner != nullptr ? Owner->GetRootComponent() : nullptr;
}

void UUOUUmbrellaLightInteractionComponent::ApplyRuntimeLightSurfacePlacement() const
{
	if (LightSurfaceComponent == nullptr)
	{
		return;
	}

	FVector WorldCenter = FVector::ZeroVector;
	FRotator WorldRotation = FRotator::ZeroRotator;
	FVector HalfExtent = FVector::ZeroVector;
	const bool bUseRainBlockerPlacement = TryResolveRainBlockerAlignedTransform(
		WorldCenter,
		WorldRotation,
		HalfExtent);
	FVector SurfaceHalfExtent = bUseRainBlockerPlacement ? HalfExtent : RuntimeSurfaceBoxExtent;
	if (bUseRainBlockerPlacement)
	{
		// 비 차단 볼륨은 높이까지 포함한 두꺼운 박스일 수 있지만, 빛 반사는 실제 우산 면처럼 얇아야 합니다.
		// 로컬 Z가 반사 법선이므로 가로·세로 범위는 유지하고 두께만 반사면 설정값으로 제한합니다.
		SurfaceHalfExtent.Z = FMath::Min(
			FMath::Max(0.0f, SurfaceHalfExtent.Z),
			FMath::Max(0.0f, RuntimeSurfaceBoxExtent.Z));
	}
	LightSurfaceComponent->SetBoxExtent(SurfaceHalfExtent);
	// 우산 반사는 조작 의도가 바로 보이도록 캐릭터가 바라보는 방향을 사용합니다.
	// 거울은 각자의 표면 설정을 유지하므로 기존 입사각·반사각 계산에 영향을 주지 않습니다.
	LightSurfaceComponent->ReflectionDirectionMode = EUOULightReflectionDirectionMode::OwnerForward;
	// 우산은 얇은 박스이므로 가장자리 충돌 노멀 대신 실제 우산 면인 로컬 Up을 반사 법선으로 고정합니다.
	LightSurfaceComponent->ReflectionNormalMode = EUOULightReflectionNormalMode::ComponentUp;
	// 우산 메시가 반사점을 가리므로 거울용 시각 여백을 적용하지 않고 입사광과 반사광을 바로 연결합니다.
	LightSurfaceComponent->ReflectionStartPadding = 0.0f;
	// 차단 박스와 동일한 회전을 사용하므로 로컬 Up이 실제 우산 면의 앞면입니다.
	LightSurfaceComponent->ReflectionFrontNormalMode = EUOULightReflectionFrontNormalMode::ComponentUp;
	LightSurfaceComponent->bPassThroughWhenReflectionRejected =
		bPassLightThroughOutsideReflectionAngle;
	LightSurfaceComponent->MaximumReflectionIncidenceAngle = FMath::Clamp(
		MaximumUmbrellaReflectionIncidenceAngle,
		0.0f,
		89.9f);
	// 우산 가장자리에 빛 중심축이 조금만 걸렸을 때 전체 굵기의 반사광이 생기지 않도록
	// 충돌 위치에 남은 유효 반사 폭을 사용합니다.
	LightSurfaceComponent->bLimitReflectionByImpactOffset = true;
	LightSurfaceComponent->ReflectionImpactEdgeInset = FMath::Max(
		0.0f,
		UmbrellaReflectionEdgeInset);
	LightSurfaceComponent->MinimumReflectionCoverageRatio = FMath::Clamp(
		MinimumUmbrellaReflectionCoverageRatio,
		0.0f,
		1.0f);
	if (bUseRainBlockerPlacement)
	{
		LightSurfaceComponent->SetWorldLocationAndRotation(WorldCenter, WorldRotation);
	}
	else
	{
		LightSurfaceComponent->SetRelativeLocation(RuntimeSurfaceRelativeLocation);
		LightSurfaceComponent->SetRelativeRotation(RuntimeSurfaceRelativeRotation);
	}
}

void UUOUUmbrellaLightInteractionComponent::ApplyRuntimeLightShadeVolumePlacement() const
{
	if (LightShadeVolumeComponent == nullptr)
	{
		return;
	}

	FVector WorldCenter = FVector::ZeroVector;
	FRotator WorldRotation = FRotator::ZeroRotator;
	FVector RainBlockerHalfExtent = FVector::ZeroVector;
	const bool bUseRainBlockerPlacement = TryResolveRainBlockerAlignedTransform(
		WorldCenter,
		WorldRotation,
		RainBlockerHalfExtent);
	const FVector SourceExtent = bUseRainBlockerPlacement
		? RainBlockerHalfExtent
		: RuntimeShadeVolumeBoxExtent;
	const FVector SafeBoxExtent(
		FMath::Max(0.0f, SourceExtent.X),
		FMath::Max(0.0f, SourceExtent.Y),
		FMath::Max(0.0f, SourceExtent.Z));
	LightShadeVolumeComponent->SetBoxExtent(SafeBoxExtent);
	const bool bUseReflectionAngle = UmbrellaComponent != nullptr &&
		UmbrellaComponent->CurrentState == EUOUUmbrellaState::LightReflecting &&
		bPassLightThroughOutsideReflectionAngle;
	LightShadeVolumeComponent->MaximumBlockingIncidenceAngle = FMath::Clamp(
		bUseReflectionAngle
			? MaximumUmbrellaReflectionIncidenceAngle
			: MaximumUmbrellaBlockingIncidenceAngle,
		0.0f,
		89.9f);
	LightShadeVolumeComponent->bBlockFrontFaceOnly = true;
	if (bUseRainBlockerPlacement)
	{
		LightShadeVolumeComponent->SetWorldLocationAndRotation(WorldCenter, WorldRotation);
	}
	else
	{
		LightShadeVolumeComponent->SetRelativeLocation(RuntimeShadeVolumeRelativeLocation);
		LightShadeVolumeComponent->SetRelativeRotation(RuntimeShadeVolumeRelativeRotation);
	}
}

bool UUOUUmbrellaLightInteractionComponent::TryResolveRainBlockerAlignedTransform(
	FVector& OutWorldCenter,
	FRotator& OutWorldRotation,
	FVector& OutHalfExtent) const
{
	OutWorldCenter = FVector::ZeroVector;
	OutWorldRotation = FRotator::ZeroRotator;
	OutHalfExtent = FVector::ZeroVector;
	if (!bAlignLightInteractionToRainBlocker || UmbrellaComponent == nullptr ||
		!UmbrellaComponent->TryGetGameplayRainBlockerVolumeData(
			OutWorldCenter,
			OutWorldRotation,
			OutHalfExtent))
	{
		return false;
	}

	if (UmbrellaComponent->CurrentState != EUOUUmbrellaState::LightReflecting)
	{
		return true;
	}

	const AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return true;
	}

	// 머리 위 비 차단 박스를 플레이어 로컬 공간에서 회전시켜 같은 크기의 판정면을 전방으로 옮깁니다.
	const FTransform OwnerTransform = Owner->GetActorTransform();
	const FQuat StateRotation = LightReflectingBlockerRotationOffset.Quaternion();
	const FVector FixedLocalCenter(
		FMath::Max(0.0f, LightReflectingSurfaceDistanceFromOwner),
		0.0f,
		LightReflectingSurfaceHeightFromOwner);
	OutWorldCenter = OwnerTransform.TransformPosition(
		FixedLocalCenter + LightReflectingBlockerAdditionalLocalOffset);

	const FQuat RainBlockerLocalRotation =
		OwnerTransform.GetRotation().Inverse() * OutWorldRotation.Quaternion();
	OutWorldRotation = (
		OwnerTransform.GetRotation() * StateRotation * RainBlockerLocalRotation).Rotator();
	return true;
}

void UUOUUmbrellaLightInteractionComponent::HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella)
{
	RefreshLightInteractionMode();
}
