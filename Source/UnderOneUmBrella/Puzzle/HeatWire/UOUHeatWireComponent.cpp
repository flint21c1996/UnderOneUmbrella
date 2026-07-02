// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/HeatWire/UOUHeatWireComponent.h"

#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightExposureReceiverComponent.h"

UUOUHeatWireComponent::UUOUHeatWireComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetAutoActivate(true);
}

void UUOUHeatWireComponent::OnRegister()
{
	Super::OnRegister();
	ResolveHeatWirePath();
}

void UUOUHeatWireComponent::BeginPlay()
{
	Super::BeginPlay();

	Activate(true);
	ValidateSettings();
	ResolveHeatWirePath();
	if (bBuildDefaultWetSectionsOnBeginPlay && WetSections.IsEmpty())
	{
		BuildDefaultWetSections();
	}
	ValidateWetSections();

	SetBurnProgressInternal(InitialProgress, false);

	if (BurnProgress >= 1.0f)
	{
		SetHeatWireState(EUOUHeatWireState::BurnedOut);
		SetSatisfiedState(true, false);
	}
	else
	{
		SetHeatWireState(EUOUHeatWireState::Unlit);
		SetSatisfiedState(false, false);
	}

	ResolveLightReceiver();
	SubscribeLightReceiver();

	if (bAutoIgniteOnBeginPlay && !IsBurnedOut())
	{
		Ignite(nullptr);
	}

	UpdateBlockedState();
	RefreshTickState();
}

void UUOUHeatWireComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeLightReceiver();
	Super::EndPlay(EndPlayReason);
}

void UUOUHeatWireComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateWetSectionDrying(DeltaTime);

	if (IsBurning())
	{
		AdvanceBurn(DeltaTime);
	}

	RefreshTickState();
}

