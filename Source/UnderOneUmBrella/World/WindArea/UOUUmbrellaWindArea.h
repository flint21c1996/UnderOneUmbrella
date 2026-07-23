// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUUmbrellaWindArea.generated.h"

class UArrowComponent;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class AUOUCharacter;

// 영역 안에서 우산을 펼친 플레이어를 지정된 목표 위치로 이동시키는 배치용 액터입니다.
UCLASS(meta = (DisplayName = "UOU Umbrella Wind Area"))
class UNDERONEUMBRELLA_API AUOUUmbrellaWindArea : public AActor
{
	GENERATED_BODY()

public:
	AUOUUmbrellaWindArea();

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// WindArea 전체 배치의 기준이 되는 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 우산을 펼친 플레이어를 탐색하는 영역입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<UBoxComponent> WindVolume = nullptr;

	// 플레이어가 이동할 최종 위치입니다. 레벨에서 이 컴포넌트를 옮겨 목적지를 지정합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<UArrowComponent> WindTarget = nullptr;

	// 에디터에서 바람의 진행 방향을 확인하기 위한 화살표입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind|Preview")
	TObjectPtr<UArrowComponent> WindDirectionArrow = nullptr;

	// WindVolume의 크기를 에디터와 게임에서 확인하기 위한 반투명 프리뷰 메쉬입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind|Preview")
	TObjectPtr<UStaticMeshComponent> PreviewVolumeMesh = nullptr;

	// WindArea의 플레이어 이동 적용 여부입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay")
	bool bWindEnabled = true;

	// 목표 위치를 향해 이동할 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "3000.0", Units = "cm/s"))
	float MoveSpeed = 1000.0f;

	// 이 거리 안에 들어오면 목표에 도착한 것으로 판단하고 추가 이동을 적용하지 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "500.0", Units = "cm"))
	float AcceptanceRadius = 30.0f;

	// 플레이 중에도 WindVolume 프리뷰 메쉬를 표시할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Preview")
	bool bShowPreviewInGame = false;

	// 플레이 중 영역에서 목표까지의 방향선을 표시할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Debug")
	bool bDrawWindDebug = false;

private:
	// WindVolume과 WindTarget의 현재 배치를 프리뷰 컴포넌트에 반영합니다.
	void RefreshPreview();

	// 영역 안에서 우산을 펼친 플레이어를 목표 이동 대상으로 등록합니다.
	void RefreshTrackedPlayers();

	// 등록된 플레이어가 영역을 벗어나도 목표에 도착할 때까지 이동을 이어갑니다.
	void ApplyWindToTrackedPlayers(float DeltaSeconds);

	// 목표 이동 중인 플레이어를 약한 참조로 보관해 파괴된 플레이어를 붙잡지 않습니다.
	TSet<TWeakObjectPtr<AUOUCharacter>> TrackedPlayers;
};
