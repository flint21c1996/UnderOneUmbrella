// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "World/Light/UOULightReflectionPathTypes.h"
#include "UOULightReflectionSpotLightComponent.generated.h"

class USpotLightComponent;
class UUOULightExposureSourceComponent;

// 계산된 반사 경로를 실제 런타임 SpotLight로 표현합니다.
UCLASS(
	ClassGroup = (Light),
	meta = (
		BlueprintSpawnableComponent,
		DisplayName = "UOU Light Reflection SpotLight",
		ToolTip = "반사 경로 구간마다 보조 SpotLight를 생성해 실제 월드 조명으로 표현합니다."))
class UUOULightReflectionSpotLightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOULightReflectionSpotLightComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Light|Reflection Visual",
		meta = (
			UseComponentPicker,
			AllowedClasses = "/Script/UnderOneUmBrella.UOULightExposureSourceComponent",
			ToolTip = "반사 경로를 제공할 Light Exposure Source 컴포넌트입니다. 비워두면 같은 액터에서 자동 탐색합니다."))
	FComponentReference SourceComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Reflection Visual", meta = (ToolTip = "같은 액터에서 Light Exposure Source 컴포넌트를 자동으로 찾습니다."))
	bool bAutoFindSourceComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection Visual", meta = (ToolTip = "반사 SpotLight 표현을 활성화합니다."))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection Visual", meta = (ClampMin = "0", ClampMax = "32", ToolTip = "동시에 사용할 수 있는 보조 SpotLight의 최대 개수입니다."))
	int32 MaxSpotLightCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection Visual", meta = (ClampMin = "0.0", ToolTip = "게임플레이 반사 광량을 Unreal SpotLight Intensity로 변환하는 배율입니다."))
	float IntensityScale = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection Visual", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Outer Cone Angle에 대한 Inner Cone Angle의 비율입니다."))
	float InnerConeRatio = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection Visual", meta = (ClampMin = "0.0", ToolTip = "이 길이보다 짧은 반사 구간은 SpotLight를 만들지 않습니다."))
	float MinimumSegmentLength = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection Visual", meta = (ToolTip = "원본 SpotLight의 색상을 보조 SpotLight에 사용합니다."))
	bool bUseSourceLightColor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection Visual", meta = (EditCondition = "!bUseSourceLightColor", EditConditionHides, ToolTip = "원본 색상을 사용하지 않을 때 적용할 반사광 색상입니다."))
	FLinearColor ReflectionLightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Reflection Visual", meta = (ToolTip = "보조 SpotLight가 동적 그림자를 생성하도록 합니다. 벽과 거울 뒤로 실제 조명이 새는 것을 줄여줍니다."))
	bool bCastShadows = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Reflection Visual", meta = (ToolTip = "현재 활성화된 반사 SpotLight 개수입니다."))
	int32 ActiveSpotLightCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Light|Reflection Visual", meta = (ToolTip = "현재 반사 경로를 사용해 보조 SpotLight를 즉시 다시 구성합니다."))
	void RefreshSpotLights();

protected:
	UPROPERTY(Transient)
	TObjectPtr<UUOULightExposureSourceComponent> BoundSourceComponent = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USpotLightComponent>> SpotLightPool;

	UFUNCTION()
	void HandleReflectionPathsUpdated(const TArray<FUOULightReflectionPathData>& ReflectionPaths);

	UUOULightExposureSourceComponent* ResolveSourceComponent() const;
	USpotLightComponent* AcquireSpotLight(int32 PoolIndex);
	void UpdateSpotLight(
		USpotLightComponent* SpotLight,
		const FUOULightReflectionSegmentData& SegmentData,
		const FLinearColor& LightColor) const;
	void HideUnusedSpotLights(int32 FirstUnusedIndex);
	FLinearColor ResolveLightColor() const;
};