TArray<FString> UUOUHeatWireComponent::GetPuzzleDebugInfo_Implementation() const
{
	return {
		FString::Printf(TEXT("HeatWire: %s"), IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(
			TEXT("State: %s"),
			*StaticEnum<EUOUHeatWireState>()->GetNameStringByValue(static_cast<int64>(HeatWireState))),
		FString::Printf(TEXT("Progress: %.0f%%"), BurnProgress * 100.0f),
		FString::Printf(TEXT("Remaining: %.2f"), GetRemainingBurnTime()),
		FString::Printf(TEXT("Light Receiver: %s"), *GetNameSafe(LightReceiver)),
		FString::Printf(TEXT("Last Igniter: %s"), *GetNameSafe(LastIgniter))
	};
}

void UUOUHeatWireComponent::GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const
{
	if (IsValid(LastIgniter))
	{
		OutInputActors.AddUnique(LastIgniter);
	}

	if (LightReceiver != nullptr && IsValid(LightReceiver->LastExposureSourceActor))
	{
		OutInputActors.AddUnique(LightReceiver->LastExposureSourceActor);
	}
}

void UUOUHeatWireComponent::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		Ignite(nullptr);
		break;

	case EOUUPuzzleResultAction::Deactivate:
		ResetHeatWire();
		break;

	case EOUUPuzzleResultAction::Pause:
		PauseHeatWire();
		break;

	case EOUUPuzzleResultAction::Resume:
		ResumeHeatWire();
		break;

	case EOUUPuzzleResultAction::Toggle:
		if (IsBurning() || IsBlockedByWetness() || HeatWireState == EUOUHeatWireState::Paused)
		{
			Extinguish();
			return;
		}

		Ignite(nullptr);
		break;

	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

void UUOUHeatWireComponent::Interact_Implementation(AActor* Interactor)
{
	if (bAllowInteractionIgnite)
	{
		Ignite(Interactor);
	}
}

bool UUOUHeatWireComponent::Ignite(AActor* Igniter)
{
	if (IsBurnedOut())
	{
		return false;
	}

	if (BurnProgress >= 1.0f)
	{
		CompleteBurn();
		return false;
	}

	if (IsBurning())
	{
		return false;
	}

	LastIgniter = Igniter;
	SetHeatWireState(EUOUHeatWireState::Burning);
	SetSatisfiedState(false, true);
	OnHeatWireIgnited.Broadcast(Igniter);
	UpdateBlockedState();
	RefreshTickState();

	if (BurnDuration <= KINDA_SMALL_NUMBER)
	{
		FinishHeatWireImmediately();
	}

	return true;
}

bool UUOUHeatWireComponent::Extinguish()
{
	if (!IsBurning() && HeatWireState != EUOUHeatWireState::Paused)
	{
		return false;
	}

	SetHeatWireState(EUOUHeatWireState::Extinguished);
	SetBlockedSectionIndex(INDEX_NONE);
	RefreshTickState();
	OnHeatWireExtinguished.Broadcast();
	return true;
}

bool UUOUHeatWireComponent::PauseHeatWire()
{
	if (!IsBurning())
	{
		return false;
	}

	SetHeatWireState(EUOUHeatWireState::Paused);
	RefreshTickState();
	return true;
}

bool UUOUHeatWireComponent::ResumeHeatWire()
{
	if (HeatWireState != EUOUHeatWireState::Paused)
	{
		return false;
	}

	SetHeatWireState(EUOUHeatWireState::Burning);
	UpdateBlockedState();
	RefreshTickState();
	return true;
}

void UUOUHeatWireComponent::ResetHeatWire()
{
	ValidateSettings();
	ValidateWetSections();
	LastIgniter = nullptr;
	SetBlockedSectionIndex(INDEX_NONE);
	SetBurnProgressInternal(InitialProgress, true);

	if (BurnProgress >= 1.0f)
	{
		SetHeatWireState(EUOUHeatWireState::BurnedOut);
		SetSatisfiedState(true, true);
	}
	else
	{
		SetHeatWireState(EUOUHeatWireState::Unlit);
		SetSatisfiedState(false, true);
	}

	RefreshTickState();
	OnHeatWireReset.Broadcast();
}

void UUOUHeatWireComponent::FinishHeatWireImmediately()
{
	SetBurnProgressInternal(1.0f, true);
	CompleteBurn();
}

void UUOUHeatWireComponent::SetBurnProgress(float NewProgress)
{
	SetBurnProgressInternal(NewProgress, true);

	if (BurnProgress >= 1.0f)
	{
		CompleteBurn();
		return;
	}

	if (HeatWireState == EUOUHeatWireState::BurnedOut)
	{
		SetHeatWireState(EUOUHeatWireState::Unlit);
		SetSatisfiedState(false, true);
	}

	UpdateBlockedState();
	RefreshTickState();
}

void UUOUHeatWireComponent::RebuildDefaultWetSections()
{
	BuildDefaultWetSections();
	ValidateWetSections();
	UpdateBlockedState();
	RefreshTickState();
}

void UUOUHeatWireComponent::ClearWetness()
{
	for (int32 SectionIndex = 0; SectionIndex < WetSections.Num(); ++SectionIndex)
	{
		SetWetSectionWetness(SectionIndex, 0.0f, true);
	}

	UpdateBlockedState();
	RefreshTickState();
}

void UUOUHeatWireComponent::ApplyRainAtProgress(float Progress, float WetnessAmount, AActor* InstigatorActor)
{
	ApplyRainToProgressRange(Progress, Progress, WetnessAmount, InstigatorActor);
}

void UUOUHeatWireComponent::ApplyRainToProgressRange(
	float StartProgress,
	float EndProgress,
	float WetnessAmount,
	AActor* InstigatorActor)
{
	if (WetnessAmount <= 0.0f)
	{
		return;
	}

	float MinProgress = FMath::Clamp(StartProgress, 0.0f, 1.0f);
	float MaxProgress = FMath::Clamp(EndProgress, 0.0f, 1.0f);
	if (MinProgress > MaxProgress)
	{
		Swap(MinProgress, MaxProgress);
	}

	for (int32 SectionIndex = 0; SectionIndex < WetSections.Num(); ++SectionIndex)
	{
		const FUOUHeatWireWetSection& Section = WetSections[SectionIndex];
		if (!Section.bReceivesRain || Section.EndProgress < MinProgress || Section.StartProgress > MaxProgress)
		{
			continue;
		}

		ApplyRainToWetSection(SectionIndex, WetnessAmount, InstigatorActor);
	}
}

void UUOUHeatWireComponent::ApplyRainAtWorldLocation(FVector WorldLocation, float WetnessAmount, AActor* InstigatorActor)
{
	ApplyRainAtProgress(GetProgressAtWorldLocation(WorldLocation), WetnessAmount, InstigatorActor);
}

void UUOUHeatWireComponent::ApplyRainToWetSection(int32 SectionIndex, float WetnessAmount, AActor* InstigatorActor)
{
	if (!WetSections.IsValidIndex(SectionIndex) || WetnessAmount <= 0.0f)
	{
		return;
	}

	FUOUHeatWireWetSection& Section = WetSections[SectionIndex];
	if (!Section.bReceivesRain)
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		Section.LastRainWorldTime = World->GetTimeSeconds();
	}

	float TargetWetness = Section.Wetness + WetnessAmount;
	if (bRainImmediatelyBlocksWetSections && Section.bBlocksFire)
	{
		TargetWetness = FMath::Max(TargetWetness, BlockingWetness);
	}

	SetWetSectionWetness(SectionIndex, TargetWetness, true);
	UpdateBlockedState();
	RefreshTickState();
}

