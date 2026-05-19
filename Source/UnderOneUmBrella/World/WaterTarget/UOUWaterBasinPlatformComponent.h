// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUWaterBasinPlatformComponent.generated.h"

class AActor;
class USceneComponent;
class UUOUWaterBasinTargetComponent;

// WaterTile의 수면 높이를 따라 플랫폼 Actor 또는 특정 SceneComponent를 이동시키는 컴포넌트입니다.
// 물의 부피 계산에는 참여하지 않고, 지정한 WaterBasinTargetComponent의 현재 수면 World Z만 읽어 위치를 갱신합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Water Basin Platform"))
class UNDERONEUMBRELLA_API UUOUWaterBasinPlatformComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUWaterBasinPlatformComponent();

	// 에디터 등록 시 Attach Parent 체인에서 WaterTile Actor를 자동으로 찾습니다.
	virtual void OnRegister() override;

	// 시작 시 Target/Platform 참조를 해석하고 현재 수면 위치에 맞춰 플랫폼 위치를 갱신합니다.
	virtual void BeginPlay() override;

	// 물이 장치에 의해 서서히 변할 수 있으므로 매 프레임 WaterTile의 수면 World Z를 따라갑니다.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 수면 높이를 읽을 WaterBasinTargetComponent를 가진 WaterTile Actor입니다.
	// 플랫폼을 WaterTile의 자식으로 두지 않을 때는 이 값을 레벨 인스턴스에서 직접 지정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "플랫폼이 따라갈 WaterTile Actor입니다. WaterTile의 자식으로 두지 않는 구조라면 이 값을 직접 지정합니다."))
	TObjectPtr<AActor> TargetActor = nullptr;

	// TargetActor가 비어 있을 때 Attach Parent 체인에서 WaterTile Actor를 자동으로 찾습니다.
	// 플랫폼을 WaterTile 자식으로 붙여서 빠르게 구성할 때 유용하지만, 수동 TargetActor를 쓰는 구조에서는 꺼두는 편이 명확합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "TargetActor가 비어 있을 때 Attach Parent 체인에서 WaterTile Actor를 자동으로 찾습니다. 수동 TargetActor를 쓰면 끄는 것을 권장합니다."))
	bool bAutoFindTargetActorFromAttachParent = true;

	// Attach Parent가 바뀔 때마다 TargetActor를 다시 자동 동기화할지 정합니다.
	// true이면 수동으로 넣은 TargetActor가 Attach Parent 기준 결과로 덮일 수 있으므로, 독립 배치 플랫폼에서는 false로 둡니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "Attach Parent 변경에 맞춰 TargetActor를 계속 갱신합니다. 수동 TargetActor를 유지하려면 false로 둡니다."))
	bool bKeepAutoTargetSyncedWithAttachParent = true;

	// 실제로 높이를 이동시킬 SceneComponent입니다.
	// 비워두면 PlatformComponentName으로 찾고, 그래도 없으면 옵션에 따라 Owner RootComponent를 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "수면 높이에 맞춰 이동시킬 SceneComponent입니다. 비우면 이름/태그로 찾고, 없으면 RootComponent를 사용할 수 있습니다."))
	TObjectPtr<USceneComponent> PlatformComponent = nullptr;

	// PlatformComponent가 비어 있을 때 찾을 Component 이름 또는 Component Tag입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "PlatformComponent 자동 검색에 사용할 Component 이름 또는 Component Tag입니다."))
	FName PlatformComponentName = TEXT("Platform");

	// PlatformComponent를 찾지 못했을 때 Owner RootComponent를 이동 대상으로 사용할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "PlatformComponent를 찾지 못했을 때 Owner Actor의 RootComponent를 이동 대상으로 사용합니다."))
	bool bUseOwnerRootWhenPlatformMissing = true;

	// Target WaterTile의 수면 World Z에 추가할 높이 오프셋입니다.
	// 0이면 플랫폼 기준점이 수면과 정확히 같은 Z에 놓입니다. 플랫폼 두께나 중심점 때문에 떠야 하면 양수로 올립니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "Target WaterTile의 수면 World Z에 더할 높이입니다. 0이면 수면과 같은 Z, 양수면 수면보다 위에 배치됩니다."))
	float SurfaceOffsetZ = 0.0f;

	// true이면 X/Y 위치는 유지하고 Z 위치만 수면 높이에 맞춥니다.
	// false이면 TargetActor의 X/Y 위치까지 함께 따라갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform", meta = (ToolTip = "켜져 있으면 플랫폼의 X/Y 위치는 유지하고 Z 위치만 변경합니다. 끄면 TargetActor의 X/Y도 따라갑니다."))
	bool bMoveOnlyZ = true;

	// true이면 목표 높이까지 부드럽게 보간 이동합니다. false이면 수면 높이에 즉시 맞춥니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform|Movement", meta = (ToolTip = "켜져 있으면 목표 수면 높이까지 부드럽게 보간 이동합니다. 꺼져 있으면 즉시 이동합니다."))
	bool bUseInterpolation = false;

	// FInterpTo에 전달되는 보간 계수입니다.
	// cm/s 같은 월드 단위 속도가 아니라, 값이 클수록 목표 Z에 더 빠르게 가까워지는 반응성 값입니다.
	// 기본 8은 수면 변화가 보이면서도 너무 늦게 따라오지 않는 테스트용 기본값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Platform|Movement", meta = (ClampMin = "0.0", ToolTip = "FInterpTo 보간 계수입니다. 월드 단위 속도가 아니라 값이 클수록 목표 Z에 빠르게 가까워지는 반응성 값입니다."))
	float InterpSpeed = 8.0f;

	// 마지막으로 계산된 플랫폼 목표 World Z입니다. TargetSurfaceWorldZ + SurfaceOffsetZ입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Water Platform|Runtime")
	float CurrentTargetWorldZ = 0.0f;

	// TargetActor를 바꾸고 즉시 플랫폼 위치를 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Platform")
	void SetTargetActor(AActor* NewTargetActor);

	// Attach Parent 체인에서 WaterBasinTargetComponent를 가진 Actor를 찾아 TargetActor로 설정합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Water Platform")
	void AutoResolveTargetActor();

	// 현재 WaterTile 수면 World Z를 기준으로 플랫폼 위치를 즉시 다시 계산합니다.
	UFUNCTION(BlueprintCallable, Category = "Water Platform")
	void RefreshPlatformPosition();

	// 현재 TargetActor WaterTile의 수면 World Z를 반환합니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Water Platform")
	float GetTargetSurfaceWorldZ() const;

private:
	// TargetActor가 마지막으로 Attach Parent 자동 검색으로 설정되었는지 추적합니다.
	bool bTargetActorAutoResolved = false;

	// TargetActor 또는 Owner에서 WaterBasinTargetComponent를 찾습니다.
	UUOUWaterBasinTargetComponent* GetTargetComponent() const;

	// 자동 TargetActor가 부모 Attach 상태와 일치하도록 갱신합니다.
	void SyncAutoResolvedTargetActor();

	// Owner의 Attach Parent 체인을 따라가며 WaterBasinTargetComponent를 가진 Actor를 찾습니다.
	AActor* FindAttachParentTargetActor() const;

	// PlatformComponent가 비어 있으면 이름/태그 또는 RootComponent로 해석합니다.
	USceneComponent* GetPlatformComponent() const;

	// PlatformComponentName과 일치하는 Owner의 SceneComponent를 찾습니다.
	USceneComponent* FindPlatformComponent() const;
};
