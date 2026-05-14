// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaLightInteractionComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightInteractionSurfaceComponent.h"

UUOUUmbrellaLightInteractionComponent::UUOUUmbrellaLightInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUUmbrellaLightInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveReferences();
	EnsureRuntimeLightSurface();

	if (UmbrellaComponent != nullptr)
	{
		UmbrellaComponent->OnUmbrellaStateChanged.AddUniqueDynamic(
			this,
			&UUOUUmbrellaLightInteractionComponent::HandleUmbrellaStateChanged);
	}

	RefreshLightInteractionMode();
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

void UUOUUmbrellaLightInteractionComponent::RefreshLightInteractionMode()
{
	if (LightSurfaceComponent == nullptr)
	{
		EnsureRuntimeLightSurface();
		if (LightSurfaceComponent == nullptr)
		{
			return;
		}
	}

	ApplyRuntimeLightSurfacePlacement();

	EUOULightInteractionMode NextMode = EUOULightInteractionMode::Disabled;
	if (UmbrellaComponent != nullptr && UmbrellaComponent->bHasUmbrella)
	{
		const EUOUUmbrellaState UmbrellaState = UmbrellaComponent->CurrentState;
		const bool bIsUpsideDown = UmbrellaState == EUOUUmbrellaState::UpsideDown ||
			UmbrellaState == EUOUUmbrellaState::Pouring;
		const bool bIsSpread = UmbrellaState == EUOUUmbrellaState::Open || bIsUpsideDown;

		if (bIsUpsideDown && bUpsideDownUmbrellaReflectsLight)
		{
			NextMode = EUOULightInteractionMode::Reflecting;
		}
		else if (bIsSpread && bSpreadUmbrellaBlocksLight)
		{
			NextMode = EUOULightInteractionMode::Blocking;
		}
	}

	LightSurfaceComponent->SetLightInteractionMode(NextMode);
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

	USceneComponent* AttachParent = GetLightSurfaceAttachParent();
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

USceneComponent* UUOUUmbrellaLightInteractionComponent::GetLightSurfaceAttachParent() const
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
	LightSurfaceComponent->SetRelativeLocation(RuntimeSurfaceRelativeLocation);
	LightSurfaceComponent->SetRelativeRotation(RuntimeSurfaceRelativeRotation);
}

void UUOUUmbrellaLightInteractionComponent::HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella)
{
	RefreshLightInteractionMode();
}
