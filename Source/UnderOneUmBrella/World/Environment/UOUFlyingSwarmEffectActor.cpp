// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUFlyingSwarmEffectActor.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

AUOUFlyingSwarmEffectActor::AUOUFlyingSwarmEffectActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	SwarmEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SwarmEffect"));
	SwarmEffect->SetupAttachment(RootScene);
	SwarmEffect->SetAutoActivate(false);
}

void AUOUFlyingSwarmEffectActor::BeginPlay()
{
	Super::BeginPlay();

	RuntimeRandomSeed = RandomSeed;
	ApplyEffectSystem();
	ApplyNiagaraParameters();

	if (bActivateOnBeginPlay)
	{
		ActivateEffect();
	}
}

void AUOUFlyingSwarmEffectActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	PlaneCount = FMath::Max(0, PlaneCount);
	OrbitRadius = FMath::Max(0.0f, OrbitRadius);
	OrbitHeight = FMath::Max(0.0f, OrbitHeight);
	TravelDuration = FMath::Max(UE_KINDA_SMALL_NUMBER, TravelDuration);
	HoldDuration = FMath::Max(0.0f, HoldDuration);
	FadeOutDuration = FMath::Max(0.0f, FadeOutDuration);

	RuntimeRandomSeed = RandomSeed;
	ApplyEffectSystem();
	ApplyNiagaraParameters();
}

void AUOUFlyingSwarmEffectActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsEffectActive)
	{
		SetActorTickEnabled(false);
		return;
	}

	EffectElapsedTime += DeltaSeconds;
	ApplyNiagaraParameters();
	DrawRuntimeDebug();
}

void AUOUFlyingSwarmEffectActor::ActivateEffect()
{
	ApplyEffectSystem();

	if (SwarmEffect == nullptr || SwarmEffect->GetAsset() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UOU Flying Swarm Effect '%s' has no Niagara system."), *GetNameSafe(this));
		return;
	}

	if (!bUseFixedRandomSeed && bAdvanceRandomSeedOnActivate)
	{
		++RuntimeRandomSeed;
	}
	else
	{
		RuntimeRandomSeed = RandomSeed;
	}

	bIsEffectActive = true;
	EffectElapsedTime = 0.0f;
	SetActorTickEnabled(true);

	SwarmEffect->SetVisibility(true, true);
	ApplyNiagaraParameters();
	SwarmEffect->Activate(bResetSystemOnActivate);

	OnEffectStarted.Broadcast(this);
}

void AUOUFlyingSwarmEffectActor::DeactivateEffect()
{
	if (!bIsEffectActive && SwarmEffect != nullptr && !SwarmEffect->IsActive())
	{
		return;
	}

	bIsEffectActive = false;
	ApplyNiagaraParameters();

	if (SwarmEffect != nullptr)
	{
		SwarmEffect->Deactivate();
		if (bHideEffectWhenInactive)
		{
			SwarmEffect->SetVisibility(false, true);
		}
	}

	SetActorTickEnabled(false);
	OnEffectStopped.Broadcast(this);
}

void AUOUFlyingSwarmEffectActor::RestartEffect()
{
	DeactivateEffect();
	ActivateEffect();
}

void AUOUFlyingSwarmEffectActor::SetEffectActive(bool bNewActive)
{
	if (bNewActive)
	{
		ActivateEffect();
		return;
	}

	DeactivateEffect();
}

bool AUOUFlyingSwarmEffectActor::IsEffectActive() const
{
	return bIsEffectActive;
}

void AUOUFlyingSwarmEffectActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		ActivateEffect();
		break;

	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		DeactivateEffect();
		break;

	case EOUUPuzzleResultAction::Toggle:
		SetEffectActive(!bIsEffectActive);
		break;

	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

TArray<FString> AUOUFlyingSwarmEffectActor::GetPuzzleDebugInfo_Implementation() const
{
	const FTransform SourceTransform = ResolveSourceTransform();
	const FTransform TargetTransform = ResolveTargetTransform();
	const bool bHasSystem = SwarmEffect != nullptr && SwarmEffect->GetAsset() != nullptr;

	return {
		FString::Printf(TEXT("Flying Swarm: %s"), bIsEffectActive ? TEXT("Active") : TEXT("Inactive")),
		FString::Printf(TEXT("Niagara System: %s"), bHasSystem ? TEXT("Yes") : TEXT("No")),
		FString::Printf(TEXT("Planes: %d"), PlaneCount),
		FString::Printf(TEXT("Age: %.2f"), EffectElapsedTime),
		FString::Printf(TEXT("Source: %.0f %.0f %.0f"), SourceTransform.GetLocation().X, SourceTransform.GetLocation().Y, SourceTransform.GetLocation().Z),
		FString::Printf(TEXT("Target: %.0f %.0f %.0f"), TargetTransform.GetLocation().X, TargetTransform.GetLocation().Y, TargetTransform.GetLocation().Z)
	};
}

