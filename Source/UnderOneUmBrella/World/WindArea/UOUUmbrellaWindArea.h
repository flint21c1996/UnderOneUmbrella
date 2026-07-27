// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUUmbrellaWindArea.generated.h"

class UArrowComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class USplineComponent;
class UStaticMeshComponent;
class AUOUCharacter;

// 영역 안에서 우산을 펼치고 점프한 플레이어를 WindPath를 따라 이동시키는 배치용 액터입니다.
UCLASS(meta = (DisplayName = "UOU Umbrella Wind Area"))
class UNDERONEUMBRELLA_API AUOUUmbrellaWindArea : public AActor
{
	GENERATED_BODY()

public:
	AUOUUmbrellaWindArea();

protected:
	virtual void BeginPlay() override;
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
	float MoveSpeed = 500.0f;

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

	// 런타임 스플라인 디버그 표시가 필요할 때만 WindArea Tick을 켭니다.
	void RefreshTickEnabled();

	// 영역 내 플레이어를 교체하며 점프 이벤트 구독 수명도 함께 관리합니다.
	void SetOverlappingPlayer(AUOUCharacter* Character);

	UFUNCTION()
	void HandleOverlappingPlayerJumped();

	UFUNCTION()
	void HandleWindVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleWindVolumeEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	// WindVolume 안에서 발동 조건을 기다리는 싱글플레이 캐릭터입니다.
	TWeakObjectPtr<AUOUCharacter> OverlappingPlayer;
};
