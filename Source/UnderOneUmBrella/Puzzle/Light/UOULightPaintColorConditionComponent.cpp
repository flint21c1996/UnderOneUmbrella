// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Light/UOULightPaintColorConditionComponent.h"

#include "GameFramework/Actor.h"

UUOULightPaintColorConditionComponent::UUOULightPaintColorConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOULightPaintColorConditionComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshNow();
}

void UUOULightPaintColorConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeColorReceiver();
	Super::EndPlay(EndPlayReason);
}

FText UUOULightPaintColorConditionComponent::GetDebugSummaryText_Implementation() const
{
	const FString RequiredStateName = StaticEnum<EUOULightColorState>() != nullptr
		? StaticEnum<EUOULightColorState>()->GetDisplayNameTextByValue(
			static_cast<int64>(RequiredColorState)).ToString()
		: TEXT("Unknown");
	const TArray<FString> DebugLines = {
		FString::Printf(
			TEXT("Paint Color: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(TEXT("Required: %s"), *RequiredStateName),
		FString::Printf(
			TEXT("Observed: %.2f, %.2f, %.2f"),
			ObservedPaintTint.R,
			ObservedPaintTint.G,
			ObservedPaintTint.B),
		FString::Printf(TEXT("Receiver: %s"), *GetNameSafe(ColorReceiver))
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOULightPaintColorConditionComponent::GetPuzzleDebugInputActors_Implementation(
	TArray<AActor*>& OutInputActors) const
{
	if (ColorReceiver != nullptr && IsValid(ColorReceiver->GetOwner()))
	{
		OutInputActors.AddUnique(ColorReceiver->GetOwner());
	}
}

void UUOULightPaintColorConditionComponent::RefreshNow()
{
	UnsubscribeColorReceiver();
	ColorReceiver = nullptr;
	ResolveColorReceiver();
	SubscribeColorReceiver();
	RefreshSatisfiedState();
}

void UUOULightPaintColorConditionComponent::HandlePaintTintChanged(
	FLinearColor NewPaintTint)
{
	ObservedPaintTint = NewPaintTint;
	RefreshSatisfiedState();
}

void UUOULightPaintColorConditionComponent::ResolveColorReceiver()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (HasColorReceiverReference())
	{
		ColorReceiver = Cast<UUOULightColorReceiverComponent>(
			ColorReceiverReference.GetComponent(Owner));
	}

	if (ColorReceiver == nullptr && bAutoFindColorReceiver)
	{
		ColorReceiver = Owner->FindComponentByClass<UUOULightColorReceiverComponent>();
	}
}

void UUOULightPaintColorConditionComponent::SubscribeColorReceiver()
{
	if (ColorReceiver == nullptr)
	{
		return;
	}

	ColorReceiver->OnPaintTintChanged.RemoveDynamic(
		this,
		&UUOULightPaintColorConditionComponent::HandlePaintTintChanged);
	ColorReceiver->OnPaintTintChanged.AddDynamic(
		this,
		&UUOULightPaintColorConditionComponent::HandlePaintTintChanged);
}

void UUOULightPaintColorConditionComponent::UnsubscribeColorReceiver()
{
	if (ColorReceiver != nullptr)
	{
		ColorReceiver->OnPaintTintChanged.RemoveDynamic(
			this,
			&UUOULightPaintColorConditionComponent::HandlePaintTintChanged);
	}
}

void UUOULightPaintColorConditionComponent::RefreshSatisfiedState()
{
	if (ColorReceiver == nullptr || RequiredColorState == EUOULightColorState::None)
	{
		ObservedPaintTint = FLinearColor::White;
		RequiredPaintTint = FLinearColor::White;
		SetSatisfiedState(false, true);
		return;
	}

	ObservedPaintTint = ColorReceiver->CurrentPaintTint;
	RequiredPaintTint = ColorReceiver->GetPaintTintForColorState(RequiredColorState);

	if (bLatchOnceSatisfied && bIsSatisfied)
	{
		return;
	}

	const float SafeMatchTolerance = FMath::Clamp(MatchTolerance, 0.0f, 1.0f);
	const float SafeReleaseTolerance = FMath::Max(
		SafeMatchTolerance,
		FMath::Clamp(ReleaseTolerance, 0.0f, 1.0f));
	const float ActiveTolerance = bIsSatisfied
		? SafeReleaseTolerance
		: SafeMatchTolerance;
	SetSatisfiedState(
		IsWithinTolerance(ObservedPaintTint, RequiredPaintTint, ActiveTolerance),
		true);
}

bool UUOULightPaintColorConditionComponent::HasColorReceiverReference() const
{
	return ColorReceiverReference.ComponentProperty != NAME_None ||
		!ColorReceiverReference.PathToComponent.IsEmpty() ||
		ColorReceiverReference.OverrideComponent.IsValid();
}

bool UUOULightPaintColorConditionComponent::IsWithinTolerance(
	const FLinearColor& Current,
	const FLinearColor& Required,
	float Tolerance) const
{
	return FMath::Abs(Current.R - Required.R) <= Tolerance &&
		FMath::Abs(Current.G - Required.G) <= Tolerance &&
		FMath::Abs(Current.B - Required.B) <= Tolerance;
}
