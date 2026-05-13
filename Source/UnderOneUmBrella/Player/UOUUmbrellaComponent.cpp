// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaComponent.h"

#include "Components/ArrowComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Player/UOURainReceiverComponent.h"
#include "Player/UOUWaterContainerComponent.h"
#include "World/WaterTarget/UOUUmbrellaWaterTarget.h"

UUOUUmbrellaComponent::UUOUUmbrellaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUUmbrellaComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveReferences();
	EnsureRuntimeHeldVisual();

	if (StoredWaterContainer != nullptr)
	{
		StoredWaterContainer->WeightMultiplier = FMath::Max(0.0f, StoredWaterWeightMultiplier);
	}

	bHasUmbrella = bStartWithUmbrella;
	CurrentState = EUOUUmbrellaState::Closed;

	if (bHasUmbrella && DefaultHeldMesh != nullptr)
	{
		ApplyHeldVisualFromAssets(DefaultHeldMesh, {}, FVector::OneVector);
	}

	RefreshVisuals();
}

void UUOUUmbrellaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHasUmbrella)
	{
		ClearPourAimFacing();
		DrawScreenDebug();
		return;
	}

	UpdatePourAimFacing();
	UpdatePouring(DeltaTime);
	DrawScreenDebug();
}

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

	OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
}

void UUOUUmbrellaComponent::AcquireUmbrellaFromMeshComponent(UStaticMeshComponent* SourceMeshComponent)
{
	AcquireUmbrella();
	ApplyHeldVisualFromMeshComponent(SourceMeshComponent);
}

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

void UUOUUmbrellaComponent::OpenUmbrella()
{
	if (!bHasUmbrella)
	{
		return;
	}

	SetState(EUOUUmbrellaState::Open);
}

void UUOUUmbrellaComponent::CloseUmbrella()
{
	if (!bHasUmbrella)
	{
		return;
	}

	SetState(EUOUUmbrellaState::Closed);
}

void UUOUUmbrellaComponent::TurnUmbrellaUpsideDown()
{
	if (!bHasUmbrella)
	{
		return;
	}

	SetState(EUOUUmbrellaState::UpsideDown);
}

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

void UUOUUmbrellaComponent::EndPour()
{
	if (CurrentState != EUOUUmbrellaState::Pouring)
	{
		return;
	}

	SetState(EUOUUmbrellaState::UpsideDown);
}

void UUOUUmbrellaComponent::AddCollectedWater(float WaterAmount)
{
	if (!CanCollectWater() || StoredWaterContainer == nullptr || WaterAmount <= 0.0f)
	{
		return;
	}

	StoredWaterContainer->AddAmount(WaterAmount);
}

void UUOUUmbrellaComponent::ApplyRainExposure(float ExposureAmount)
{
	if (ExposureAmount <= 0.0f)
	{
		return;
	}

	if (IsOpen())
	{
		OnRainBlocked.Broadcast(ExposureAmount);
		return;
	}

	if (RainReceiver != nullptr)
	{
		RainReceiver->ApplyRainExposure(ExposureAmount);
	}

	if (CanCollectWater())
	{
		AddCollectedWater(ExposureAmount);
	}
}

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

void UUOUUmbrellaComponent::HandleInputPressed(FKey InputKey)
{
	if (bEnableDebugFillKey && InputKey == DebugFillKey)
	{
		if (StoredWaterContainer != nullptr)
		{
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

void UUOUUmbrellaComponent::HandleInputReleased(FKey InputKey)
{
	if (InputKey == PourKey)
	{
		EndPour();
	}
}

bool UUOUUmbrellaComponent::CanCollectWater() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::UpsideDown;
}

bool UUOUUmbrellaComponent::IsOpen() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::Open;
}

bool UUOUUmbrellaComponent::IsUpsideDown() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::UpsideDown;
}

bool UUOUUmbrellaComponent::IsPouring() const
{
	return bHasUmbrella && CurrentState == EUOUUmbrellaState::Pouring;
}

bool UUOUUmbrellaComponent::BlocksJumping() const
{
	return bHasUmbrella && (CurrentState == EUOUUmbrellaState::UpsideDown || CurrentState == EUOUUmbrellaState::Pouring);
}

bool UUOUUmbrellaComponent::IsBlockingRain() const
{
	return IsOpen();
}