#if WITH_EDITOR
bool AUOUFlyingSwarmEffectActor::ShouldTickIfViewportsOnly() const
{
	return bIsEffectActive;
}
#endif

void AUOUFlyingSwarmEffectActor::ApplyEffectSystem()
{
	if (SwarmEffect == nullptr)
	{
		return;
	}

	if (SwarmEffectSystem != nullptr)
	{
		if (SwarmEffect->GetAsset() != SwarmEffectSystem)
		{
			SwarmEffect->SetAsset(SwarmEffectSystem);
		}
		return;
	}

	SwarmEffectSystem = SwarmEffect->GetAsset();
}

void AUOUFlyingSwarmEffectActor::ApplyNiagaraParameters()
{
	if (SwarmEffect == nullptr)
	{
		return;
	}

	const FTransform SourceTransform = ResolveSourceTransform();
	const FTransform TargetTransform = ResolveTargetTransform();
	const FVector SourceForward = GetSafeTransformAxis(SourceTransform, EAxis::X, GetActorForwardVector());
	const FVector SourceRight = GetSafeTransformAxis(SourceTransform, EAxis::Y, GetActorRightVector());
	const FVector TargetForward = GetSafeTransformAxis(TargetTransform, EAxis::X, FVector::ForwardVector);
	const FVector TargetRight = GetSafeTransformAxis(TargetTransform, EAxis::Y, FVector::RightVector);
	const FVector TargetUp = GetSafeTransformAxis(TargetTransform, EAxis::Z, FVector::UpVector);

	if (!SourcePositionParameterName.IsNone())
	{
		SwarmEffect->SetVariableVec3(SourcePositionParameterName, SourceTransform.GetLocation());
	}
	if (!SourceForwardParameterName.IsNone())
	{
		SwarmEffect->SetVariableVec3(SourceForwardParameterName, SourceForward);
	}
	if (!SourceRightParameterName.IsNone())
	{
		SwarmEffect->SetVariableVec3(SourceRightParameterName, SourceRight);
	}
	if (!TargetPositionParameterName.IsNone())
	{
		SwarmEffect->SetVariableVec3(TargetPositionParameterName, TargetTransform.GetLocation());
	}
	if (!TargetForwardParameterName.IsNone())
	{
		SwarmEffect->SetVariableVec3(TargetForwardParameterName, TargetForward);
	}
	if (!TargetRightParameterName.IsNone())
	{
		SwarmEffect->SetVariableVec3(TargetRightParameterName, TargetRight);
	}
	if (!TargetUpParameterName.IsNone())
	{
		SwarmEffect->SetVariableVec3(TargetUpParameterName, TargetUp);
	}
	if (!PlaneCountParameterName.IsNone())
	{
		SwarmEffect->SetVariableInt(PlaneCountParameterName, FMath::Max(0, PlaneCount));
	}
	if (!OrbitRadiusParameterName.IsNone())
	{
		SwarmEffect->SetVariableFloat(OrbitRadiusParameterName, FMath::Max(0.0f, OrbitRadius));
	}
	if (!OrbitHeightParameterName.IsNone())
	{
		SwarmEffect->SetVariableFloat(OrbitHeightParameterName, FMath::Max(0.0f, OrbitHeight));
	}
	if (!TravelDurationParameterName.IsNone())
	{
		SwarmEffect->SetVariableFloat(TravelDurationParameterName, FMath::Max(UE_KINDA_SMALL_NUMBER, TravelDuration));
	}
	if (!HoldDurationParameterName.IsNone())
	{
		SwarmEffect->SetVariableFloat(HoldDurationParameterName, FMath::Max(0.0f, HoldDuration));
	}
	if (!FadeOutDurationParameterName.IsNone())
	{
		SwarmEffect->SetVariableFloat(FadeOutDurationParameterName, FMath::Max(0.0f, FadeOutDuration));
	}
	if (!EffectAgeParameterName.IsNone())
	{
		SwarmEffect->SetVariableFloat(EffectAgeParameterName, EffectElapsedTime);
	}
	if (!EffectActiveParameterName.IsNone())
	{
		SwarmEffect->SetVariableBool(EffectActiveParameterName, bIsEffectActive);
	}
	if (!RandomSeedParameterName.IsNone())
	{
		SwarmEffect->SetVariableInt(RandomSeedParameterName, RuntimeRandomSeed);
	}
}

