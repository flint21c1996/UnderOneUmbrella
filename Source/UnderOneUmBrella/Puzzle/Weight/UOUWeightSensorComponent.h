// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleWeightSource.h"
#include "UOUWeightSensorComponent.generated.h"

class AActor;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightSensorWeightChangedSignature, float, CurrentWeight);

// 센서 볼륨 안에 들어온 액터들의 퍼즐 무게를 합산해 주는 무게 센서 컴포넌트입니다.
// 버튼과 저울 같은 장치는 이 센서를 통해 현재 올라간 무게를 공통 방식으로 읽습니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUWeightSensorComponent : public UActorComponent, public IUOUPuzzleWeightSource
{
	GENERATED_BODY()

public:
	UUOUWeightSensorComponent();

	// 시작 시 센서 볼륨을 찾고 오버랩 이벤트를 연결합니다.
	virtual void BeginPlay() override;

	// 종료 시 센서 이벤트 구독을 정리합니다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 매 틱마다 현재 무게 합계와 겹침 상태를 갱신합니다.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 현재 센서가 계산한 퍼즐용 총 무게를 반환합니다.
	virtual float GetPuzzleWeight() const override;

	// 센서 무게가 바뀔 때 외부에 알리는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Weight")
	FOnWeightSensorWeightChangedSignature OnWeightChanged;

	// 같은 액터 안의 센서 볼륨을 자동으로 찾을지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	bool bAutoFindSensorVolume = true;

	// 자동 탐색 시 우선 찾을 센서 볼륨 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	FName PreferredSensorVolumeName = TEXT("WeightSensorVolume");

	// 수동으로 연결할 센서 볼륨 참조입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	FComponentReference SensorVolumeReference;

	// 실제로 연결된 센서 볼륨 프리미티브입니다.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	TObjectPtr<UPrimitiveComponent> SensorVolume = nullptr;

	// 현재 겹친 액터들에서 계산한 총 무게입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float CurrentWeight = 0.0f;

	// 현재 센서에 들어와 있는 액터 수입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	int32 OverlappingActorCount = 0;

	// 현재 겹침 목록을 다시 읽어 총 무게를 즉시 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "Puzzle|Sensor")
	void RefreshCurrentWeight();

protected:
	// 액터별 겹침 횟수를 세어 중복 오버랩을 안정적으로 처리합니다.
	TMap<TObjectPtr<AActor>, int32> OverlapActorCounts;

	// 센서 볼륨에 액터가 들어왔을 때 무게 계산 목록에 등록합니다.
	UFUNCTION()
	void HandleSensorBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// 센서 볼륨에서 액터가 나갔을 때 무게 계산 목록에서 해제합니다.
	UFUNCTION()
	void HandleSensorEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	// 센서 볼륨을 자동 탐색하거나 수동 참조로 해석합니다.
	void ResolveSensorVolume();

	// 센서 볼륨의 오버랩 이벤트를 바인딩합니다.
	void BindSensorVolume();

	// 센서 볼륨 이벤트 바인딩을 해제합니다.
	void UnbindSensorVolume();

	// 겹친 액터를 내부 카운트 맵에 등록합니다.
	void RegisterOverlappingActor(AActor* OtherActor);

	// 겹친 액터를 내부 카운트 맵에서 해제합니다.
	void UnregisterOverlappingActor(AActor* OtherActor);

	// 하나의 액터에서 실제 퍼즐 무게를 계산합니다.
	float ResolveActorWeight(const AActor* OtherActor) const;
};