bool UUOUHeatWireComponent::IsBurning() const
{
	return HeatWireState == EUOUHeatWireState::Burning;
}

bool UUOUHeatWireComponent::IsBlockedByWetness() const
{
	return IsBurning() && BlockedSectionIndex != INDEX_NONE;
}

bool UUOUHeatWireComponent::IsBurnedOut() const
{
	return HeatWireState == EUOUHeatWireState::BurnedOut;
}

float UUOUHeatWireComponent::GetRemainingBurnTime() const
{
	if (BurnProgress >= 1.0f || BurnDuration <= KINDA_SMALL_NUMBER || BurnRateMultiplier <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return (1.0f - BurnProgress) * BurnDuration / BurnRateMultiplier;
}

FVector UUOUHeatWireComponent::GetFireWorldLocation() const
{
	return GetWorldLocationAtProgress(BurnProgress);
}

FVector UUOUHeatWireComponent::GetWorldLocationAtProgress(float Progress) const
{
	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	if (HeatWirePathComponent != nullptr)
	{
		const float SplineLength = HeatWirePathComponent->GetSplineLength();
		if (SplineLength > KINDA_SMALL_NUMBER)
		{
			return HeatWirePathComponent->GetLocationAtDistanceAlongSpline(
				SplineLength * ClampedProgress,
				ESplineCoordinateSpace::World);
		}
	}

	if (const AActor* Owner = GetOwner())
	{
		const FVector LocalLocation((ClampedProgress - 0.5f) * FMath::Max(1.0f, FallbackHeatWireWorldLength), 0.0f, 0.0f);
		return Owner->GetActorTransform().TransformPosition(LocalLocation);
	}

	return FVector::ZeroVector;
}

float UUOUHeatWireComponent::GetProgressAtWorldLocation(FVector WorldLocation) const
{
	if (HeatWirePathComponent != nullptr)
	{
		const float SplineLength = HeatWirePathComponent->GetSplineLength();
		if (SplineLength > KINDA_SMALL_NUMBER)
		{
			const int32 SampleCount = FMath::Max(4, SplineWorldSampleCount);
			float BestProgress = 0.0f;
			float BestDistanceSquared = TNumericLimits<float>::Max();

			for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
			{
				const float Progress = SampleCount > 1
					? static_cast<float>(SampleIndex) / static_cast<float>(SampleCount - 1)
					: 0.0f;
				const FVector SampleLocation = HeatWirePathComponent->GetLocationAtDistanceAlongSpline(
					SplineLength * Progress,
					ESplineCoordinateSpace::World);
				const float DistanceSquared = FVector::DistSquared(WorldLocation, SampleLocation);
				if (DistanceSquared < BestDistanceSquared)
				{
					BestDistanceSquared = DistanceSquared;
					BestProgress = Progress;
				}
			}

			return BestProgress;
		}
	}

	if (const AActor* Owner = GetOwner())
	{
		const FVector LocalLocation = Owner->GetActorTransform().InverseTransformPosition(WorldLocation);
		return FMath::Clamp((LocalLocation.X / FMath::Max(1.0f, FallbackHeatWireWorldLength)) + 0.5f, 0.0f, 1.0f);
	}

	return 0.0f;
}

bool UUOUHeatWireComponent::CanReceiveRainInput() const
{
	return IsActive() && GetOwner() != nullptr && WetSections.Num() > 0;
}

int32 UUOUHeatWireComponent::GetWetSectionCount() const
{
	return WetSections.Num();
}

bool UUOUHeatWireComponent::CanWetSectionReceiveRain(int32 SectionIndex) const
{
	return WetSections.IsValidIndex(SectionIndex) && WetSections[SectionIndex].bReceivesRain;
}

bool UUOUHeatWireComponent::GetWetSectionWorldLocation(int32 SectionIndex, FVector& OutWorldLocation) const
{
	if (!WetSections.IsValidIndex(SectionIndex))
	{
		OutWorldLocation = FVector::ZeroVector;
		return false;
	}

	const FUOUHeatWireWetSection& Section = WetSections[SectionIndex];
	OutWorldLocation = GetWorldLocationAtProgress((Section.StartProgress + Section.EndProgress) * 0.5f);
	return true;
}

float UUOUHeatWireComponent::GetWetSectionRainCoverageRadius(int32 SectionIndex) const
{
	return WetSections.IsValidIndex(SectionIndex)
		? FMath::Max(0.0f, WetSections[SectionIndex].RainCoverageRadius)
		: 0.0f;
}

void UUOUHeatWireComponent::HandleLightExposureReceived(const FUOULightExposureData& ExposureData)
{
	if (!bIgniteFromLightExposure || IsBurnedOut() || IsBurning())
	{
		return;
	}

	if (ExposureData.Intensity < LightIgnitionIntensityThreshold)
	{
		return;
	}

	Ignite(ResolveExposureSourceActor(ExposureData));
}

void UUOUHeatWireComponent::ValidateSettings()
{
	BurnDuration = FMath::Max(0.0f, BurnDuration);
	BurnRateMultiplier = FMath::Max(0.0f, BurnRateMultiplier);
	InitialProgress = FMath::Clamp(InitialProgress, 0.0f, 1.0f);
	FallbackHeatWireWorldLength = FMath::Max(1.0f, FallbackHeatWireWorldLength);
	SplineWorldSampleCount = FMath::Max(4, SplineWorldSampleCount);
	DefaultWetSectionCount = FMath::Max(1, DefaultWetSectionCount);
	BlockingWetness = FMath::Max(0.0f, BlockingWetness);
	DryingGraceTime = FMath::Max(0.0f, DryingGraceTime);
	DryRate = FMath::Max(0.0f, DryRate);
	LightIgnitionIntensityThreshold = FMath::Max(0.0f, LightIgnitionIntensityThreshold);
	BurnProgress = FMath::Clamp(BurnProgress, 0.0f, 1.0f);
}

void UUOUHeatWireComponent::ValidateWetSections()
{
	for (int32 SectionIndex = 0; SectionIndex < WetSections.Num(); ++SectionIndex)
	{
		FUOUHeatWireWetSection& Section = WetSections[SectionIndex];
		Section.StartProgress = FMath::Clamp(Section.StartProgress, 0.0f, 1.0f);
		Section.EndProgress = FMath::Clamp(Section.EndProgress, 0.0f, 1.0f);
		if (Section.StartProgress > Section.EndProgress)
		{
			Swap(Section.StartProgress, Section.EndProgress);
		}

		if (Section.SectionName.IsNone())
		{
			Section.SectionName = *FString::Printf(TEXT("HeatWireSection_%02d"), SectionIndex + 1);
		}

		Section.MaxWetness = FMath::Max(0.0f, Section.MaxWetness);
		if (Section.bBlocksFire)
		{
			Section.MaxWetness = FMath::Max(Section.MaxWetness, BlockingWetness);
		}
		Section.RainCoverageRadius = FMath::Max(0.0f, Section.RainCoverageRadius);
		Section.Wetness = FMath::Clamp(Section.Wetness, 0.0f, Section.MaxWetness);
	}
}

void UUOUHeatWireComponent::BuildDefaultWetSections()
{
	WetSections.Reset();

	const int32 SectionCount = FMath::Max(1, DefaultWetSectionCount);
	for (int32 SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
	{
		FUOUHeatWireWetSection Section;
		Section.SectionName = *FString::Printf(TEXT("HeatWireSection_%02d"), SectionIndex + 1);
		Section.StartProgress = static_cast<float>(SectionIndex) / static_cast<float>(SectionCount);
		Section.EndProgress = static_cast<float>(SectionIndex + 1) / static_cast<float>(SectionCount);
		WetSections.Add(Section);
	}
}

void UUOUHeatWireComponent::ResolveHeatWirePath()
{
	if (HeatWirePathComponent != nullptr || !bAutoFindHeatWirePath)
	{
		return;
	}

	if (AActor* Owner = GetOwner())
	{
		HeatWirePathComponent = Owner->FindComponentByClass<USplineComponent>();
	}
}

void UUOUHeatWireComponent::ResolveLightReceiver()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (HasLightReceiverReference())
	{
		LightReceiver = Cast<UUOULightExposureReceiverComponent>(LightReceiverReference.GetComponent(Owner));
	}

	if (LightReceiver == nullptr && bAutoFindLightReceiver)
	{
		LightReceiver = Owner->FindComponentByClass<UUOULightExposureReceiverComponent>();
	}
}

void UUOUHeatWireComponent::SubscribeLightReceiver()
{
	if (LightReceiver == nullptr)
	{
		return;
	}

	LightReceiver->OnLightExposureReceived.RemoveDynamic(this, &UUOUHeatWireComponent::HandleLightExposureReceived);
	LightReceiver->OnLightExposureReceived.AddDynamic(this, &UUOUHeatWireComponent::HandleLightExposureReceived);
}

void UUOUHeatWireComponent::UnsubscribeLightReceiver()
{
	if (LightReceiver != nullptr)
	{
		LightReceiver->OnLightExposureReceived.RemoveDynamic(this, &UUOUHeatWireComponent::HandleLightExposureReceived);
	}
}

bool UUOUHeatWireComponent::HasLightReceiverReference() const
{
	return LightReceiverReference.ComponentProperty != NAME_None ||
		!LightReceiverReference.PathToComponent.IsEmpty() ||
		LightReceiverReference.OverrideComponent.IsValid();
}

AActor* UUOUHeatWireComponent::ResolveExposureSourceActor(const FUOULightExposureData& ExposureData) const
{
	if (AActor* SourceActor = Cast<AActor>(ExposureData.Source))
	{
		return SourceActor;
	}

	if (const UActorComponent* SourceComponent = Cast<UActorComponent>(ExposureData.Source))
	{
		return SourceComponent->GetOwner();
	}

	return nullptr;
}

void UUOUHeatWireComponent::UpdateWetSectionDrying(float DeltaTime)
{
	if (DeltaTime <= 0.0f || DryRate <= 0.0f)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World != nullptr ? World->GetTimeSeconds() : BIG_NUMBER;
	bool bAnyWetnessChanged = false;

	for (int32 SectionIndex = 0; SectionIndex < WetSections.Num(); ++SectionIndex)
	{
		const FUOUHeatWireWetSection& Section = WetSections[SectionIndex];
		if (Section.Wetness <= 0.0f || CurrentTime - Section.LastRainWorldTime < DryingGraceTime)
		{
			continue;
		}

		SetWetSectionWetness(SectionIndex, Section.Wetness - DryRate * DeltaTime, true);
		bAnyWetnessChanged = true;
	}

	if (bAnyWetnessChanged)
	{
		UpdateBlockedState();
	}
}

void UUOUHeatWireComponent::UpdateBlockedState()
{
	SetBlockedSectionIndex(FindBlockingSectionAtProgress(BurnProgress));
}

int32 UUOUHeatWireComponent::FindBlockingSectionAtProgress(float Progress) const
{
	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	for (int32 SectionIndex = 0; SectionIndex < WetSections.Num(); ++SectionIndex)
	{
		const FUOUHeatWireWetSection& Section = WetSections[SectionIndex];
		if (ClampedProgress + KINDA_SMALL_NUMBER < Section.StartProgress ||
			ClampedProgress - KINDA_SMALL_NUMBER > Section.EndProgress)
		{
			continue;
		}

		if (IsWetSectionBlocking(Section))
		{
			return SectionIndex;
		}
	}

	return INDEX_NONE;
}

int32 UUOUHeatWireComponent::FindFirstBlockingSectionInRange(
	float StartProgress,
	float EndProgress,
	float& OutBlockProgress) const
{
	float MinProgress = FMath::Clamp(StartProgress, 0.0f, 1.0f);
	float MaxProgress = FMath::Clamp(EndProgress, 0.0f, 1.0f);
	if (MinProgress > MaxProgress)
	{
		Swap(MinProgress, MaxProgress);
	}

	int32 BlockingIndex = INDEX_NONE;
	float BestBlockProgress = MaxProgress;
	for (int32 SectionIndex = 0; SectionIndex < WetSections.Num(); ++SectionIndex)
	{
		const FUOUHeatWireWetSection& Section = WetSections[SectionIndex];
		if (!IsWetSectionBlocking(Section))
		{
			continue;
		}

		if (Section.EndProgress + KINDA_SMALL_NUMBER < MinProgress ||
			Section.StartProgress - KINDA_SMALL_NUMBER > MaxProgress)
		{
			continue;
		}

		const float CandidateBlockProgress = FMath::Max(MinProgress, Section.StartProgress);
		if (BlockingIndex == INDEX_NONE || CandidateBlockProgress < BestBlockProgress)
		{
			BlockingIndex = SectionIndex;
			BestBlockProgress = CandidateBlockProgress;
		}
	}

	OutBlockProgress = BestBlockProgress;
	return BlockingIndex;
}

bool UUOUHeatWireComponent::IsWetSectionBlocking(const FUOUHeatWireWetSection& Section) const
{
	return Section.bBlocksFire && Section.Wetness >= BlockingWetness;
}

void UUOUHeatWireComponent::SetHeatWireState(EUOUHeatWireState NewState)
{
	if (HeatWireState == NewState)
	{
		return;
	}

	HeatWireState = NewState;
	OnHeatWireStateChanged.Broadcast(HeatWireState);
}

void UUOUHeatWireComponent::SetBlockedSectionIndex(int32 NewBlockedSectionIndex)
{
	if (BlockedSectionIndex == NewBlockedSectionIndex)
	{
		return;
	}

	BlockedSectionIndex = NewBlockedSectionIndex;
	OnBlockedSectionChanged.Broadcast(BlockedSectionIndex);
}

void UUOUHeatWireComponent::SetWetSectionWetness(int32 SectionIndex, float NewWetness, bool bBroadcastChange)
{
	if (!WetSections.IsValidIndex(SectionIndex))
	{
		return;
	}

	FUOUHeatWireWetSection& Section = WetSections[SectionIndex];
	const float ClampedWetness = FMath::Clamp(NewWetness, 0.0f, FMath::Max(0.0f, Section.MaxWetness));
	if (FMath::IsNearlyEqual(Section.Wetness, ClampedWetness))
	{
		Section.Wetness = ClampedWetness;
		return;
	}

	Section.Wetness = ClampedWetness;
	if (bBroadcastChange)
	{
		OnWetSectionChanged.Broadcast(SectionIndex, Section.Wetness);
	}
}

void UUOUHeatWireComponent::SetBurnProgressInternal(float NewProgress, bool bBroadcastChange)
{
	const float ClampedProgress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(BurnProgress, ClampedProgress))
	{
		BurnProgress = ClampedProgress;
		return;
	}

	BurnProgress = ClampedProgress;
	if (bBroadcastChange)
	{
		OnHeatWireProgressChanged.Broadcast(BurnProgress, GetRemainingBurnTime());
	}
}

