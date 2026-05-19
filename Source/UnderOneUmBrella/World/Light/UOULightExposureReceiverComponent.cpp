// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightExposureReceiverComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UUOULightExposureReceiverComponent::UUOULightExposureReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOULightExposureReceiverComponent::BeginPlay()
{
	Super::BeginPlay();

	ValidateTemperatureSettings();
	if (bStartAtAmbientTemperature)
	{
		CurrentTemperature = AmbientTemperature;
	}

	CurrentTemperature = FMath::Clamp(CurrentTemperature, MinTemperature, MaxTemperature);
}

void UUOULightExposureReceiverComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (bIsReceivingLight && World != nullptr)
	{
		const float TimeSinceLastExposure = World->GetTimeSeconds() - LastExposureWorldTime;
		if (TimeSinceLastExposure > ExposureEndGraceTime)
		{
			SetReceivingLight(false);
		}
	}

	RecoverTemperature(DeltaTime);
	DrawTemperatureDebug();
}

TArray<FString> UUOULightExposureReceiverComponent::GetPuzzleDebugInfo_Implementation() const
{
	return {
		FString::Printf(TEXT("Light Receiver: %s"), bIsReceivingLight ? TEXT("Lit") : TEXT("Not Lit")),
		FString::Printf(TEXT("Temp: %.1f C"), CurrentTemperature),
		FString::Printf(TEXT("Exposure: %.2f from %s"), LastExposureIntensity, *LastExposureSourceName)
	};
}

FVector UUOULightExposureReceiverComponent::GetLightReceiverPosition_Implementation() const
{
	if (const USceneComponent* ReceiverTransform = GetReferencedReceiverTransform())
	{
		return ReceiverTransform->GetComponentLocation();
	}

	if (const USceneComponent* AutoReceiverTransform = FindAutoReceiverTransform())
	{
		return AutoReceiverTransform->GetComponentLocation();
	}

	if (const AActor* Owner = GetOwner())
	{
		if (const USceneComponent* RootComponent = Owner->GetRootComponent())
		{
			return RootComponent->GetComponentLocation();
		}

		return Owner->GetActorLocation();
	}

	return FVector::ZeroVector;
}

void UUOULightExposureReceiverComponent::ReceiveLightExposure_Implementation(const FUOULightExposureData& ExposureData)
{
	if (ExposureData.Intensity <= 0.0f || ExposureData.DeltaTime <= 0.0f)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		LastExposureWorldTime = World->GetTimeSeconds();
	}

	LastExposureIntensity = ExposureData.Intensity;
	LastExposureSourceName = GetNameSafe(ExposureData.Source);
	SetReceivingLight(true);

	OnLightExposureReceived.Broadcast(ExposureData);
	ApplyTemperatureDelta(ExposureData.Intensity * ExposureData.DeltaTime * TemperatureRisePerIntensity);
}

void UUOULightExposureReceiverComponent::SetTemperature(float NewTemperature)
{
	const float ClampedTemperature = FMath::Clamp(NewTemperature, MinTemperature, MaxTemperature);
	if (FMath::IsNearlyEqual(CurrentTemperature, ClampedTemperature))
	{
		return;
	}

	const float PreviousTemperature = CurrentTemperature;
	CurrentTemperature = ClampedTemperature;

	OnTemperatureChanged.Broadcast(CurrentTemperature, PreviousTemperature);
}

void UUOULightExposureReceiverComponent::ApplyTemperatureDelta(float DeltaTemperature)
{
	if (FMath::IsNearlyZero(DeltaTemperature))
	{
		return;
	}

	SetTemperature(CurrentTemperature + DeltaTemperature);
}

bool UUOULightExposureReceiverComponent::IsReceivingLight() const
{
	return bIsReceivingLight;
}

void UUOULightExposureReceiverComponent::ValidateTemperatureSettings()
{
	if (MinTemperature > MaxTemperature)
	{
		Swap(MinTemperature, MaxTemperature);
	}

	AmbientTemperature = FMath::Clamp(AmbientTemperature, MinTemperature, MaxTemperature);
	CurrentTemperature = FMath::Clamp(CurrentTemperature, MinTemperature, MaxTemperature);
	TemperatureRisePerIntensity = FMath::Max(0.0f, TemperatureRisePerIntensity);
	TemperatureRecoveryRate = FMath::Max(0.0f, TemperatureRecoveryRate);
	ExposureEndGraceTime = FMath::Max(0.0f, ExposureEndGraceTime);
	TemperatureDebugDrawTime = FMath::Max(0.0f, TemperatureDebugDrawTime);
	TemperatureDebugTextScale = FMath::Max(0.1f, TemperatureDebugTextScale);
}

USceneComponent* UUOULightExposureReceiverComponent::GetReferencedReceiverTransform() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	if (ReceiverTransformReference.ComponentProperty == NAME_None &&
		ReceiverTransformReference.PathToComponent.IsEmpty() &&
		!ReceiverTransformReference.OverrideComponent.IsValid())
	{
		return nullptr;
	}

	return Cast<USceneComponent>(ReceiverTransformReference.GetComponent(Owner));
}

USceneComponent* UUOULightExposureReceiverComponent::FindAutoReceiverTransform() const
{
	if (!bAutoFindReceiverTransform)
	{
		return nullptr;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent != Owner->GetRootComponent())
		{
			return PrimitiveComponent;
		}
	}

	TInlineComponentArray<USceneComponent*> SceneComponents(Owner);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent != nullptr && SceneComponent != Owner->GetRootComponent())
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void UUOULightExposureReceiverComponent::SetReceivingLight(bool bNewReceivingLight)
{
	if (bIsReceivingLight == bNewReceivingLight)
	{
		return;
	}

	bIsReceivingLight = bNewReceivingLight;
	if (bIsReceivingLight)
	{
		OnLightExposureStarted.Broadcast();
		return;
	}

	LastExposureIntensity = 0.0f;
	LastExposureSourceName = TEXT("None");
	OnLightExposureEnded.Broadcast();
}

void UUOULightExposureReceiverComponent::RecoverTemperature(float DeltaTime)
{
	if (!bRecoverToAmbientWhenNotExposed || bIsReceivingLight || TemperatureRecoveryRate <= 0.0f || DeltaTime <= 0.0f)
	{
		return;
	}

	const float RecoveredTemperature = FMath::FInterpConstantTo(
		CurrentTemperature,
		AmbientTemperature,
		DeltaTime,
		TemperatureRecoveryRate);

	SetTemperature(RecoveredTemperature);
}

void UUOULightExposureReceiverComponent::DrawTemperatureDebug() const
{
	if (!bDrawTemperatureDebug || !UUOUDebugSubsystem::IsDebugCategoryEnabled(this, EUOUDebugCategory::Puzzle))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const FVector DebugLocation = GetLightReceiverPosition_Implementation() + TemperatureDebugOffset;
	const FString DebugText = FString::Printf(
		TEXT("Temp: %.1f C\nLight: %s\nIntensity: %.2f"),
		CurrentTemperature,
		bIsReceivingLight ? TEXT("On") : TEXT("Off"),
		LastExposureIntensity);

	DrawDebugString(
		World,
		DebugLocation,
		DebugText,
		nullptr,
		bIsReceivingLight ? ExposedTemperatureDebugColor : TemperatureDebugColor,
		TemperatureDebugDrawTime,
		true,
		TemperatureDebugTextScale);
}
