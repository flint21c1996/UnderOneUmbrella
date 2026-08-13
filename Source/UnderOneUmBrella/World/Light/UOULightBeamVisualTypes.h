// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/Light/UOULightReflectionPathTypes.h"
#include "UOULightBeamVisualTypes.generated.h"

// 하나의 직접광 또는 반사광 VFX 구간을 표현하기 위한 월드 공간 데이터입니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOULightBeamVisualSegmentData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	int32 SegmentIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	bool bReflected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	FVector End = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual", meta = (Units = "cm"))
	float Length = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual", meta = (Units = "cm"))
	float ReferenceLength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual", meta = (Units = "cm"))
	float StartRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual", meta = (Units = "cm"))
	float EndRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual", meta = (Units = "deg"))
	float ConeAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	float Intensity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	float VisualBrightnessMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	float VisualOpacityMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	int32 LumenDynamicRayPresetOverride = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	int32 LumenStaticRayPresetOverride = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Visual")
	EUOULightReflectionPathEndReason EndReason = EUOULightReflectionPathEndReason::None;
};
