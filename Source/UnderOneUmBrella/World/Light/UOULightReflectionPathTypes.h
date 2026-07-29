// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOULightReflectionPathTypes.generated.h"

class UPrimitiveComponent;
class UUOULightInteractionSurfaceComponent;

UENUM(BlueprintType)
enum class EUOULightReflectionPathEndReason : uint8
{
	None UMETA(DisplayName = "진행 중"),
	RangeEnded UMETA(DisplayName = "반사 거리 종료"),
	Blocked UMETA(DisplayName = "장애물에 차단"),
	MinimumIntensity UMETA(DisplayName = "최소 광량 미만"),
	MaxBounces UMETA(DisplayName = "최대 반사 횟수 도달"),
	InvalidReflection UMETA(DisplayName = "유효하지 않은 반사")
};

// 반사 경로를 구성하는 한 구간의 런타임 데이터입니다.
USTRUCT(BlueprintType)
struct FUOULightReflectionSegmentData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	int32 BounceIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	TObjectPtr<UUOULightInteractionSurfaceComponent> Reflector = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	TObjectPtr<UUOULightInteractionSurfaceComponent> NextReflector = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	TObjectPtr<UPrimitiveComponent> BlockingComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	TArray<TObjectPtr<UObject>> ReachedReceivers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	FVector IncomingStart = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	FVector ImpactPoint = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	FVector ReflectionStart = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	FVector SegmentEnd = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	FVector IncomingDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	FVector ReflectedDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	float SegmentLength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	float BeamStartRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	float BeamEndRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path", meta = (Units = "deg", ToolTip = "이 반사 구간에서 실제로 사용한 빛의 확산각입니다."))
	float BeamConeAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	float IncomingIntensity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	float ReflectedIntensity = 0.0f;
};

// 한 광원에서 시작해 연속된 반사면을 통과하는 경로 데이터입니다.
USTRUCT(BlueprintType)
struct FUOULightReflectionPathData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	int32 PathIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	FVector SourcePosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	TArray<FUOULightReflectionSegmentData> Segments;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	EUOULightReflectionPathEndReason EndReason = EUOULightReflectionPathEndReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Path")
	float FinalIntensity = 0.0f;
};
