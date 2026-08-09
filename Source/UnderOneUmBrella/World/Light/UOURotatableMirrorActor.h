// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOURotatableMirrorActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOULightInteractionSurfaceComponent;
class UUOURotatableMirrorComponent;

// 회전축, 거울 메시, 빛 반사면과 플레이어 밀기 영역을 한 번에 제공하는 기본 거울 액터입니다.
UCLASS(Blueprintable)
class AUOURotatableMirrorActor : public AActor
{
	GENERATED_BODY()

public:
	AUOURotatableMirrorActor();

	// 거울 전체가 회전하는 중앙 회전축입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<USceneComponent> MirrorPivot = nullptr;

	// 화면 표시와 플레이어 차단을 담당하는 거울 메시입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<UStaticMeshComponent> MirrorMesh = nullptr;

	// 빛 차단 및 반사 판정을 담당하는 표면입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<UUOULightInteractionSurfaceComponent> LightInteractionSurface = nullptr;

	// 플레이어가 거울을 밀고 있는지 감지하는 Overlap 영역입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<UBoxComponent> PushVolume = nullptr;

	// 거울 왼쪽에서 플레이어가 잡을 기본 손잡이 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<USceneComponent> PushHandleLeft = nullptr;

	// 거울 오른쪽에서 플레이어가 잡을 기본 손잡이 위치입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<USceneComponent> PushHandleRight = nullptr;

	// Push Volume 안의 플레이어 이동을 중앙 회전축 기준 회전으로 변환합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mirror|Components")
	TObjectPtr<UUOURotatableMirrorComponent> RotatableMirror = nullptr;
};
