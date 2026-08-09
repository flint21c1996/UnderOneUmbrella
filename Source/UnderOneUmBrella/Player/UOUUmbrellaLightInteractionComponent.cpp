// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaLightInteractionComponent.h"

#include "Components/SceneComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
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
	DrawReflectorDebug();
}

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

	LightSurfaceComponent->SetBoxExtent(RuntimeSurfaceBoxExtent);
	LightSurfaceComponent->ReflectionDirectionMode = EUOULightReflectionDirectionMode::OwnerForward;
	LightSurfaceComponent->ReflectionFrontNormalMode = EUOULightReflectionFrontNormalMode::OwnerForward;
	LightSurfaceComponent->bPassThroughWhenReflectionRejected =
		bPassLightThroughOutsideReflectionAngle;
	LightSurfaceComponent->MaximumReflectionIncidenceAngle = FMath::Clamp(
		MaximumUmbrellaReflectionIncidenceAngle,
		0.0f,
		89.9f);
	LightSurfaceComponent->SetRelativeLocation(RuntimeSurfaceRelativeLocation);
	LightSurfaceComponent->SetRelativeRotation(RuntimeSurfaceRelativeRotation);
}

void UUOUUmbrellaLightInteractionComponent::ApplyRuntimeLightShadeVolumePlacement() const
{
	if (LightShadeVolumeComponent == nullptr)
	{
		return;
	}

	const FVector SafeBoxExtent(
		FMath::Max(0.0f, RuntimeShadeVolumeBoxExtent.X),
		FMath::Max(0.0f, RuntimeShadeVolumeBoxExtent.Y),
		FMath::Max(0.0f, RuntimeShadeVolumeBoxExtent.Z));
	LightShadeVolumeComponent->SetBoxExtent(SafeBoxExtent);
	LightShadeVolumeComponent->SetRelativeLocation(RuntimeShadeVolumeRelativeLocation);
	LightShadeVolumeComponent->SetRelativeRotation(RuntimeShadeVolumeRelativeRotation);
}

void UUOUUmbrellaLightInteractionComponent::DrawReflectorDebug() const
{
	if (!bDrawReflectorDebug ||
		LightSurfaceComponent == nullptr ||
		!UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const EUOULightInteractionMode InteractionMode = LightSurfaceComponent->LightInteractionMode;
	const FColor SurfaceColor = InteractionMode == EUOULightInteractionMode::Reflecting
		? FColor::Magenta
		: (InteractionMode == EUOULightInteractionMode::Blocking ? FColor::Yellow : FColor::Silver);
	const FVector SurfaceLocation = LightSurfaceComponent->GetComponentLocation();
	const float SafeThickness = FMath::Max(0.0f, ReflectorDebugThickness);

	DrawDebugBox(
		World,
		SurfaceLocation,
		LightSurfaceComponent->GetScaledBoxExtent(),
		LightSurfaceComponent->GetComponentQuat(),
		SurfaceColor,
		false,
		0.0f,
		0,
		SafeThickness);

	const AActor* Owner = GetOwner();
	const FVector ReflectionDirection = Owner != nullptr
		? Owner->GetActorForwardVector().GetSafeNormal()
		: LightSurfaceComponent->GetForwardVector().GetSafeNormal();
	if (InteractionMode == EUOULightInteractionMode::Reflecting && !ReflectionDirection.IsNearlyZero())
	{
		const FVector ArrowEnd =
			SurfaceLocation + ReflectionDirection * FMath::Max(0.0f, ReflectorDebugArrowLength);
		DrawDebugDirectionalArrow(
			World,
			SurfaceLocation,
			ArrowEnd,
			24.0f,
			FColor::Green,
			false,
			0.0f,
			0,
			SafeThickness);
	}

	if (UUOUDebugSubsystem::IsDebugWorldLabelEnabled(this, EUOUDebugCategory::Puzzle))
	{
		const TCHAR* ModeText = InteractionMode == EUOULightInteractionMode::Reflecting
			? TEXT("Reflecting")
			: (InteractionMode == EUOULightInteractionMode::Blocking ? TEXT("Blocking") : TEXT("Disabled"));
		DrawDebugString(
			World,
			SurfaceLocation + FVector(0.0f, 0.0f, LightSurfaceComponent->GetScaledBoxExtent().Z + 20.0f),
			FString::Printf(
				TEXT("Umbrella Reflector: %s\nExtent: %.1f %.1f %.1f"),
				ModeText,
				LightSurfaceComponent->GetScaledBoxExtent().X,
				LightSurfaceComponent->GetScaledBoxExtent().Y,
				LightSurfaceComponent->GetScaledBoxExtent().Z),
			nullptr,
			SurfaceColor,
			0.0f,
			false,
			1.0f);
	}
}

void UUOUUmbrellaLightInteractionComponent::HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella)
{
	RefreshLightInteractionMode();
}
