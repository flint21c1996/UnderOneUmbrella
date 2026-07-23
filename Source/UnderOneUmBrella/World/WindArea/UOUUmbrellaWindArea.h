// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUUmbrellaWindArea.generated.h"

class UArrowComponent;
class UBoxComponent;
class USceneComponent;
class USplineComponent;
class UStaticMeshComponent;
class AUOUCharacter;
class UUOUPlayerInteractionExecutorComponent;

// 영역 안에서 우산을 펼치고 점프한 플레이어를 WindPath를 따라 이동시키는 배치용 액터입니다.
UCLASS(meta = (DisplayName = "UOU Umbrella Wind Area"))
class UNDERONEUMBRELLA_API AUOUUmbrellaWindArea : public AActor
{
	GENERATED_BODY()

public:
	AUOUUmbrellaWindArea();

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// WindArea 전체 배치의 기준이 되는 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 우산을 펼친 상태로 점프한 플레이어를 탐색하는 영역입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<UBoxComponent> WindVolume = nullptr;

	// 플레이어가 따라갈 경로입니다. 스플라인 포인트 타입으로 직선과 곡선 구간을 구성합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
	TObjectPtr<USplineComponent> WindPath = nullptr;

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

	// 이 거리 안에 들어오면 스플라인 시작점에 도착한 것으로 판단하고 경로 이동을 시작합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Gameplay", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "500.0", Units = "cm"))
	float AcceptanceRadius = 30.0f;

	// 플레이 중에도 WindVolume 프리뷰 메쉬를 표시할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Preview")
	bool bShowPreviewInGame = false;

	// 플레이 중 영역에서 목표까지의 방향선을 표시할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind|Debug")
	bool bDrawWindDebug = false;

private:
	// WindVolume과 WindPath의 현재 배치를 프리뷰 컴포넌트에 반영합니다.
	void RefreshPreview();

	// 이동 중인 플레이어가 없으면 영역 안에서 우산을 펼치고 점프한 플레이어를 등록합니다.
	void TryCapturePlayer();

	// 현재 플레이어를 시작점까지 이동시키거나 스플라인 위로 이동시킵니다.
	void UpdateActivePlayerTravel(float DeltaSeconds);

	// 이동을 끝내고 캐릭터의 기본 낙하 이동을 복구합니다.
	void FinishActivePlayerTravel();

	// 싱글플레이에서 현재 WindPath를 따라가는 플레이어입니다.
	TWeakObjectPtr<AUOUCharacter> ActivePlayer;

	// 플레이어가 스플라인 시작점으로 진입하는 단계인지 나타냅니다.
	bool bMovingToPathStart = false;

	// 현재 스플라인 시작점에서부터 이동한 거리입니다.
	float CurrentDistanceAlongPath = 0.0f;

	// WindPath 이동 중 게임플레이 입력만 차단하도록 요청한 플레이어 입력 컴포넌트입니다.
	TWeakObjectPtr<UUOUPlayerInteractionExecutorComponent> LockedInputExecutorComponent;
};
