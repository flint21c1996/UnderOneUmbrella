// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightExposureReceiverComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
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

void UUOULightExposureReceiverComponent::GetLightReceiverSamplePositions(
	const FVector& BeamDirection,
	TArray<FVector>& OutSamplePositions) const
{
	OutSamplePositions.Reset();

	const UPrimitiveComponent* ReceiverVolume = GetReceiverVolume();
	const FVector SafeBeamDirection = BeamDirection.GetSafeNormal();
	if (!bUseReceiverVolumeSampling || ReceiverVolume == nullptr || SafeBeamDirection.IsNearlyZero())
	{
		OutSamplePositions.Add(GetLightReceiverPosition_Implementation());
		return;
	}

	const FBoxSphereBounds ReceiverBounds = ReceiverVolume->Bounds;
	const FVector Center = ReceiverBounds.Origin;
	const FVector Extent = ReceiverBounds.BoxExtent;
	OutSamplePositions.Add(Center);

	FVector SampleAxisA = FVector::ZeroVector;
	FVector SampleAxisB = FVector::ZeroVector;
	SafeBeamDirection.FindBestAxisVectors(SampleAxisA, SampleAxisB);

	const auto CalculateAxisDistance = [&Extent](const FVector& Axis)
	{
		float Distance = TNumericLimits<float>::Max();
		for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
		{
			const float AxisAmount = FMath::Abs(Axis[AxisIndex]);
			if (AxisAmount > KINDA_SMALL_NUMBER)
			{
				Distance = FMath::Min(Distance, Extent[AxisIndex] / AxisAmount);
			}
		}
		return Distance == TNumericLimits<float>::Max() ? 0.0f : Distance;
	};

	const float SafeInset = FMath::Clamp(ReceiverSampleInset, 0.0f, 1.0f);
	const float AxisDistanceA = CalculateAxisDistance(SampleAxisA) * SafeInset;
	const float AxisDistanceB = CalculateAxisDistance(SampleAxisB) * SafeInset;
	if (AxisDistanceA > KINDA_SMALL_NUMBER)
	{
		OutSamplePositions.Add(Center + SampleAxisA * AxisDistanceA);
		OutSamplePositions.Add(Center - SampleAxisA * AxisDistanceA);
	}
	if (AxisDistanceB > KINDA_SMALL_NUMBER)
	{
		OutSamplePositions.Add(Center + SampleAxisB * AxisDistanceB);
		OutSamplePositions.Add(Center - SampleAxisB * AxisDistanceB);
	}
}

int32 UUOULightExposureReceiverComponent::GetRequiredLightSampleHits(int32 AvailableSampleCount) const
{
	if (!bUseReceiverVolumeSampling || AvailableSampleCount <= 1)
	{
		return AvailableSampleCount > 0 ? 1 : 0;
	}

	return FMath::Clamp(RequiredReceiverSampleHits, 1, AvailableSampleCount);
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
	LastExposureSourceActor = Cast<AActor>(ExposureData.Source);
	if (LastExposureSourceActor == nullptr)
	{
		if (const UActorComponent* SourceComponent = Cast<UActorComponent>(ExposureData.Source))
		{
			LastExposureSourceActor = SourceComponent->GetOwner();
		}
	}
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
	ReceiverSampleInset = FMath::Clamp(ReceiverSampleInset, 0.0f, 1.0f);
	RequiredReceiverSampleHits = FMath::Clamp(RequiredReceiverSampleHits, 1, 5);

	if (MinTemperature > MaxTemperature)
	{
		Swap(MinTemperature, MaxTemperature);
	}

	AmbientTemperature = FMath::Clamp(AmbientTemperature, MinTemperature, MaxTemperature);
	CurrentTemperature = FMath::Clamp(CurrentTemperature, MinTemperature, MaxTemperature);
	TemperatureRisePerIntensity = FMath::Max(0.0f, TemperatureRisePerIntensity);
	TemperatureRecoveryRate = FMath::Max(0.0f, TemperatureRecoveryRate);
	ExposureEndGraceTime = FMath::Max(0.0f, ExposureEndGraceTime);
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

UPrimitiveComponent* UUOULightExposureReceiverComponent::GetReceiverVolume() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	if (UPrimitiveComponent* ReferencedVolume =
		Cast<UPrimitiveComponent>(ReceiverVolumeReference.GetComponent(Owner)))
	{
		return ReferencedVolume;
	}

	if (UPrimitiveComponent* ReceiverTransform = Cast<UPrimitiveComponent>(GetReferencedReceiverTransform()))
	{
		return ReceiverTransform;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent != Owner->GetRootComponent())
		{
			return PrimitiveComponent;
		}
	}

	return Cast<UPrimitiveComponent>(Owner->GetRootComponent());
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
