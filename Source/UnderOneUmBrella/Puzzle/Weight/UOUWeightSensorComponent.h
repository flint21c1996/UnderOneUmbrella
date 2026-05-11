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

// ???대옒?ㅻ뒗 ?몃━嫄??곸뿭???ㅼ뼱???≫꽣?ㅼ쓽 臾닿쾶瑜??⑹궛??踰꾪듉怨???몄뿉 ?꾨떖?쒕떎.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUWeightSensorComponent : public UActorComponent, public IUOUPuzzleWeightSource
{
	GENERATED_BODY()

public:
	UUOUWeightSensorComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual float GetPuzzleWeight() const override;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Weight")
	FOnWeightSensorWeightChangedSignature OnWeightChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	bool bAutoFindSensorVolume = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	FName PreferredSensorVolumeName = TEXT("WeightSensorVolume");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	FComponentReference SensorVolumeReference;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	TObjectPtr<UPrimitiveComponent> SensorVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float CurrentWeight = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	int32 OverlappingActorCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Sensor")
	void RefreshCurrentWeight();

protected:
	TMap<TObjectPtr<AActor>, int32> OverlapActorCounts;

	UFUNCTION()
	void HandleSensorBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleSensorEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void ResolveSensorVolume();
	void BindSensorVolume();
	void UnbindSensorVolume();
	void RegisterOverlappingActor(AActor* OtherActor);
	void UnregisterOverlappingActor(AActor* OtherActor);
	float ResolveActorWeight(const AActor* OtherActor) const;
};
