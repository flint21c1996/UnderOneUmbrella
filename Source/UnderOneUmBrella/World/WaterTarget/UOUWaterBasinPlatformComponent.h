// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUWaterBasinPlatformComponent.generated.h"

class UUOUWaterBasinTargetComponent;
class USceneComponent;
class AActor;

// WaterBasin의 현재 수면 높이를 따라 움직이는 플랫폼 Actor용 컴포넌트입니다.
// 이 컴포넌트는 Platform Actor에 붙이고, TargetActor로 물을 가진 WaterTile Actor를 참조합니다.
// 물의 부피 계산에는 참여하지 않고, 지정한 Basin Target의 로컬 물 깊이만큼 Owner Actor 또는 지정 컴포넌트의 높이를 올립니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Water Basin Platform"))
class UNDERONEUMBRELLA_API UUOUWaterBasinPlatformComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUWaterBasinPlatformComponent();

	// 시작 시 Target/Platform 참조를 해석하고 플랫폼의 기준 월드 Z를 저장합니다.
	virtual void BeginPlay() override;

	// 물이 장치에 의해 서서히 변할 수 있으므로 매 프레임 WaterTile의 로컬 물 깊이를 따라갑니다.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 수면 높이를 읽을 WaterBasinTargetComponent를 가진 WaterTile Actor입니다.
	// Platform Actor와 WaterTile Actor가 같은 경우에만 비워둘 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "플랫폼이 따라갈 WaterBasinTargetComponent를 가진 WaterTile Actor입니다. 플랫폼 Actor와 WaterTile Actor가 같을 때만 비워둘 수 있습니다."))
	TObjectPtr<AActor> TargetActor = nullptr;

	// 실제로 높이를 이동시킬 SceneComponent입니다.
	// 비워두면 이름/태그로 찾고, 그래도 없으면 기본적으로 Platform Actor의 RootComponent를 움직입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "WaterTile의 물 깊이만큼 이동시킬 플랫폼 SceneComponent입니다. 비워두면 이름/태그 탐색 후 Platform Actor의 RootComponent를 사용합니다."))
	TObjectPtr<USceneComponent> PlatformComponent = nullptr;

	// PlatformComponent가 비어 있을 때 사용할 Component 이름 또는 Component Tag입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "Platform Component 자동 탐색에 사용할 Component 이름 또는 Component Tag입니다."))
	FName PlatformComponentName = TEXT("Platform");

	// PlatformComponent를 찾지 못했을 때 Platform Actor의 RootComponent를 움직일지 정합니다.
	// 이 컴포넌트는 플랫폼 Actor에 붙이는 것을 기본 구조로 하므로 true가 기본값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "Platform Component를 찾지 못했을 때 Platform Actor의 RootComponent를 대신 움직입니다."))
	bool bUseOwnerRootWhenPlatformMissing = true;

	// true이면 BeginPlay 시 현재 플랫폼 Z를 기준 높이로 자동 저장합니다.
	// 이후 플랫폼은 이 기준 높이에서 자신이 속한 WaterTile의 물 깊이만큼만 올라갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "게임 시작 시 현재 플랫폼 Z를 기준 높이로 저장하고, 이후 WaterTile의 물 깊이만큼만 상승합니다."))
	bool bCaptureBaseZOnBeginPlay = true;

	// bCaptureBaseZOnBeginPlay가 꺼져 있을 때 사용할 플랫폼 기준 월드 Z입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "플랫폼이 물 깊이 0일 때 위치할 기준 월드 Z입니다."))
	float BasePlatformWorldZ = 0.0f;

	// 물 깊이에 추가로 더할 월드 Z 오프셋입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "WaterTile의 물 깊이에 추가로 더할 월드 Z 오프셋입니다."))
	float WaterDepthOffsetZ = 0.0f;

	// true이면 X/Y는 유지하고 Z만 수면 높이에 맞춥니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "켜져 있으면 플랫폼의 X/Y 위치는 유지하고 Z 위치만 변경합니다."))
	bool bMoveOnlyZ = true;

	// true이면 목표 높이까지 보간 이동합니다. false이면 수면 높이에 즉시 맞춥니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform|Movement", meta = (ToolTip = "켜져 있으면 목표 수면 높이까지 부드럽게 보간 이동합니다."))
	bool bUseInterpolation = false;

	// 보간 이동 속도입니다. 값이 클수록 수면 높이를 빠르게 따라갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform|Movement", meta = (ClampMin = "0.0", ToolTip = "보간 이동 속도입니다. 0이면 보간 없이 즉시 목표 높이로 이동합니다."))
	float InterpSpeed = 8.0f;

	// 마지막으로 계산된 플랫폼 목표 월드 Z입니다. BasePlatformWorldZ + TargetWaterDepthWorld + WaterDepthOffsetZ입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Platform|Runtime")
	float CurrentTargetWorldZ = 0.0f;

	// TargetActor를 바꾸고 즉시 플랫폼 위치를 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Platform")
	void SetTargetActor(AActor* NewTargetActor);

	// 현재 WaterTile의 물 깊이를 기준으로 플랫폼 위치를 즉시 다시 계산합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Platform")
	void RefreshPlatformPosition();

	// 현재 TargetActor WaterTile의 로컬 물 깊이를 월드 단위로 반환합니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Platform")
	float GetTargetWaterDepthWorld() const;

private:
	// TargetActor 또는 Owner에서 WaterBasinTargetComponent를 찾습니다.
	UUOUWaterBasinTargetComponent* GetTargetComponent() const;

	// PlatformComponent가 비어 있으면 이름/태그 또는 RootComponent로 해석합니다.
	USceneComponent* GetPlatformComponent() const;

	// PlatformComponentName과 일치하는 Owner의 SceneComponent를 찾습니다.
	USceneComponent* FindPlatformComponent() const;
};