void AUOUFlyingSwarmEffectActor::DrawRuntimeDebug() const
{
	if (!bDrawRuntimeDebug || GetWorld() == nullptr)
	{
		return;
	}

	const FTransform SourceTransform = ResolveSourceTransform();
	const FTransform TargetTransform = ResolveTargetTransform();
	const FVector SourceLocation = SourceTransform.GetLocation();
	const FVector TargetLocation = TargetTransform.GetLocation();
	const float Thickness = FMath::Max(0.0f, RuntimeDebugThickness);

	DrawDebugSphere(GetWorld(), SourceLocation, 24.0f, 12, FColor::Cyan, false, 0.0f, 0, Thickness);
	DrawDebugSphere(GetWorld(), TargetLocation, 28.0f, 16, FColor::Yellow, false, 0.0f, 0, Thickness);
	DrawDebugLine(GetWorld(), SourceLocation, TargetLocation, FColor::Cyan, false, 0.0f, 0, Thickness);
	DrawDebugCircle(
		GetWorld(),
		TargetLocation,
		FMath::Max(0.0f, OrbitRadius),
		32,
		FColor::Yellow,
		false,
		0.0f,
		0,
		Thickness,
		GetSafeTransformAxis(TargetTransform, EAxis::X, FVector::ForwardVector),
		GetSafeTransformAxis(TargetTransform, EAxis::Y, FVector::RightVector),
		false);
	DrawDebugLine(
		GetWorld(),
		TargetLocation - FVector::UpVector * OrbitHeight * 0.5f,
		TargetLocation + FVector::UpVector * OrbitHeight * 0.5f,
		FColor::Yellow,
		false,
		0.0f,
		0,
		Thickness);
}

FTransform AUOUFlyingSwarmEffectActor::ResolveSourceTransform() const
{
	const AActor* ResolvedSourceActor = IsValid(SourceActor) ? SourceActor.Get() : this;
	return ResolveReferenceTransform(ResolvedSourceActor, SourceSocketName, SourceLocalOffset);
}

FTransform AUOUFlyingSwarmEffectActor::ResolveTargetTransform() const
{
	const AActor* ResolvedTargetActor = ResolveTargetActor();
	return ResolveReferenceTransform(ResolvedTargetActor != nullptr ? ResolvedTargetActor : this, TargetSocketName, TargetLocalOffset);
}

FTransform AUOUFlyingSwarmEffectActor::ResolveReferenceTransform(const AActor* ReferenceActor, FName SocketName, const FVector& LocalOffset) const
{
	if (!IsValid(ReferenceActor))
	{
		return GetActorTransform();
	}

	FTransform ReferenceTransform = ReferenceActor->GetActorTransform();
	const USceneComponent* ReferenceRootComponent = ReferenceActor->GetRootComponent();
	if (ReferenceRootComponent != nullptr && !SocketName.IsNone() && ReferenceRootComponent->DoesSocketExist(SocketName))
	{
		ReferenceTransform = ReferenceRootComponent->GetSocketTransform(SocketName, RTS_World);
	}

	ReferenceTransform.SetLocation(ReferenceTransform.TransformPosition(LocalOffset));
	return ReferenceTransform;
}

AActor* AUOUFlyingSwarmEffectActor::ResolveTargetActor() const
{
	if (IsValid(TargetActor))
	{
		return TargetActor.Get();
	}

	if (!bUsePlayerPawnWhenTargetMissing)
	{
		return const_cast<AUOUFlyingSwarmEffectActor*>(this);
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return const_cast<AUOUFlyingSwarmEffectActor*>(this);
	}

	const APlayerController* PlayerController = World->GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	return PlayerPawn != nullptr ? static_cast<AActor*>(PlayerPawn) : const_cast<AUOUFlyingSwarmEffectActor*>(this);
}

FVector AUOUFlyingSwarmEffectActor::GetSafeTransformAxis(const FTransform& Transform, EAxis::Type Axis, const FVector& Fallback)
{
	const FVector AxisVector = Transform.GetUnitAxis(Axis).GetSafeNormal();
	if (!AxisVector.IsNearlyZero())
	{
		return AxisVector;
	}

	const FVector FallbackVector = Fallback.GetSafeNormal();
	return FallbackVector.IsNearlyZero() ? FVector::ForwardVector : FallbackVector;
}
