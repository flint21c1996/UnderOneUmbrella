// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOULightExposureTypes.generated.h"

USTRUCT(BlueprintType)
struct FUOULightExposureData
{
	GENERATED_BODY()

	FUOULightExposureData() = default;

	FUOULightExposureData(
		UObject* InSource,
		const FVector& InSourcePosition,
		const FVector& InReceiverPosition,
		const FVector& InDirectionFromSource,
		float InDistance,
		float InIntensity,
		float InDistanceFactor,
		float InAngleFactor,
		float InDeltaTime)
		: Source(InSource)
		, SourcePosition(InSourcePosition)
		, ReceiverPosition(InReceiverPosition)
		, DirectionFromSource(InDirectionFromSource)
		, Distance(InDistance)
		, Intensity(InIntensity)
		, DistanceFactor(InDistanceFactor)
		, AngleFactor(InAngleFactor)
		, DeltaTime(InDeltaTime)
	{
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	TObjectPtr<UObject> Source = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	FVector SourcePosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	FVector ReceiverPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	FVector DirectionFromSource = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	float Distance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	float Intensity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	float DistanceFactor = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	float AngleFactor = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure")
	float DeltaTime = 0.0f;
};
