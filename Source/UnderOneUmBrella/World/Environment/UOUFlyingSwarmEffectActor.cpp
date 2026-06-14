// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUFlyingSwarmEffectActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr int32 MaxPaperPlaneSwarmFlightPatternCount = 4;
constexpr TCHAR DefaultPaperPlaneMeshPath[] = TEXT("/Engine/BasicShapes/Cone.Cone");

FName NormalizeNiagaraUserParameterName(FName ParameterName)
{
	if (ParameterName.IsNone())
	{
		return NAME_None;
	}

	FString ParameterNameString = ParameterName.ToString();
	if (ParameterNameString.StartsWith(TEXT("User.")))
	{
		ParameterNameString.RightChopInline(5);
		return FName(*ParameterNameString);
	}

	return ParameterName;
}

float SmoothStep01(float Value)
{
	const float T = FMath::Clamp(Value, 0.0f, 1.0f);
	return T * T * (3.0f - 2.0f * T);
}

FVector GetSafeDirection(const FVector& Direction, const FVector& Fallback)
{
	const FVector NormalizedDirection = Direction.GetSafeNormal();
	if (!NormalizedDirection.IsNearlyZero())
	{
		return NormalizedDirection;
	}

	const FVector NormalizedFallback = Fallback.GetSafeNormal();
	return NormalizedFallback.IsNearlyZero() ? FVector::ForwardVector : NormalizedFallback;
}

FVector SolveQuadraticBezier(const FVector& P0, const FVector& P1, const FVector& P2, float T)
{
	const float OneMinusT = 1.0f - T;
	return P0 * OneMinusT * OneMinusT
		+ P1 * 2.0f * OneMinusT * T
		+ P2 * T * T;
}

float RandomRangeFloat(FRandomStream& RandomStream, float MinValue, float MaxValue)
{
	const float RangeMin = FMath::Min(MinValue, MaxValue);
	const float RangeMax = FMath::Max(MinValue, MaxValue);
	return RandomStream.FRandRange(RangeMin, RangeMax);
}

int32 MakeParticleRandomSeed(int32 ParticleIndex, int32 RandomSeed)
{
	const uint32 SeedA = GetTypeHash(RandomSeed);
	const uint32 SeedB = GetTypeHash(FMath::Max(0, ParticleIndex));
	return static_cast<int32>(HashCombine(SeedA, SeedB));
}

FVector RandomRangeVector(FRandomStream& RandomStream, const FVector& MinValue, const FVector& MaxValue)
{
	return FVector(
		RandomRangeFloat(RandomStream, MinValue.X, MaxValue.X),
		RandomRangeFloat(RandomStream, MinValue.Y, MaxValue.Y),
		RandomRangeFloat(RandomStream, MinValue.Z, MaxValue.Z));
}
}

AUOUFlyingSwarmEffectActor::AUOUFlyingSwarmEffectActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	SwarmEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SwarmEffect"));
	SwarmEffect->SetupAttachment(RootScene);
	SwarmEffect->SetAutoActivate(false);

	PaperPlaneInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PaperPlaneInstances"));
	PaperPlaneInstances->SetupAttachment(RootScene);
	PaperPlaneInstances->SetMobility(EComponentMobility::Movable);
	PaperPlaneInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PaperPlaneInstances->SetCanEverAffectNavigation(false);
	PaperPlaneInstances->SetVisibility(false, true);
	PaperPlaneInstances->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PaperPlaneMeshFinder(DefaultPaperPlaneMeshPath);
	if (PaperPlaneMeshFinder.Succeeded())
	{
		PaperPlaneMesh = PaperPlaneMeshFinder.Object;
		PaperPlaneInstances->SetStaticMesh(PaperPlaneMesh);
	}
}

void AUOUFlyingSwarmEffectActor::BeginPlay()
{
	Super::BeginPlay();

	RuntimeRandomSeed = RandomSeed;
	ApplyEffectSystem();
	ApplyRenderMode();
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
	WrapRadius = FMath::Max(0.0f, WrapRadius);
	WrapHeight = FMath::Max(0.0f, WrapHeight);
	OrbitSpeed = FMath::Max(0.0f, OrbitSpeed);
	WobbleRightAmount = FMath::Max(0.0f, WobbleRightAmount);
	WobbleUpAmount = FMath::Max(0.0f, WobbleUpAmount);
	FlightPatternCount = FMath::Clamp(FlightPatternCount, 1, MaxPaperPlaneSwarmFlightPatternCount);
	GlideSwoopAmount = FMath::Max(0.0f, GlideSwoopAmount);
	GlideSwoopHeight = FMath::Max(0.0f, GlideSwoopHeight);
	GlideSideAmount = FMath::Max(0.0f, GlideSideAmount);
	GlideOvershootAmount = FMath::Max(0.0f, GlideOvershootAmount);
	FarReachAlpha = FMath::Clamp(FarReachAlpha, 0.05f, 0.95f);
	FlightDuration = FMath::Max(UE_KINDA_SMALL_NUMBER, FlightDuration);
	WrapStartTime = FMath::Max(0.0f, WrapStartTime);
	WrapDuration = FMath::Max(UE_KINDA_SMALL_NUMBER, WrapDuration);

	RuntimeRandomSeed = RandomSeed;
	ApplyEffectSystem();
	ApplyRenderMode();
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
	if (RenderMode == EUOUPaperPlaneSwarmRenderMode::CodeDrivenMesh)
	{
		UpdateCodeDrivenPlaneInstances(DeltaSeconds);
	}
	else
	{
		ApplyNiagaraParameters();
	}
	DrawRuntimeDebug();
}

