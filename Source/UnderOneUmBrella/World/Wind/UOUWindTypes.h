// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUWindTypes.generated.h"

class AActor;

// 런타임에 계산된 하나의 직선 바람 구간입니다.
USTRUCT(BlueprintType)
struct FUOUWindPathSegment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector End = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	float Strength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	int32 ReflectionIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<AActor> HitActor = nullptr;

	float GetLength() const
	{
		return FVector::Distance(Start, End);
	}
};

// 바람 수신체 하나에 전달되는 방향과 세기 정보입니다.
USTRUCT(BlueprintType)
struct FUOUWindExposureData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	FVector ClosestPointOnPath = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	float Strength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	float DeltaTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	int32 ReflectionIndex = 0;
};
