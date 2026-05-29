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
// 충돌 켜기와 끄기를 에디터 버튼과 런타임 함수 양쪽에서 사용할 수 있습니다.
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

	// 에디터에서 벽의 범위를 눈으로 확인하기 위한 반투명 미리보기 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall|Preview")
	TObjectPtr<UStaticMeshComponent> PreviewMesh = nullptr;

	// 벽 충돌이 현재 켜져 있는지 정합니다.
	// 꺼지면 미리보기는 남겨둘 수 있지만 플레이어를 막지는 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall|State")
	bool bWallEnabled = true;

	// 박스 충돌의 반지름 값입니다.
	// 언리얼의 Box Extent와 같아서 실제 전체 크기는 이 값의 두 배입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall", meta = (ClampMin = "1.0"))
	FVector WallExtent = FVector(100.0f, 20.0f, 150.0f);

	// 벽이 막을 충돌 채널입니다.
	// 기본값은 플레이어 캡슐이 사용하는 Pawn 채널입니다.
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
	// 끄면 Preview Mesh 컴포넌트에서 머티리얼을 직접 바꿀 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Blocking Wall|Preview")
	bool bOverridePreviewMeshMaterial = true;

	// 투명 벽을 켜고 충돌을 다시 적용합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Player Blocking Wall|Actions")
	void EnableWall();

	// 투명 벽을 끄고 충돌을 비활성화합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Player Blocking Wall|Actions")
	void DisableWall();

	// 현재 상태를 반대로 바꿉니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Player Blocking Wall|Actions")
	void ToggleWall();

	// 퍼즐 결과나 블루프린트 이벤트에서 투명 벽 상태를 직접 지정할 때 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Player Blocking Wall|Actions")
	void SetWallEnabled(bool bNewEnabled);

	// 현재 투명 벽이 플레이어를 막는 상태인지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Player Blocking Wall|Runtime")
	bool IsWallEnabled() const;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// 현재 활성 상태와 충돌 채널 설정을 BlockingVolume에 적용합니다.
	void ApplyCollisionSettings();

	// 미리보기 메쉬를 충돌 박스 크기와 표시 옵션에 맞춥니다.
	void ApplyPreviewSettings();
};