void AUOUFlyingSwarmEffectActor::ActivateEffect()
{
	ApplyEffectSystem();
	ApplyRenderMode();

	if (RenderMode == EUOUPaperPlaneSwarmRenderMode::Niagara && (SwarmEffect == nullptr || SwarmEffect->GetAsset() == nullptr))
	{
		UE_LOG(LogTemp, Warning, TEXT("UOU Paper Plane Swarm Effect '%s' has no Niagara system."), *GetNameSafe(this));
		return;
	}

	if (RenderMode == EUOUPaperPlaneSwarmRenderMode::CodeDrivenMesh && (PaperPlaneInstances == nullptr || !EnsurePaperPlaneMesh()))
	{
		UE_LOG(LogTemp, Warning, TEXT("UOU Paper Plane Swarm Effect '%s' has no code-driven static mesh. Expected: %s"), *GetNameSafe(this), DefaultPaperPlaneMeshPath);
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
	ActiveStartTransform = ResolveSourceTransform();
	ActiveTargetTransform = ResolveTargetTransform();
	SetActorTickEnabled(true);

	if (RenderMode == EUOUPaperPlaneSwarmRenderMode::CodeDrivenMesh)
	{
		RebuildCodeDrivenPlaneInstances();
		UpdateCodeDrivenPlaneInstances(1.0f / 60.0f);
		UE_LOG(LogTemp, Log, TEXT("UOU Paper Plane Swarm Effect '%s' activated in CodeDrivenMesh mode. Planes=%d Mesh=%s"),
			*GetNameSafe(this),
			RuntimePlaneMeshComponents.Num(),
			*GetNameSafe(PaperPlaneMesh));
	}
	else
	{
		SwarmEffect->SetVisibility(true, true);
		ApplyNiagaraParameters();
		SwarmEffect->Activate(bResetSystemOnActivate);
	}

	OnEffectStarted.Broadcast(this);
}

void AUOUFlyingSwarmEffectActor::DeactivateEffect()
{
	const bool bHasActiveNiagara = SwarmEffect != nullptr && SwarmEffect->IsActive();
	const bool bHasCodeInstances = RuntimePlaneMeshComponents.Num() > 0 || (PaperPlaneInstances != nullptr && PaperPlaneInstances->GetInstanceCount() > 0);
	if (!bIsEffectActive && !bHasActiveNiagara && !bHasCodeInstances)
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

	if (PaperPlaneInstances != nullptr)
	{
		PaperPlaneInstances->ClearInstances();
		PaperPlaneInstances->SetVisibility(false, true);
		PaperPlaneInstances->SetHiddenInGame(true);
	}
	ClearCodeDrivenPlaneComponents();
	RuntimeParticles.Reset();
	RuntimePreviousPositions.Reset();

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
	const FTransform SourceTransform = GetCurrentStartTransform();
	const FTransform TargetTransform = GetCurrentTargetTransform();
	const bool bHasSystem = SwarmEffect != nullptr && SwarmEffect->GetAsset() != nullptr;

	return {
		FString::Printf(TEXT("Paper Plane Swarm: %s"), bIsEffectActive ? TEXT("Active") : TEXT("Inactive")),
		FString::Printf(TEXT("Render Mode: %s"), RenderMode == EUOUPaperPlaneSwarmRenderMode::CodeDrivenMesh ? TEXT("Code Driven Mesh") : TEXT("Niagara")),
		FString::Printf(TEXT("Niagara System: %s"), bHasSystem ? TEXT("Yes") : TEXT("No")),
		FString::Printf(TEXT("Planes: %d"), PlaneCount),
		FString::Printf(TEXT("Time: %.2f / Flight %.2f / Wrap %.2f"), EffectElapsedTime, GetFlightAlpha(), GetWrapAlpha()),
		FString::Printf(TEXT("Source: %.0f %.0f %.0f"), SourceTransform.GetLocation().X, SourceTransform.GetLocation().Y, SourceTransform.GetLocation().Z),
		FString::Printf(TEXT("Target: %.0f %.0f %.0f"), TargetTransform.GetLocation().X, TargetTransform.GetLocation().Y, TargetTransform.GetLocation().Z),
		FString::Printf(TEXT("Follow Target: %s"), bFollowTarget ? TEXT("Yes") : TEXT("No"))
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

	const FTransform StartTransform = GetCurrentStartTransform();
	const FTransform TargetTransform = GetCurrentTargetTransform();
	const FVector SourceForward = GetSafeTransformAxis(StartTransform, EAxis::X, GetActorForwardVector());
	const FVector SourceRight = GetSafeTransformAxis(StartTransform, EAxis::Y, GetActorRightVector());
	const FVector TargetForward = GetSafeTransformAxis(TargetTransform, EAxis::X, FVector::ForwardVector);
	const FVector TargetRight = GetSafeTransformAxis(TargetTransform, EAxis::Y, FVector::RightVector);
	const FVector TargetUp = GetSafeTransformAxis(TargetTransform, EAxis::Z, FVector::UpVector);
	const float FlightAlpha = GetFlightAlpha();
	const float WrapAlpha = GetWrapAlpha();

	auto SetVec3Parameter = [this](FName ParameterName, const FVector& Value)
	{
		ParameterName = NormalizeNiagaraUserParameterName(ParameterName);
		if (!ParameterName.IsNone())
		{
			SwarmEffect->SetVariableVec3(ParameterName, Value);
		}
	};

	auto SetFloatParameter = [this](FName ParameterName, float Value)
	{
		ParameterName = NormalizeNiagaraUserParameterName(ParameterName);
		if (!ParameterName.IsNone())
		{
			SwarmEffect->SetVariableFloat(ParameterName, Value);
		}
	};

	auto SetIntParameter = [this](FName ParameterName, int32 Value)
	{
		ParameterName = NormalizeNiagaraUserParameterName(ParameterName);
		if (!ParameterName.IsNone())
		{
			SwarmEffect->SetVariableInt(ParameterName, Value);
		}
	};

	auto SetBoolParameter = [this](FName ParameterName, bool bValue)
	{
		ParameterName = NormalizeNiagaraUserParameterName(ParameterName);
		if (!ParameterName.IsNone())
		{
			SwarmEffect->SetVariableBool(ParameterName, bValue);
		}
	};

	SetVec3Parameter(StartPositionParameterName, StartTransform.GetLocation());
	SetVec3Parameter(TargetPositionParameterName, TargetTransform.GetLocation());
	SetFloatParameter(FlightAlphaParameterName, FlightAlpha);
	SetFloatParameter(WrapAlphaParameterName, WrapAlpha);
	SetFloatParameter(TimeParameterName, EffectElapsedTime);
	SetFloatParameter(WrapRadiusParameterName, FMath::Max(0.0f, WrapRadius));
	SetFloatParameter(WrapHeightParameterName, FMath::Max(0.0f, WrapHeight));
	SetFloatParameter(OrbitSpeedParameterName, FMath::Max(0.0f, OrbitSpeed));
	SetFloatParameter(WobbleRightAmountParameterName, FMath::Max(0.0f, WobbleRightAmount));
	SetFloatParameter(WobbleUpAmountParameterName, FMath::Max(0.0f, WobbleUpAmount));
	SetIntParameter(FlightPatternCountParameterName, FMath::Clamp(FlightPatternCount, 1, MaxPaperPlaneSwarmFlightPatternCount));
	SetFloatParameter(GlideSwoopAmountParameterName, FMath::Max(0.0f, GlideSwoopAmount));
	SetFloatParameter(GlideSwoopHeightParameterName, FMath::Max(0.0f, GlideSwoopHeight));
	SetFloatParameter(GlideSideAmountParameterName, FMath::Max(0.0f, GlideSideAmount));
	SetFloatParameter(GlideOvershootAmountParameterName, FMath::Max(0.0f, GlideOvershootAmount));
	SetIntParameter(PlaneCountParameterName, FMath::Max(0, PlaneCount));
	SetVec3Parameter(TargetForwardParameterName, TargetForward);
	SetVec3Parameter(TargetRightParameterName, TargetRight);
	SetVec3Parameter(TargetUpParameterName, TargetUp);
	SetIntParameter(RandomSeedParameterName, RuntimeRandomSeed);

	SetVec3Parameter(LegacySourcePositionParameterName, StartTransform.GetLocation());
	SetVec3Parameter(LegacySourceForwardParameterName, SourceForward);
	SetVec3Parameter(LegacySourceRightParameterName, SourceRight);
	SetFloatParameter(LegacyOrbitRadiusParameterName, FMath::Max(0.0f, WrapRadius));
	SetFloatParameter(LegacyOrbitHeightParameterName, FMath::Max(0.0f, WrapHeight));
	SetFloatParameter(LegacyEffectAgeParameterName, EffectElapsedTime);
	SetFloatParameter(LegacyTravelDurationParameterName, FMath::Max(UE_KINDA_SMALL_NUMBER, FlightDuration));
	SetFloatParameter(LegacyHoldDurationParameterName, 86400.0f);
	SetFloatParameter(LegacyFadeOutDurationParameterName, 0.6f);
	SetBoolParameter(LegacyEffectActiveParameterName, bIsEffectActive);
}

void AUOUFlyingSwarmEffectActor::ApplyRenderMode()
{
	const bool bUseCodeDrivenMesh = RenderMode == EUOUPaperPlaneSwarmRenderMode::CodeDrivenMesh;

	if (PaperPlaneInstances != nullptr)
	{
		if (bUseCodeDrivenMesh)
		{
			EnsurePaperPlaneMesh();
		}

		if (PaperPlaneMesh != nullptr && PaperPlaneInstances->GetStaticMesh() != PaperPlaneMesh)
		{
			PaperPlaneInstances->SetStaticMesh(PaperPlaneMesh);
		}

		const bool bShowCodeInstances = false;
		PaperPlaneInstances->SetVisibility(bShowCodeInstances, true);
		PaperPlaneInstances->SetHiddenInGame(!bShowCodeInstances);

		if (!bUseCodeDrivenMesh)
		{
			PaperPlaneInstances->ClearInstances();
			ClearCodeDrivenPlaneComponents();
			RuntimeParticles.Reset();
			RuntimePreviousPositions.Reset();
		}
	}

	const bool bShowRuntimeComponents = bUseCodeDrivenMesh && bIsEffectActive;
	for (UStaticMeshComponent* PlaneComponent : RuntimePlaneMeshComponents)
	{
		if (IsValid(PlaneComponent))
		{
			PlaneComponent->SetVisibility(bShowRuntimeComponents, true);
			PlaneComponent->SetHiddenInGame(!bShowRuntimeComponents);
		}
	}

	if (SwarmEffect != nullptr)
	{
		if (bUseCodeDrivenMesh)
		{
			SwarmEffect->Deactivate();
			SwarmEffect->SetVisibility(false, true);
			SwarmEffect->SetHiddenInGame(true);
		}
		else
		{
			SwarmEffect->SetHiddenInGame(false);
			if (!bHideEffectWhenInactive || bIsEffectActive)
			{
				SwarmEffect->SetVisibility(true, true);
			}
		}
	}
}

bool AUOUFlyingSwarmEffectActor::EnsurePaperPlaneMesh()
{
	if (bUseDefaultConeMesh || PaperPlaneMesh == nullptr)
	{
		PaperPlaneMesh = LoadObject<UStaticMesh>(nullptr, DefaultPaperPlaneMeshPath);
	}

	if (PaperPlaneInstances != nullptr && PaperPlaneMesh != nullptr && PaperPlaneInstances->GetStaticMesh() != PaperPlaneMesh)
	{
		PaperPlaneInstances->SetStaticMesh(PaperPlaneMesh);
	}

	return PaperPlaneMesh != nullptr;
}

void AUOUFlyingSwarmEffectActor::ClearCodeDrivenPlaneComponents()
{
	if (PaperPlaneInstances != nullptr)
	{
		PaperPlaneInstances->ClearInstances();
		PaperPlaneInstances->SetVisibility(false, true);
		PaperPlaneInstances->SetHiddenInGame(true);
	}

	for (UStaticMeshComponent* PlaneComponent : RuntimePlaneMeshComponents)
	{
		if (IsValid(PlaneComponent))
		{
			PlaneComponent->DestroyComponent();
		}
	}
	RuntimePlaneMeshComponents.Reset();
}

FUOUPaperPlaneSwarmRandomRanges AUOUFlyingSwarmEffectActor::BuildRandomRanges() const
{
	FUOUPaperPlaneSwarmRandomRanges RandomRanges;
	RandomRanges.FlightPatternCount = FMath::Clamp(FlightPatternCount, 1, MaxPaperPlaneSwarmFlightPatternCount);
	RandomRanges.FarPointMin = FarPointMin;
	RandomRanges.FarPointMax = FarPointMax;
	return RandomRanges;
}

FUOUPaperPlaneSwarmMotionInput AUOUFlyingSwarmEffectActor::BuildMotionInput(float DeltaSeconds) const
{
	const FTransform StartTransform = GetCurrentStartTransform();
	const FTransform TargetTransform = GetCurrentTargetTransform();

	FUOUPaperPlaneSwarmMotionInput MotionInput;
	MotionInput.StartPosition = StartTransform.GetLocation();
	MotionInput.TargetPosition = TargetTransform.GetLocation();
	MotionInput.FlightAlpha = GetFlightAlpha();
	MotionInput.WrapAlpha = GetWrapAlpha();
	MotionInput.Time = EffectElapsedTime;
	MotionInput.WrapRadius = FMath::Max(0.0f, WrapRadius);
	MotionInput.WrapHeight = FMath::Max(0.0f, WrapHeight);
	MotionInput.OrbitSpeed = FMath::Max(0.0f, OrbitSpeed);
	MotionInput.WobbleRightAmount = FMath::Max(0.0f, WobbleRightAmount);
	MotionInput.WobbleUpAmount = FMath::Max(0.0f, WobbleUpAmount);
	MotionInput.FlightPatternCount = FMath::Clamp(FlightPatternCount, 1, MaxPaperPlaneSwarmFlightPatternCount);
	MotionInput.GlideSwoopAmount = FMath::Max(0.0f, GlideSwoopAmount);
	MotionInput.GlideSwoopHeight = FMath::Max(0.0f, GlideSwoopHeight);
	MotionInput.GlideSideAmount = FMath::Max(0.0f, GlideSideAmount);
	MotionInput.GlideOvershootAmount = FMath::Max(0.0f, GlideOvershootAmount);
	MotionInput.FarReachAlpha = FMath::Clamp(FarReachAlpha, 0.05f, 0.95f);
	MotionInput.TargetForward = GetSafeTransformAxis(TargetTransform, EAxis::X, FVector::ForwardVector);
	MotionInput.TargetRight = GetSafeTransformAxis(TargetTransform, EAxis::Y, FVector::RightVector);
	MotionInput.TargetUp = GetSafeTransformAxis(TargetTransform, EAxis::Z, FVector::UpVector);
	MotionInput.DeltaTime = FMath::Max(DeltaSeconds, 0.0001f);
	return MotionInput;
}

void AUOUFlyingSwarmEffectActor::RebuildCodeDrivenPlaneInstances()
{
	if (PaperPlaneInstances == nullptr)
	{
		return;
	}

	EnsurePaperPlaneMesh();

	if (PaperPlaneMesh != nullptr && PaperPlaneInstances->GetStaticMesh() != PaperPlaneMesh)
	{
		PaperPlaneInstances->SetStaticMesh(PaperPlaneMesh);
	}

	ClearCodeDrivenPlaneComponents();
	RuntimeParticles.Reset();
	RuntimePreviousPositions.Reset();

	const int32 SafePlaneCount = FMath::Max(0, PlaneCount);
	if (SafePlaneCount <= 0 || PaperPlaneMesh == nullptr)
	{
		ApplyRenderMode();
		return;
	}

	RuntimeParticles.Reserve(SafePlaneCount);
	RuntimePreviousPositions.Reserve(SafePlaneCount);

	const FVector StartPosition = GetCurrentStartTransform().GetLocation();
	const FUOUPaperPlaneSwarmRandomRanges RandomRanges = BuildRandomRanges();
	const FVector SafeBaseScale(
		FMath::Max(0.001f, PaperPlaneMeshScale.X),
		FMath::Max(0.001f, PaperPlaneMeshScale.Y),
		FMath::Max(0.001f, PaperPlaneMeshScale.Z));

	for (int32 PlaneIndex = 0; PlaneIndex < SafePlaneCount; ++PlaneIndex)
	{
		FUOUPaperPlaneSwarmParticleRandom ParticleRandom = MakePaperPlaneSwarmParticleRandom(PlaneIndex, RuntimeRandomSeed, RandomRanges);
		RuntimeParticles.Add(ParticleRandom);
		RuntimePreviousPositions.Add(StartPosition);

		FTransform InitialTransform;
		InitialTransform.SetLocation(StartPosition);
		InitialTransform.SetRotation(PaperPlaneMeshRotationOffset.Quaternion());
		InitialTransform.SetScale3D(SafeBaseScale * ParticleRandom.ScaleRandom);

		UStaticMeshComponent* PlaneComponent = NewObject<UStaticMeshComponent>(this);
		if (PlaneComponent != nullptr)
		{
			PlaneComponent->SetupAttachment(RootScene);
			PlaneComponent->SetMobility(EComponentMobility::Movable);
			PlaneComponent->SetStaticMesh(PaperPlaneMesh);
			PlaneComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			PlaneComponent->SetGenerateOverlapEvents(false);
			PlaneComponent->SetCanEverAffectNavigation(false);
			PlaneComponent->SetReceivesDecals(false);
			PlaneComponent->SetVisibility(bIsEffectActive, true);
			PlaneComponent->SetHiddenInGame(!bIsEffectActive);
			PlaneComponent->RegisterComponent();
			PlaneComponent->SetWorldTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
			RuntimePlaneMeshComponents.Add(PlaneComponent);
		}
	}

	ApplyRenderMode();
}

void AUOUFlyingSwarmEffectActor::UpdateCodeDrivenPlaneInstances(float DeltaSeconds)
{
	if (PaperPlaneInstances == nullptr || !EnsurePaperPlaneMesh())
	{
		return;
	}

	const int32 SafePlaneCount = FMath::Max(0, PlaneCount);
	if (RuntimeParticles.Num() != SafePlaneCount || RuntimePreviousPositions.Num() != SafePlaneCount || RuntimePlaneMeshComponents.Num() != SafePlaneCount)
	{
		RebuildCodeDrivenPlaneInstances();
	}

	if (SafePlaneCount <= 0)
	{
		return;
	}

	const FVector SafeBaseScale(
		FMath::Max(0.001f, PaperPlaneMeshScale.X),
		FMath::Max(0.001f, PaperPlaneMeshScale.Y),
		FMath::Max(0.001f, PaperPlaneMeshScale.Z));
	FUOUPaperPlaneSwarmMotionInput MotionInput = BuildMotionInput(DeltaSeconds);

	for (int32 PlaneIndex = 0; PlaneIndex < SafePlaneCount; ++PlaneIndex)
	{
		MotionInput.PreviousPosition = RuntimePreviousPositions[PlaneIndex];
		const FUOUPaperPlaneSwarmMotionResult MotionResult = SolvePaperPlaneSwarmMotion(MotionInput, RuntimeParticles[PlaneIndex]);

		FTransform InstanceTransform;
		InstanceTransform.SetLocation(MotionResult.Position);
		InstanceTransform.SetRotation((MotionResult.Rotation.Quaternion() * PaperPlaneMeshRotationOffset.Quaternion()).GetNormalized());
		InstanceTransform.SetScale3D(SafeBaseScale * MotionResult.Scale);

		if (UStaticMeshComponent* PlaneComponent = RuntimePlaneMeshComponents.IsValidIndex(PlaneIndex) ? RuntimePlaneMeshComponents[PlaneIndex].Get() : nullptr)
		{
			PlaneComponent->SetWorldTransform(InstanceTransform, false, nullptr, ETeleportType::None);
		}
		RuntimePreviousPositions[PlaneIndex] = MotionResult.Position;
	}

	ApplyRenderMode();
}

void AUOUFlyingSwarmEffectActor::DrawRuntimeDebug() const
{
	if (!bDrawRuntimeDebug || GetWorld() == nullptr)
	{
		return;
	}

	const FTransform SourceTransform = GetCurrentStartTransform();
	const FTransform TargetTransform = GetCurrentTargetTransform();
	const FVector SourceLocation = SourceTransform.GetLocation();
	const FVector TargetLocation = TargetTransform.GetLocation();
	const FVector TargetForward = GetSafeTransformAxis(TargetTransform, EAxis::X, FVector::ForwardVector);
	const FVector TargetRight = GetSafeTransformAxis(TargetTransform, EAxis::Y, FVector::RightVector);
	const FVector TargetUp = GetSafeTransformAxis(TargetTransform, EAxis::Z, FVector::UpVector);
	const float Thickness = FMath::Max(0.0f, RuntimeDebugThickness);

	DrawDebugSphere(GetWorld(), SourceLocation, 24.0f, 12, FColor::Cyan, false, 0.0f, 0, Thickness);
	DrawDebugSphere(GetWorld(), TargetLocation, 28.0f, 16, FColor::Yellow, false, 0.0f, 0, Thickness);
	DrawDebugLine(GetWorld(), SourceLocation, TargetLocation, FColor::Cyan, false, 0.0f, 0, Thickness);
	DrawDebugCircle(
		GetWorld(),
		TargetLocation,
		FMath::Max(0.0f, WrapRadius),
		32,
		FColor::Yellow,
		false,
		0.0f,
		0,
		Thickness,
		TargetForward,
		TargetRight,
		false);
	DrawDebugCircle(
		GetWorld(),
		TargetLocation,
		FMath::Max(0.0f, WrapRadius),
		32,
		FColor::Yellow,
		false,
		0.0f,
		0,
		Thickness * 0.5f,
		TargetForward,
		TargetUp,
		false);
	DrawDebugCircle(
		GetWorld(),
		TargetLocation,
		FMath::Max(0.0f, WrapRadius),
		32,
		FColor::Yellow,
		false,
		0.0f,
		0,
		Thickness * 0.5f,
		TargetRight,
		TargetUp,
		false);
	DrawDebugLine(
		GetWorld(),
		TargetLocation - TargetUp * WrapHeight * 0.5f,
		TargetLocation + TargetUp * WrapHeight * 0.5f,
		FColor::Yellow,
		false,
		0.0f,
		0,
		Thickness);

	const int32 DebugSampleCount = FMath::Clamp(RuntimeDebugSampleCount, 0, FMath::Min(FMath::Max(0, PlaneCount), 24));
	if (DebugSampleCount <= 0)
	{
		return;
	}

	const int32 DebugPathSegments = FMath::Clamp(RuntimeDebugPathSegments, 1, 32);
	const float CurrentFlightAlpha = GetFlightAlpha();
	const float CurrentWrapAlpha = GetWrapAlpha();
	const FUOUPaperPlaneSwarmRandomRanges RandomRanges = BuildRandomRanges();

	FUOUPaperPlaneSwarmMotionInput BaseMotionInput = BuildMotionInput(1.0f / 60.0f);
	BaseMotionInput.StartPosition = SourceLocation;
	BaseMotionInput.TargetPosition = TargetLocation;
	BaseMotionInput.TargetForward = TargetForward;
	BaseMotionInput.TargetRight = TargetRight;
	BaseMotionInput.TargetUp = TargetUp;
	BaseMotionInput.DeltaTime = 1.0f / 60.0f;

	for (int32 SampleIndex = 0; SampleIndex < DebugSampleCount; ++SampleIndex)
	{
		const int32 ParticleIndex = DebugSampleCount > 1
			? FMath::RoundToInt(static_cast<float>(FMath::Max(0, PlaneCount - 1)) * static_cast<float>(SampleIndex) / static_cast<float>(DebugSampleCount - 1))
			: SampleIndex;
		const FUOUPaperPlaneSwarmParticleRandom ParticleRandom = MakePaperPlaneSwarmParticleRandom(ParticleIndex, RuntimeRandomSeed, RandomRanges);
		const FColor PathColor = SampleIndex % 2 == 0 ? FColor::Cyan : FColor::Green;

		FVector PreviousPathPosition = SourceLocation;
		FUOUPaperPlaneSwarmMotionResult PathResult;
		for (int32 SegmentIndex = 1; SegmentIndex <= DebugPathSegments; ++SegmentIndex)
		{
			FUOUPaperPlaneSwarmMotionInput PathInput = BaseMotionInput;
			PathInput.FlightAlpha = static_cast<float>(SegmentIndex) / static_cast<float>(DebugPathSegments);
			PathInput.WrapAlpha = 0.0f;
			PathInput.Time = FlightDuration * PathInput.FlightAlpha;
			PathInput.PreviousPosition = PreviousPathPosition;

			PathResult = SolvePaperPlaneSwarmMotion(PathInput, ParticleRandom);
			DrawDebugLine(GetWorld(), PreviousPathPosition, PathResult.Position, PathColor, false, 0.0f, 0, Thickness * 0.5f);
			PreviousPathPosition = PathResult.Position;
		}

		DrawDebugSphere(GetWorld(), PathResult.PreWrapPosition, 10.0f, 8, FColor::Green, false, 0.0f, 0, Thickness * 0.5f);
		DrawDebugSphere(GetWorld(), ParticleRandom.FarPoint, 14.0f, 8, FColor::Blue, false, 0.0f, 0, Thickness * 0.5f);

		FUOUPaperPlaneSwarmMotionInput CurrentInput = BaseMotionInput;
		CurrentInput.FlightAlpha = CurrentFlightAlpha;
		CurrentInput.WrapAlpha = CurrentWrapAlpha;
		CurrentInput.PreviousPosition = SourceLocation;

		const FUOUPaperPlaneSwarmMotionResult CurrentResult = SolvePaperPlaneSwarmMotion(CurrentInput, ParticleRandom);
		DrawDebugPoint(GetWorld(), CurrentResult.Position, 10.0f, FColor::White, false, 0.0f, 0);
		DrawDebugDirectionalArrow(
			GetWorld(),
			CurrentResult.Position,
			CurrentResult.Position + CurrentResult.ForwardDirection * 60.0f,
			18.0f,
			FColor::Orange,
			false,
			0.0f,
			0,
			Thickness * 0.75f);
	}
}

float AUOUFlyingSwarmEffectActor::GetFlightAlpha() const
{
	return CalculateFlightAlpha(EffectElapsedTime, FlightDuration);
}

float AUOUFlyingSwarmEffectActor::GetWrapAlpha() const
{
	return CalculateWrapAlpha(EffectElapsedTime, WrapStartTime, WrapDuration);
}

float AUOUFlyingSwarmEffectActor::CalculateFlightAlpha(float InElapsedTime, float InFlightDuration)
{
	return FMath::Clamp(InElapsedTime / FMath::Max(UE_KINDA_SMALL_NUMBER, InFlightDuration), 0.0f, 1.0f);
}

float AUOUFlyingSwarmEffectActor::CalculateWrapAlpha(float InElapsedTime, float InWrapStartTime, float InWrapDuration)
{
	const float RawWrapTime = InElapsedTime - FMath::Max(0.0f, InWrapStartTime);
	return FMath::Clamp(RawWrapTime / FMath::Max(UE_KINDA_SMALL_NUMBER, InWrapDuration), 0.0f, 1.0f);
}

FUOUPaperPlaneSwarmMotionResult AUOUFlyingSwarmEffectActor::SolvePaperPlaneSwarmMotion(const FUOUPaperPlaneSwarmMotionInput& MotionInput, const FUOUPaperPlaneSwarmParticleRandom& ParticleRandom)
{
	FUOUPaperPlaneSwarmMotionResult Result;

	const float RawFlightT = FMath::Clamp(MotionInput.FlightAlpha * FMath::Max(ParticleRandom.SwoopSpeed, 0.001f) - ParticleRandom.RandomDelay, 0.0f, 1.0f);
	const float RawWrapT = FMath::Clamp(MotionInput.WrapAlpha - ParticleRandom.RandomDelay, 0.0f, 1.0f);
	Result.FlightT = SmoothStep01(RawFlightT);
	Result.WrapT = SmoothStep01(RawWrapT);
	Result.Radius = FMath::Max(MotionInput.WrapRadius + ParticleRandom.RandomRadius, 50.0f);
	const int32 ActivePatternCount = FMath::Clamp(MotionInput.FlightPatternCount, 1, MaxPaperPlaneSwarmFlightPatternCount);
	Result.PatternIndex = FMath::Clamp(ParticleRandom.PatternIndex, 0, ActivePatternCount - 1);

	const FVector OrbitUp = GetSafeDirection(MotionInput.TargetUp, FVector::UpVector);
	const FVector TargetDirection = GetSafeDirection(MotionInput.TargetPosition - MotionInput.StartPosition, FVector::ForwardVector);
	const FVector OrbitForward = GetSafeDirection(MotionInput.TargetForward, TargetDirection);
	const FVector DerivedRight = GetSafeDirection(FVector::CrossProduct(OrbitUp, OrbitForward), FVector::RightVector);
	const FVector OrbitRight = GetSafeDirection(MotionInput.TargetRight, DerivedRight);

	float PhaseSin = 0.0f;
	float PhaseCos = 1.0f;
	FMath::SinCos(&PhaseSin, &PhaseCos, ParticleRandom.RandomPhase);
	float PatternSin = 0.0f;
	float PatternCos = 1.0f;
	FMath::SinCos(&PatternSin, &PatternCos, ParticleRandom.PatternPhase);

	const FVector EntryDirection = GetSafeDirection(
		OrbitForward * PhaseCos
		+ OrbitRight * PhaseSin * PatternCos
		+ OrbitUp * PatternSin * 0.65f,
		OrbitForward);
	Result.PreWrapPosition = MotionInput.TargetPosition
		+ EntryDirection * Result.Radius
		+ OrbitUp * ParticleRandom.RandomHeight;
	Result.FarPoint = ParticleRandom.FarPoint;

	const float PatternT = Result.FlightT;
	const float FarReachT = FMath::Clamp(MotionInput.FarReachAlpha, 0.05f, 0.95f);
	const float ToFarT = SmoothStep01(PatternT / FarReachT);
	const float ToTargetT = SmoothStep01((PatternT - FarReachT) / FMath::Max(1.0f - FarReachT, 0.001f));
	const bool bUseOutPath = PatternT < FarReachT;

	const FVector ToFarVector = Result.FarPoint - MotionInput.StartPosition;
	const FVector FromFarVector = Result.PreWrapPosition - Result.FarPoint;
	const FVector TravelVector = Result.PreWrapPosition - MotionInput.StartPosition;
	const float ToFarDistance = FMath::Max(ToFarVector.Size(), 1.0f);
	const float FromFarDistance = FMath::Max(FromFarVector.Size(), 1.0f);
	const FVector ToFarDirection = GetSafeDirection(ToFarVector, OrbitForward);
	const FVector FromFarDirection = GetSafeDirection(FromFarVector, OrbitForward);
	const FVector TravelDirection = GetSafeDirection(TravelVector, OrbitForward);
	const FVector TravelRight = GetSafeDirection(FVector::CrossProduct(OrbitUp, TravelDirection), OrbitRight);
	const float SideSign = ParticleRandom.SideOffset >= 0.0f ? 1.0f : -1.0f;
	const FVector OutSideDirection = GetSafeDirection(TravelRight * SideSign + OrbitUp * 0.15f, TravelRight);
	const FVector FarRight = GetSafeDirection(FVector::CrossProduct(OrbitUp, FromFarDirection), TravelRight);
	const FVector FarSideDirection = FarRight * -SideSign;
	const float BoomerangHeight = MotionInput.GlideSwoopHeight * ParticleRandom.SwoopHeight;
	const float BoomerangSideAmount = MotionInput.GlideSideAmount * ParticleRandom.SwoopSideAmount;
	const float OvershootDistance = FMath::Max(MotionInput.GlideOvershootAmount, MotionInput.GlideSwoopAmount * 0.35f) * ParticleRandom.SwoopAmount;

	auto SelectPath = [bUseOutPath](const FVector& OutPath, const FVector& BackPath)
	{
		return bUseOutPath ? OutPath : BackPath;
	};

	switch (static_cast<EUOUPaperPlaneSwarmFlightPattern>(Result.PatternIndex))
	{
	case EUOUPaperPlaneSwarmFlightPattern::DiveAndRise:
		Result.ControlPointA = MotionInput.StartPosition
			+ ToFarDirection * ToFarDistance * 0.35f
			+ OutSideDirection * BoomerangSideAmount * 0.25f
			+ OrbitUp * BoomerangHeight * 1.15f;
		Result.ControlPointB = Result.FarPoint
			+ FromFarDirection * FromFarDistance * 0.40f
			- FarSideDirection * BoomerangSideAmount * 0.15f
			+ OrbitUp * BoomerangHeight * 0.75f;
		Result.BezierPosition = SelectPath(
			SolveQuadraticBezier(MotionInput.StartPosition, Result.ControlPointA, Result.FarPoint, ToFarT),
			SolveQuadraticBezier(Result.FarPoint, Result.ControlPointB, Result.PreWrapPosition, ToTargetT));
		break;

	case EUOUPaperPlaneSwarmFlightPattern::SCurve:
		Result.ControlPointA = MotionInput.StartPosition
			+ ToFarDirection * ToFarDistance * 0.40f
			+ OutSideDirection * BoomerangSideAmount * 1.20f
			+ OrbitUp * BoomerangHeight * 0.15f;
		Result.ControlPointB = Result.FarPoint
			+ FromFarDirection * FromFarDistance * 0.48f
			- OutSideDirection * BoomerangSideAmount * 1.10f
			+ OrbitUp * BoomerangHeight * 0.20f;
		Result.BezierPosition = SelectPath(
			SolveQuadraticBezier(MotionInput.StartPosition, Result.ControlPointA, Result.FarPoint, ToFarT),
			SolveQuadraticBezier(Result.FarPoint, Result.ControlPointB, Result.PreWrapPosition, ToTargetT));
		Result.BezierPosition += FarSideDirection
			* FMath::Sin(PatternT * UE_TWO_PI + ParticleRandom.PatternPhase)
			* FMath::Sin(PatternT * UE_PI)
			* BoomerangSideAmount
			* 0.30f;
		break;

	case EUOUPaperPlaneSwarmFlightPattern::OverpassTurnback:
		Result.OvershootPosition = Result.FarPoint
			+ ToFarDirection * OvershootDistance * 0.25f
			+ OutSideDirection * BoomerangSideAmount * 0.25f
			+ OrbitUp * BoomerangHeight * 0.15f;
		Result.ControlPointA = MotionInput.StartPosition
			+ ToFarDirection * ToFarDistance * 0.55f
			+ OutSideDirection * BoomerangSideAmount * 0.35f
			+ OrbitUp * BoomerangHeight * 0.15f;
		Result.ControlPointB = Result.OvershootPosition
			+ FromFarDirection * FromFarDistance * 0.42f
			- FarSideDirection * BoomerangSideAmount * 0.55f
			+ OrbitUp * BoomerangHeight * 0.25f;
		Result.BezierPosition = SelectPath(
			SolveQuadraticBezier(MotionInput.StartPosition, Result.ControlPointA, Result.OvershootPosition, ToFarT),
			SolveQuadraticBezier(Result.OvershootPosition, Result.ControlPointB, Result.PreWrapPosition, ToTargetT));
		break;

	case EUOUPaperPlaneSwarmFlightPattern::WideGlide:
	default:
		Result.ControlPointA = MotionInput.StartPosition
			+ ToFarDirection * ToFarDistance * 0.45f
			+ OutSideDirection * BoomerangSideAmount * 0.65f
			+ OrbitUp * BoomerangHeight * 0.25f;
		Result.ControlPointB = Result.FarPoint
			+ FromFarDirection * FromFarDistance * 0.45f
			- FarSideDirection * BoomerangSideAmount * 0.30f
			+ OrbitUp * BoomerangHeight * 0.20f;
		Result.BezierPosition = SelectPath(
			SolveQuadraticBezier(MotionInput.StartPosition, Result.ControlPointA, Result.FarPoint, ToFarT),
			SolveQuadraticBezier(Result.FarPoint, Result.ControlPointB, Result.PreWrapPosition, ToTargetT));
		break;
	}

	Result.ControlPoint = Result.ControlPointA;
	if (Result.OvershootPosition.IsNearlyZero())
	{
		Result.OvershootPosition = Result.FarPoint;
	}

	const float WobbleFade = FMath::Sin(PatternT * UE_PI);
	const float WobbleA = FMath::Sin(MotionInput.Time * 1.2f * ParticleRandom.RandomSpeed + ParticleRandom.RandomPhase);
	const float WobbleB = FMath::Cos(MotionInput.Time * 0.85f * ParticleRandom.RandomSpeed + ParticleRandom.RandomPhase * 1.7f);
	const FVector WobbleOffset =
		(TravelRight * WobbleA * MotionInput.WobbleRightAmount
			+ OrbitUp * WobbleB * MotionInput.WobbleUpAmount)
		* WobbleFade;
	Result.PathPosition = Result.BezierPosition + WobbleOffset;

	const float OrbitAngle = MotionInput.Time * MotionInput.OrbitSpeed * ParticleRandom.RandomSpeed + ParticleRandom.RandomPhase;
	float OrbitSin = 0.0f;
	float OrbitCos = 1.0f;
	FMath::SinCos(&OrbitSin, &OrbitCos, OrbitAngle);
	const float TiltAmount = PatternSin * 0.85f;
	float TiltSin = 0.0f;
	float TiltCos = 1.0f;
	FMath::SinCos(&TiltSin, &TiltCos, TiltAmount);
	const FVector SphereDirection = GetSafeDirection(
		OrbitForward * OrbitCos
		+ OrbitRight * OrbitSin * TiltCos
		+ OrbitUp * FMath::Sin(OrbitAngle + ParticleRandom.PatternPhase) * TiltSin,
		OrbitForward);
	const float BreathRadius = Result.Radius + FMath::Sin(OrbitAngle * 1.37f + ParticleRandom.PatternPhase) * MotionInput.WrapHeight * 0.25f;
	Result.WrapPosition = MotionInput.TargetPosition + SphereDirection * BreathRadius;

	Result.Position = FMath::Lerp(Result.PathPosition, Result.WrapPosition, Result.WrapT);
	Result.Velocity = (Result.Position - MotionInput.PreviousPosition) / FMath::Max(MotionInput.DeltaTime, 0.0001f);
	Result.ForwardDirection = GetSafeDirection(Result.Velocity, OrbitForward);
	Result.BankRadians = FMath::Sin(MotionInput.Time * 4.0f * ParticleRandom.RandomSpeed + ParticleRandom.RandomPhase)
		* FMath::DegreesToRadians(ParticleRandom.BankAmount);
	Result.Scale = ParticleRandom.ScaleRandom;

	const FQuat BaseRotation = FRotationMatrix::MakeFromXZ(Result.ForwardDirection, OrbitUp).ToQuat();
	const FQuat BankRotation(Result.ForwardDirection, Result.BankRadians);
	const FQuat MeshRotation = BankRotation * BaseRotation;
	Result.Rotation = MeshRotation.Rotator();
	Result.UpDirection = BankRotation.RotateVector(BaseRotation.RotateVector(FVector::UpVector)).GetSafeNormal();

	return Result;
}

FUOUPaperPlaneSwarmParticleRandom AUOUFlyingSwarmEffectActor::MakePaperPlaneSwarmParticleRandom(int32 ParticleIndex, int32 InRandomSeed, const FUOUPaperPlaneSwarmRandomRanges& RandomRanges)
{
	FRandomStream RandomStream(MakeParticleRandomSeed(ParticleIndex, InRandomSeed));

	FUOUPaperPlaneSwarmParticleRandom ParticleRandom;
	ParticleRandom.RandomPhase = RandomRangeFloat(RandomStream, RandomRanges.RandomPhaseMin, RandomRanges.RandomPhaseMax);
	ParticleRandom.RandomDelay = RandomRangeFloat(RandomStream, RandomRanges.RandomDelayMin, RandomRanges.RandomDelayMax);
	ParticleRandom.RandomSpeed = RandomRangeFloat(RandomStream, RandomRanges.RandomSpeedMin, RandomRanges.RandomSpeedMax);
	ParticleRandom.RandomRadius = RandomRangeFloat(RandomStream, RandomRanges.RandomRadiusMin, RandomRanges.RandomRadiusMax);
	ParticleRandom.RandomHeight = RandomRangeFloat(RandomStream, RandomRanges.RandomHeightMin, RandomRanges.RandomHeightMax);
	ParticleRandom.SideOffset = RandomRangeFloat(RandomStream, RandomRanges.SideOffsetMin, RandomRanges.SideOffsetMax);
	ParticleRandom.HeightOffset = RandomRangeFloat(RandomStream, RandomRanges.HeightOffsetMin, RandomRanges.HeightOffsetMax);
	ParticleRandom.BankAmount = RandomRangeFloat(RandomStream, RandomRanges.BankAmountMin, RandomRanges.BankAmountMax);
	ParticleRandom.ScaleRandom = RandomRangeFloat(RandomStream, RandomRanges.ScaleRandomMin, RandomRanges.ScaleRandomMax);
	ParticleRandom.PatternIndex = RandomStream.RandRange(0, FMath::Clamp(RandomRanges.FlightPatternCount, 1, MaxPaperPlaneSwarmFlightPatternCount) - 1);
	ParticleRandom.PatternPhase = RandomRangeFloat(RandomStream, RandomRanges.PatternPhaseMin, RandomRanges.PatternPhaseMax);
	ParticleRandom.SwoopAmount = RandomRangeFloat(RandomStream, RandomRanges.SwoopAmountMin, RandomRanges.SwoopAmountMax);
	ParticleRandom.SwoopHeight = RandomRangeFloat(RandomStream, RandomRanges.SwoopHeightMin, RandomRanges.SwoopHeightMax);
	ParticleRandom.SwoopSideAmount = RandomRangeFloat(RandomStream, RandomRanges.SwoopSideAmountMin, RandomRanges.SwoopSideAmountMax);
	ParticleRandom.SwoopSpeed = RandomRangeFloat(RandomStream, RandomRanges.SwoopSpeedMin, RandomRanges.SwoopSpeedMax);
	ParticleRandom.FarPoint = RandomRangeVector(RandomStream, RandomRanges.FarPointMin, RandomRanges.FarPointMax);

	return ParticleRandom;
}

FTransform AUOUFlyingSwarmEffectActor::GetCurrentStartTransform() const
{
	return bIsEffectActive ? ActiveStartTransform : ResolveSourceTransform();
}

FTransform AUOUFlyingSwarmEffectActor::GetCurrentTargetTransform() const
{
	if (!bIsEffectActive || bFollowTarget)
	{
		return ResolveTargetTransform();
	}

	return ActiveTargetTransform;
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