bool UUOUUmbrellaComponent::TryGetRainBlockerData(FVector& OutWorldLocation, float& OutRadius) const
{
	OutWorldLocation = FVector::ZeroVector;
	OutRadius = 0.0f;

	if (!IsBlockingRain() || RainBlockerRadius <= 0.0f)
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
		OutWorldLocation = BlockerComponent->GetComponentTransform().TransformPosition(RainBlockerLocalOffset);
		OutRadius = RainBlockerRadius;
		return true;
	}

	if (const AActor* Owner = GetOwner())
	{
		OutWorldLocation = Owner->GetActorTransform().TransformPosition(RainBlockerLocalOffset);
		OutRadius = RainBlockerRadius;
		return true;
	}

	return false;
}

float UUOUUmbrellaComponent::GetCurrentStoredWater() const
{
	return StoredWaterContainer != nullptr ? StoredWaterContainer->CurrentAmount : 0.0f;
}

float UUOUUmbrellaComponent::GetCurrentPlayerRainAmount() const
{
	return RainReceiver != nullptr ? RainReceiver->CurrentExposure : 0.0f;
}

void UUOUUmbrellaComponent::SetState(EUOUUmbrellaState NewState)
{
	const EUOUUmbrellaState PreviousState = CurrentState;
	const EUOUUmbrellaState ResolvedState = bHasUmbrella ? NewState : EUOUUmbrellaState::Closed;
	if (PreviousState == ResolvedState)
	{
		RefreshVisuals();
		return;
	}

	if (ShouldSpillStoredWater(PreviousState, ResolvedState))
	{
		SpillStoredWater();
	}

	CurrentState = ResolvedState;

	if (CurrentState != EUOUUmbrellaState::Pouring)
	{
		LastPourHitName = TEXT("None");
		LastPourTargetName = TEXT("None");
	}

	RefreshVisuals();
	OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
}

void UUOUUmbrellaComponent::RefreshVisuals()
{
	const bool bHasDedicatedVisuals = ClosedVisual != nullptr || OpenVisual != nullptr || UpsideDownVisual != nullptr;

	if (!bHasUmbrella)
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

		return;
	}

	if (bHasDedicatedVisuals)
	{
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
		RuntimeHeldVisual->SetVisibility(true, true);
	}
}

void UUOUUmbrellaComponent::ResolveReferences()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (PickupAttachPoint == nullptr)
	{
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
		StoredWaterContainer = Owner->FindComponentByClass<UUOUWaterContainerComponent>();
	}

	if (RainReceiver == nullptr)
	{
		RainReceiver = Owner->FindComponentByClass<UUOURainReceiverComponent>();
	}
}

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

	Owner->AddInstanceComponent(RuntimeHeldVisual);
	RuntimeHeldVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RuntimeHeldVisual->SetGenerateOverlapEvents(false);
	RuntimeHeldVisual->SetCastShadow(false);
	RuntimeHeldVisual->SetVisibility(false, true);

	USceneComponent* AttachParent = PickupAttachPoint != nullptr ? PickupAttachPoint.Get() : Owner->GetRootComponent();
	RuntimeHeldVisual->SetupAttachment(AttachParent);
	RuntimeHeldVisual->RegisterComponent();
	RuntimeHeldVisual->SetRelativeTransform(GetHeldVisualRelativeTransform(FVector::OneVector));
}

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
		Materials.Add(SourceMeshComponent->GetMaterial(MaterialIndex));
	}

	ApplyHeldVisualFromAssets(SourceMeshComponent->GetStaticMesh(), Materials, SourceMeshComponent->GetRelativeScale3D());
}

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

void UUOUUmbrellaComponent::DrawScreenDebug() const
{
	if (!bShowScreenDebug || GEngine == nullptr)
	{
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	const UEnum* UmbrellaStateEnum = StaticEnum<EUOUUmbrellaState>();
	const FString StateText = UmbrellaStateEnum != nullptr
		? UmbrellaStateEnum->GetNameStringByValue(static_cast<int64>(CurrentState))
		: TEXT("Unknown");

	const FString DebugText = FString::Printf(
		TEXT("Umbrella Owned: %s\nState: %s\nStored Water: %.2f\nRain Exposure: %.2f\nBlocks Jump: %s\nLast Pour Target: %s"),
		bHasUmbrella ? TEXT("Yes") : TEXT("No"),
		*StateText,
		GetCurrentStoredWater(),
		GetCurrentPlayerRainAmount(),
		BlocksJumping() ? TEXT("Yes") : TEXT("No"),
		*LastPourTargetName);

	GEngine->AddOnScreenDebugMessage(
		0x554F5531,
		0.0f,
		FColor::Cyan,
		DebugText,
		false,
		FVector2D(1.0f, 1.0f));
}

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
	AimRotation.Pitch = 0.0f;
	AimRotation.Roll = 0.0f;
	Owner->SetActorRotation(AimRotation);
}