void UUOUHeatWireComponent::AdvanceBurn(float DeltaTime)
{
	if (DeltaTime <= 0.0f || BurnRateMultiplier <= 0.0f)
	{
		return;
	}

	UpdateBlockedState();
	if (IsBlockedByWetness())
	{
		return;
	}

	if (!IsBurning())
	{
		return;
	}

	if (BurnDuration <= KINDA_SMALL_NUMBER)
	{
		FinishHeatWireImmediately();
		return;
	}

	const float NextProgress = FMath::Clamp(BurnProgress + DeltaTime * BurnRateMultiplier / BurnDuration, 0.0f, 1.0f);
	float BlockProgress = NextProgress;
	const int32 NextBlockingSectionIndex = FindFirstBlockingSectionInRange(BurnProgress, NextProgress, BlockProgress);
	if (NextBlockingSectionIndex != INDEX_NONE)
	{
		SetBurnProgressInternal(BlockProgress, true);
		SetBlockedSectionIndex(NextBlockingSectionIndex);
		return;
	}

	SetBurnProgressInternal(NextProgress, true);
	if (BurnProgress >= 1.0f)
	{
		CompleteBurn();
	}
}

void UUOUHeatWireComponent::CompleteBurn()
{
	if (IsBurnedOut())
	{
		return;
	}

	SetBurnProgressInternal(1.0f, true);
	SetBlockedSectionIndex(INDEX_NONE);
	SetHeatWireState(EUOUHeatWireState::BurnedOut);
	SetSatisfiedState(true, true);
	RefreshTickState();
	OnHeatWireBurnedOut.Broadcast();
}

void UUOUHeatWireComponent::RefreshTickState()
{
	const bool bShouldTickForBurn = IsBurning() && BurnProgress < 1.0f;
	const bool bShouldTickForDrying = DryRate > 0.0f && WetSections.ContainsByPredicate(
		[](const FUOUHeatWireWetSection& Section)
		{
			return Section.Wetness > 0.0f;
		});

	SetComponentTickEnabled(bShouldTickForBurn || bShouldTickForDrying);
}
