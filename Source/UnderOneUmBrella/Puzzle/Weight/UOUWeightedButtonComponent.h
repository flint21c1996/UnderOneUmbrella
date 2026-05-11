// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "Puzzle/Core/UOUPuzzleWeightSource.h"
#include "UOUWeightedButtonComponent.generated.h"

class USceneComponent;
class UUOUWeightSensorComponent;

// ???대옒?ㅻ뒗 ?쇱꽌媛 ?쎌? 臾닿쾶瑜??뚮┝ ?곹깭? 踰꾪듉 ?대룞?쇰줈 諛붽퓭二쇰뒗 議곌굔 ?뚯뒪??
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUWeightedButtonComponent : public UUOUPuzzleConditionSourceComponent, public IUOUPuzzleWeightSource
{
	GENERATED_BODY()

public:
	UUOUWeightedButtonComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual float GetPuzzleWeight() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	bool bAutoFindSensor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	FComponentReference SensorReference;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight")
	TObjectPtr<UUOUWeightSensorComponent> Sensor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float PressWeight = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Weight", meta = (ClampMin = "0.0"))
	float ReleaseWeight = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FComponentReference ButtonVisualReference;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	TObjectPtr<USceneComponent> ButtonVisual = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FComponentReference ReleasedPointReference;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	TObjectPtr<USceneComponent> ReleasedPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FComponentReference PressedPointReference;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	TObjectPtr<USceneComponent> PressedPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	bool bAutoFindMotionReferences = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FName PreferredButtonVisualName = TEXT("ButtonVisual");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FName PreferredReleasedPointName = TEXT("ReleasedPoint");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion")
	FName PreferredPressedPointName = TEXT("PressedPoint");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Motion", meta = (ClampMin = "0.0"))
	float MoveSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Debug")
	bool bShowScreenDebug = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	float CurrentWeight = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Puzzle|Weight")
	bool IsPressed() const;

protected:
	void DrawScreenDebug() const;
	void ResolveReferences();
	void RefreshPressedState();
	void MoveButtonVisual(float DeltaTime);
	void SnapVisualToCurrentState();
};