void UUOUUmbrellaComponent::ClearPourAimFacing()
{
}

void UUOUUmbrellaComponent::UpdatePouring(float DeltaTime)
{
	if (CurrentState != EUOUUmbrellaState::Pouring || StoredWaterContainer == nullptr)
	{
		return;
	}

	const float PourAmount = PourRate * DeltaTime;
	StoredWaterContainer->RemoveAmount(PourAmount);

	FVector TraceStart = FVector::ZeroVector;
	FVector TraceDirection = FVector::ForwardVector;
	LastPourHitName = TEXT("No Hit");
	LastPourTargetName = TEXT("None");

	if (TryGetPourDirection(TraceStart, TraceDirection))
	{
		UWorld* World = GetWorld();
		AActor* Owner = GetOwner();
		if (World != nullptr && Owner != nullptr)
		{
			FHitResult HitResult;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UmbrellaPourTrace), false, Owner);
			QueryParams.AddIgnoredActor(Owner);

			const FVector TraceEnd = TraceStart + TraceDirection * PourDistance;
			if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, PourTraceChannel, QueryParams))
			{
				LastPourHitName = GetNameSafe(HitResult.GetComponent());
				TryReceiveWaterAtHit(HitResult, PourAmount);
			}
		}
	}
	else
	{
		LastPourHitName = TEXT("Invalid Ray");
	}

	if (GetCurrentStoredWater() <= 0.0f)
	{
		EndPour();
	}
}

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
		const FVector MouseAimDirection = AimPoint - PourOriginLocation;
		if (!MouseAimDirection.IsNearlyZero())
		{
			PourDirection = MouseAimDirection.GetSafeNormal();
		}
	}

	return !PourDirection.IsNearlyZero();
}

bool UUOUUmbrellaComponent::TryReceiveWaterAtHit(const FHitResult& HitResult, float WaterAmount)
{
	AActor* HitActor = HitResult.GetActor();
	if (HitActor == nullptr)
	{
		return false;
	}

	if (AUOUUmbrellaWaterTarget* WaterTargetActor = Cast<AUOUUmbrellaWaterTarget>(HitActor))
	{
		LastPourTargetName = HitActor->GetName();
		WaterTargetActor->ReceiveWater(WaterAmount);
		return true;
	}

	if (AUOUUmbrellaWaterTarget* ParentWaterTargetActor = HitActor->GetAttachParentActor() != nullptr ? Cast<AUOUUmbrellaWaterTarget>(HitActor->GetAttachParentActor()) : nullptr)
	{
		LastPourTargetName = ParentWaterTargetActor->GetName();
		ParentWaterTargetActor->ReceiveWater(WaterAmount);
		return true;
	}

	if (UUOUWaterContainerComponent* WaterTargetContainer = HitActor->FindComponentByClass<UUOUWaterContainerComponent>())
	{
		LastPourTargetName = HitActor->GetName();
		WaterTargetContainer->AddAmount(WaterAmount);
		return true;
	}

	return false;
}

bool UUOUUmbrellaComponent::ShouldSpillStoredWater(EUOUUmbrellaState PreviousState, EUOUUmbrellaState NextState) const
{
	const bool bWasHoldingWater = PreviousState == EUOUUmbrellaState::UpsideDown || PreviousState == EUOUUmbrellaState::Pouring;
	const bool bWillNotHoldWater = NextState == EUOUUmbrellaState::Open || NextState == EUOUUmbrellaState::Closed;
	return bWasHoldingWater && bWillNotHoldWater && GetCurrentStoredWater() > 0.0f;
}

void UUOUUmbrellaComponent::SpillStoredWater()
{
	if (StoredWaterContainer != nullptr)
	{
		StoredWaterContainer->SetAmount(0.0f);
	}
}
