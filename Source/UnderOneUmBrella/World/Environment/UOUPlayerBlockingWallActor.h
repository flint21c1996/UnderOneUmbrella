// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUPlayerBlockingWallActor.generated.h"

class UBoxComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

// 플레이어 이동만 막기 위한 투명 벽 액터입니다.
// 기본값은 Pawn 채널만 막고 나머지는 무시하므로 카메라, 라인트레이스, 물리 오브젝트에는 영향을 주지 않습니다.
UCLASS(Blueprintable, meta=(DisplayName="UOU Player Blocking Wall Actor"))
class AUOUPlayerBlockingWallActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUPlayerBlockingWallActor();

	// 투명 벽 액터의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 실제로 플레이어 이동을 막는 박스 충돌입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall")
	TObjectPtr<UBoxComponent> BlockingVolume = nullptr;

	// 에디터에서 벽의 범위를 눈으로 보기 위한 반투명 미리보기 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall|Preview")
	TObjectPtr<UStaticMeshComponent> PreviewMesh = nullptr;

	// 박스 충돌의 반지름 값입니다.
	// 언리얼 Box Extent와 같아서 실제 전체 크기는 이 값의 두 배입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall", meta = (ClampMin = "1.0"))
	FVector WallExtent = FVector(100.0f, 20.0f, 150.0f);

	// 벽이 막을 충돌 채널입니다.
	// 현재 프로젝트의 플레이어 캡슐은 Pawn 채널을 쓰므로 기본값은 Pawn입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall|Collision")
	TEnumAsByte<ECollisionChannel> BlockedChannel = ECC_Pawn;

	// 에디터에서 반투명 벽 미리보기를 표시할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall|Preview")
	bool bShowPreviewInEditor = true;

	// 플레이 중에도 반투명 벽 미리보기를 표시할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall|Preview")
	bool bShowPreviewInGame = false;

	// 미리보기 메쉬에 사용할 반투명 머티리얼입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall|Preview")
	TObjectPtr<UMaterialInterface> PreviewMaterial = nullptr;

	// 켜져 있으면 Preview Material 값을 Preview Mesh에 계속 적용합니다.
	// 끄면 Preview Mesh 컴포넌트의 머티리얼 슬롯을 직접 수정할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall|Preview")
	bool bOverridePreviewMeshMaterial = true;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// 충돌 채널을 다시 적용합니다.
	void ApplyCollisionSettings();

	// 미리보기 메쉬를 충돌 박스 크기와 표시 옵션에 맞춥니다.
	void ApplyPreviewSettings();
};
