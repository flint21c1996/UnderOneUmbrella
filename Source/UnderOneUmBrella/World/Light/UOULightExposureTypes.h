// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOULightExposureTypes.generated.h"

// 통합 광원 액터에서 빠르게 선택할 수 있는 가산 혼합용 색상 프리셋입니다.
UENUM(BlueprintType)
enum class EUOULightColorPreset : uint8
{
	UseSourceSpotLight UMETA(DisplayName = "Source SpotLight 색상 유지"),
	Red UMETA(DisplayName = "빨강 (R)"),
	Green UMETA(DisplayName = "초록 (G)"),
	Blue UMETA(DisplayName = "파랑 (B)"),
	Custom UMETA(DisplayName = "사용자 지정"),
	White UMETA(DisplayName = "흰색 (RGB / 물감 제거)")
};

// 수신체에 도달한 게임플레이 빛 샘플 하나의 데이터입니다.
USTRUCT(BlueprintType, meta = (ToolTip = "게임플레이 빛이 수신체에 도달했을 때 전달되는 데이터입니다."))
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
		float InDeltaTime,
		const FLinearColor& InLightColor = FLinearColor::White)
		: Source(InSource)
		, SourcePosition(InSourcePosition)
		, ReceiverPosition(InReceiverPosition)
		, DirectionFromSource(InDirectionFromSource)
		, Distance(InDistance)
		, Intensity(InIntensity)
		, DistanceFactor(InDistanceFactor)
		, AngleFactor(InAngleFactor)
		, DeltaTime(InDeltaTime)
		, LightColor(InLightColor)
	{
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "이 노출 샘플을 발생시킨 오브젝트입니다."))
	TObjectPtr<UObject> Source = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "노출 레이가 시작된 월드 위치입니다."))
	FVector SourcePosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "이 샘플에서 수신체가 사용한 월드 위치입니다."))
	FVector ReceiverPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "광원에서 수신체로 향하는 정규화된 방향입니다."))
	FVector DirectionFromSource = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "이 샘플에서 광원과 수신체 사이의 거리입니다."))
	float Distance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "감쇠 계산 뒤 최종적으로 적용된 게임플레이 빛 세기입니다."))
	float Intensity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "최종 빛 세기를 계산할 때 사용한 거리 감쇠 계수입니다."))
	float DistanceFactor = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "최종 빛 세기를 계산할 때 사용한 원뿔 각도 감쇠 계수입니다."))
	float AngleFactor = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "빔과 Receiver Volume의 겹침 깊이 판정을 사용한 노출이면 true입니다."))
	bool bUsedReceiverVolumeOverlap = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (Units = "cm", ToolTip = "빔과 Receiver Volume이 겹친 선형 깊이입니다."))
	float ReceiverVolumeOverlapDepth = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (Units = "cm", ToolTip = "수광 판정을 통과하기 위해 필요했던 최소 겹침 깊이입니다."))
	float RequiredReceiverVolumeOverlapDepth = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "이 노출 샘플이 나타내는 DeltaTime입니다."))
	float DeltaTime = 0.0f;

	// 렌더링용 SpotLight와 같은 색입니다. 여러 광원의 게임플레이 색상 혼합에 사용합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Exposure", meta = (ToolTip = "이 노출을 발생시킨 광원의 선형 색상입니다."))
	FLinearColor LightColor = FLinearColor::White;
};
