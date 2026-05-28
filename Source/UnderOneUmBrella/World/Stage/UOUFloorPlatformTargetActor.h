// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUFloorPlatformTargetActor.generated.h"

class UMaterialInterface;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

// 층 플랫폼의 목표 위치와 목표 회전을 월드에 직접 배치해서 정하는 마커 액터입니다.
// 디자이너가 이 액터를 움직이고 돌리면 플랫폼은 해당 트랜스폼을 최종 상태로 사용합니다.
UCLASS(meta=(DisplayName="UOU Floor Platform Target Actor"))
class AUOUFloorPlatformTargetActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUFloorPlatformTargetActor();

	// 목표 마커 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Target")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 목표 위치의 기준점을 작은 구체로 보여주는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Target")
	TObjectPtr<USphereComponent> TargetOriginMarker = nullptr;

	// 목표 위치에 놓일 플랫폼 모습을 보여주는 확인용 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Target")
	TObjectPtr<UStaticMeshComponent> TargetPreviewMesh = nullptr;

	// 목표 발판을 실제 발판과 헷갈리지 않게 보여주기 위한 반투명 확인용 머티리얼입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Target")
	TObjectPtr<UMaterialInterface> TargetPreviewMaterial = nullptr;

	// 플랫폼 메쉬와 같은 리소스를 목표 미리보기 메쉬에 복사합니다.
	void SyncPreviewFromMesh(const UStaticMeshComponent* SourceMesh);

	// 목표 위치에 놓인 결과 플랫폼 미리보기 메쉬만 켜고 끕니다.
	void SetTargetPreviewMeshVisible(bool bVisible);
};
